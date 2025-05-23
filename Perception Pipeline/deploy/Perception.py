from transformers import AutoImageProcessor, AutoModelForDepthEstimation, Mask2FormerForUniversalSegmentation
import torch
import cv2
import numpy as np
import time
from torchvision import transforms
import gc

def readCameraTransform(file_path):
    # Load the data from the file into a NumPy array
    data = np.loadtxt(file_path, delimiter=',')

    # Split the data into translation and rotation vectors
    translation_vectors = data[:, :3]/100  # First three columns are translation vectors... div 100 to go from cm to m
    rotation_vectors = np.radians(data[:, 3:])     # Last three columns are rotation vectors
    remapped_translation_vectors = translation_vectors[:, [1, 2, 0]]
    remapped_translation_vectors[:, 1] *= -1
    #print(translation_vectors)
    #translation_vectors = np.zeros_like(translation_vectors)
    #rotation_vectors = np.zeros_like(rotation_vectors)
    # Display the translation and rotation vectors
    # print("Translation Vectors:\n", translation_vectors)
    # print("Rotation Vectors:\n", rotation_vectors)
    return remapped_translation_vectors, rotation_vectors

def readCalibrationData(file_path):
    data = np.load(file_path)
    camera_matrix = data['mtx']
    distortion_params = data['dist']
    #print(distortion_params)
    #print(camera_matrix)
    return camera_matrix, distortion_params

class Detection():
    def __init__(self, world_points, colors, label) -> None:
        self.world_points = world_points
        self.colors = colors
        self.label = label
        pass


class PerceptionPipe():
    def __init__(self, depth_processor_path, depth_model_path, segmentation_processor_path, segmentation_model_path, 
                 camera_params_path, camera_transforms_path,
                 device = 'none') -> None:
        if(device == 'none'):
            self.device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
        else:
            self.device = device
        print(f"(Perception Pipe) Placing models on: {self.device}")

        self.depth_processor = AutoImageProcessor.from_pretrained(depth_processor_path)
        self.depth_model = AutoModelForDepthEstimation.from_pretrained(depth_model_path).to(self.device)
        self.depth_model.eval()
        self.segmentation_processor = AutoImageProcessor.from_pretrained(segmentation_processor_path)
        self.segmentation_model = Mask2FormerForUniversalSegmentation.from_pretrained(segmentation_model_path).to(self.device)
        self.segmentation_model.eval()
        self.label_ids_to_fuse = []
        self.distorted_coords = None

        # 2. Apply mixed precision (FP16) if needed
        if self.device != 'cpu':
            self.depth_model.half()
            self.segmentation_model.half()
            # 3. Compile the model with torch.compile for optimizations
            # self.depth_model = torch.compile(self.depth_model, backend='onnxrt')
            # self.segmentation_model = torch.compile(self.segmentation_model, backend='onnxrt')
        
        #considering same camera for all:
        camera_params, distortions = readCalibrationData(camera_params_path)
        translation_vectors, rotation_vectors = readCameraTransform(camera_transforms_path)
        self.rotation_vectors = torch.tensor(rotation_vectors, device=self.device)
        self.translation_vectors = torch.tensor(translation_vectors, device=self.device)
        
        
        self.num_cameras = self.translation_vectors.shape[0]

        camera_matrices = np.tile(camera_params, (self.num_cameras, 1, 1))  # Adjust dimensions as needed
        distortion_params = np.tile(distortions, (self.num_cameras, 1))         # Adjust dimensions as needed 
        self.camera_matrices = torch.tensor(camera_matrices, device=self.device)
        self.distortion_params = torch.tensor(distortion_params, device=self.device)

        self.transform = transforms.ToTensor()

    def __call__(self, images):
        # torch.cuda.empty_cache()

        segmentation_start = time.perf_counter()
        images_processed = self.segmentation_processor(images=images, return_tensors="pt")
        if self.device != 'cpu': 
            images_processed = {key: value.half().to(self.device) for key, value in images_processed.items()}
        else:
            images_processed = {key: value.to(self.device) for key, value in images_processed.items()}
        with torch.no_grad():
            segmentations = self.segmentation_model(**images_processed)
        sizes = [images[0].size[::-1]]
        num_repeats = len(images)
        new_sizes = [size for size in sizes for _ in range(num_repeats)]
        segmentations = self.segmentation_processor.post_process_panoptic_segmentation(segmentations, target_sizes=new_sizes, label_ids_to_fuse = self.label_ids_to_fuse)
    
        #del images_processed #take this off gpu asap
        segmentation_end = time.perf_counter()

        depth_start = time.perf_counter()
        images_processed = self.depth_processor(images=images, return_tensors="pt")
        if self.device != 'cpu':
            images_processed = {key: value.half().to(self.device) for key, value in images_processed.items()}
        else:
            images_processed = {key: value.to(self.device) for key, value in images_processed.items()}
        with torch.no_grad():
            depth_outputs = self.depth_model(**images_processed)
        # # Interpolate each depth map to the original size
        original_size = images[0].size[::-1]# Assuming all images have the same size
        depth_outputs = torch.nn.functional.interpolate(
            depth_outputs.predicted_depth.unsqueeze(1),
            size=original_size,
            mode="bicubic",
            align_corners=False,
        ).squeeze()
        depth_end = time.perf_counter()    

        depth_to_3d_start = time.perf_counter()
        positions_3d = self.depth_to_3d_batch(depth_outputs.half())
        depth_to_3d_end =  time.perf_counter()
        
        fusion_start = time.perf_counter()
        # Using defa# Initialize an empty dictionary
        formated_segmentation = {}
        # Iterate over each dictionary in the list
        # segmentations = segmentations.to('cpu')
        for d in segmentations:
            for key, value in d.items():
                if key == 'segmentation':
                    value = value
                # Append the value to the list corresponding to the key
                formated_segmentation.setdefault(key, []).append(value)
        
        # torch.cuda.empty_cache()
        
        # print(tensor_images.shape)
        # Define a transform to convert PIL images to tensors

        images_tensor = torch.stack([self.transform(img) for img in images]).permute(0, 2, 3, 1).to(self.device)
        #f2 = time.perf_counter()
        # print(f'current time: {f2-f1}')
        
        #images_tensor = torch.from_numpy(np.array(images)).to(self.device)
        # transform = transforms.ToTensor()
        # # Convert list of PIL images to tensors
        # tensor_images = torch.stack([transform(img) for img in images]).to(self.device)

        # # Permute the dimensions to (N, H, W, C)
        # images_tensor = tensor_images.permute(0, 2, 3, 1)

        detections = self.fuse(images_tensor, formated_segmentation['segmentation'], positions_3d, formated_segmentation['segments_info'])
        fusion_end = time.perf_counter()
        

        timings = {'segmentation':segmentation_end-segmentation_start, 
                   'depth':depth_end-depth_start,
                   '3d-reconstruction':depth_to_3d_end-depth_to_3d_start,
                   'fusion':fusion_end-fusion_start}
        
        return detections, timings


    def depth_to_3d_batch(self, depth_maps):
        """
        Converts a batch of depth maps to 3D points in world coordinates.
        """
        rotation_vectors = self.rotation_vectors
        translation_vectors = self.translation_vectors

        device = self.device
        batch_size, h, w = depth_maps.shape

        depth_maps = depth_maps.unsqueeze(-1)

        if self.distorted_coords == None:
            camera_matrices = self.camera_matrices
            distortion_params = self.distortion_params
            # Create pixel coordinate grid
            u, v = torch.meshgrid(torch.arange(w, device=device), torch.arange(h, device=device), indexing='xy')
            u = u.float()
            v = v.float()

            points = torch.stack((u, v, torch.ones_like(u, device=device)), dim=-1).unsqueeze(0)
            points = points.repeat(batch_size, 1, 1, 1)

            # Normalize pixel coordinates
            fx = camera_matrices[:, 0, 0].view(-1, 1, 1)
            fy = camera_matrices[:, 1, 1].view(-1, 1, 1)
            cx = camera_matrices[:, 0, 2].view(-1, 1, 1)
            cy = camera_matrices[:, 1, 2].view(-1, 1, 1)

            x = (points[..., 0] - cx) / (fx)
            y = (points[..., 1] - cy) / (fy)

            # Compute radial distortion
            r2 = x**2 + y**2
            
            # Expand distortion_params to match depth_maps dimensions
            k1 = distortion_params[:, 0].view(-1, 1, 1).expand(-1, h, w)
            k2 = distortion_params[:, 1].view(-1, 1, 1).expand(-1, h, w)
            k3 = distortion_params[:, 2].view(-1, 1, 1).expand(-1, h, w)
            p1 = distortion_params[:, 3].view(-1, 1, 1).expand(-1, h, w)
            p2 = distortion_params[:, 4].view(-1, 1, 1).expand(-1, h, w)
            
            # Compute distortions
            radial_distortion = 1 + k1 * r2 + k2 * r2**2 + k3 * r2**3
            x_tangential = 2 * p1 * x * y + p2 * (r2 + 2 * x**2)
            y_tangential = p1 * (r2 + 2 * y**2) + 2 * p2 * x * y

            x_distorted = x * radial_distortion + x_tangential
            y_distorted = y * radial_distortion + y_tangential

            self.distorted_coords = torch.stack((x_distorted, y_distorted, torch.ones_like(x, device=device)), dim=-1)
        points_3d = self.distorted_coords * depth_maps
        points_3d_flat = points_3d.view(batch_size, -1, 3)

        def rodrigues_batch(rot_vecs):
            batch_size = rot_vecs.shape[0]
            theta = torch.norm(rot_vecs, dim=1, keepdim=True)
            v = rot_vecs / (theta + 1e-8)
            cos_theta = torch.cos(theta).unsqueeze(-1)
            sin_theta = torch.sin(theta).unsqueeze(-1)

            I = torch.eye(3, device=rot_vecs.device).unsqueeze(0).repeat(batch_size, 1, 1)
            v_cross = torch.zeros((batch_size, 3, 3), device=rot_vecs.device)
            v_cross[:, 0, 1] = -v[:, 2]
            v_cross[:, 0, 2] = v[:, 1]
            v_cross[:, 1, 0] = v[:, 2]
            v_cross[:, 1, 2] = -v[:, 0]
            v_cross[:, 2, 0] = -v[:, 1]
            v_cross[:, 2, 1] = v[:, 0]

            R = cos_theta * I + sin_theta * v_cross + (1 - cos_theta) * torch.bmm(v.unsqueeze(2), v.unsqueeze(1))
            return R

        rotation_matrices = rodrigues_batch(rotation_vectors)
        points_rotated = torch.bmm(points_3d_flat, rotation_matrices.transpose(1, 2))
        points_world = points_rotated + translation_vectors.unsqueeze(1)
        points_world = points_world.view(batch_size, h, w, 3)

        return points_world


    def fuse(self, images, segments, positions, labels):
        detections = []
        for image, segment, position, label_set in zip(images, segments, positions, labels):
            for label in label_set:
                id = label['id']
                label_id = label['label_id']

                indices = torch.where(segment == id)
                colors = image[indices[0], indices[1], :]
                points = position[indices[0], indices[1], :]

                detection = Detection(world_points=points, colors=colors, label=label_id)
                detections.append(detection)
        return detections


    def end(self):
        if self.camera_visualizer_on:
            self.camera_visualizer.destroy()
        if self.depth_visualizer_on:
            self.depth_visualizer.destroy()
        if self.segmentation_visualizer_on:
            self.segmentation_visualizer.destroy()
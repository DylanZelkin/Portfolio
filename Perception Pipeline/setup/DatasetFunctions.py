import numpy as np
from PIL import Image
import matplotlib.pyplot as plt
from matplotlib.colors import ListedColormap
import json

#----------------------------------------------------------------
#              READING FUNCTIONS (NEW) - index the files and general purpose
#----------------------------------------------------------------

def getFileDataLen(file_name, image_size, channels, data_type):
    with open(file_name, 'rb') as file:
        file.seek(0, 2)  # Move to the end of the file (whence=2)
        file_size = file.tell()  # Get the current position, which is the file size
    array_size = image_size**2 * channels * np.dtype(data_type).itemsize
    item_count = file_size//array_size
    return item_count

def readFileData(file_name, index, image_size, channels, data_type): #data_type ex: np.uint8
    array_size = (image_size**2) * channels * np.dtype(data_type).itemsize
    byte_position = array_size * index
    with open(file_name, 'rb') as file:
        file.seek(byte_position)  # Move to the specific byte position
        byte_data = file.read(array_size)
        data = np.frombuffer(byte_data, dtype=data_type).reshape(image_size, image_size, channels)
    return data#.reshape(image_size, image_size, channels)

def load_map_from_json_file(file_path):
    with open(file_path, 'r') as file:
        data = json.load(file)
    
    # Convert keys to int if necessary (JSON keys are always strings)
    id2label = {int(key): value for key, value in data.items()}
    return id2label

#----------------------------------------------------------------
#              READING FUNCTIONS (OLD) - these load an entire file at once
#----------------------------------------------------------------
def readImageData(file_name, width, height):
    channels = 3
    array_size = channels * (height*width)  # Assuming each pixel has RGB components
    all_image_data = []
    # Read the file in chunks of array_size bytes
    with open(file_name, 'rb') as f:
        while True:
            # Read array_size bytes from the file
            byte_data = f.read(array_size)
            
            if not byte_data:
                break  # end of file
            
            # Convert byte data to numpy array with shape (num_pixels, 3)
            image_data = np.frombuffer(byte_data, dtype=np.uint8).reshape(-1, 3)
            
            # Append image data to the list
            all_image_data.append(image_data)
    all_images = [image_data.reshape(height, width, channels) for image_data in all_image_data]
    pil_images = [Image.fromarray(image_data) for image_data in all_images]
    return pil_images


def readDepthData(file_name, width, height):
    channels = 1
    all_image_data = []

    # Define the size of each array
    array_size = channels * (height*width)  # Assuming each pixel has RGB components

    # Read the file in chunks of array_size bytes
    with open(file_name, 'rb') as f:
        while True:
            # Read array_size bytes from the file
            byte_data = f.read(array_size*4)
        
            if not byte_data:
                break  # end of file
                
            if len(byte_data) % array_size*4 != 0:
                print(len(byte_data))
                break  # end of file or incomplete data
            # Convert byte data to numpy array with shape (num_pixels, channels)
            
            image_data = np.frombuffer(byte_data, dtype=np.float32).reshape(height, width, channels)
            #print(image_data.shape)
            # Append image data to the list
            all_image_data.append(image_data)

    # Now all_image_data contains image data from all arrays in the file
    # Reshape each array in all_image_data
    #print(image_data.shape)
    return all_image_data


def readSegmentationData(file_name, width, height):
    channels = 1
    all_image_data = []

    # Define the size of each array
    array_size = channels * (height*width)  # Assuming each pixel has RGB components

    # Read the file in chunks of array_size bytes
    with open(file_name, 'rb') as f:
        while True:
            # Read array_size bytes from the file
            byte_data = f.read(array_size*4)
        
            if not byte_data:
                break  # end of file
                
            if len(byte_data) % array_size*4 != 0:
                print(len(byte_data))
                break  # end of file or incomplete data
            # Convert byte data to numpy array with shape (num_pixels, channels)
            
            image_data = np.frombuffer(byte_data, dtype=np.float32).reshape(-1, channels)
            #print(image_data.shape)
            # Append image data to the list
            all_image_data.append(image_data)

    # Now all_image_data contains image data from all arrays in the file
    # Reshape each array in all_image_data
    #print(image_data.shape)
    all_images = [(image_data.reshape(height, width, channels)).astype(int) for image_data in all_image_data]
    return all_images

#----------------------------------------------------------------
#              VISUALIZATION FUNCTIONS
#----------------------------------------------------------------
def display_images_horizontally(images):
    fig, axes = plt.subplots(1, len(images), figsize=(len(images) * 5, 5))
    for i, img in enumerate(images):
        axes[i].imshow(img)
        axes[i].axis('off')
    plt.show()


def display_depths_horizontally(depths):
    fig, axes = plt.subplots(1, len(depths), figsize=(len(depths) * 5, 5))
    for i, depth_map in enumerate(depths):
        im = axes[i].imshow(depth_map, cmap='inferno')
        axes[i].axis('off')
        fig.colorbar(im, ax=axes[i])
    plt.show()


def display_segmentations_horizontally(segmentations):
    
    fig, axes = plt.subplots(1, len(segmentations), figsize=(len(segmentations) * 5, 5))
    
    for i, segmentation_int in enumerate(segmentations):
        # Get unique values and their indices
        unique_integers, remapped_values = np.unique(segmentation_int, return_inverse=True)
        
        # Create a random color map for each unique value
        num_unique = len(unique_integers)
        random_colors = np.random.rand(num_unique, 3)
        color_map = ListedColormap(random_colors)
        
        # Print colormap colors (for debugging)
        #print(color_map.colors)
        
        # Reshape remapped_values back to the original segmentation shape
        remapped_segmentation = remapped_values.reshape(segmentation_int.shape)
        
        # Display the remapped segmentation with the color map
        axes[i].imshow(remapped_segmentation, cmap=color_map)
        axes[i].axis('off')

    plt.show()
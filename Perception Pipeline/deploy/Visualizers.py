import numpy as np

from PIL import Image, ImageTk
from io import BytesIO
import tkinter as tk
import pyvista as pv
import random

import matplotlib.pyplot as plt
from matplotlib.colors import ListedColormap


class Visualizer(tk.Toplevel):
    def __init__(self, rows, columns, image_size):
        super().__init__()
        self.rows = rows
        self.columns = columns
        self.image_size = image_size
        self.image_labels = []
        self.phots = []
        self.resizable(True, True)
        self.configure(padx=0, pady=0)
        self.setup_ui()

    def setup_ui(self):
        # Configure grid rows and columns
        for i in range(self.rows):
            self.grid_rowconfigure(i, weight=1, pad=0)  # Give equal weight to each row
        for j in range(self.columns):
            self.grid_columnconfigure(j, weight=1, pad=0)  # Give equal weight to each column

        for i in range(self.rows):
            for j in range(self.columns):
                image_label = tk.Label(self, padx=0, pady=0)
                image_label.grid(row=i, column=j, padx=0, pady=0)
                #image_label.pack()
                self.image_labels.append(image_label)
        #self.update()

    def update_images(self, images):
        for i, image in enumerate(images):
            if i >= len(self.image_labels):
                break
            image2 = image.resize((self.image_size, self.image_size), Image.LANCZOS)
            #image2.save('name1.png')
            photo = ImageTk.PhotoImage(image2)

            # Update image label
            self.image_labels[i].configure(image=photo, anchor="center", padx=0, pady=0)
            self.image_labels[i].image = photo
        self.update()

    def close_window(self):
        self.destroy()


class FusionVisualizer(Visualizer):
    def __init__(self, image_size, labels_to_skip, color_dict):
        super().__init__(rows = 1, columns = 2, image_size = image_size)
        self.labels_to_skip = labels_to_skip
        self.color_dict = color_dict
        self.plotter = pv.Plotter(off_screen=True)
        self.custom_size = (image_size, image_size)  # Example size, change as needed
        self.plotter.window_size = self.custom_size  # Set the window size
        self.setup_ui()

    def update_detections(self, detections):
        # Initialize empty lists for all world points and their corresponding colors
        all_x, all_y, all_z = [], [], []
        all_marker_colors = []

        # Loop through all detections
        for detection in detections:
            world_points = detection.world_points.to('cpu').numpy()  # Convert to numpy array
            colors = detection.colors  # Assuming this is numpy array of RGB values
            label = detection.label

            if label in self.labels_to_skip:
                continue
            
            # Generate a random color hue for this detection
            # TODO check if label is in the dictionary, if so use that color, else do the random_hue
            if label in self.color_dict:
                rgb_color = self.color_dict[label]  # Use predefined color
            else:
                rgb_color = (random.random(), random.random(), random.random())
            
            # Assign color to all points in this detection
            detection_color = np.tile(rgb_color, (world_points.shape[0], 1))  # Repeat RGB for each point
            
            # Flatten world points
            x = world_points[..., 0].flatten()
            y = world_points[..., 1].flatten()
            z = world_points[..., 2].flatten()

            # Append the points and colors to global lists
            all_x.append(x)
            all_y.append(y)
            all_z.append(z)
            all_marker_colors.append(detection_color)

        # Concatenate all detections into one array
        all_x = np.concatenate(all_x)
        all_y = np.concatenate(all_y)
        all_z = np.concatenate(all_z)
        all_marker_colors = np.concatenate(all_marker_colors)

        if np.max(all_marker_colors) > 1.0:
            all_marker_colors = all_marker_colors / 255.0
        point_cloud = np.column_stack((all_x, all_y, all_z))
        
        self.plotter.clear()
        self.plotter.add_points(point_cloud, scalars=all_marker_colors, rgb=True, point_size=1.4, opacity=0.8)

        # Set camera position, focal point, and view up vector
        self.plotter.camera.position = [0, -45.5, -7.5]  # Camera position
        self.plotter.camera.focal_point = [0, 0, 1]          # Point to look at
        self.plotter.camera.viewup = [0, 1, 0]               # Defines which direction is up

        # Optional: You can also adjust the camera's view angle if needed
        self.plotter.camera.view_angle = 30  # Adjust view angle (in degrees)

        #print(len(all_x))
        # Render the scene to an image
        self.plotter.render()
        image = self.plotter.screenshot()     # Take a screenshot

        # Convert to PIL Image
        image1 = Image.fromarray(image, mode="RGB")

        # Set camera position, focal point, and view up vector
        self.plotter.camera.position = [0, -4.5, -7.5]  # Camera position
        self.plotter.camera.focal_point = [0, 0, 2]          # Point to look at
        self.plotter.camera.viewup = [0, 1, 0]               # Defines which direction is up

        # Optional: You can also adjust the camera's view angle if needed
        self.plotter.camera.view_angle = 30  # Adjust view angle (in degrees)
        self.plotter.render()
        image = self.plotter.screenshot()     # Take a screenshot
        image2 = Image.fromarray(image, mode="RGB")
        self.update_images([image1, image2])
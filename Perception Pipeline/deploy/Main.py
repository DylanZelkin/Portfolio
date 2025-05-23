from datetime import datetime, timedelta
import sys
import os
import pandas as pd

import SimulationInterface
import Perception
import Visualizers

time_length = 5
num_cameras = 6
image_size = 256
port = 4563

spawn = 2
recording = False
robot_brain_on = True


config = SimulationInterface.RobotConfig(num_cameras = num_cameras, image_size = image_size)
robot = SimulationInterface.Robot(port = port, config = config)
if robot.connected:
    print("connected to game")
else:
    print("didnt connect to game")
    sys.exit()

robot.sendSetupMessage(spawn = spawn, recording = recording)
end_time = datetime.now() + timedelta(minutes=time_length)
print("starting control")

controller = SimulationInterface.PS5()
if(controller.setup):
    print("controller connected")
else:
    print("controller failure")
    sys.exit()
output_folder = "deploy\\Captures"



# Ensure the output folder exists
if not os.path.exists(output_folder):
    os.makedirs(output_folder)

# Function to get the next available image number, even with gaps
def get_next_available_number(output_folder):
    existing_files = os.listdir(output_folder)
    used_numbers = set()

    for filename in existing_files:
        if filename.startswith("image_") and filename.endswith(".jpg"):
            try:
                # Extract the number from the filename
                number = int(filename.split("_")[1].split(".")[0])
                used_numbers.add(number)
            except ValueError:
                pass

    # Find the smallest available number not in used_numbers
    number = 0
    while number in used_numbers:
        number += 1

    return number

labels_to_skip = 118, 22, 122, -1  # ignoring  118 for ceiling, 22 for zebra (i used as null during training)
color_dict = {
    115: (0.4, 0.4, 0.4),  # window
    #122: (0.5, 0.25, 0.1),  # floor
    131: (0.4, 0.4, 0.4), # wall
    109: (0.4, 0.4, 0.4) # wall brick
    # Add more labels and their corresponding colors as needed
}

#when recording we dont care about model running
camera_visualizer = Visualizers.Visualizer(rows = 2, columns = 3, image_size = 256)
if robot_brain_on:
    percieve = Perception.PerceptionPipe(depth_processor_path = "setup\\Models\\Depth", 
                                    depth_model_path = "setup\\Models\\Depth\\results\\checkpoint-81", 
                                    segmentation_processor_path = "setup\\Models\\Segmentation", 
                                    segmentation_model_path = "setup\\Models\\Segmentation\\results\\checkpoint-324", 
                                    camera_params_path = "setup\\calibration_data.npz",
                                    camera_transforms_path = "setup\\camera_setup.txt")
    fusion_visualizer = Visualizers.FusionVisualizer(image_size=350, labels_to_skip=labels_to_skip, color_dict=color_dict)

while datetime.now() < end_time:
    output = robot.readFromRobot()
    input, capturing = controller.getControls()

    if(output != None):
        #print("read")
        if(capturing):
            print('capturing')
            for idx in range(len(output.images)):
                    next_number = get_next_available_number(output_folder)  # Get the next available number
                    filename = f"image_{next_number}.jpg"  # Single continuous number
                    output_path = os.path.join(output_folder, filename)
                    output.images[idx].save(output_path)
        
        camera_visualizer.update_images(output.images)
        if robot_brain_on:
            detections, info = percieve(output.images)
            df = pd.DataFrame({'Timings': info})
            # Transpose the DataFrame to have keys as the index
            df = df.T
            # Display the DataFrame as a table
            print(df)
            
            fusion_visualizer.update_detections(detections=detections)

    if(robot.sendToRobot(input)):
        #print("sent")
        pass

camera_visualizer.destroy()
fusion_visualizer.destroy()
percieve.end()

print("ending control")
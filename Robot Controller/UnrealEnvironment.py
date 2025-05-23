import socket
import struct
import json

import time

import numpy as np
from enum import Enum
import math
import gymnasium as gym
from gymnasium import spaces

from ray.tune.registry import register_env

class UnrealEngineConnection:
    def __init__(self, server_ip = '127.0.0.1', server_port = 4567) -> None:
        self.server_ip = server_ip
        self.server_port = server_port
        self.client_socket = self.new_connection()

    def setup(self, env_id, is_dynamic):
        data = struct.pack("BB", env_id, 1 if is_dynamic else 0)
        self.client_socket.sendall(data)

    def new_connection(self):
        buffer_size = 10000
        new_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        new_socket.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, buffer_size) #TODO potentially add buffer_size if needed...
        try:
            # Connect to the server
            new_socket.connect((self.server_ip, self.server_port))
            new_socket.setblocking(True)

        except ConnectionRefusedError:
            raise RuntimeError("UnrealRLClient couldn't connect to the server! :c")
        return new_socket

    def send_cmd_code(self,value):
        # Ensure the value fits into a single byte (0-255)
        if not (0 <= value <= 255):
            raise ValueError("Code must be between 0 and 255")
        self.client_socket.sendall(bytes([value]))

    def recieve_state(self):
        self.send_cmd_code(0)
        try:
            # Step 1: Receive the 4-byte length prefix
            length_prefix = self.client_socket.recv(4)
            if len(length_prefix) < 4:
                raise ValueError("Failed to receive the message length prefix.")

            # Step 2: Convert the length prefix to an integer
            message_length = struct.unpack("!I", length_prefix)[0]

            # Step 3: Receive the actual JSON message
            json_data = b""
            while len(json_data) < message_length:
                chunk = self.client_socket.recv(message_length - len(json_data))
                if not chunk:
                    raise ValueError("Connection closed before receiving full data.")
                json_data += chunk

            json_str = json_data.decode("utf-8")
            return json.loads(json_str)
        
        except Exception as e:
            print(f"Error receiving json: {e}")
            return None
    
    def send_action(self, json_data):
        self.send_cmd_code(1)
        try:
            # Step 1: Convert the JSON object to a string
            json_str = json.dumps(json_data)

            # Step 2: Encode the string to bytes
            json_bytes = json_str.encode("utf-8")

            # Step 3: Send the 4-byte length prefix
            message_length = struct.pack("!I", len(json_bytes))
            self.client_socket.sendall(message_length)

            # Step 4: Send the actual JSON message
            self.client_socket.sendall(json_bytes)

        except Exception as e:
            
            print(f"Error sending action: {e}")
            print(json_data)

# ------------------------
# ------------------------------------------------
# ------------------------

max_transform = 50
max_rotation = 0.2

def create_transform_space():
    return spaces.Dict({
        "Translation": spaces.Dict({
            'X': spaces.Box(low=-max_transform, high=max_transform, shape=(1,), dtype=np.double),
            'Y': spaces.Box(low=-max_transform, high=max_transform, shape=(1,), dtype=np.double),
            'Z': spaces.Box(low=-max_transform, high=max_transform, shape=(1,), dtype=np.double)
            }),
        "Rotation": spaces.Dict({
            'W': spaces.Box(low=-max_rotation, high=max_rotation, shape=(1,), dtype=np.double),
            'X': spaces.Box(low=-max_rotation, high=max_rotation, shape=(1,), dtype=np.double),
            'Y': spaces.Box(low=-max_rotation, high=max_rotation, shape=(1,), dtype=np.double),
            'Z': spaces.Box(low=-max_rotation, high=max_rotation, shape=(1,), dtype=np.double)
            })
    })

def create_constraint_space():
    return spaces.Dict({
        "Swing1": spaces.Box(low=-2, high=2, shape=(2,), dtype=np.double),
        "Swing2": spaces.Box(low=-2, high=2, shape=(2,), dtype=np.double),
        "Twist": spaces.Box(low=-2, high=2, shape=(2,), dtype=np.double)
    })

def create_constraint_control_space():
    return spaces.Dict({
        "Swing1": spaces.Box(low=-1, high=1, shape=(1,), dtype=np.double),
        "Swing2": spaces.Box(low=-1, high=1, shape=(1,), dtype=np.double),
        "Twist": spaces.Box(low=-1, high=1, shape=(1,), dtype=np.double)
    })

def angle_to_unit_circle(theta):
    x = math.cos(theta)
    y = math.sin(theta)
    return np.array([x, y], dtype=np.double)

def apply_angle_to_nested_dict(d):
    if isinstance(d, dict):
        return {k: apply_angle_to_nested_dict(v) for k, v in d.items()}
    else:
        return angle_to_unit_circle(d) 

bones = ['pelvis', 'spine_02', 'spine_03', 'spine_04', 'spine_05', 'neck_01', 'neck_02', 'head',
         'clavicle_l', 'upperarm_l', 'lowerarm_l', 'hand_l',
         'clavicle_r', 'upperarm_r', 'lowerarm_r', 'hand_r',
         'thigh_l', 'calf_l', 'foot_l', 'ball_l',
         'thigh_r', 'calf_r', 'foot_r', 'ball_r',
         ]
constraints = [bone for bone in bones if bone != 'pelvis']

# ------------------------
# ------------------------------------------------
# ------------------------

class Move(Enum):
    NONE = 0
    FORWARD = 1

class State(Enum):
    WALK = 0
    JOG = 1
    SPRINT = 2

class CharacterEnvironment():
    def __init__(self, envID):
        super().__init__()
        self.action_space = spaces.Dict({
            "Move": spaces.Discrete(len(Move)),
            "State": spaces.Discrete(len(State)),
        })
        self.client_connection = UnrealEngineConnection()
        self.client_connection.setup(envID, is_dynamic=False)
        self.reset()

    def reset(self):
        reset_json = { 'Reset': True }
        self.client_connection.send_action(reset_json)
        state_json = self.client_connection.recieve_state()
        transforms = state_json['Transforms']
        #TODO not sure...
        time.sleep(1.5)
        return transforms
    
    def step(self, action):
        action_strings = {
            "Move": Move(action["Move"]).name,
            "State": State(action["State"]).name
        }
        #print(action_strings)
        formatted_action = {'KeyControls': action_strings}
        self.client_connection.send_action(formatted_action)
        state_json = self.client_connection.recieve_state()
        time_passed = state_json['TimePassed']
        transforms = state_json['Transforms']
        done = state_json['Done']
        
        return transforms, done, time_passed

# ------------------------
# ------------------------------------------------
# ------------------------

def convert_numpy_arrays(data):
    if isinstance(data, np.ndarray):
        #print(data.shape)
        if data.shape == ():
            return data.item()
        #print(data.shape)
        return data.tolist()[0]  # Convert ndarray to list and return the first element (single value)
    elif isinstance(data, dict):
        return {key: convert_numpy_arrays(value) for key, value in data.items()}
    elif isinstance(data, list):
        if len(data) == 1:
            return data[0]  # If list has a single element, return it as the value
        return [convert_numpy_arrays(item) for item in data]
    else:
        return data

class RobotEnvironment():
    def __init__(self, envID):
        super().__init__()
        self.observation_space = spaces.Dict({
            "Bones": spaces.Dict({name: create_transform_space() for name in bones}),
            "Constraints": spaces.Dict({name: create_constraint_space() for name in constraints}),
            "RootOrientation": spaces.Dict({
                'W': spaces.Box(low=-1.0, high=1.0, shape=(1,), dtype=np.double),
                'X': spaces.Box(low=-1.0, high=1.0, shape=(1,), dtype=np.double),
                'Y': spaces.Box(low=-1.0, high=1.0, shape=(1,), dtype=np.double),
                'Z': spaces.Box(low=-1.0, high=1.0, shape=(1,), dtype=np.double)
            }),
            "DeltaTime": spaces.Box(low=0.0, high=1.0, shape=(1,))
        })
        self.action_space = spaces.Dict({
            name: create_constraint_control_space() for name in constraints
        })
        self.client_connection = UnrealEngineConnection()
        self.client_connection.setup(envID, is_dynamic=True)
        self.root_bone = bones[0]
        self.reset()

    def reset(self):
        reset_json = { 'Reset': True }
        self.client_connection.send_action(reset_json)
        state_json = self.client_connection.recieve_state()
        transforms = state_json['Transforms']
        #print(state_json['Constraints'])
        constraints = apply_angle_to_nested_dict(state_json['Constraints'])

        return transforms, constraints

    def step(self, action):
        formatted_action = {'ConstraintPowers': action}
        #print("formatted:", formatted_action)
        converted_action = convert_numpy_arrays(formatted_action)
        #print("converted:", converted_action)
        self.client_connection.send_action(converted_action)
        state_json = self.client_connection.recieve_state()
        time_passed = state_json['TimePassed']
        transforms = state_json['Transforms']
        constraints = apply_angle_to_nested_dict(state_json['Constraints'])
        done = state_json['Done']
        #print("returning step")
        return transforms, constraints, done, time_passed

# ------------------------
# ------------------------------------------------
# ------------------------

def subtract_dicts(d1, d2):
    if isinstance(d1, dict) and isinstance(d2, dict):
        result = {}
        for key in d1.keys() | d2.keys():
            if key in d1 and key in d2:
                result[key] = subtract_dicts(d1[key], d2[key])
            elif key in d1:
                result[key] = d1[key]
            else:
                result[key] = -d2[key]
        return result
    elif isinstance(d1, (int, float)) and isinstance(d2, (int, float)):
        return d1 - d2
    elif isinstance(d1, (int, float)):
        return d1
    elif isinstance(d2, (int, float)):
        return -d2
    else:
        return None  # Handle cases where values are not numeric

def mse_dicts(d1, d2):
    """Recursively computes the Mean Squared Error (MSE) between two nested dictionaries."""
    differences = []

    def recurse(dict1, dict2):
        if isinstance(dict1, dict) and isinstance(dict2, dict):
            for key in dict1.keys() | dict2.keys():
                recurse(dict1.get(key, 0), dict2.get(key, 0))
        elif isinstance(dict1, (int, float)) and isinstance(dict2, (int, float)):
            differences.append((dict1 - dict2) ** 2)
    
    recurse(d1, d2)

    return sum(differences) / len(differences) if differences else 0  # Avoid division by zero

def mae_dicts(d1, d2):
    """Recursively computes the Mean Absolute Error (MAE) between two nested dictionaries."""
    differences = []

    def recurse(dict1, dict2):
        if isinstance(dict1, dict) and isinstance(dict2, dict):
            for key in dict1.keys() | dict2.keys():
                recurse(dict1.get(key, 0), dict2.get(key, 0))
        elif isinstance(dict1, (int, float)) and isinstance(dict2, (int, float)):
            differences.append(abs(dict1 - dict2))
    
    recurse(d1, d2)

    return sum(differences) / len(differences) if differences else 0  # Avoid division by zero

def weighted_mae_dicts(d1, d2, rotation_weight=1.0):
    """Recursively computes the Weighted Mean Absolute Error (MAE) between two nested dictionaries."""
    differences = []
    
    def recurse(dict1, dict2, weight=1.0):
        if isinstance(dict1, dict) and isinstance(dict2, dict):
            for key in dict1.keys() | dict2.keys():
                new_weight = rotation_weight if key == 'Rotation' else weight
                recurse(dict1.get(key, 0), dict2.get(key, 0), new_weight)
        elif isinstance(dict1, (int, float, np.ndarray)) and isinstance(dict2, (int, float, np.ndarray)):
            # Handle np.ndarray by converting to scalar
            val1 = dict1.item() if isinstance(dict1, np.ndarray) else dict1
            val2 = dict2.item() if isinstance(dict2, np.ndarray) else dict2
            differences.append(weight * abs(val1 - val2))
    
    recurse(d1, d2)
    return sum(differences) / len(differences) if differences else 0  # Avoid division by zero

def weighted_mse_dicts(d1, d2, rotation_weight=1.0):
    """Recursively computes the Weighted Mean Squared Error (MSE) between two nested dictionaries."""
    differences = []

    def recurse(dict1, dict2, weight=1.0):
        if isinstance(dict1, dict) and isinstance(dict2, dict):
            for key in dict1.keys() | dict2.keys():
                new_weight = rotation_weight if key == 'Rotation' else weight
                recurse(dict1.get(key, 0), dict2.get(key, 0), new_weight)
        elif isinstance(dict1, (int, float, np.ndarray)) and isinstance(dict2, (int, float, np.ndarray)):
            val1 = dict1.item() if isinstance(dict1, np.ndarray) else dict1
            val2 = dict2.item() if isinstance(dict2, np.ndarray) else dict2
            diff = val1 - val2
            differences.append(weight * (diff ** 2))

    recurse(d1, d2)
    return sum(differences) / len(differences) if differences else 0  # Avoid division by zero

def mae_single_dict(d):
    """Recursively computes the Mean Absolute Error (MAE) between the values in a nested dictionary and zero."""
    differences = []

    def recurse(sub_dict):
        if isinstance(sub_dict, dict):
            for value in sub_dict.values():
                recurse(value)
        elif isinstance(sub_dict, (int, float)):
            differences.append(abs(sub_dict))
    
    recurse(d)
    return sum(differences) / len(differences) if differences else 0  # Avoid division by zero

def mse_single_dict(d):
    """Recursively computes the Mean Squared Error (MSE) between the values in a nested dictionary and zero."""
    differences = []

    def recurse(sub_dict):
        if isinstance(sub_dict, dict):
            for value in sub_dict.values():
                recurse(value)
        elif isinstance(sub_dict, (int, float)):
            differences.append(sub_dict ** 2)

    recurse(d)
    return sum(differences) / len(differences) if differences else 0  # Avoid division by zero

max_action_power = 1
max_action_diff = 2 ** 2
max_transform_diff = (2 * max_transform) ** 2

class UnrealEnvironment(gym.Env):
    def __init__(self, config=None):
        super(UnrealEnvironment, self).__init__()
        
        envID = config.get("envID", 0)
        self.robot = RobotEnvironment(envID)
        self.character = CharacterEnvironment(envID)

        character_action_choice = config.get("action", 0)
        if character_action_choice == 0:
            self.character_action = {
                "Move": Move.NONE,   
                "State": State.WALK  
            }
        elif character_action_choice == 1:
            self.character_action = {
                "Move": Move.FORWARD,  
                "State": State.WALK   
            }
        elif character_action_choice == 2:
            self.character_action = {
                "Move": Move.FORWARD,  
                "State": State.JOG   
            }
        elif character_action_choice == 3:
            self.character_action = {
                "Move": Move.FORWARD,   
                "State": State.SPRINT   
            }

        self.observation_space = self.robot.observation_space
        self.action_space = self.robot.action_space

        self.action_less_energy_weight = config.get('action_less_energy_weight', 0.25)
        self.transformation_rotation_weight = config.get('transformation_rotation_weight', 0.5)
        self.action_diff_scaler = config.get('action_diff_scaler', 0.5)
        self.transform_diff_scaler = config.get('transform_diff_scaler', 1)

        self.max_steps = config.get('max_steps', 500)
        self.reset()

    def reset(self, seed=None, options=None):
        info = {}
        self.steps = 0
        self.first_step = True

        self.previous_robot_transforms = []

        character_transforms = self.character.reset()
        robot_transforms, robot_constraints = self.robot.reset()
        self.previous_robot_transforms.append(robot_transforms)

        bone_movement = subtract_dicts(robot_transforms, self.previous_robot_transforms[-1])
        self.state = {
            "Bones": bone_movement,
            "Constraints": robot_constraints,
            "RootOrientation": robot_transforms[self.robot.root_bone]['Rotation'],
            "DeltaTime": 0
        }
        self.state = self.state      
        self.previous_action = None

        return self.state, info

    def step(self, action):
        self.steps += 1
        terminated = False
        truncated = False
        reward = 0
        info = {}

        action_minimal_energy_diff = mse_single_dict(action)
        amed_clamped = max(0, min(action_minimal_energy_diff, max_action_power))
        action_minimal_energy_diff_scaled = 1-(amed_clamped/max_action_power)
        reward += action_minimal_energy_diff_scaled * self.action_less_energy_weight

        current_time = time.time()
        if self.first_step:
            time_between_actions = 0
            self.first_step = False
        else:
            time_between_actions = current_time - self.action_end_time
            
            action_diff = mse_dicts(action, self.previous_action)
            action_diff_clamped = max(0, min(action_diff, max_action_diff))
            action_diff_scaled = 1-(action_diff_clamped/max_action_diff)
            reward += action_diff_scaled * self.action_diff_scaler

        self.previous_action = action

        info['time_between_actions'] = time_between_actions   

        r_transforms, constraints, r_done, r_time_passed = self.robot.step(action)
        c_transforms, c_done, c_time_passed = self.character.step(self.character_action)
        
        bone_movement = subtract_dicts(r_transforms, self.previous_robot_transforms[-1])
        self.previous_robot_transforms.append(r_transforms)
        
        self.state = {
            "Bones": bone_movement,
            "Constraints": constraints,
            "RootOrientation": r_transforms[self.robot.root_bone]['Rotation'],
            "DeltaTime": r_time_passed
        }    

        transform_diff = weighted_mse_dicts(r_transforms, c_transforms, rotation_weight = max_transform_diff * self.transformation_rotation_weight)
        transform_diff_clamped = max(0, min(transform_diff, max_transform_diff))
        transform_diff_scaled = 1-(transform_diff_clamped/max_transform_diff)
        reward += transform_diff_scaled * self.transform_diff_scaler

        if r_done or c_done: #TODO FIX
             terminated = True
        elif self.steps >= self.max_steps:
            terminated = True
        
        self.action_end_time = time.time()
        action_time = self.action_end_time - current_time
        info['action_time'] = action_time

        return self.state, reward, terminated, truncated, info

    def render(self):
        pass
    
# ------------------------
# ------------------------------------------------
# ------------------------

class UnrealManager():
    def __init__(self):
        self.connection = UnrealEngineConnection()

    def getFPS(self):
        self.connection.send_cmd_code(0)
        fps_json = self.connection.recieve_state()
        fps = fps_json['FPS']
        return fps
    
# ------------------------
# ------------------------------------------------
# ------------------------

imus = ['head']


class DictToGraphWrapper(gym.ActionWrapper, gym.ObservationWrapper): #NOTE: action wrapper must be first here otherwise it wont use action, no idea why the observation is still found
    def __init__(self, env, joints = constraints, imus = imus, joint_feature_dim = 6, imu_feature_dim = 7, joint_actions = 3):
        super().__init__(env)
        self.joints = joints
        self.imus = imus
        self.observation_space = spaces.Dict({
            "joints": spaces.Box(low=-1, high=1, shape=(len(joints), joint_feature_dim+len(joints)), dtype=np.double),
            "imus": spaces.Box(low=-1, high=1, shape=(len(imus), imu_feature_dim), dtype=np.double)
        })
        self.action_space = spaces.Box(low=-1, high=1, shape=(len(joints), joint_actions), dtype=np.double)
        
    def observation(self, observation):
        joints = np.array([])
        for i in range(len(self.joints)):
            constraint_name = self.joints[i]
            constraint = observation['Constraints'][constraint_name]
            Swing1 = constraint['Swing1']
            Swing2 = constraint['Swing2']
            Twist = constraint['Twist']
            ohe = np.eye(len(self.joints))[i]
            joint = np.hstack([Swing1, Swing2, Twist, ohe])
            if joints.size == 0:
                joints = np.empty((0, joint.shape[0]))
                joint = np.atleast_2d(joint)
            joints = np.vstack([joints, joint])

        imus = np.array([])
        for i in range(len(self.imus)):
            bone_name = self.imus[i]
            bone = observation['Bones'][bone_name]
            rotation_dict = bone['Rotation']
            rotation_diff = np.array([rotation_dict['W'], rotation_dict['X'], rotation_dict['Y'], rotation_dict['Z']])
            translation_dict = bone['Translation']
            translation_diff = np.array([translation_dict['X'], translation_dict['Y'], translation_dict['Z']])
            imu = np.hstack([rotation_diff, translation_diff])
            
            if imus.size == 0:
                imus = np.empty((0, imu.shape[0]))
                imu = np.atleast_2d(imu)
            imus = np.vstack([imus, imu])
            
        graph_observation = {
            'joints': joints,
            'imus': imus
        }

        return graph_observation
    
    def action(self, action):
        formatted_action = {}
        for i in range(len(self.joints)):
            joint_name = self.joints[i]
            Swing1 = action[i][0]
            Swing2 = action[i][1]
            Twist = action[i][2]
            formatted_action[joint_name] = {
                'Swing1': np.array(Swing1),
                'Swing2': np.array(Swing2),
                'Twist': np.array(Twist)
            }
        return formatted_action
    
    def step(self, action):
        fixed_action = self.action(action)
        obs = self.env.step(fixed_action)
        o = obs[0]
        o2 = self.observation(o)
        corrected_obs = (o2,)+obs[1:]
        return corrected_obs

def create_unreal_env(config):
    return DictToGraphWrapper(UnrealEnvironment(config))

register_env("UnrealGraphEnv", create_unreal_env)

# ------------------------
# ------------------------------------------------
# ------------------------

class DictToTransformerWrapper(DictToGraphWrapper):
    def __init__(self, env, joints = constraints, imus = imus, joint_feature_dim = 6, imu_feature_dim = 7, joint_actions = 3):
        super().__init__(env)
        self.joints = joints
        self.imus = imus
        self.observation_space = spaces.Dict({
            "joints": spaces.Box(low=-1, high=1, shape=(len(joints), joint_feature_dim), dtype=np.double),
            "imus": spaces.Box(low=-1, high=1, shape=(len(imus), imu_feature_dim), dtype=np.double),
            "clocks": spaces.Box(low=-1, high=1, shape=(1, 1), dtype=np.double),
        })
        self.action_space = spaces.Box(low=-1, high=1, shape=(len(joints), joint_actions), dtype=np.double)

    def observation(self, observation):
        joints = np.array([])
        for i in range(len(self.joints)):
            constraint_name = self.joints[i]
            constraint = observation['Constraints'][constraint_name]
            Swing1 = constraint['Swing1']
            Swing2 = constraint['Swing2']
            Twist = constraint['Twist']
            joint = np.hstack([Swing1, Swing2, Twist])
            if joints.size == 0:
                joints = np.empty((0, joint.shape[0]))
                joint = np.atleast_2d(joint)
            joints = np.vstack([joints, joint])

        imus = np.array([])
        for i in range(len(self.imus)):
            bone_name = self.imus[i]
            bone = observation['Bones'][bone_name]
            rotation_dict = bone['Rotation']
            rotation_diff = np.array([rotation_dict['W'], rotation_dict['X'], rotation_dict['Y'], rotation_dict['Z']])
            translation_dict = bone['Translation']
            translation_diff = np.array([translation_dict['X'], translation_dict['Y'], translation_dict['Z']])
            imu = np.hstack([rotation_diff, translation_diff])
            
            if imus.size == 0:
                imus = np.empty((0, imu.shape[0]))
                imu = np.atleast_2d(imu)
            imus = np.vstack([imus, imu])
            
        clocks = np.empty((0, 1),dtype=np.double)
        clock = np.atleast_2d(observation['DeltaTime'])
        clocks = np.vstack([clocks, clock], dtype=np.double)

        #print("clock type: ", clocks.dtype)

        graph_observation = {
            'joints': joints,
            'imus': imus,
            'clocks': clocks
        }

        return graph_observation

def create_unreal_transformer_env(config):
    return DictToTransformerWrapper(UnrealEnvironment(config))

register_env("UnrealTransformerEnv", create_unreal_transformer_env)


# ------------------------
# ------------------------------------------------
# ------------------------

# print("starting...")
# a = UnrealManager()
# print("connected?...")
# print(f"FPS: {a.getFPS()}")

#uc = UnrealEngineConnection()
# config = {
#     'envID': 0,
#     'action': 0
# }
# ue= UnrealEnvironment(config)
# print(ue.action_space.sample())
# user_input = input("Please enter something: ")
# print(f"You entered: {user_input}")

# env = 0
# action_choice = 0
# action = {}

# config = {
#     'envID': 0,
#     'action': 0
# }

# #print(config.get('envID'))
#print('hi')
# re = CharacterEnvironment(0)
# re.reset()
# input("Enter something: ")
# action = {
#     'Move': Move.FORWARD,
#     'State': State.SPRINT
# }
#action = re.action_space.sample()
# re.step(action)
# input("Enter something: ")
# re.step(action)
# input("Enter something: ")
# re.reset()
# input("Enter something: ")
# re.step(action)
#     print("here")
# ue = UnrealEnvironment(config)
# print(ue.observation_space)    
# state, info = ue.reset()
# print(state)
#     while( True):
#         input("Enter something: ")
#         ue.reset()

# except (socket.error, BrokenPipeError, ConnectionResetError) as e:
#             print(f"Send failed: {e}")
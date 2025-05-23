import socket
import struct
import time
import numpy as np
from PIL import Image
import pygame
from pygame.locals import *


class RobotConfig():
    def __init__(self, num_cameras, image_size):
        self.num_cameras = num_cameras
        self.image_size = image_size #size of a side (assumes square images only)


class RobotOutput():
    def __init__(self, images):
        self.images = images 


class RobotInput():
    def __init__(self, throttle, brake, steering):
        self.throttle = throttle
        self.brake = brake
        self.steering = steering


class Robot():
    def __init__(self, port, config, control_tick_time_ms = 100):
        self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.config = config
        self.buffer_timesteps = 10
        buffer_size = ((self.config.image_size ** 2) * 3) * 4 * (self.buffer_timesteps) # 3 channels, 4 cameras, 10 timesteps
        self.client_socket.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, buffer_size)
        self.last_control_send_time = time.time()
        self.control_tick_time_ms = control_tick_time_ms
        try:
            # Connect to the server
            ip = '127.0.0.1' 
            self.client_socket.connect((ip, port))
            self.connected = True
            
        except ConnectionRefusedError:
            self.connected = False

    def readFromRobot(self):
        #TODO FIRST READ THE IMAGES
        images = []
        bytes_to_read = ((self.config.image_size ** 2) * 3)
        #readable, writable, exceptional = select.select([self.client_socket], [], [], 0)
        if self.client_socket:
            # Query the number of bytes available to be read from the socket
            self.client_socket.setblocking(False)
            try:
                bytes_available = self.client_socket.recv(bytes_to_read*self.config.num_cameras*self.buffer_timesteps, socket.MSG_PEEK) 
                #print("len: "+str(len(bytes_available))+ "needed")
                #print(f'bytes available: {len()}')
                if len(bytes_available) < (bytes_to_read*self.config.num_cameras):
                    return None
                for i in range(len(bytes_available)//(bytes_to_read*self.config.num_cameras)-1):
                    #flush old extras
                    self.client_socket.setblocking(True)
                    byte_data = self.client_socket.recv(bytes_to_read*self.config.num_cameras)
            except BlockingIOError:
                # No data available to be read at the moment
                # print("No data available to be read.")
                return None
        else:
            return None
        self.client_socket.setblocking(True)
        for camera in range(self.config.num_cameras):
            #print("bytes to read", bytes_to_read)
            
            #self.client_socket.setblocking(False)
            byte_data = self.client_socket.recv(bytes_to_read)
            image_data = np.frombuffer(byte_data, dtype=np.uint8).reshape(-1, 3).reshape(self.config.image_size, self.config.image_size, 3)
            image = Image.fromarray(image_data)
            images.append(image)
        output = RobotOutput(images)
        return output

    def sendToRobot(self, input):
        input
        new_time = time.time()
        elapsed_time_ms = (new_time - self.last_control_send_time) * 1000
        if(elapsed_time_ms>self.control_tick_time_ms):
            self.last_control_send_time = new_time
        else:
            return False

        throttle = float(input.throttle)
        brake = float(input.brake)
        steering = float(input.steering)

        byte_stream = struct.pack('f', throttle)
        byte_stream += struct.pack('f', brake)
        byte_stream += struct.pack('f', steering)

        self.client_socket.setblocking(True)
        self.client_socket.sendall(byte_stream)

        return True

    def sendSetupMessage(self, spawn, recording):
        spawn = int(spawn)
        recording = bool(recording)
        
        byte_stream = struct.pack('i', spawn)
        byte_stream += struct.pack('?', recording)

        self.client_socket.setblocking(True)
        self.client_socket.sendall(byte_stream)
        
        return True

class PS5():
    def __init__(self):
        pygame.init()
        pygame.joystick.init()
        if pygame.joystick.get_count() == 0:
            print("No joysticks detected.")
            self.setup = False
        else:
            self.joystick = pygame.joystick.Joystick(0)
            self.joystick.init()
            print("Initialized joystick:", self.joystick.get_name())
            self.setup = True
    
    def getControls(self):
        #axis_indeces:
        left_stick_hz = 0 
        left_trigger = 4 #maps from -1->1
        right_trigger = 5 #maps from -1->1
        
        #buttons:
        triangle = 3

        pygame.event.pump()
        steering = self.joystick.get_axis(left_stick_hz)
        throttle = (self.joystick.get_axis(right_trigger)+1)/2
        brake = (self.joystick.get_axis(left_trigger)+1)/2 

        capturing = self.joystick.get_button(triangle)
        # print(steering)
        # print(throttle)
        # print(brake)
        
        return RobotInput(throttle=throttle, brake=brake, steering=steering), capturing
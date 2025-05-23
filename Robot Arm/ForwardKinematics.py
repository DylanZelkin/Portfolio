import serial.tools.list_ports
import serial
import time
import threading
import numpy as np
import matplotlib.pyplot as plt
import copy

import torch
import torch.nn as nn
import torch.nn.functional as F
import random

# =======================================================================
# ROBOT CONFIGURATION
# =======================================================================

# Get a list of available serial ports
def displaySerialPorts():
    ports = serial.tools.list_ports.comports()
    for port, desc, hwid in sorted(ports):
        print(f"Port: {port}, Description: {desc}, Hardware ID: {hwid}")


def connectToRobot(port_name, baud_rate):
    # Create a serial object
    ser = serial.Serial(port_name, baud_rate, timeout=1)  # Adjust timeout as needed
    try:
        # Check if the port is open
        if ser.is_open:
            print(f"Connected to {port_name} at {baud_rate} baud.")
        else:
            print(f"Failed to open {port_name}. Check the port and try again.")
    finally:
        return ser

def closeRobot(serial):
    serial.close()
    print("Serial port closed.")


# =======================================================================
# ROBOT COMMUNICATION
# =======================================================================

def sendCommand(serial, servo_id, cmd):
    cmd = '#'+str(servo_id)+cmd+'\r'
    serial.write(cmd.encode())

def readBack(serial):
    response = b""  # Using bytes to store the received data
    while True:
        char = serial.read()
        response += char
        if char == b'\r':
            break

    # Convert the bytes to a string
    return response.decode()

def sendAction(serial, servo_id, action_cmd, action_val):
    sendCommand(serial, servo_id, action_cmd + str(action_val))

def sendModifiedAction(servo_id, action_cmd, action_val, modifier_cmd, modifier_val):
    pass

def sendQuery(serial, servo_id, query_cmd):
    sendCommand(serial, servo_id, query_cmd)

def sendConfig():
    pass

def getServoPos(serial, servo_id):
    sendQuery(serial, servo_id, 'QD')
    return readBack(serial)

def getServoPosChain(serial, num_servos):
    outs = []
    for i in range(num_servos):
        response = getServoPos(serial, i+1)
        # Extracting the relevant substring (discarding first 4 and last 1 characters)
        substring = response[4:-1]
        # Converting the substring to a float and dividing by 10
        result = float(substring) / 10.0
        outs.append(result)
    return outs

def driveServo(serial, servoId, degs_per_sec):
    sendAction(serial, servoId, "WD", degs_per_sec)
    return readBack(serial)

def relaxServo(serial, servoId):
    sendAction(serial, servoId, "L", "")
    return

def freezeServo(serial, servoId):
    sendAction(serial, servoId, "H", "")
    return

# =======================================================================
# FORWARD KINEMATICS
# =======================================================================

class Joint:
    def __init__(self, a,alpha,d,theta):
        self.a = a
        self.alpha = np.deg2rad(alpha)
        self.d = d
        self.theta = np.deg2rad(theta)
        self.theta_norm = self.theta

    def get_T(self):
        return [[np.cos(self.theta), -np.sin(self.theta)*np.cos(self.alpha), np.sin(self.theta)*np.sin(self.alpha), self.a*np.cos(self.theta)],
                [np.sin(self.theta), np.cos(self.theta)*np.cos(self.alpha), -np.cos(self.theta)*np.sin(self.alpha), self.a*np.sin(self.theta)],
                [0, np.sin(self.alpha), np.cos(self.alpha), self.d],
                [0,0,0,1]
                ]

def compute_forward_kinematics(joints):
    Ts = []
    T0 = joints[0].get_T()
    Ts.append(copy.deepcopy(T0))
    for i in range(1, len(joints)):
        T1 = joints[i].get_T()
        T2 = np.dot(T0, T1)
        Ts.append(copy.deepcopy(T2))
        T0=T2
    return Ts


fig = plt.figure(figsize=(6, 6))
ax = fig.add_subplot(111, projection='3d')
def update_plot(skeleton_points, fig=fig, ax=ax):
    ax.clear()
    ax.set_box_aspect([1,1,1])

    skeleton_points = np.array(skeleton_points)
    ax.scatter(skeleton_points[:, 0], skeleton_points[:, 1], skeleton_points[:, 2], c='r', marker='o')
    for i in range(len(skeleton_points)-1):
        ax.plot(skeleton_points[i:i+2, 0], skeleton_points[i:i+2, 1], skeleton_points[i:i+2, 2], c='b')

    ax.set_xlabel('X')
    ax.set_ylabel('Y')
    ax.set_zlabel('Z')
    # Set limits to ensure constant plot space size
    ax.set_xlim([-10, 30])
    ax.set_ylim([-30, 30])
    ax.set_zlim([0, 40])

    plt.pause(0.05)
    plt.show(block=False)

# =======================================================================
# ROBOT CONTROLLER
# =======================================================================

def cmd_arm(joints, joint_dps):
    joints = joints[1:]
    success = []
    for i in range(len(joints)):
        #if dps too high then decline
        #if going past joint limit then decline
        pass
    return success

def freeze_arm(serial, num_motors):
    for servo in range(num_motors):
        freezeServo(serial, servo)

def relax_arm(serial, num_motors):
    for servo in range(num_motors):
        relaxServo(serial, servo)

# =======================================================================
# MAIN
# =======================================================================

def user_controls():
    global user_in, running
    while(running):
        user_input = input("Enter Command: ")
        lock.acquire()
        try:
            user_in = user_input
        finally:
            lock.release()
user_in = ""
running = True
lock=threading.Lock()
thread = threading.Thread(target=user_controls)

displaySerialPorts()
port_name = 'COM7'
baud_rate = 115200
serial = connectToRobot(port_name, baud_rate)

num_motors = 4
joints = [  #a,alpha,d,theta
    Joint(0, 180, 0, 0),
    Joint(0, -90, -10.5, 0),
    Joint(-14.2215, 0, 0, -79.8745),
    Joint(-16.5, 0, 0, 79.8745),
    Joint(-4.5, 0, 0, 0)
]

robot_running = False
thread.start()
while(True):
    arm_ang = getServoPosChain(serial, num_motors)
    for i in range(len(arm_ang)):
        joints[i+1].theta = joints[i+1].theta_norm+np.deg2rad(arm_ang[i])
    Ts = compute_forward_kinematics(joints)

    joint_pos = []
    for T in Ts:
        point = []
        for j in range(len(T)):
            if(j<3): #3 since the fourth isnt useful to us
                point.append(T[j][3]) #3 is the point pos in the matrix
        joint_pos.append(point)

    update_plot(joint_pos)

    lock.acquire()
    try:
        if(user_in != ""):
            robot_running = False
            if(user_in == "relax"):
                relax_arm(serial, num_motors)
            elif(user_in == "freeze"):
                freeze_arm(serial, num_motors)
            elif(user_in == "run"):
                robot_running = True
            elif(user_in == "exit"):
                running = False
                break
            user_in = ""
    finally:
        lock.release()

    if(robot_running): 
        state = (joint_pos)
        #run_robot(serial, state)
        

thread.join()

closeRobot(serial)

#xovsoG-8rebqa-gimken
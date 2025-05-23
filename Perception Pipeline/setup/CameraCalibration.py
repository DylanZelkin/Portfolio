import numpy as np
import cv2
import glob

# Define the number of inner corners in the checkerboard pattern
CHECKERBOARD_SIZE = (8, 5)  # Change this if your checkerboard has a different size

# Prepare object points, like (0,0,0), (1,0,0), (2,0,0) ....,(8,5,0)
objp = np.zeros((CHECKERBOARD_SIZE[0]*CHECKERBOARD_SIZE[1],3), np.float32)
objp[:,:2] = np.mgrid[0:CHECKERBOARD_SIZE[0], 0:CHECKERBOARD_SIZE[1]].T.reshape(-1,2)\

criteria = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 30, 0.001)

# Arrays to store object points and image points from all the images
objpoints = [] # 3D points in real world space
imgpoints = [] # 2D points in image plane

# Load images
images = glob.glob('setup\CalibrationImages\*.jpg')  # Change the path to your images directory
print(images)
images_failed = 0
images_denied = 0
images_accepted = 0

# Iterate through each image
for fname in images:
    img = cv2.imread(fname)
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

    #print(gray.shape)
    # Find the chessboard corners
    blurred = cv2.GaussianBlur(gray, (5,5), 0)
    thresh = cv2.adaptiveThreshold(blurred, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C, cv2.THRESH_BINARY, 11, 2)
    flags = cv2.ADAPTIVE_THRESH_GAUSSIAN_C
    ret, corners = cv2.findChessboardCorners(gray, CHECKERBOARD_SIZE, flags)
    #print(corners)
    # If corners are found, add object points, image points    
    if ret == True:
        #print("hi")
        corners2 = cv2.cornerSubPix(gray, corners, (11,11),(-1,-1), criteria)

        # Draw and display the corners
        img = cv2.drawChessboardCorners(img, CHECKERBOARD_SIZE, corners, ret)
        cv2.imshow('img', img)
        cv2.waitKey(1)
        #cv2.waitKey(500)  # Adjust the delay as needed
        
        #is accpetable?
        response = input("Acceptable? enter Y/y for yes: ").strip().upper()
        if response == 'Y' or response == 'y':
            print("image accepted")
            images_accepted+=1
            objpoints.append(objp)
            imgpoints.append(corners2)
        else:
            print("image denied")
            images_denied+=1
        cv2.destroyAllWindows()
    else:
        print("image failure")
        images_failed += 1

rate = images_accepted/(images_accepted+images_denied+images_failed)
print(f'success rate = {rate:.2f}')

# Calibrate camera
ret, mtx, dist, rvecs, tvecs = cv2.calibrateCamera(objpoints, imgpoints, gray.shape[::-1], None, None)
#cv2.calibra

# Print calibration results
print("Success:")
print(ret)
print("Camera matrix:")
print(mtx)
print("\nDistortion coefficients:")
print(dist)

# save results
np.savez("setup\\calibration_data.npz", mtx=mtx, dist=dist)
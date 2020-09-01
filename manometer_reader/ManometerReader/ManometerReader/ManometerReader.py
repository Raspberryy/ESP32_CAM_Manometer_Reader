import cv2
import numpy as np
import math
from matplotlib import pyplot as plt
import time

start = time.time()
scale_percent = 100
img = cv2.imread('ex2.jpg',0)

# Scale down image
width = int(img.shape[1] * scale_percent / 100)
height = int(img.shape[0] * scale_percent / 100)
dim = (width, height)
img = cv2.resize(img, dim, interpolation = cv2.INTER_AREA)

# Prepare Detection
imr = cv2.medianBlur(img,5)
imr = cv2.threshold(imr, 100, 255, cv2.THRESH_TOZERO)[1] 
print("[*] Image preprocessing\tSUCCESS\t\t\t" + str(int((time.time()-start)*1000)) + "ms")

# Circle Detection
circles = cv2.HoughCircles(imr,cv2.HOUGH_GRADIENT,1,20,param1=200,param2=100,minRadius=0,maxRadius=0)

if (circles is None):
    print("[!] Circle detection\tFAILED\tNone found\t" + str(int((time.time()-start)*1000)) + "ms")
else:
    print("[*] Circle detection\tSUCCESS\tFound: "+ str(len(circles)) +"\t" + str(int((time.time()-start)*1000)) + "ms")
    circles = np.uint16(np.around(circles))
    maxRad = -1;
    max = []

    for i in circles[0,:]:
        if (i[2] > maxRad and 0.45 <= i[0]/imr.shape[0] <= 0.55 and 0.45 <= i[1]/imr.shape[1] <= 0.55):
            max = i
            maxRad = i[2]
    
    edges = cv2.Canny(imr, 20, 50)
    rho = 1  # distance resolution in pixels of the Hough grid
    theta = np.pi / 180  # angular resolution in radians of the Hough grid
    threshold = 15  # minimum number of votes (intersections in Hough grid cell)
    min_line_length = 50  # minimum number of pixels making up a line
    max_line_gap = 20  # maximum gap in pixels between connectable line segments
    line_image = np.copy(img) * 0  # creating a blank to draw lines on

    # Run Hough on edge detected image
    # Output "lines" is an array containing endpoints of detected line segments
    lines = cv2.HoughLinesP(edges, rho, theta, threshold, np.array([]),
                        min_line_length, max_line_gap)
    avgSlope = 0
    linecntr = 0

    if(len(max) == 0):
        print("No circle in image center found")
    else:
        for line in lines:
            for x1,y1,x2,y2 in line:
                dist = math.sqrt( pow(y2-max[1],2) + pow(x2-max[0],2)) + math.sqrt( pow(y1-max[1],2) + pow(x1-max[0],2))
                if (abs( (y2-y1)*max[0] - (x2-x1)*max[1] + x2*y1 - x1*y2 ) / math.sqrt( pow(y2-y1,2) + pow(x2-x1,2))) < 30 and dist <= (max[2]):
                    cv2.line(imr,(x1,y1),(x2,y2),(0,0,0),2)
                    linecntr+=1
                    steigung = (y2-y1)/(x2-x1)
                    if steigung < 0:
                         avgSlope += 180 - (math.atan(abs((y2-y1)/(x2-x1))) * 180 / math.pi)
                    else: 
                        avgSlope += (math.atan(abs((y2-y1)/(x2-x1))) * 180 / math.pi)
                

        avgSlope = avgSlope/linecntr
        print(avgSlope)
        xcirc = int(max[0]+math.cos(avgSlope * math.pi / 180) * max[2])
        ycirc = int(max[1]-math.sin(avgSlope * math.pi / 180) * max[2])
        cv2.line(imr,(max[0],max[1]),(xcirc,ycirc),(255,0,0),5)

        # draw the outer circle
        cv2.circle(imr,(max[0],max[1]),max[2],(255,0,255),2)
        # draw the center of the circle
        cv2.circle(imr,(max[0],max[1]),2,(0,0,255),3)
    

cv2.imshow('detected circles',imr)
cv2.waitKey(0)

cv2.destroyAllWindows()
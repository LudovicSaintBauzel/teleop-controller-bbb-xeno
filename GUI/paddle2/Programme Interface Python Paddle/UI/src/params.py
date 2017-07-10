#!/usr/bin/env python

#Code written by Lucas Roche
#March 2015

import pygame, sys, math, os
from pygame.locals import *
import random
import socket
import threading
import time


#DECLARATION OF THE DIFFERENT PARAMETERS #########################

START = False

#Screens positions
SCREEN_POSITION_1 = (1500, 250)
SCREEN_POSITION_2 = (250, 250)

# Window size
WINDOW_WIDTH = 800
WINDOW_LENGTH = 600

# Scrolling speed (pixel/second)
VITESSE = 120 # /!\ garder un ratio VITESSE/FPS entier sous peine de desynchronisation
FPS=30
DEPLACEMENT = int(float(VITESSE)/float(FPS))

# Path (surface) size
PATH_WIDTH=WINDOW_WIDTH
PATH_DURATION = 161
PATH_LENGTH=PATH_DURATION*VITESSE
PATH_LENGTH_TRAINING = 30*VITESSE

# Path (line) width
LINE_WIDTH=4
LINE_WIDTH_BOLD=LINE_WIDTH*10

#Vertical position of the cursor
Y_POS_CURSOR = WINDOW_LENGTH-100

# Duration of the path's subparts (second)
PART_DURATION_BODY   = 1
PART_DURATION_CHOICE = 1
PART_DURATION_FORK   = 3
PART_DURATION_REGRP  = 1
PART_DURATION_START  = 2


# Dimensions of the path's subparts (pixels)
PART_LENGTH_BODY   = PART_DURATION_BODY*VITESSE
PART_LENGTH_CHOICE = PART_DURATION_CHOICE*VITESSE
PART_LENGTH_FORK   = PART_DURATION_FORK*VITESSE
PART_LENGTH_REGRP  = PART_DURATION_REGRP*VITESSE

PART_WIDTH = PATH_WIDTH
GAUCHE= 2*PART_WIDTH/5
MILIEU= PART_WIDTH/2
DROITE= 3*PART_WIDTH/5

#Creating the vector containing the reference positions
PATH_POS=[None]*PATH_LENGTH
PATH_POS_TRAINING=[None]*PATH_LENGTH_TRAINING

# Size of the cursor
CURSOR_WIDTH = 15
CURSOR_LENGTH = 15

#Define colors
WHITE = (255, 255, 255)
BLACK = (0, 0, 0)
GREY = (59, 59, 59)
LIGHTGREY = (110, 110, 110)
BLUE = (0, 0, 255)
GREEN = (0, 255, 0)
YELLOW = (255, 255, 0)
ORANGE = (255, 80, 0)
RED = (255, 0, 0)


# Sockets parameters
data = 0.0
thread_alive = True

UDP_IP = "10.0.0.1"     # IP address of UI computer
UDP_IP2 = "10.0.0.2"    # IP address of paddle computer
TCP_IP = "10.0.0.1"
UDP_PORT1 = 5005    # Retrieve position of subject 1 in UI 1
UDP_PORT2 = 5006    # Retrieve position of subject 2 in UI 1
UDP_PORT3 = 5007    # Send record start and stop messages (UI 1)
UDP_PORT4 = 5008    # Retrieve position of subject 1 in UI 2
UDP_PORT5 = 5009    # Retrieve position of subject 2 in UI 2
TCP_PORT1 = 5010    # Receive pause signals in UI 1
TCP_PORT2 = 5011    # Receive pause signals in UI 2
TCP_PORT3 = 5012    # Send part changes messages (UI 1)
TCP_PORT4 = 5013    # Send part changes messages (UI 2)


#Fork type "DROIT" or "COURBE"
FORK_TYPE = "DROIT"

#Aquisition parameters
AQUISITION_FREQUENCY = 2000.0

#Cursor control parameters
POSITION_OFFSET = 3450 #number of ticks to the middle position
SENSIBILITY = 3 #ratio : cursor pos/ handle pos


####################################################################################
# Obstacle type UI specific parameters
####################################################################################

DUREE_COURTE = 2
DUREE_LONGUE = 4

#Creating the vector containing the obstacles positions
OBSTACLE=[0]*PATH_LENGTH
OBSTACLE_RADIUS_MIN = LINE_WIDTH_BOLD/2
OBSTACLE_RADIUS_MAX = 2*LINE_WIDTH_BOLD
GRILLE_OBST_X = PATH_WIDTH/10
GRILLE_OBST_Y = PATH_WIDTH/10
CHANCE_OBST=0.1

#!/usr/bin/env python

#Code written by Lucas Roche
#March 2015

############################################################################################################################################
#IMPORTS 
############################################################################################################################################
import pygame, sys, math, os
from pygame.locals import *
import random
import socket
import threading
import time

from params import *


#Define the destination screen (for subject 1 or 2)
SUJET = int(sys.argv[2])


#Define position of the window on the screen
if SUJET == 1:
    position = SCREEN_POSITION_1
    os.environ['SDL_VIDEO_WINDOW_POS'] = str(position[0]) + "," + str(position[1])
elif SUJET == 2:
    position = SCREEN_POSITION_2
    os.environ['SDL_VIDEO_WINDOW_POS'] = str(position[0]) + "," + str(position[1])


#Initialisation of pygame, creation of the main window
pygame.init()
FPSCLOCK = pygame.time.Clock()
DISPLAYSURF= pygame.display.set_mode((WINDOW_WIDTH, WINDOW_LENGTH))
DISPLAYSURF.fill(GREY)
TITLE = 'UI v7 - Sujet ' + str(SUJET)
pygame.display.set_caption(TITLE)
pygame.display.update()


fontObj = pygame.font.Font('freesansbold.ttf', 59)
textSurfaceObj = fontObj.render('Saturation moteurs', True, RED)
textRectObj = textSurfaceObj.get_rect()
textRectObj.center = (WINDOW_WIDTH/2, WINDOW_LENGTH/2)


if SUJET ==1:
    sock1 = socket.socket(socket.AF_INET, # Internet
                      socket.SOCK_DGRAM) # UDP
    sock1.bind((UDP_IP, UDP_PORT1))

    sock_envoi = socket.socket(socket.AF_INET, # Internet
                      socket.SOCK_DGRAM) # UDP
    sock_envoi.bind((UDP_IP, UDP_PORT3))

    sock2 = socket.socket(socket.AF_INET, # Internet
                      socket.SOCK_DGRAM) # UDP
    sock2.bind((UDP_IP, UDP_PORT2))
    
    sock3 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock3.bind((TCP_IP, TCP_PORT1))

if SUJET ==2:
    sock1 = socket.socket(socket.AF_INET, # Internet
                      socket.SOCK_DGRAM) # UDP
    sock1.bind((UDP_IP, UDP_PORT4))

    sock2 = socket.socket(socket.AF_INET, # Internet
                      socket.SOCK_DGRAM) # UDP
    sock2.bind((UDP_IP, UDP_PORT5))

    sock3 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock3.bind((TCP_IP, TCP_PORT2))
    
#    sock4 = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
#    sock4.bind((UDP_IP, TCP_PORT3))    
    
############################################################################################################################################
#FUNCTIONS 
############################################################################################################################################


############################################################################################################################################
#Thread used to recover data from the robot
def receive_data():
    global pos1, pos2, thread_alive
    while thread_alive:
        #Read socket to get info about cursor
        data1, addr = sock1.recvfrom(1024)
        data1 = float(data1[0:data1.find("\n")])
        pos1 = (data1-POSITION_OFFSET)/POSITION_OFFSET*SENSIBILITY
        data2, addr = sock2.recvfrom(1024)
        data2 = float(data2[0:data2.find("\n")])
        pos2 = (data2-POSITION_OFFSET)/POSITION_OFFSET*SENSIBILITY

############################################################################################################################################


############################################################################################################################################
#Thread used to recover pause signals
def receive_pause_signal():
    global DEPL, thread_alive
    while thread_alive:
        #Read socket to get info about cursor
        while thread_alive:
            sock3.listen(1)

            conn3, addr3 = sock3.accept()
            print 'Connection address:', addr3
            while (1):
                data3 = conn3.recv(1024)
                if not data3: break
                data3 = data3[0:data3.find("\n")]
                if data3 == 'PAUSE':
                    DEPL = 0
                elif data3 == 'UNPAUSE':
                    DEPL = DEPLACEMENT
                print "received data:", data3
                conn3.send(data3)  # echo        
                
        conn3.close()
        
############################################################################################################################################



############################################################################################################################################
#Thread used to send path positions to the paddle
def send_part_changes():
    global ptChgEvent, ptChgMessage, thread_alive
    while thread_alive:
        ptChgEvent.wait()
        if thread_alive == False:
            break

#        sock4.sendto(ptChgMessage, (UDP_IP2, TCP_PORT3))
#        time.sleep(0.2)
        if SUJET == 1:
            port = TCP_PORT3
        elif SUJET == 2:
            port = TCP_PORT4
        
        sock4 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            sock4.connect((UDP_IP2, port))
            sock4.send(ptChgMessage)
            data4 = sock4.recv(1024)
            sock4.close()
        except:
#            print "Failed to connect"
            sock4.close()

#        print "received callback from address:", data4


############################################################################################################################################


############################################################################################################################################
# MAIN
###############
def main():
    global PATH, partChanges, FPSCLOCK, DISPLAYSURF, CURSOR, results_file, pos1, pos2, thread_alive, START, sock_envoi, DEPL, ptChgEvent, ptChgMessage
    results_str=""
    file_name=""
    pos1=0.0
    pos2=0.0
    dist = 0
    cursorColor = GREEN
    DEPL = DEPLACEMENT
        
    t = threading.Thread(group=None, target=receive_data, args = ())
    t.start()
    t2 = threading.Thread(group=None, target=receive_pause_signal, args = ())
    t2.start()
    
    ptChgEvent = threading.Event()
    ptChgEvent.clear()
    t3 = threading.Thread(group=None, target=send_part_changes, args = ())
    t3.start()
    
    #Starting positions of cursor and path
    Y_PATH= -PATH_LENGTH
    X_POS_CURSOR = WINDOW_WIDTH/2

    # Generate the cursor and path objects
    generatePATH()
    generateCURSOR(CURSOR_WIDTH, CURSOR_LENGTH, cursorColor)
    if SUJET == 2 and int(sys.argv[1]) > 100 and int(sys.argv[1]) <= 300:
        generateObstacle()


    #Synchronization of the 2 subjects
    while START == False:
        if time.time() >= float(sys.argv[3]):
            START = True

    #Sending signal to start registering data on the robot
    if SUJET==1:
        sock_envoi.sendto("START\0", (UDP_IP2, UDP_PORT3))


    while True: #main loop

        #Refresh the base display
        if Y_PATH <= - PATH_LENGTH + Y_POS_CURSOR or Y_PATH >= 0:
            DISPLAYSURF.fill(GREY)

        #Refresh the path position
        DISPLAYSURF.blit(PATH, (0,Y_PATH))
	#DISPLAYSURF.blit(OBSTACLE_SURF, (0, Y_PATH))
        Y_PATH += DEPL
        if DEPL == 0:
            DISPLAYSURF.blit(textSurfaceObj, textRectObj)
        
        if Y_PATH >= WINDOW_LENGTH: #Stops the program when the path is finished
            terminate()            
        checkForQuit()

        #Update cursor position

        OLD_X_POS_CURSOR = X_POS_CURSOR

        pos_moy = (pos1 + pos2 )/2

        try:
            X_POS_CURSOR = WINDOW_WIDTH/2*(1 - pos_moy)
        except:
            X_POS_CURSOR = OLD_X_POS_CURSOR	

        if (Y_PATH + PATH_LENGTH >= Y_POS_CURSOR+1 and Y_PATH <= Y_POS_CURSOR):
            pathPosX = PATH_POS[Y_POS_CURSOR - Y_PATH]
            if pathPosX == GAUCHE or pathPosX == DROITE or pathPosX == 10000:
                diff = min(abs(X_POS_CURSOR - GAUCHE),abs(X_POS_CURSOR - DROITE))
            else:                
                diff = abs(X_POS_CURSOR - pathPosX)
        
            if 0 <= diff and diff <= 15:
                cursorColor = GREEN
            elif 15 <= diff and diff <= 30:
                cursorColor = YELLOW
            elif 30 <= diff and diff <= 45:
                cursorColor = ORANGE
            else:
                cursorColor = RED
                
            if partChanges[Y_POS_CURSOR - Y_PATH] != 0:
                ptChgEvent.set()
                ptChgMessage = partChanges[Y_POS_CURSOR - Y_PATH] + '\0'
                ptChgEvent.clear()
                

	#print X_POS_CURSOR
        generateCURSOR(CURSOR_WIDTH, CURSOR_LENGTH, cursorColor)
        DISPLAYSURF.blit(CURSOR, (X_POS_CURSOR-CURSOR_WIDTH/2, Y_POS_CURSOR-CURSOR_LENGTH/2))
        
        #Refresh 
        pygame.display.update()
        FPSCLOCK.tick(FPS)

    return 0;
############################################################################################################################################


############################################################################################################################################
#TERMINATE
##########
def terminate():
    global thread_alive, ptChangeEvent
    thread_alive = False
    if SUJET==1:
        sock_envoi.sendto("STOP\0", ("10.0.0.2", UDP_PORT3))
        sock_envoi.close()
        sock_kill = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock_kill.connect((TCP_IP, TCP_PORT1))
        sock_kill.close()     
    elif SUJET == 2:
        sock_kill = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock_kill.connect((TCP_IP, TCP_PORT2))
        sock_kill.close()
    ptChgEvent.set()
    print "UI", SUJET, " exiting"
    pygame.quit()
    sys.exit()
############################################################################################################################################


############################################################################################################################################
#CHECK FOR QUIT
###############
def checkForQuit():
    for event in pygame.event.get(QUIT): # get all the QUIT events
        terminate() # terminate if any QUIT events are present
    for event in pygame.event.get(KEYUP): # get all the KEYUP events
        if event.key == K_ESCAPE:
            terminate() # terminate if the KEYUP event was for the Esc key
        pygame.event.post(event) # put the other KEYUP event objects back
############################################################################################################################################


############################################################################################################################################
# GENERATE CURSOR
#################
def generateCURSOR(CURSOR_WIDTH, CURSOR_LENGTH, cursorColor): #generate the cursor shape
    global CURSOR
    CURSOR = pygame.Surface((CURSOR_WIDTH,CURSOR_LENGTH), pygame.SRCALPHA)
    CURSOR.fill((0,0,0,0))
    pygame.draw.ellipse(CURSOR, cursorColor, (0,0,CURSOR_WIDTH,CURSOR_LENGTH))
############################################################################################################################################


############################################################################################################################################
#GENERATE PATH
##############
def generatePATH():
    global PATH, partChanges
    PATH=pygame.Surface((PATH_WIDTH, PATH_LENGTH))
    PATH.fill(BLACK)

    #Generate the path assembling predefined subparts, and update the reference vector
    TYPE = 0
    SUBTYPE = 0
    scenario_number= sys.argv[1]
    scenario_file_name = "../scenarios/SCENARIO_" + scenario_number + ".txt"
    scenarioFile = open(scenario_file_name,'r')
    Y_POS = PATH_LENGTH
    limite = PART_LENGTH_FORK
    partLength = 0 
    partWithObstacle = 0  
    
    partChanges = [0]*PATH_LENGTH

    # The loop is reversed to start at the bottom of the path's surface ( = at the start)
#------------------------------------  
    if  (100 <= int(scenario_number) and int(scenario_number) < 300): #Scenarii 101-300 : Trials with obstacles
        while Y_POS > 0:
            if Y_POS == PATH_LENGTH: #Impose 2 sec of straight line at the start of the path
                TYPE = 0
                SUBTYPE = 0
                partLength = PART_DURATION_START*VITESSE 
            elif Y_POS <= limite: #Impose a straight line at the end of the path
                TYPE = 0
                SUBTYPE = 0
                partLength = Y_POS
            else:   #Read the scenario file to build the path according to the predetermined sequency
                line = scenarioFile.readline()
                TYPE     = int( line[ line.find("_TYPE") + 6 ])
                SUBTYPE  = int( line[ line.find("_SUBTYPE") + 9  ]) 
                partLength = PART_LENGTH_BODY           
                if (TYPE == 1 or TYPE == 2) :
                    partLength = DUREE_COURTE*VITESSE
                elif (TYPE == 3 or TYPE == 4):
                    partLength = DUREE_LONGUE*VITESSE

            try:
                generate_PART_Obstacles(PATH, partLength, Y_POS - partLength, TYPE, SUBTYPE)
            except:
                print "Failed to generate the path"
 #           try:
 #               generatePositionVector_Obstacles(partLength, Y_POS - partLength, TYPE, SUBTYPE)
 #           except:
#                print "Failed to generate the position vector"
                
            Y_POS = Y_POS - partLength #Decrement
             
#------------------------------------  
    else: #Scenarii 1-100 (or others) Trials type Groten
        while Y_POS > 0:
            if Y_POS == PATH_LENGTH: #Impose a straight line at the start of the path
                TYPE = 0
                SUBTYPE = 0
                partLength = PART_DURATION_START*VITESSE
            elif Y_POS <= limite: #Impose a straight line at the end of the path
                TYPE = 0
                SUBTYPE = 0
                partLength = Y_POS
            else:   #Read the scenario file to build the path according to the predetermined sequency
                line = scenarioFile.readline()
                TYPE     = int( line[ line.find("_TYPE") + 6 ])
                SUBTYPE  = int( line[ line.find("_SUBTYPE") + 9  ])
                partLength = PART_LENGTH_BODY           
                if TYPE ==3 :
                    partLength = PART_LENGTH_FORK
                elif TYPE == 0 and SUBTYPE == 0:
                    partLength = PART_LENGTH_CHOICE
                elif TYPE == 0 and SUBTYPE == 1:
                    partLength = PART_LENGTH_REGRP
                elif TYPE == 1 or TYPE == 2:
                    partLength = PART_LENGTH_BODY
            

            editTransition(Y_POS, TYPE, SUBTYPE)

            try:
                generate_PART_Groten(PATH, partLength, Y_POS - partLength, TYPE, SUBTYPE)
            except:
                print "Failed to generate the path"
            try:
                generatePositionVector_Groten(partLength, Y_POS - partLength, TYPE, SUBTYPE)
            except:
                print "Failed to generate the position vector"

            Y_POS = Y_POS - partLength #Decrement
#------------------------------------  

    scenarioFile.close() #Close the scenario file at the end of the while loop


    #Create file with path position, synchronised for aquisition frequency"
    path_file_name = "../path/PATH_scenario_" + scenario_number + "_subject_" + str(SUJET) + ".txt"
    path_file = open(path_file_name, 'w')
    
	
    if  (100 <= int(scenario_number) and int(scenario_number) < 300): #Scenarii 101-300 : Trials with obstacles
        for i in range(0, len(PATH_POS)):
	    line = str(i) + "\t" + str(PATH_POS[len(PATH_POS)-i-1]) + "\t" + str(OBSTACLE[len(PATH_POS)-i-1]) + "\n"
            path_file.write(line)
            
    else:
        for i in range(0, len(PATH_POS)):
	    line = str(i) + "\t" + str(PATH_POS[len(PATH_POS)-i-1])  + "\t0\n"
            path_file.write(line)
    path_file.close()


############################################################################################################################################
# Each UI sends the infos of the opposite subject
def editTransition(Y_POS, TYPE, SUBTYPE):
    global partChanges
    if TYPE == 0 and SUBTYPE == 0 and Y_POS == PATH_LENGTH:
        for i in range (0, DEPLACEMENT + 1):
            partChanges[Y_POS - i - 1] = 'START'
            
    elif TYPE == 0 and SUBTYPE == 0 and Y_POS <= PART_LENGTH_FORK:
        for i in range (0, DEPLACEMENT + 1):
            partChanges[Y_POS - i - 1] = 'END'
            
    elif TYPE == 3 and ((SUJET == 2 and (SUBTYPE == 0 or SUBTYPE == 2 or SUBTYPE == 6)) or (SUJET == 1 and (SUBTYPE == 0 or SUBTYPE == 4 or SUBTYPE == 7))): #fork gauche
        for i in range (0, DEPLACEMENT + 1):
            partChanges[Y_POS + PART_LENGTH_CHOICE - i - 1] = 'FORK_GAUCHE'
        
    elif TYPE == 3 and ((SUJET == 2 and (SUBTYPE == 1 or SUBTYPE == 3 or SUBTYPE == 7)) or (SUJET == 1 and(SUBTYPE == 1 or SUBTYPE == 5 or SUBTYPE == 6))):
        for i in range (0, DEPLACEMENT + 1):
            partChanges[Y_POS + PART_LENGTH_CHOICE - i - 1] = 'FORK_DROITE'
        
    elif TYPE == 3 and ((SUJET == 2 and (SUBTYPE == 4 or SUBTYPE == 5)) or (SUJET == 1 and (SUBTYPE == 2 or SUBTYPE == 3))):
        for i in range (0, DEPLACEMENT + 1):
            partChanges[Y_POS + PART_LENGTH_CHOICE - i - 1] = 'FORK_MILIEU'
        
    elif TYPE == 1:
        for i in range (0, DEPLACEMENT + 1):
            partChanges[Y_POS - i - 1] = 'BODY_GAUCHE'
            
    elif TYPE == 2:
        for i in range (0, DEPLACEMENT + 1):
            partChanges[Y_POS - i - 1] = 'BODY_DROITE'
            
    elif TYPE == 0 and SUBTYPE == 1: #REGRP
        partChanges[Y_POS] = 0
        
    elif TYPE == 0 and SUBTYPE == 0 and Y_POS >= PART_LENGTH_FORK and Y_POS <= PATH_LENGTH: #CHOICE
        partChanges[Y_POS] = 0
        
    else:
        print 'Erreur dans la creation du vecteur partChanges', TYPE, SUBTYPE, Y_POS


############################################################################################################################################
# FUNCTIONS - TRIAL : GROTEN
############################################################################################################################################

############################################################################################################################################
#GENERATE PART - GROTEN
#######################
def generate_PART_Groten(SURFACE, partLength, Y_POS, TYPE, SUBTYPE): #generate the different kind of subparts
    GAUCHE= 2*PART_WIDTH/5
    MILIEU= PART_WIDTH/2
    DROITE= 3*PART_WIDTH/5
    lineColor = WHITE
    lineBoldColor = LIGHTGREY
    
    if TYPE == 0: #Ligne droite
        #pygame.draw.line(SURFACE, lineBoldColor, (MILIEU, 0+ Y_POS + 2*LINE_WIDTH/3), (MILIEU, partLength + Y_POS - 2*LINE_WIDTH/3), LINE_WIDTH_BOLD)
        pygame.draw.line(SURFACE, lineColor    , (MILIEU, 0+ Y_POS), (MILIEU, partLength + Y_POS), LINE_WIDTH)
        
    elif TYPE == 1:  #sinusoide gauche
        for i in range(0+Y_POS - 2*LINE_WIDTH/3, partLength+Y_POS + 2*LINE_WIDTH/3):
            X = MILIEU + (MILIEU-GAUCHE)*(math.cos(float((i - Y_POS))/float(partLength)*2.0*3.14159) - 1)/2
            Y = i
            #pygame.draw.line(SURFACE, lineBoldColor , (X,Y), (X,Y), LINE_WIDTH_BOLD)
            pygame.draw.line(SURFACE, lineColor     , (X,Y), (X,Y), LINE_WIDTH)
           
    elif TYPE == 2:  #sinusoide droite
        for i in range(0+Y_POS - 2*LINE_WIDTH/3, partLength+Y_POS + 2*LINE_WIDTH/3):
            X = MILIEU - (MILIEU-GAUCHE)*(math.cos(float((i - Y_POS))/float(partLength)*2.0*3.14159) - 1)/2
            Y = i
            #pygame.draw.line(SURFACE, lineBoldColor , (X,Y), (X,Y), LINE_WIDTH_BOLD)
            pygame.draw.line(SURFACE, lineColor     , (X,Y), (X,Y), LINE_WIDTH) 
          
######################################################
#Fourche sinusoide
######################################################

    elif (TYPE == 3 and FORK_TYPE == "COURBE"): #fork
        if (SUJET == 1 and (SUBTYPE == 0 or SUBTYPE == 2 or SUBTYPE == 6)) or (SUJET ==2 and (SUBTYPE == 0 or SUBTYPE == 4 or SUBTYPE == 7)): #Gauche gras
            pygame.draw.line(SURFACE, lineBoldColor, (MILIEU, 0+ Y_POS)          , (MILIEU, 0 + Y_POS - LINE_WIDTH)         , LINE_WIDTH_BOLD)
            pygame.draw.line(SURFACE, lineColor    , (MILIEU, 0+ Y_POS)          , (MILIEU, 0 + Y_POS - LINE_WIDTH)         , LINE_WIDTH     )
            pygame.draw.line(SURFACE, lineBoldColor, (MILIEU, partLength + Y_POS), (MILIEU, partLength + Y_POS + LINE_WIDTH), LINE_WIDTH_BOLD)
            pygame.draw.line(SURFACE, lineColor    , (MILIEU, partLength + Y_POS), (MILIEU, partLength + Y_POS + LINE_WIDTH), LINE_WIDTH     )
            for i in range(0 + Y_POS, partLength/5 + Y_POS):
                X=MILIEU + (1 - math.cos(float((i-Y_POS)/float(partLength/5)*3.14159)))/2* (GAUCHE - MILIEU)
                Y=i
                pygame.draw.line(SURFACE, lineBoldColor, (X,Y), (X,Y), LINE_WIDTH_BOLD)
            for i in range(4*partLength/5 + Y_POS, partLength + Y_POS):
                X=MILIEU + (1 + math.cos(float((i-(4*partLength/5 + Y_POS))/float(partLength/5)*3.14159)))/2* (GAUCHE - MILIEU)
                Y=i
                pygame.draw.line(SURFACE, lineBoldColor, (X,Y), (X,Y), LINE_WIDTH_BOLD)
            pygame.draw.line(SURFACE, lineBoldColor, (GAUCHE, partLength/5 + Y_POS), (GAUCHE, 4*partLength/5 + Y_POS), LINE_WIDTH_BOLD)
	    
           
            for i in range(0 + Y_POS, partLength/5 + Y_POS):
                X=MILIEU + (1 - math.cos(float((i-Y_POS)/float(partLength/5)*3.14159)))/2* (DROITE - MILIEU)
                Y=i
                pygame.draw.line(SURFACE, lineColor, (X,Y), (X,Y), LINE_WIDTH)
            for i in range(4*partLength/5 + Y_POS, partLength + Y_POS):
                X=MILIEU + (1 + math.cos(float((i-(4*partLength/5 + Y_POS))/float(partLength/5)*3.14159)))/2* (DROITE - MILIEU)
                Y=i
                pygame.draw.line(SURFACE, lineColor, (X,Y), (X,Y), LINE_WIDTH)
            pygame.draw.line(SURFACE, lineColor, (DROITE, partLength/5 + Y_POS), (DROITE, 4*partLength/5 + Y_POS), LINE_WIDTH)

            for i in range(0 + Y_POS, partLength/5 + Y_POS):
                X=MILIEU + (1 - math.cos(float((i-Y_POS)/float(partLength/5)*3.14159)))/2* (GAUCHE - MILIEU)
                Y=i
                pygame.draw.line(SURFACE, lineColor, (X,Y), (X,Y), LINE_WIDTH)
            for i in range(4*partLength/5 + Y_POS, partLength + Y_POS):
                X=MILIEU + (1 + math.cos(float((i-(4*partLength/5 + Y_POS))/float(partLength/5)*3.14159)))/2* (GAUCHE - MILIEU)
                Y=i
                pygame.draw.line(SURFACE, lineColor, (X,Y), (X,Y), LINE_WIDTH)
            pygame.draw.line(SURFACE, lineColor, (GAUCHE, partLength/5 + Y_POS), (GAUCHE, 4*partLength/5 + Y_POS), LINE_WIDTH)



        elif (SUJET ==1 and (SUBTYPE == 1 or SUBTYPE == 3 or SUBTYPE == 7)) or (SUJET == 2 and(SUBTYPE == 1 or SUBTYPE == 5 or SUBTYPE == 6)):#Droite gras
            pygame.draw.line(SURFACE, lineBoldColor, (MILIEU, 0+ Y_POS)          , (MILIEU, 0 + Y_POS - LINE_WIDTH)         , LINE_WIDTH_BOLD)
            pygame.draw.line(SURFACE, lineColor    , (MILIEU, 0+ Y_POS)          , (MILIEU, 0 + Y_POS - LINE_WIDTH)         , LINE_WIDTH     )
            pygame.draw.line(SURFACE, lineBoldColor, (MILIEU, partLength + Y_POS), (MILIEU, partLength + Y_POS + LINE_WIDTH), LINE_WIDTH_BOLD)
            pygame.draw.line(SURFACE, lineColor    , (MILIEU, partLength + Y_POS), (MILIEU, partLength + Y_POS + LINE_WIDTH), LINE_WIDTH     )
            
            for i in range(0 + Y_POS, partLength/5 + Y_POS):
                X=MILIEU + (1 - math.cos(float((i-Y_POS)/float(partLength/5)*3.14159)))/2* (DROITE - MILIEU)
                Y=i
                pygame.draw.line(SURFACE, lineBoldColor, (X,Y), (X,Y), LINE_WIDTH_BOLD)
            for i in range(4*partLength/5 + Y_POS, partLength + Y_POS):
                X=MILIEU + (1 + math.cos(float((i-(4*partLength/5 + Y_POS))/float(partLength/5)*3.14159)))/2* (DROITE - MILIEU)
                Y=i
                pygame.draw.line(SURFACE, lineBoldColor, (X,Y), (X,Y), LINE_WIDTH_BOLD)
            pygame.draw.line(SURFACE, lineBoldColor, (DROITE, partLength/5 + Y_POS), (DROITE, 4*partLength/5 + Y_POS), LINE_WIDTH_BOLD)
           
            for i in range(0 + Y_POS, partLength/5 + Y_POS):
                X=MILIEU + (1 - math.cos(float((i-Y_POS)/float(partLength/5)*3.14159)))/2* (DROITE - MILIEU)
                Y=i
                pygame.draw.line(SURFACE, lineColor, (X,Y), (X,Y), LINE_WIDTH)
            for i in range(4*partLength/5 + Y_POS, partLength + Y_POS):
                X=MILIEU + (1 + math.cos(float((i-(4*partLength/5 + Y_POS))/float(partLength/5)*3.14159)))/2* (DROITE - MILIEU)
                Y=i
                pygame.draw.line(SURFACE, lineColor, (X,Y), (X,Y), LINE_WIDTH)
            pygame.draw.line(SURFACE, lineColor, (DROITE, partLength/5 + Y_POS), (DROITE, 4*partLength/5 + Y_POS), LINE_WIDTH)

            for i in range(0 + Y_POS, partLength/5 + Y_POS):
                X=MILIEU + (1 - math.cos(float((i-Y_POS)/float(partLength/5)*3.14159)))/2* (GAUCHE - MILIEU)
                Y=i
                pygame.draw.line(SURFACE, lineColor, (X,Y), (X,Y), LINE_WIDTH)
            for i in range(4*partLength/5 + Y_POS, partLength + Y_POS):
                X=MILIEU + (1 + math.cos(float((i-(4*partLength/5 + Y_POS))/float(partLength/5)*3.14159)))/2* (GAUCHE - MILIEU)
                Y=i
                pygame.draw.line(SURFACE, lineColor, (X,Y), (X,Y), LINE_WIDTH)
            pygame.draw.line(SURFACE, lineColor, (GAUCHE, partLength/5 + Y_POS), (GAUCHE, 4*partLength/5 + Y_POS), LINE_WIDTH)

            

        elif (SUJET ==1 and (SUBTYPE == 4 or SUBTYPE == 5)) or (SUJET == 2 and (SUBTYPE == 2 or SUBTYPE == 3)): #Pas de gras - pas de chocolat
            for i in range(0 + Y_POS, partLength/5 + Y_POS):
                X=MILIEU + (1 - math.cos(float((i-Y_POS)/float(partLength/5)*3.14159)))/2* (DROITE - MILIEU)
                Y=i
                pygame.draw.line(SURFACE, lineColor, (X,Y), (X,Y), LINE_WIDTH)
            for i in range(4*partLength/5 + Y_POS, partLength + Y_POS):
                X=MILIEU + (1 + math.cos(float((i-(4*partLength/5 + Y_POS))/float(partLength/5)*3.14159)))/2* (DROITE - MILIEU)
                Y=i
                pygame.draw.line(SURFACE, lineColor, (X,Y), (X,Y), LINE_WIDTH)
            pygame.draw.line(SURFACE, lineColor, (DROITE, partLength/5 + Y_POS), (DROITE, 4*partLength/5 + Y_POS), LINE_WIDTH)

            for i in range(0 + Y_POS, partLength/5 + Y_POS):
                X=MILIEU + (1 - math.cos(float((i-Y_POS)/float(partLength/5)*3.14159)))/2* (GAUCHE - MILIEU)
                Y=i
                pygame.draw.line(SURFACE, lineColor, (X,Y), (X,Y), LINE_WIDTH)
            for i in range(4*partLength/5 + Y_POS, partLength + Y_POS):
                X=MILIEU + (1 + math.cos(float((i-(4*partLength/5 + Y_POS))/float(partLength/5)*3.14159)))/2* (GAUCHE - MILIEU)
                Y=i
                pygame.draw.line(SURFACE, lineColor, (X,Y), (X,Y), LINE_WIDTH)
            pygame.draw.line(SURFACE, lineColor, (GAUCHE, partLength/5 + Y_POS), (GAUCHE, 4*partLength/5 + Y_POS), LINE_WIDTH)	

##################################################
#Fourche angles droits
##################################################

    elif (TYPE == 3 and FORK_TYPE == "DROIT"): #fork
        if (SUJET == 1 and (SUBTYPE == 0 or SUBTYPE == 2 or SUBTYPE == 6)) or (SUJET ==2 and (SUBTYPE == 0 or SUBTYPE == 4 or SUBTYPE == 7)): #Gauche gras
	    pygame.draw.circle(SURFACE, lineBoldColor, (MILIEU, 0+Y_POS), LINE_WIDTH_BOLD/2)
	    pygame.draw.circle(SURFACE, lineBoldColor, (MILIEU, partLength+Y_POS), (LINE_WIDTH_BOLD-1)/2)
	    pygame.draw.line(SURFACE, lineColor, (MILIEU, 0 + Y_POS - LINE_WIDTH_BOLD/2), (MILIEU, 0 + Y_POS), LINE_WIDTH)
	    pygame.draw.line(SURFACE, lineColor, (MILIEU, partLength + Y_POS + LINE_WIDTH_BOLD/2), (MILIEU, partLength + Y_POS), LINE_WIDTH)

            pygame.draw.lines(SURFACE, lineBoldColor , False, ((MILIEU - 2*LINE_WIDTH/3, 0 +Y_POS), (GAUCHE, 0 + Y_POS), (GAUCHE, partLength + Y_POS), (MILIEU - 2*LINE_WIDTH/3, partLength + Y_POS)), LINE_WIDTH_BOLD)
            pygame.draw.circle(SURFACE, lineBoldColor, (GAUCHE, 0+Y_POS), (LINE_WIDTH_BOLD-1)/2)
            pygame.draw.circle(SURFACE, lineBoldColor, (GAUCHE, partLength+Y_POS), LINE_WIDTH_BOLD/2)
            pygame.draw.lines(SURFACE, lineColor, False, ((MILIEU, 0 +Y_POS), (GAUCHE, 0 + Y_POS), (GAUCHE, partLength + Y_POS), (MILIEU, partLength + Y_POS)), LINE_WIDTH)
            pygame.draw.lines(SURFACE, lineColor, False, ((MILIEU, 0 +Y_POS), (DROITE, 0 + Y_POS), (DROITE, partLength + Y_POS), (MILIEU, partLength + Y_POS)), LINE_WIDTH)


        elif (SUJET ==1 and (SUBTYPE == 1 or SUBTYPE == 3 or SUBTYPE == 7)) or (SUJET == 2 and(SUBTYPE == 1 or SUBTYPE == 5 or SUBTYPE == 6)):#Droite gras
	    pygame.draw.circle(SURFACE, lineBoldColor, (MILIEU, 0+Y_POS), LINE_WIDTH_BOLD/2)
	    pygame.draw.circle(SURFACE, lineBoldColor, (MILIEU, partLength+Y_POS), (LINE_WIDTH_BOLD-1)/2)
	    pygame.draw.line(SURFACE, lineColor, (MILIEU, 0 + Y_POS - LINE_WIDTH_BOLD/2), (MILIEU, 0 + Y_POS), LINE_WIDTH)
	    pygame.draw.line(SURFACE, lineColor, (MILIEU, partLength + Y_POS + LINE_WIDTH_BOLD/2), (MILIEU, partLength + Y_POS), LINE_WIDTH)

            pygame.draw.lines(SURFACE, lineColor, False, ((MILIEU, 0 +Y_POS), (GAUCHE, 0 + Y_POS), (GAUCHE, partLength + Y_POS), (MILIEU, partLength + Y_POS)), LINE_WIDTH)
            pygame.draw.lines(SURFACE, lineBoldColor , False, ((MILIEU + 2*LINE_WIDTH/3, 0 +Y_POS), (DROITE, 0 + Y_POS), (DROITE, partLength + Y_POS), (MILIEU + 2*LINE_WIDTH/3, partLength + Y_POS)), LINE_WIDTH_BOLD)
            pygame.draw.circle(SURFACE, lineBoldColor, (DROITE, 0+Y_POS), (LINE_WIDTH_BOLD-1)/2)
            pygame.draw.circle(SURFACE, lineBoldColor, (DROITE, partLength+Y_POS), LINE_WIDTH_BOLD/2)
            pygame.draw.lines(SURFACE, lineColor, False, ((MILIEU, 0 +Y_POS), (DROITE, 0 + Y_POS), (DROITE, partLength + Y_POS), (MILIEU, partLength + Y_POS)), LINE_WIDTH)


        elif (SUJET ==1 and (SUBTYPE == 4 or SUBTYPE == 5)) or (SUJET == 2 and (SUBTYPE == 2 or SUBTYPE == 3)): #Pas de gras - pas de chocolat
            pygame.draw.lines(SURFACE, lineColor, False, ((MILIEU, 0 +Y_POS), (GAUCHE, 0 + Y_POS), (GAUCHE, partLength + Y_POS), (MILIEU, partLength + Y_POS)), LINE_WIDTH)
            pygame.draw.lines(SURFACE, lineColor, False, ((MILIEU, 0 +Y_POS), (DROITE, 0 + Y_POS), (DROITE, partLength + Y_POS), (MILIEU, partLength + Y_POS)), LINE_WIDTH)



############################################################################################################################################


############################################################################################################################################
# GENERATE POSITION VECTOR - GROTEN
###################################
def generatePositionVector_Groten(partLength, Y_POS, TYPE, SUBTYPE): #Generate the reference position vector
    WIDTH=float(PART_WIDTH)
    LENGTH=float(partLength)
    milieu = float(MILIEU)
    gauche= float(GAUCHE)
    droite = float(DROITE)
           
    if TYPE == 0: #Ligne droite
        for i in range(Y_POS, Y_POS + partLength):
            PATH_POS[i]=milieu

    elif TYPE == 1:#Sinusoide gauche
        for i in range (Y_POS, Y_POS + partLength):
            PATH_POS[i]=milieu + (milieu-gauche)*(math.cos(float((i - Y_POS))/float(partLength)*2.0*3.14159) - 1)/2

    elif TYPE == 2: #Sinusoide droite
        for i in range (Y_POS, Y_POS + partLength):
            PATH_POS[i]=milieu - (milieu-gauche)*(math.cos(float((i - Y_POS))/float(partLength)*2.0*3.14159) - 1)/2

################################################
                
    elif (TYPE == 3 and FORK_TYPE == "COURBE"): #Fork sinusoides
        for i in range(Y_POS, Y_POS + partLength):
            if ((SUBTYPE == 0 or SUBTYPE == 2 or SUBTYPE == 6) and SUJET == 1) or ((SUBTYPE == 0 or SUBTYPE == 4 or SUBTYPE == 7) and SUJET == 2): #Gauche gras
                if (i >= 0+Y_POS and i < int(float(partLength/5)) + Y_POS):
                    PATH_POS[i] = milieu + (1 - math.cos(float((i-Y_POS)/float(partLength/5)*3.14159)))/2 * (gauche - milieu)
                elif (i >= partLength/5+Y_POS and i < 4*partLength/5 +Y_POS):
                    PATH_POS[i] = gauche
                elif (i >= 4*partLength/5+Y_POS and i <= partLength + Y_POS):
                    PATH_POS[i] = milieu + (1 + math.cos(float((i-(4*partLength/5 + Y_POS))/float(partLength/5)*3.14159)))/2* (gauche - milieu)
                    
            elif ((SUBTYPE == 1 or SUBTYPE == 3 or SUBTYPE == 7)and SUJET == 1) or ((SUBTYPE == 1 or SUBTYPE == 5 or SUBTYPE == 6) and SUJET ==2 ): #Droite gras
                if (i >= 0+Y_POS and i < partLength/5 + Y_POS):
                    PATH_POS[i] = milieu + (1 - math.cos(float((i-Y_POS)/float(partLength/5)*3.14159)))/2 * (droite - milieu)
                elif (i >= partLength/5+Y_POS and i < 4*partLength/5 +Y_POS):
                    PATH_POS[i] = droite
                elif (i >= 4*partLength/5+Y_POS and i <= partLength + Y_POS):
                    PATH_POS[i] = milieu + (1 + math.cos(float((i-(4*partLength/5 + Y_POS))/float(partLength/5)*3.14159)))/2* (droite - milieu)
                 
            elif (SUJET ==1 and (SUBTYPE == 4 or SUBTYPE == 5)) or (SUJET == 2 and (SUBTYPE == 2 or SUBTYPE == 3)):
                 PATH_POS[i] = 10000

################################################

    elif (TYPE == 3 and FORK_TYPE == "DROIT"): #Fork angles droits
	for i in range(Y_POS, Y_POS + partLength):
	    if ((SUBTYPE == 0 or SUBTYPE == 2 or SUBTYPE == 6) and SUJET == 1) or ((SUBTYPE == 0 or SUBTYPE == 4 or SUBTYPE == 7) and SUJET == 2): #Gauche gras
                PATH_POS[i] = gauche
	    elif ((SUBTYPE == 1 or SUBTYPE == 3 or SUBTYPE == 7)and SUJET == 1) or ((SUBTYPE == 1 or SUBTYPE == 5 or SUBTYPE == 6) and SUJET ==2 ): #Droite gras
                 PATH_POS[i] = droite
	    elif (SUJET ==1 and (SUBTYPE == 4 or SUBTYPE == 5)) or (SUJET == 2 and (SUBTYPE == 2 or SUBTYPE == 3)):
		PATH_POS[i] = 10000
 


############################################################################################################################################




############################################################################################################################################
# FUNCTIONS - TRIAL : OBSTACLES
############################################################################################################################################


############################################################################################################################################
#GENERATE PART - OBSTACLES
##########################
def generate_PART_Obstacles(SURFACE, partLength, Y_POS, TYPE, SUBTYPE): #generate the different kind of subparts
    MILIEU= PART_WIDTH/2
    GAUCHE= MILIEU - SUBTYPE*PART_WIDTH/20
    DROITE= MILIEU + SUBTYPE*PART_WIDTH/20
    marge = 4*LINE_WIDTH_BOLD
    milieu = float(MILIEU)
    gauche= float(GAUCHE)
    droite = float(DROITE)


    scenario_number = int(sys.argv[1])

#-------------------------------------
    if (SUJET == 1 or (SUJET == 2 and 101 <= scenario_number and scenario_number <= 200)): # Scenrio 101-200 : Follower can see the path and obstacles
    
        if TYPE == 0: #Ligne droite
            pygame.draw.line(SURFACE, LIGHTGREY, (MILIEU, 0+ Y_POS), (MILIEU, partLength + Y_POS), LINE_WIDTH_BOLD)
            pygame.draw.line(SURFACE, BLACK    , (MILIEU, 0+ Y_POS), (MILIEU, partLength + Y_POS), LINE_WIDTH)
            for i in range(Y_POS, Y_POS + partLength):
                PATH_POS[i]=milieu
          
      
        elif (TYPE == 1 or TYPE == 3):  #sinusoide gauche (1 : courte, 3: longue)
            for i in range(0 + Y_POS, partLength + Y_POS):
                X = MILIEU + (MILIEU-GAUCHE)*(math.cos(float((i - Y_POS))/float(partLength)*2.0*3.14159) - 1)/2
                Y = i
                pygame.draw.line(SURFACE, LIGHTGREY , (X,Y), (X,Y), LINE_WIDTH_BOLD)
                pygame.draw.line(SURFACE, BLACK     , (X,Y), (X,Y), LINE_WIDTH)
                PATH_POS[i]=milieu + (milieu-gauche)*(math.cos(float((i - Y_POS))/float(partLength)*2.0*3.14159) - 1)/2
               

        elif (TYPE == 2 or TYPE == 4):  #sinusoide droite (2 : courte, 4: longue)
            for i in range(0 + Y_POS, partLength + Y_POS):
                X = MILIEU - (MILIEU-GAUCHE)*(math.cos(float((i - Y_POS))/float(partLength)*2.0*3.14159) - 1)/2
                Y = i
                pygame.draw.line(SURFACE, LIGHTGREY , (X,Y), (X,Y), LINE_WIDTH_BOLD)
                pygame.draw.line(SURFACE, BLACK     , (X,Y), (X,Y), LINE_WIDTH)
                PATH_POS[i]=milieu - (milieu-gauche)*(math.cos(float((i - Y_POS))/float(partLength)*2.0*3.14159) - 1)/2

#-------------------------------------
    elif (SUJET == 2 and 201 <= scenario_number and scenario_number <= 300): #scenario 201-300 : Follower only sees obstacles only
	j = 0
	while j <= partLength: #Graduations pour donner l'impression de mouvement meme sans le path
	    pygame.draw.line(SURFACE, BLACK, (0, j + Y_POS), (PART_WIDTH/10, j + Y_POS), 1)
	    pygame.draw.line(SURFACE, BLACK, (PART_WIDTH, j + Y_POS), (9*PART_WIDTH/10, j + Y_POS), 1)
	    j += 1*VITESSE

        if TYPE == 0: #Ligne droite
            for i in range(Y_POS, Y_POS + partLength):
                PATH_POS[i]=milieu
         
      
        elif (TYPE == 1 or TYPE == 3):  #sinusoide gauche (1 : courte, 3: longue)
            for i in range(0 + Y_POS, partLength + Y_POS):
                PATH_POS[i]=milieu + (milieu-gauche)*(math.cos(float((i - Y_POS))/float(partLength)*2.0*3.14159) - 1)/2
               

        elif (TYPE == 2 or TYPE == 4):  #sinusoide droite (2 : courte, 4: longue)
            for i in range(0 + Y_POS, partLength + Y_POS):
                PATH_POS[i]=milieu - (milieu-gauche)*(math.cos(float((i - Y_POS))/float(partLength)*2.0*3.14159) - 1)/2

#-------------------------------------

    else:
	print "Error in scenario"
#-------------------------------------


############################################################################################################################################

def generateObstacle():
    global PATH #,OBSTACLE_SURF
    #OBSTACLE_SURF = pygame.Surface((PATH_WIDTH,PATH_LENGTH), pygame.SRCALPHA)
    #OBSTACLE_SURF.fill((0,0,0,0))

    scenario_number= sys.argv[1]
    scenario_file_name = "../scenarios/SCENARIO_" + scenario_number + ".txt"
    scenarioFile = open(scenario_file_name,'r')
    line = scenarioFile.readline()
    obstacleFileNumber = int( line[ line.find("_OBSTACLE") + 10 : line.find("\n")])
    scenarioFile.close()
    
    obstacleFileName = "../obstacles/OBSTACLES_" + str(obstacleFileNumber) + ".txt"
    obstacleFile = open(obstacleFileName, 'r')
    for line in obstacleFile:
	lineRead = line[0 : line.find("\n")]
	obstList = lineRead.split("\t")
	x_obst = int(obstList[0])
	y_obst = int(obstList[1])
	radius_obst = int(obstList[2])
	pygame.draw.circle(PATH, RED, (x_obst, y_obst), radius_obst)
	#pygame.draw.circle(OBSTACLE_SURF, RED, (x_obst, y_obst), radius_obst)
    obstacleFile.close()
   

##############################################################################################################################################
### GENERATE POSITION VECTOR - OBSTACLES (OBSOLETE)
########################################
##def generatePositionVector_Obstacles(partLength, Y_POS, TYPE, SUBTYPE): #Generate the reference position vector
##    WIDTH=float(PART_WIDTH)
##    LENGTH=float(partLength)
##    milieu = float(MILIEU)
##    gauche= float(GAUCHE)
##    droite = float(DROITE)
##           
##    if TYPE == 0: #Ligne droite
##        for i in range(Y_POS, Y_POS + partLength):
##            PATH_POS[i]=milieu
##
##    elif (TYPE == 1 or TYPE == 3):#Sinusoide gauche
##        for i in range (Y_POS, Y_POS + partLength):
##            PATH_POS[i]=milieu + (milieu-gauche)*(math.cos(float((i - Y_POS))/float(partLength)*2.0*3.14159) - 1)/2
##
##    elif (TYPE == 2 or TYPE == 4): #Sinusoide droite
##        for i in range (Y_POS, Y_POS + partLength):
##            PATH_POS[i]=milieu - (milieu-gauche)*(math.cos(float((i - Y_POS))/float(partLength)*2.0*3.14159) - 1)/2
##
##############################################################################################################################################



if __name__ == '__main__':
    main()



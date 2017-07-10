#!/usr/bin/python

#Code written by Lucas Roche
#March 2015

import sys
import subprocess
import threading
import time
import random
import pickle

from params import *


##############################################################################################
#Recuperation des infos sur le trial
####################################

fpickle = open('./pickle_file', 'r')

try:
    param_list = pickle.load(fpickle)
except:
    param_list = ["sujet1", "sujet2", "10", "a", "1"]
   
print param_list

fpickle.close()

try:
    subject_name_1 = raw_input("Nom du sujet 1 : \n")
    if subject_name_1 == "":
        subject_name_1 = param_list[0]
except:
    print "error in subject name - quitting...."
    quit()

try:
    subject_name_2 = raw_input("Nom du sujet 2 : \n")
    if subject_name_2 == "":
        subject_name_2 = param_list[1]
except:
    print "error in subject name - quitting...."
    quit()

try:
    scenario_number = raw_input("Numero de scenario :\n")
    if scenario_number == "":
        scenario_number = param_list[2]
except:
    print "error in scenario number - quitting...."
    quit()

try:
    haptic_feedback = raw_input("Type de retour haptique (a/avec/s/sans/u/ultron/w/alone): \n")
    if haptic_feedback == "":
        haptic_feedback = param_list[3]
except:
    print "wrong haptic feedback type"
    quit()
haptic_feedback = haptic_feedback.upper()
if haptic_feedback == "AVEC" or haptic_feedback == "A" :
    haptic_feedback = "a"
elif haptic_feedback == "SANS" or haptic_feedback == "S" :
    haptic_feedback = "s"
elif haptic_feedback == "ULTRON" or  haptic_feedback == "U":
    haptic_feedback = "u"
elif haptic_feedback == 'ALONE' or haptic_feedback == 'W':
    haptic_feedback = 'w'
else:
    print "wrong haptic feedback type"
    quit()

try:
    trial_number = raw_input("Numero d'essai : \n")
    if trial_number == "":
        trial_number = param_list[4]
except:
    print "error in trial number - quitting...."
    quit()

fpickle = open('./pickle_file', 'w')
param_list = [subject_name_1, subject_name_2, scenario_number, haptic_feedback, trial_number]
pickle.dump(param_list, fpickle)
fpickle.close()
##############################################################################################
#Creation des differents noms de fichiers utilises

date = time.gmtime(None)
date = str(date.tm_mday) + "-" + str(date.tm_mon) + "-" +  str(date.tm_hour+2) + "-" +  str(date.tm_min)

results_file_name =  "../results/RESULTS_scenario_" + scenario_number + "_trial_" + trial_number + "_" + haptic_feedback + "_" + date + ".txt"
data_file_name = "../data/DATA_scenario_" + scenario_number + "_trial_" + trial_number + "_" + haptic_feedback + "_" + date + ".txt"
scenario_file_name = "../scenarios/SCENARIO_" + scenario_number + ".txt"
path_file_name_1 = "../path/PATH_scenario_" + scenario_number + "_subject_" + "1" + ".txt"
path_file_name_2 = "../path/PATH_scenario_" + scenario_number + "_subject_" + "2" + ".txt"


if (int(scenario_number) > 100 and int(scenario_number) <= 300):
    scenarioFile = open(scenario_file_name,'r')
    line = scenarioFile.readline()
    obstacleFileNumber = int( line[ line.find("_OBSTACLE") + 10 : line.find("\n")])
    scenarioFile.close()
    obstacles_file_name = "../obstacles/OBSTACLES_" + str(obstacleFileNumber) + ".txt"

##############################################################################################
#Definition et lancement des deux threads (UI sujet 1 et UI sujet 2)
####################################################################

#Definition d'un temps de depart pour la synchronisation des deux threads
start_time = time.time() + 1.5


def UI_Sujet1():
    print "UI Sujet 1 Starting ....\n"
    subprocess.call(["./UI_v7.py", scenario_number, "1" , str(start_time)])



def UI_Sujet2():
    print "UI Sujet 2 Starting ....\n"
    subprocess.call(["./UI_v7.py", scenario_number, "2" , str(start_time)])



t1 = threading.Thread(group = None, target = UI_Sujet1, args = () )
t1.start()

t2 = threading.Thread(group = None, target = UI_Sujet2, args = () )
t2.start()

#Tache de fond pour garder le script en pause pendant que les threads tournent
while (t1.isAlive() or t2.isAlive()):
    time.sleep(1)
    
print "UI Stopped...."

##############################################################################################
#Recuperation du fichier de donnees par ssh

subprocess.call(["scp","paddle@10.0.0.2:/home/paddle/Manip/files/Paddle/data/data.txt","../data/"])

subprocess.call(["mv","../data/data.txt", data_file_name])




##############################################################################################
#Creation d'un fichier de resultats combinant tous les fichiers
###############################################################

fResults = open(results_file_name, 'w')

# Ecriture d'un header avec tous les parametres
fResults.write('PARAMETERS\n')
fResults.write("\n")
fResults.write('SUBJECT NAME1 : ' + subject_name_1 + "\n")
fResults.write('SUBJECT NAME2 : ' + subject_name_2 + "\n")
fResults.write('SCENARIO : ' + scenario_number + "\n")
fResults.write('TRIAL : ' + trial_number + "\n")
fResults.write("\n")
fResults.write('PATH_DURATION : ' + str(PATH_DURATION) + "\n")
fResults.write('VITESSE : ' + str(VITESSE) + "\n")
fResults.write('Y_POS_CURSOR : ' + str(Y_POS_CURSOR) + "\n")
fResults.write('PART_DURATION_BODY : ' + str(PART_DURATION_BODY) + "\n")
fResults.write('PART_DURATION_CHOICE : ' + str(PART_DURATION_CHOICE) + "\n")
fResults.write('PART_DURATION_FORK : ' + str(PART_DURATION_FORK) + "\n")
fResults.write('PART_DURATION_REGRP : ' + str(PART_DURATION_REGRP) + "\n")
fResults.write('PART_DURATION_START : ' + str(PART_DURATION_START) + "\n")
fResults.write('POSITION_OFFSET : ' + str(POSITION_OFFSET) + "\n")
fResults.write('SENSIBILITY : ' + str(SENSIBILITY) + "\n")
fResults.write('WINDOW_WIDTH : ' + str(WINDOW_WIDTH) + "\n")
fResults.write('WINDOW_LENGTH : ' + str(WINDOW_LENGTH) + "\n")
fResults.write("\n")
fResults.write("\n")
fResults.write("RESULTS\n")
fResults.write("\n")
fResults.write("ROBOT TIME" + "\t" + "PATH POSITION 1" + "\t" + "PATH POSITION 2" + "\t" + "POSITION SUBJECT 1" + "\t" + "POSITION SUBJECT 2" + "\t" + "FORCE SUBJECT 1" + "\t" + "FORCE SUBJECT 2" + "\t" + "POSITION ROBOT 1"  + "\t" + "POSITION ROBOT 2"+ "\n")
fResults.write("\n")

PATH_POS1 = [None]*PATH_LENGTH
PATH_POS2 = [None]*PATH_LENGTH
OBSTACLE2 = [None]*PATH_LENGTH
OBST_VECT_LEN=int(PATH_WIDTH/GRILLE_OBST_X*PATH_LENGTH/GRILLE_OBST_Y)
X_OBST = [0]*OBST_VECT_LEN
Y_OBST = [0]*OBST_VECT_LEN
RADIUS_OBST = [0]*OBST_VECT_LEN

fPos1 = open(path_file_name_1, 'r')
for line in fPos1: #Creation d'un vecteur contenant les positions du path du sujet 1
    lineReadPosition1 = line
    lineReadPosition1 = lineReadPosition1[ 0 : lineReadPosition1.find("\n")]
    posList1 = lineReadPosition1.split('\t')
    pixel = int(posList1[0])
    PATH_POS1[pixel] = posList1[1]
fPos1.close()


fPos2 = open(path_file_name_2, 'r')
for line in fPos2: #Creation d'un vecteur contenant les positions du path du sujet 2
    lineReadPosition2 = line
    lineReadPosition2 = lineReadPosition2[ 0 : lineReadPosition2.find("\n")]
    posList2 = lineReadPosition2.split('\t')
    pixel = int(posList2[0])
    PATH_POS2[pixel] = posList2[1]
    OBSTACLE2[pixel] = posList2[2]
fPos2.close()


if (int(scenario_number) > 100 and int(scenario_number) <=300): #Creation d'un vecteur contenant les positions des obstacles du sujet 2
    fObstacles = open(obstacles_file_name, 'r')
    i=0
    for line in fObstacles:
        lineRead = line[0 : line.find("\n")]
        obstList = lineRead.split("\t")
        X_OBST[i] = int(obstList[0])
        Y_OBST[i] = int(obstList[1])
        RADIUS_OBST[i] = int(obstList[2])
        i+=1
    fObstacles.close()


#Retrouver le temps final de la manip
fData = open(data_file_name, 'r')
for line in fData:
    lineReadData = line[ 0 : line.find("\n")]
    finalTime = lineReadData.split('\t')[0]
fData.close()

fData = open(data_file_name, 'r')
j=0

time_offset = float(Y_POS_CURSOR)/float(VITESSE)
facteurDilatation = (PATH_DURATION + float(WINDOW_LENGTH)/VITESSE)/float(finalTime)

#print time_offset, PATH_DURATION + float(WINDOW_LENGTH)/VITESSE, float(finalTime), facteurDilatation

for line in fData: #Ecriture des donnes (avec synchronisation)
    lineReadData = line
    lineReadData = lineReadData[ 0 : lineReadData.find("\n")]
    dataList=lineReadData.split('\t')

    timePaddle = float(dataList[0])
    position1 = dataList[1]
    force1 = dataList[2]
    position2 = dataList[3]
    force2 = dataList[4]

    timePaddleModified = timePaddle * facteurDilatation
    if (timePaddleModified <= time_offset or int(timePaddleModified-time_offset) >= PATH_DURATION):
        pathPos1 = -1
        pathPos2 = -1
        obstPos = -1
    else:
        pathPos1 = PATH_POS1[int((timePaddleModified-time_offset)*VITESSE)]
        pathPos2 = PATH_POS2[int((timePaddleModified-time_offset)*VITESSE)]
        obstPos = OBSTACLE2[int((timePaddleModified-time_offset)*VITESSE)]
 
#    if j < OBST_VECT_LEN and (int(scenario_number) > 100):
#        x_obst = X_OBST[j]
#        y_obst = Y_OBST[j]
#        radius_obst = RADIUS_OBST[j]
#        j+=1
#    else:
#        x_obst = 0
#        y_obst = 0
#        radius_obst = 0
    try:
        pos_virtuelle1 = dataList[5]
        pos_virtuelle2 = dataList[6]
    except:
        pos_virtuelle1 = 0
        pos_virtuelle2 = 0        
             


    lineWritten = str(timePaddle) + "\t" + str(pathPos1) + "\t" + str(pathPos2) + "\t" + str(position1) + "\t" + str(position2) + "\t" + str(force1) + "\t" + str(force2) + "\t" + str(pos_virtuelle1) + "\t" + str(pos_virtuelle2) + "\n"

    fResults.write(lineWritten)


fData.close()


fResults.close()

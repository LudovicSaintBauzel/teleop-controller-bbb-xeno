#!/usr/bin/env python

#Code written by Lucas Roche
#March 2015

import random


def main():
    #Creation of an empty file
    try:
        scenario_number= input("Entrez un numero de scenario\n")
        if (scenario_number == ""):
            scenario_number = 1
    except:
        print "Error while entering scenario number - generating default scenario 1000\n"
        scenario_number = 1000

    if  (100 <= scenario_number and scenario_number < 200):
	generateFileObstacles(scenario_number)
    else:
	generateFileGroten(scenario_number)

    


def generateFileGroten(scenario_number):
    
    file_name="../scenarios/SCENARIO_" + str(scenario_number) + ".txt"

    scenarioFile= open(file_name, 'w')
    
    TYPE = 0
    SUBTYPE = 0
    for i in range(0, 1000):
    
        if i%5==0 or i%5==1:
            TYPE = random.randint(1,2)
            SUBTYPE = 0
        
        elif i%5==2:
            TYPE = 0
            SUBTYPE = 0
        
        elif i%5 == 3:
            TYPE = 3
	    test = random.randint(0,2)
	    if test ==0:
		SUBTYPE = random.randint(0,1)
            elif test ==1:
		SUBTYPE = random.randint(2,5)
	    else:
		SUBTYPE = random.randint(6,7)
        
        elif i%5 == 4:
            TYPE = 0
            SUBTYPE = 1

        line = "_TYPE" + "\t" + str(TYPE) + "\t" + "_SUBTYPE" + "\t" + str(SUBTYPE) + "\n"
        scenarioFile.write(line)
        
    scenarioFile.close()


def generateFileObstacles(scenario_number):
    
    file_name_1="../scenarios/SCENARIO_" + str(scenario_number) + ".txt"
    file_name_2="../scenarios/SCENARIO_" + str(scenario_number+100) + ".txt"
    scenarioFile1= open(file_name_1, 'w')
    scenarioFile2= open(file_name_2, 'w')
    
    TYPE = 0
    SUBTYPE = 0
    OBSTACLE = random.randint(0,100)
    for i in range(0, 1000):
    
	TYPE = random.randint(0,4)
	SUBTYPE = random.randint(0,3)

        line1 = "_TYPE" + "\t" + str(TYPE) + "\t" + "_SUBTYPE" + "\t" + str(SUBTYPE) + "\t" + "_OBSTACLES" + "\t" + str(OBSTACLE) + "\n"
        line2 = line1
        scenarioFile1.write(line1)
	scenarioFile2.write(line2)
        
    scenarioFile1.close()
    scenarioFile2.close()




if __name__ == '__main__':
    main()

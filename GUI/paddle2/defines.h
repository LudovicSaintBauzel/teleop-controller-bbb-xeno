#ifndef DEFINES
#define DEFINES

// DECLARATION OF THE DIFFERENT PARAMETERS

//#define START false

// Screens positions
#define SCREEN_POSITION_1X 1760.0
#define SCREEN_POSITION_1Y 150.0
#define SCREEN_POSITION_2X 320.0
#define SCREEN_POSITION_2Y 150.0

// Window size
#define WINDOW_WIDTH 800.0
#define WINDOW_LENGTH 600.0
#define SCORE_WIDTH 200.0
#define SCORE_HEIGTH 50.0

// Scrolling speed (pixel/second)
#define VITESSE 120.0 // /!\ garder un ratio VITESSE/FPS entier sous peine de desynchronisation
#define FPS 60.0
#define DEPLACEMENT int(float(VITESSE)/float(FPS))

// Path (surface) size
#define PATH_WIDTH WINDOW_WIDTH
//#define PATH_DURATION 30.0
//#define PATH_LENGTH PATH_DURATION*VITESSE
#define PATH_LENGTH_TRAINING 30.0*VITESSE

// Path (line) width
#define LINE_WIDTH 4.0
#define LINE_WIDTH_BOLD LINE_WIDTH*6.0//LINE_WIDTH*10

//Vertical position of the cursor
#define Y_POS_CURSOR WINDOW_LENGTH-100

// Duration of the path's subparts (second)
#define PART_DURATION_BODY   1
#define PART_DURATION_CHOICE 1
#define PART_DURATION_FORK   3
#define PART_DURATION_REGRP  1
#define PART_DURATION_START  2


// Dimensions of the path's subparts (pixels)
#define PART_LENGTH_BODY   PART_DURATION_BODY*VITESSE
#define PART_LENGTH_CHOICE PART_DURATION_CHOICE*VITESSE
#define PART_LENGTH_FORK   PART_DURATION_FORK*VITESSE
#define PART_LENGTH_REGRP  PART_DURATION_REGRP*VITESSE

#define PART_WIDTH PATH_WIDTH
#define GAUCHE 2.0*PART_WIDTH/5
#define MILIEU PART_WIDTH/2
#define DROITE 3.0*PART_WIDTH/5

//Creating the vector containing the reference positions
//#define PATH_POS [None]*PATH_LENGTH
//#define PATH_POS_TRAINING [None]*PATH_LENGTH_TRAINING

// Size of the cursor
#define CURSOR_WIDTH 15.0
#define CURSOR_LENGTH 15.0

//Define colors
static const unsigned char WHITE[] = {255, 255, 255};
static const unsigned char BLACK[] = {0, 0, 0};
static const unsigned char GREY[] = {59, 59, 59};
static const unsigned char LIGHTGREY[] = {110, 110, 110};
static const unsigned char BLUE[] = { 0,0,255 };
static const unsigned char GREEN[] = { 0,255,0 };
static const unsigned char YELLOW[] = { 255,255,0 };
static const unsigned char ORANGE[] = {255, 80, 0};
static const unsigned char RED[] = { 255,0,0 };


// Sockets parameters
#define datas 0.0
#define thread_alive true

//#define UDP_IP "127.0.0.1"
//#define UDP_IP2 "127.0.0.1"
//#define TCP_IP "127.0.0.1"

#define UDP_IP  "192.168.0.1"     // IP address of UI computer
#define UDP_IP2 "192.168.0.2"     // IP address of paddle computer
#define TCP_IP  "192.168.0.2"
#define SERVER_IP "127.0.0.1"
#define UDP_PORT1 5005    // Retrieve position of subject 1 in UI 1
#define UDP_PORT2 5006    // Retrieve position of subject 2 in UI 1
#define UDP_PORT3 5007    // Send record start and stop messages (UI 1)
#define UDP_PORT4 5008    // Retrieve position of subject 1 in UI 2
#define UDP_PORT5 5009    // Retrieve position of subject 2 in UI 2
#define TCP_PORT1 5010    // Receive pause signals in UI 1
#define TCP_PORT2 5011    // Receive pause signals in UI 2
#define TCP_PORT3 5012    // Send part changes messages (UI 1)
#define TCP_PORT4 5013    // Send part changes messages (UI 2)
#define SERVER_PORT 5020
#define SERVER_PORT2 5022


//Fork type "DROIT" or "COURBE"
#define FORK_TYPE "DROIT"

//Aquisition parameters
#define AQUISITION_FREQUENCY 2000.0

//Cursor control parameters
#define POSITION_OFFSET 0.082 //number of ticks to the middle position
#define SENSITIVITY 0.0125 //number of encoder turns to reach side positions in the task


//####################################################################################
//# Obstacle type UI specific parameters
//####################################################################################

#define DUREE_COURTE 2
#define DUREE_LONGUE 4

//Creating the vector containing the obstacles positions
#define OBSTACLE [0]*PATH_LENGTH
#define OBSTACLE_RADIUS_MIN LINE_WIDTH_BOLD/2
#define OBSTACLE_RADIUS_MAX 2*LINE_WIDTH_BOLD
#define GRILLE_OBST_X PATH_WIDTH/10
#define GRILLE_OBST_Y PATH_WIDTH/10
#define CHANCE_OBST 0.1

#endif // DEFINES


#ifndef XENO_HPC_H_
#define XENO_HPC_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <setjmp.h>
#include <fcntl.h>
#include <termios.h>

#include <math.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/wait.h>
#include <sys/times.h>
#include <sys/types.h>

#include <native/pipe.h>
#include <native/timer.h>
#include <native/task.h>
#include <native/sem.h>
#include <native/mutex.h>
#include <native/heap.h>
#include <native/buffer.h>

//#include "s526.h"
#include <stdio.h>                                              
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>              
#include <unistd.h>
 
#define AM33XX_CONTROL_BASE		0x44e10000
// spruh73m.pdf  Issue de L3 memory et L4 memory p179 p181 p182
#define GPIO0_REGISTER 0x44e07000
#define GPIO1_REGISTER 0x4804C000
#define GPIO2_REGISTER 0x481AC000
#define GPIO3_REGISTER 0x481AE000

// spruh73m.pdf Issue de register description GPIO p 4881 ...
#define GPIO_OE 0x134
#define GPIO_DATAIN 0x138
#define GPIO_DATAOUT 0x13C
#define GPIO_CTRL 0x130
#define GPIO_CLRDATAOUT 0x190
#define GPIO_SETDATAOUT 0x194


#define OFFSET_PIN9_12 0x878
#define GPIO1_28_PIN9_12 28

#define OFFSET_PIN_9_14 0x848
#define GPIO1_18_PIN9_14 18

// HPC_CALIB_TASK_STKSZ 0
// HPC_CALIB_TASK_PRIO 90
// HPC_CALIB_TASK_MODE T_FPU|T_CPU(0)

/* définitions des propriétés de la tâche Hpc_main_task */
#define HPC_MAIN_TASK_PRIO 		90 				//Heigher Task priority
//#define HPC_MAIN_TASK_MODE 		T_FPU|T_CPU(0)	//no flags
#define HPC_MAIN_TASK_STKSZ 		0				//Stack size 

#define HPC_MES_TASK_STKSZ 0
#define HPC_MES_TASK_PRIO 80
//#define HPC_MES_TASK_MODE  T_FPU|T_CPU(0)

#define HPC_CALIB_TASK_STKSZ 0
#define HPC_CALIB_TASK_PRIO 90
//#define HPC_CALIB_TASK_MODE T_FPU|T_CPU(0)



#define MAIN_TASK_PERIOD		 500000000
#define NB_SAMPLES			 90000

#define REDUCTION			 -17.559
#define OFFSET_POS                       0.82
//#define DEBUG 				0

double force_channels_offset[ 6 ];
int num_sujet = 0;
int choix_exp = 0;
int num_exp=0;
char nom[100];
char* buf;
double consigne=0.0;


struct TrajParams
{
	double stTime;
	double endTime;
	double t_thr_int;
	double t_thr_ext;
	double stPos;
	double endPos;
	double stT_Or;
	double endT_Or;
	double kp[2];
	double kd[2];
	double ki[2];
};

double generateTarget(double pos_actuelle, int MODE, struct TrajParams params, int i);
void genTrajParams(struct TrajParams *params, double pos_virtuelle_i, int MODE, int chg, int i);
void generateMode(double pos_actuelle_0, double pos_actuelle_km1_0, int *MODE, int *p, double *timer, struct TrajParams params, int i);
void killSockets();
double sign(double x);
double MAXnum(double x, double y);
double MINnum(double x, double y);

typedef struct
{
	double ref_velocity;
	double meas_velocity;
}
MOTOR_STRUCT;

typedef struct
{
	MOTOR_STRUCT motor;
	 
}
HPC_DATA_STRUCT;

#endif

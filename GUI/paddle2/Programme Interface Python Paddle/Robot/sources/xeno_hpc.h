#ifndef XENO_HPC_H_
#define XENO_HPC_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <setjmp.h>
#include <fcntl.h>
#include <time.h>
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
#include "divers.h"

#include <alchemy/pipe.h>
#include <alchemy/timer.h>
#include <alchemy/task.h>
#include <alchemy/sem.h>
#include <alchemy/mutex.h>
#include <alchemy/heap.h>
#include <alchemy/buffer.h>

#include "s526.h"


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

int generateTarget(int pos_actuelle, int MODE, struct TrajParams params, int i);
void genTrajParams(struct TrajParams *params, int pos_actuelle1, int MODE, int chg, int i);
void generateMode(int pos_actuelle_0, int pos_actuelle_km1_0, double consigne, int *MODE, int *p, double *timer, struct TrajParams params, int i);
	
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

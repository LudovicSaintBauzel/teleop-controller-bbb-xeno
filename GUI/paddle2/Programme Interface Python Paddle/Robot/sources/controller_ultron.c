#include "xeno_hpc.h" /* main header */

#include <sys/types.h> 
#include <sys/socket.h> 
#include <stdio.h> /* for fprintf */ 
#include <string.h> /* for memcpy */ 
#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define BUFLEN 1024
#define PORT1 5005
#define PORT2 5006
#define PORT3 5007
#define PORT4 5008
#define PORT5 5009
#define TCP_PORT1 5010
#define TCP_PORT2 5011
#define TCP_PORT3 5012
#define TCP_PORT4 5013
#define SRV_IP "10.0.0.1"
#define TCP_IP "10.0.0.2"

#define FREQUENCY 2000.0 /*Hz*/
#define PERIOD 1.0/FREQUENCY


S526_STRUCT io_card;

/* Xenomai : Declaration of the differents real-time tasks and buffer */
RT_BUFFER buffer;
RT_TASK Hpc_Main_task;
RT_TASK Stock_data;
RT_TASK Recording_signal;
RT_TASK Receive_position1;
RT_TASK Receive_position2;
FILE *fp;

/* Variables used for internal communication (writing data on the recording file) */
char data[100];
int MESS_SIZE = sizeof(data);
int BUFF_SIZE = 1024;
int ENREGISTREMENT = 0;

/* Variables used for pause management */
double t0 = 0.0;
double totalPause = 0.;

/* Variables used for trajectory generation */
int STATE[2] = {100, 100};  /* STATE = type of part being played */
double t0part[2] = {0.0};
double GAUCHE = 320.;
double DROITE = 480.;
double MILIEU = 400.;


//////////////////////////////////////
/* Error function */
void error(char *s)
{
	perror(s);
	exit(1);
}


///////////////////////////////////////////////////////////////////////////////////
/* Thread used to store system's state data in a file */
void Stock_data_thread (void* unused)
{

	rt_buffer_bind(&buffer, "BUFFER", TM_INFINITE);
/*
	fp = fopen("./data/data.txt","w+");
	if(fp == NULL)
	{
		perror("Error while opening file\n");
	}
*/
	while(1)
	{
		rt_buffer_read(&buffer, &data, MESS_SIZE, TM_INFINITE);
		//printf("received : %s\n", data);
		fprintf(fp, "%s\n", data);

	}
}


///////////////////////////////////////////////////////////////////////////////////
/* Thread listening to the socket sock3, waiting for recording signal to be sent from the UI */
void Recording_signal_thread (void* unused)
{

	struct sockaddr_in sock_struct_3;

	int s3,j=0, slen3 = sizeof(sock_struct_3), r=59;
	char buf3[BUFLEN];

	s3=socket(AF_INET, SOCK_DGRAM, 0);

	bzero(&sock_struct_3, sizeof(sock_struct_3));

	sock_struct_3.sin_family = AF_INET;
	sock_struct_3.sin_addr.s_addr=htonl(INADDR_ANY);
	sock_struct_3.sin_port = htons(PORT3);
	bind(s3, (struct sockaddr *) &sock_struct_3, sizeof(sock_struct_3));

	while(1)
	{
		bzero(buf3, BUFLEN);
		r = recvfrom(s3, buf3, BUFLEN, 0, (struct sockaddr *) 0, (int *) 0);
		printf("%s\n",buf3);
		if (strcmp(buf3, "START")==0)
		{
			fp = fopen("./data/data.txt","w+");
			ENREGISTREMENT = 1;
			totalPause=0;
			t0=rt_timer_ticks2ns(rt_timer_read());
			printf("Recording starting ...\n");
		}
		else if (strcmp(buf3, "STOP") == 0)
		{
			ENREGISTREMENT = 0;
			STATE[0] = 100;
			STATE[1] = 100;
			fclose(fp);
			printf("Recording stopping ...\n");
		}
		else
		{
			printf("Wrong message received\n");
		}
	}
}


///////////////////////////////////////////////////////////////////////////////////
/* Thread used to receive the current part of the UI being played, the thread change the variable STATE depending on the part */
void Receive_position_thread1 (void* unused)
{
	struct sockaddr_in serv_addr, cli_addr;

	int s8, newS8, slen8 = sizeof(serv_addr), r=59;
	char buf8[BUFLEN];
	socklen_t clilen;
	clilen = sizeof(cli_addr);

	bzero(&serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	inet_aton(TCP_IP, &serv_addr.sin_addr);
	serv_addr.sin_port = htons(TCP_PORT3);

/*
//Version UDP
	s8=socket(AF_INET, SOCK_DGRAM, 0);
	bind(s8, (struct sockaddr *) &serv_addr, sizeof(serv_addr));


	while(1)
	{
		bzero(buf8, BUFLEN);
		r = recvfrom(s8, buf8, BUFLEN, 0, (struct sockaddr *) 0, (int *) 0);
		printf("%s\n",buf8);
*/

//Version TCP 
	s8=socket(AF_INET, SOCK_STREAM, 0);
	r = bind(s8, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	if (r<0) error("ERROR ON BINDING");
	while(1)
	{
		listen(s8,5);
		newS8 = accept(s8, (struct sockaddr *) &cli_addr, &clilen);
		if (newS8 < 0) error("ERROR ON ACCEPT");
		bzero(buf8, BUFLEN);
		r = read(newS8, buf8, 255);
		if (r<0) error("ERROR reading from socket");
		printf("Sujet 1 : %s\n",buf8);
		r = write(newS8, "data received", 13);
		if (r<0) error("ERROR on sending callback");
		close(newS8);	

		if (strcmp(buf8, "START")==0)
		{
			STATE[0] = 0;
			t0part[0] = rt_timer_ticks2ns(rt_timer_read());
		}
		else if (strcmp(buf8, "BODY_GAUCHE")==0 && (STATE[0] != 1))
		{
			STATE[0] = 1;
			t0part[0] = rt_timer_ticks2ns(rt_timer_read());
		}
		else if ((strcmp(buf8, "BODY_GAUCHE")==0) && (STATE[0] == 1))
		{
			STATE[0] = 11;
			t0part[0] = rt_timer_ticks2ns(rt_timer_read());
		}
		else if (strcmp(buf8, "BODY_DROITE")==0 && (STATE[0] != 2))
		{
			STATE[0] = 2;
			t0part[0] = rt_timer_ticks2ns(rt_timer_read());
		}
		else if ((strcmp(buf8, "BODY_DROITE")==0) && (STATE[0] == 2))
		{
			STATE[0] = 21;
			t0part[0] = rt_timer_ticks2ns(rt_timer_read());
		}
		else if (strcmp(buf8, "FORK_GAUCHE")==0)
		{
			STATE[0] = 30;
			t0part[0] = rt_timer_ticks2ns(rt_timer_read());
		}
		else if (strcmp(buf8, "FORK_DROITE")==0)
		{
			STATE[0] = 31;
			t0part[0] = rt_timer_ticks2ns(rt_timer_read());
		}
		else if (strcmp(buf8, "FORK_MILIEU")==0)
		{
			STATE[0] = 32;
			t0part[0] = rt_timer_ticks2ns(rt_timer_read());
		}
		else if (strcmp(buf8, "END")==0)
		{
			STATE[0] = 200;
			t0part[0] = rt_timer_ticks2ns(rt_timer_read());
		}
	}
		close(s8);	
}


///////////////////////////////////////////////////////////////////////////////////
/* Thread used to receive the current part of the UI being played, the thread change the variable STATE depending on the part */
void Receive_position_thread2 (void* unused)
{
	struct sockaddr_in serv_addr, cli_addr;

	int s9, newS9, slen9 = sizeof(serv_addr), r=59;
	char buf9[BUFLEN];
	socklen_t clilen;
	clilen = sizeof(cli_addr);

	bzero(&serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	inet_aton(TCP_IP, &serv_addr.sin_addr);
	serv_addr.sin_port = htons(TCP_PORT4);

/*
//Version UDP
	s9=socket(AF_INET, SOCK_DGRAM, 0);
	bind(s9, (struct sockaddr *) &serv_addr, sizeof(serv_addr));


	while(1)
	{
		bzero(buf9, BUFLEN);
		r = recvfrom(s9, buf9, BUFLEN, 0, (struct sockaddr *) 0, (int *) 0);
		printf("%s\n",buf9);
*/

//Version TCP 
	s9=socket(AF_INET, SOCK_STREAM, 0);
	r = bind(s9, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	if (r<0) error("ERROR ON BINDING");
	while(1)
	{
		listen(s9,5);
		newS9 = accept(s9, (struct sockaddr *) &cli_addr, &clilen);
		if (newS9 < 0) error("ERROR ON ACCEPT");
		bzero(buf9, BUFLEN);
		r = read(newS9, buf9, 255);
		if (r<0) error("ERROR reading from socket");
		printf("Sujet 2 : %s\n",buf9);
		r = write(newS9, "data received", 13);
		if (r<0) error("ERROR on sending callback");
		close(newS9);	

		if (strcmp(buf9, "START")==0)
		{
			STATE[1] = 0;
			t0part[1] = rt_timer_ticks2ns(rt_timer_read());
		}
		else if (strcmp(buf9, "BODY_GAUCHE")==0 && (STATE[1] != 1))
		{
			STATE[1] = 1;
			t0part[1] = rt_timer_ticks2ns(rt_timer_read());
		}
		else if ((strcmp(buf9, "BODY_GAUCHE")==0) && (STATE[1] == 1))
		{
			STATE[1] = 11;
			t0part[1] = rt_timer_ticks2ns(rt_timer_read());
		}
		else if (strcmp(buf9, "BODY_DROITE")==0 && (STATE[1] != 2))
		{
			STATE[1] = 2;
			t0part[1] = rt_timer_ticks2ns(rt_timer_read());
		}
		else if ((strcmp(buf9, "BODY_DROITE")==0) && (STATE[1] == 2))
		{
			STATE[1] = 21;
			t0part[1] = rt_timer_ticks2ns(rt_timer_read());
		}
		else if (strcmp(buf9, "FORK_GAUCHE")==0)
		{
			STATE[1] = 30;
			t0part[1] = rt_timer_ticks2ns(rt_timer_read());
		}
		else if (strcmp(buf9, "FORK_DROITE")==0)
		{
			STATE[1] = 31;
			t0part[1] = rt_timer_ticks2ns(rt_timer_read());
		}
		else if (strcmp(buf9, "FORK_MILIEU")==0)
		{
			STATE[1] = 32;
			t0part[1] = rt_timer_ticks2ns(rt_timer_read());
		}
		else if (strcmp(buf9, "END")==0)
		{
			STATE[1] = 200;
			t0part[1] = rt_timer_ticks2ns(rt_timer_read());
		}
	}
		close(s9);	
}


///////////////////////////////////////////////////////////////////////////////////
/* 
Function used to generate the automatized paddle's (1) trajectory's parameters, depending on the STATE and MODE
Initial trajectory is a minimum-jerk tractory going from the current paddle position to the task's goal. The timing comes from observation means (plus some pseudo random variation).
If the mode changes (variable chg), a new trajectory is defined to reach the opposite goal from current position.
*/ 
void genTrajParams(struct TrajParams *params, int pos_virtuelle_i, int MODE, int chg, int i)
{
	double a1 = 0.;
	double a2 = 0.;
	double u1 = 0.;
	double u2 = 0.;
	double t1 = 0.;
	double t2 = 0.;
	if (chg == 0)
	{
		u1 = ((double) rand())/RAND_MAX;
		u2 = ((double) rand())/RAND_MAX;
		a1 = sqrt(-2*log(u1))*cos(2*3.14159*u2);
		a2 = sqrt(-2*log(u1))*sin(2*3.14159*u2);
		if (MODE != 32)
		{
			a1 = (u1)*0.10 + 0.85;
			a2 = (-0.5 + u1)*2*0.05 + 1.05;
		}
		else
		{
			a1 = (u1)*0.10 + 0.95;
			a2 = (-0.5 + u1)*2*0.05 + 1.15;
		}
		t1 = (a1/0.318 - a2/0.682)/(1/0.318 - 1/0.682);
		t2 = (a1 - t1*(1 - 0.318))/0.318;
		(*(params + i)).stTime = t1;
		(*(params + i)).endTime = t2;
		(*(params + i)).stT_Or = t1;
		(*(params + i)).endT_Or = t2;
		(*(params + i)).t_thr_int = a1;
		(*(params + i)).t_thr_ext = a2;
		(*(params + i)).stPos = MILIEU;	
	}
	else if (chg == 1) 
	{
		(*(params + i)).stTime = MAX( (*(params + i)).stT_Or, (rt_timer_ticks2ns(rt_timer_read()) -t0part[i] )/1000000000.);
		(*(params + i)).endTime = (*(params + i)).stTime + 0.25;
		(*(params + i)).stPos = MILIEU*(1 - (pos_virtuelle_i - 3450.)/3450.*3.); 
	}

	if((STATE[i] == 30 && MODE == 0) || (STATE[i] == 31 && MODE == 1) || (STATE[i] == 32 && MODE == 2)) { (*(params + i)).endPos = GAUCHE; }
	else if((STATE[i] == 31 && MODE == 0) || (STATE[i] == 30 && MODE == 1) || (STATE[i] == 32 && MODE == 3)) { (*(params + i)).endPos = DROITE; }
}


///////////////////////////////////////////////////////////////////////////////////
/* 
Function used to evaluate the MODE (behavior) of the automated paddle.
MODE = 0 : the robot follows its own trajeectory.
MODE = 1 : the robot aims for the opposite (its partner's) trajectory.
The robot is initially in mode 0 and follow his planned trajectory.
If the human partners starts a trajectory before the robot, the robot follows this one.
Once the robot starts its movement, but the interaction forces reaches the threshold for long enough, he switches to the other chocie.
*/
void generateMode(int pos_actuelle_0, int pos_actuelle_km1_0, double consigne, int *MODE, int *p, double *timer, struct TrajParams params, int i)
{
	double t = (rt_timer_ticks2ns(rt_timer_read()) -t0part[i] )/1000000000.;
	double threshold = (15./MILIEU)/3.*3450;
	double force = (consigne-2.5)*1.5/2.5;
	double t_max = 2;
	double force_thr = 0.;
	double t_thr = 0.;

	if (STATE[i] == 32) {force_thr = 0.4; t_thr = 0.2;}
	else if (STATE[i] != 32 && *(MODE + i) == 1) {force_thr = 0.5; t_thr = 0.2;}
	else {force_thr = 0.7; t_thr = 0.3;}

	int C = (STATE[i] == 30 && *(MODE + i) == 0);
	int D = (STATE[i] == 31 && *(MODE + i) == 1);
	int E = (STATE[i] == 32 && *(MODE + i) == 2);

	int F = (STATE[i] == 30 && *(MODE + i) == 1);
	int G = (STATE[i] == 31 && *(MODE + i) == 0);
	int H = (STATE[i] == 32 && *(MODE + i) == 3);

	int A1 = force < -force_thr && (C || D || E);
	int B1 = force > force_thr && (F || G || H);
	int A2 = force > -force_thr && (C || D || E);
	int B2 = force < force_thr && (F || G || H);

	if ( *(p + i) == 0 && ( A1 || B1 ) )
	{
		*(p + i) = 1;
		*(timer + i) = (rt_timer_ticks2ns(rt_timer_read()) -t0part[i] )/1000000000.; 
	}
	else if (*(p + i) == 1 && ( A2 || B2 ) )
	{
		*(p + i) = 0;
	}

	if (STATE[i] == 0 || STATE[i] == 1 || STATE[i] == 11 || STATE[i] == 2 || STATE[i] == 21 || STATE[i] == 100)
	{	
		*(MODE + i) = 0;
	}
	else if (STATE[i] == 30)
	{
		if (t > 0.3 && t < params.t_thr_int && pos_actuelle_0 - 3450 < -threshold  && *(MODE + i) == 0)
		{
			*(MODE + i) = 1;
		}
		if ( (t > params.t_thr_int) && (t < t_max) && (t - *(timer + i) > t_thr && *(p + i) == 1) && (*(MODE + i) == 0) )
		{
			*(MODE + i) = 1;
		}
		else if ( (t > params.t_thr_int) && (t < t_max) && (t - *(timer + i) > t_thr && *(p + i) == 1) && (*(MODE + i) == 1) )
		{
			*(MODE + i) = 0;
		}
	}
	else if (STATE[i] == 31)
	{
		if (t > 0.3 && t < params.t_thr_int && pos_actuelle_0 - 3450 > threshold  && *(MODE + i) == 0)
		{
			*(MODE + i) = 1;
		}
		if ( (t > params.t_thr_int) && (t < t_max) && (t - *(timer + i) > t_thr && *(p + i) == 1) && (*(MODE + i) == 0) )
		{
			*(MODE + i) = 1;
		}
		else if ( (t > params.t_thr_int) && (t < t_max) && (t - *(timer + i) > t_thr && *(p + i) == 1) && (*(MODE + i) == 1) )
		{
			*(MODE + i) = 0;
		}
	}
	else if (STATE[i] == 32)
	{
		if (t > params.t_thr_int && *(MODE + i) == 0)
		{
			*(MODE + i) = 2;
		}
		if (t > 0.3 && t < params.t_thr_int && pos_actuelle_0 - 3450 > threshold  && *(MODE + i) == 0)
		{
			*(MODE + i) = 2;
		}
		else if (t > 0.3 && t < params.t_thr_int && pos_actuelle_0 - 3450 < -threshold  && *(MODE + i) == 0)
		{
			*(MODE + i) = 3;
		}
		if ( (t > params.t_thr_int) && (t < t_max) && (t - *(timer + i) > t_thr) && (*(p + i) == 1) && (*(MODE + i) == 2 || *(MODE + i) == 0) )
		{
			*(MODE + i) = 3;
 		}
		else if ( (t > params.t_thr_int) && (t < t_max) && (t - *(timer + i) > t_thr) && (*(p + i) == 1) && (*(MODE + i) == 3 || *(MODE + i) == 0) )
		{
			*(MODE + i) = 2;
 		}
	}
	else
	{
		*(MODE + i) = 1;
	}
}


///////////////////////////////////////////////////////////////////////////////////
/* 
Function used to generate the trajectory, based on the parameters defined in the function genTrajParams().
*/
int generateTarget(int pos_actuelle, int MODE, struct TrajParams params, int i)
{
	double y = 0.;
	double a = 0.;
	double t = 0.;
	double y1 = params.stPos;
	double y2 = params.endPos;
	double t1 = params.stTime;
	double t2 = params.endTime;
	int pos_cible = 0;
	double t3 = params.stT_Or;
	double t4 = params.endT_Or;
	t = (rt_timer_ticks2ns(rt_timer_read()) -t0part[i] )/1000000000.;
	if (STATE[i] == 100)
	{
		pos_cible = pos_actuelle;
	}
	if (STATE[i] == 200)
	{
		if (t < 3.)
		{
			a = MILIEU;
			y = (1 - a/MILIEU)/3.*3450+3450;
			pos_cible = (int) y;
		}
		else
		{
			pos_cible = pos_actuelle;
		}
	}
	if (STATE[i] == 0)
	{
		a = MILIEU;
		y = (1 - a/MILIEU)/3.*3450+3450;
		pos_cible = (int) y;
	}
	else if (STATE[i] == 1 || STATE[i] == 11)
	{				
		a = ( MILIEU + 80*(-1+cos(2*3.14159*t*1))/2);
		y = (1 - a/MILIEU)/3.*3450+3450;
		pos_cible = (int) y;
	}
	else if (STATE[i] == 2 || STATE[i] == 21)
	{
		a = ( MILIEU - 80*(-1+cos(2*3.14159*t*1))/2);
		y = (1 - a/MILIEU)/3.*3450+3450;
		pos_cible = (int) y;
	}
	else if ((STATE[i] == 30 || STATE[i] == 31 || STATE[i] == 32))
	{

		if (t < t1)
		{
			a = y1;
		}
		else if ((t > t1) && (t < t2))
		{
			a = y1 + (y2 - y1)*( 10*pow( (double) (t-t1)/(t2 - t1) , 3) - 15*pow( (double)  (t-t1)/(t2 - t1) , 4) + 6*pow( (double)  (t-t1)/(t2-t1) , 5) );
		}
		else if ((t > t2) && (t < 4-1+t3))
		{
			a = y2;
		}
		else if ((t > 4-1+t3) && (t < 4-1+t4))
		{
			a = y2 + (MILIEU - y2)*( 10*pow( (double) (t-(4-1+t3))/(t4 - t3) , 3) - 15*pow( (double)  (t-(4-1+t3))/(t4 - t3) , 4) + 6*pow( (double)  (t-(4-1+t3))/(t4 - t3) , 5) );
		}
		else if (t > 4-1+t4)
		{
			a = MILIEU;
		}
		else
		{
			pos_cible = pos_actuelle;
		}
		y = (1 - a/MILIEU)/3.*3450+3450;
		pos_cible = (int) y;
	}
/*			else if (STATE[i] == 32)
	{
		if ((t < 1) || (t > 4))
		{
			a = MILIEU;
			y = (1 - a/MILIEU)/3.*3450+3450;
			pos_cible = (int) y;
		}
		else
		{
			pos_cible = pos_actuelle;
		}

	}*/
	return(pos_cible);
}


/* Place les paddles sur les butees avant de demarrer les RT threads */
void Homing()
{	
	double t, t0;
	t0 = rt_timer_ticks2ns(rt_timer_read());
	t = rt_timer_ticks2ns(rt_timer_read());

	while (t - t0 < 1500000000)
	{
		s526_DACSet(0, 3);
		s526_DACSet(1, 3);
		t = rt_timer_ticks2ns(rt_timer_read());
	}	
}


///////////////////////////////////////////////////////////////////////////////////
/* Main thread, control of the paddles and communication with the UI  */
void Hpc_main_thread (void* unused)
{

	if( rt_task_set_periodic( NULL, TM_NOW, rt_timer_ns2ticks(1000000000*PERIOD)))  //periode en nanos
		printf("erreur (task_set_periodic) \n");
    
	int i = 0;
    
	for(i = 0; i < 4 ; i++) // 4 -> cf doc s526.c
	{
		s526_CounterReset(i);
	}

	char mess[100];
	char str_tmp[100];
	char pauseMess[100];
	char bufferRcv[100];
	int n;
	double t;

	int pos_cible[2] = {0};
	int pos_actuelle[2] = {0};
	int pos_virtuelle[2] = {0};
	double d_pos_actuelle[2] = {0};
	double d2_pos_actuelle[2] = {0};
	double pos_actuelle_km1[2] = {0};
	double pos_actuelle_km2[2] = {0};
	double pos_actuelle_km3[2] = {0};
	double pos_actuelle_km4[2] = {0};
	double d_pos_actuelle_km1[2] = {0};
	double d_pos_actuelle_km2[2] = {0};
	double d_pos_actuelle_km3[2] = {0};
	double d_pos_actuelle_km4[2] = {0};
	double d2_pos_actuelle_km1[2] = {0};
	double d2_pos_actuelle_km2[2] = {0};
	double d2_pos_actuelle_km3[2] = {0};
	double d2_pos_actuelle_km4[2] = {0};
	double consigne_km1[2] = {0};
	double consigne[2] = {0};
	double consigne_sent[2] = {0};
	double erreur[ 2 ] = {0};
	double d_erreur[ 2 ] = {0};
	double s_erreur[ 2 ] = {0};
	double erreur_km1[ 2 ] = {0};
	double erreur_km3[ 2 ] = {0};
	double erreur_km2[ 2 ] = {0};
	double erreur_km4[ 2 ] = {0};
	int first = 1;
	int firstConnect = 1;
	int cptr_satu = 0;
	int isPaused = 0;
	double delaiPause = 0.;
	double alpha = 0.8;
	double F = 0.00;
	double M = 0.315;
	double K = 0.0052;
	int previous_state[2] = {0};
	struct TrajParams params[2];
	params[0].kp[0] = params[1].kp[0] = 10.;//15
	params[0].kd[0] = params[1].kd[0] = 0.05;//0.2
	params[0].ki[0] = params[1].ki[0] = 3.;//5
	params[0].kp[1] = params[1].kp[1] = 50.;//15
	params[0].kd[1] = params[1].kd[1] = 0.2;//0.2
	params[0].ki[1] = params[1].ki[1] = 10.;//5	
	int MODE[2] = {0};  // MODE = 0 --> Follow initial target    // MODE = 1 --> Go for the other side
	int previous_mode[2] = {0};
	double force_inter[2] = {0.};
	int p[2] = {0};
	double timer[2] = {0.};
	double t_part;


/*
I = K*M*accel_angulaire
avec M une masse ponctuelle virtuelle placee a une distance l = 0.102m de l'axe de rotation de la poignee
sachant que r (rayon du cylindre d'enroulement du cable cote moteur) = 5mm 
et que R (rayon de l'arc de cercle de serrage de cable cote poignee) = 68mm
et que km la constante de proportionnalite couple/intensite du moteur (C = km*I)
On obtient K = r*l^2/(km*R)* (1/2pi)  (divise par 2pi car les positions sont calculees en tours et non pas en radians)
Soit K = 0.0052
*/

//////////////////////////////////////////
/*
Creation of differents sockets for communication with the UI:
sock 1 : UDP socket used to send position of paddle 1 to UI 1
sock 2 : UDP socket used to send position of paddle 2 to UI 1
sock 4 : UDP socket used to send position of paddle 1 to UI 2
sock 5 : UDP socket used to send position of paddle 2 to UI 2
sock 6 : TCP socket used for pause management of UI 1
sock 7 : TCP socket used for pause management of UI 2
*/
	struct sockaddr_in sock_struct_1;
	struct sockaddr_in sock_struct_2;
	struct sockaddr_in sock_struct_4;
	struct sockaddr_in sock_struct_5;
	struct sockaddr_in sock_struct_6;
	struct sockaddr_in sock_struct_7;

	int s1, s2, s4, s5, s6, s7, j, slen1=sizeof(sock_struct_1), slen2 = sizeof(sock_struct_2), slen4 = sizeof(sock_struct_4), slen5 = sizeof(sock_struct_5), slen6 = sizeof(sock_struct_6), slen7 = sizeof(sock_struct_7);
	char buf1[BUFLEN], buf2[BUFLEN], buf4[BUFLEN], buf5[BUFLEN], buf6[BUFLEN], buf7[BUFLEN];


//Creation des sockets UDP
	if ((s1=socket(AF_INET, SOCK_DGRAM, 0))==-1)
		error("socket");
	if ((s2=socket(AF_INET, SOCK_DGRAM, 0))==-1)
		error("socket");
	if ((s4=socket(AF_INET, SOCK_DGRAM, 0))==-1)
		error("socket");
	if ((s5=socket(AF_INET, SOCK_DGRAM, 0))==-1)
		error("socket");


//Creation des structures contenants les infos des sockets (adresse, port, taille....)
	memset((char *) &sock_struct_1, 0, sizeof(sock_struct_1));
	sock_struct_1.sin_family = AF_INET;
	sock_struct_1.sin_port = htons(PORT1);
	if (inet_aton(SRV_IP, &sock_struct_1.sin_addr)==0)
	{
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}

	memset((char *) &sock_struct_2, 0, sizeof(sock_struct_2));
	sock_struct_2.sin_family = AF_INET;
	sock_struct_2.sin_port = htons(PORT2);
	if (inet_aton(SRV_IP, &sock_struct_2.sin_addr)==0)
	{
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}

	memset((char *) &sock_struct_4, 0, sizeof(sock_struct_4));
	sock_struct_4.sin_family = AF_INET;
	sock_struct_4.sin_port = htons(PORT4);
	if (inet_aton(SRV_IP, &sock_struct_4.sin_addr)==0)
	{
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}

	memset((char *) &sock_struct_5, 0, sizeof(sock_struct_5));
	sock_struct_5.sin_family = AF_INET;
	sock_struct_5.sin_port = htons(PORT5);
	if (inet_aton(SRV_IP, &sock_struct_5.sin_addr)==0)
	{
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}

	memset((char *) &sock_struct_6, 0, sizeof(sock_struct_6));
	sock_struct_6.sin_family = AF_INET;
	sock_struct_6.sin_port = htons(TCP_PORT1);
	if (inet_aton(SRV_IP, &sock_struct_6.sin_addr)==0)
	{
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}

	memset((char *) &sock_struct_7, 0, sizeof(sock_struct_7));
	sock_struct_7.sin_family = AF_INET;
	sock_struct_7.sin_port = htons(TCP_PORT2);
	if (inet_aton(SRV_IP, &sock_struct_7.sin_addr)==0)
	{
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}

///////////////////////////////////////////



//Boucle principale (controle PID + envoi des donnees)
	while(1)
	{

/* New data acquisition is placed first in a sigle loop to avoid discrepencies between the acquisition timing of the two paddles */
		for(i = 0; i <2 ; i++)
		{
			pos_actuelle_km4[i] = pos_actuelle_km3[i];
			pos_actuelle_km3[i] = pos_actuelle_km2[i];
			pos_actuelle_km2[i] = pos_actuelle_km1[i];
			pos_actuelle_km1[i] = (double)pos_actuelle[i]/4000.;
	    
			d_pos_actuelle_km4[i] = d_pos_actuelle_km3[i];
			d_pos_actuelle_km3[i] = d_pos_actuelle_km2[i];
			d_pos_actuelle_km2[i] = d_pos_actuelle_km1[i];
			d_pos_actuelle_km1[i] = d_pos_actuelle[i];

			d2_pos_actuelle_km4[i] = d2_pos_actuelle_km3[i];
			d2_pos_actuelle_km3[i] = d2_pos_actuelle_km2[i];
			d2_pos_actuelle_km2[i] = d2_pos_actuelle_km1[i];
			d2_pos_actuelle_km1[i] = d2_pos_actuelle[i];

			s526_CounterRefresh(i,&pos_actuelle[i]);

			if(pos_actuelle[i] > 16777215/2.0)    //compteur en 24 bits
				pos_actuelle[i] -= 16777215;
		}


		for(i = 0; i <2 ; i++)
		{	
/* Calculate the velocities and accelerations of the paddles */
			d_pos_actuelle[i] = (2 * (double)pos_actuelle[i]/4000. + 1 * pos_actuelle_km1[i] + 0 * pos_actuelle_km2[i] - 1 * pos_actuelle_km3[i] - 2 * pos_actuelle_km4[i] )/(10.* PERIOD);

			d2_pos_actuelle[i] = (2 * d_pos_actuelle[i] + 1 * d_pos_actuelle_km1[i] + 0 * d_pos_actuelle_km2[i] - 1 * d_pos_actuelle_km3[i] - 2 * d_pos_actuelle_km4[i] )/(10.* PERIOD);

			d2_pos_actuelle[i] = (d2_pos_actuelle[i] + d2_pos_actuelle_km1[i] + d2_pos_actuelle_km2[i] + d2_pos_actuelle_km3[i])/4.;

/* Generate a new trajectory upon changing STATE or MODE */
			if (previous_state[i] != STATE[i])
			{
				genTrajParams(params, pos_virtuelle[i], MODE[i], 0, i);
			}
			previous_state[i] = STATE[i];

			if (previous_mode[i] != MODE[i])
			{
				genTrajParams(params, pos_virtuelle[i], MODE[i], 1, i);
			}
			previous_mode[i] = MODE[i];
			generateMode(pos_actuelle[i], (int) pos_actuelle_km1[i]*4000, force_inter[i], MODE, p, timer, params[i], i);

/* Define the PID target for each paddle */		
			pos_virtuelle[ i ] = generateTarget(pos_actuelle[ i ], MODE[i], params[i], i);

			if (STATE[i] == 100 || STATE[i] == 200)
			{
				pos_cible[i] = pos_actuelle[1-i];
			}
			else
			{	
				pos_cible[ i ] = pos_virtuelle [ i ];
			}
	
			erreur_km4[ i ] = erreur_km3[ i ];
			erreur_km3[ i ] = erreur_km2[ i ];
			erreur_km2[ i ] = erreur_km1[ i ];
			erreur_km1[ i ] = erreur[ i ];
	
	
			erreur[ i ] = (double)( pos_cible[i] - pos_actuelle[i] )/4000.0;
			d_erreur[ i ] = ( 2 * erreur[i] + 1 * erreur_km1[i] + 0 * erreur_km2[i] - 1 * erreur_km3[ i ] - 2 * erreur_km4[ i ] )/( 10. * PERIOD);
			s_erreur[i] += erreur[i]*PERIOD;
			if (s_erreur[i] >= 0.1)
				s_erreur[i] = 0.1;
			if (s_erreur[i] <= -0.1)
				s_erreur[i] = -0.1;
	
/* We use different PID parameters wether the paddles are free to move or there is a trajectory to follow */
			t_part = (rt_timer_ticks2ns(rt_timer_read()) - t0part[i])/1000000000.;
			if (STATE[i] == 100 || STATE[i] == 200)
			{
				params[i].kp[1] = params[i].kp[0] = 10.;
				params[i].kd[1] = params[i].kd[0] = 0.05;
				params[i].ki[1] = params[i].ki[0] = 3.;
			}
			else if (STATE[i] == 0)
			{
				params[i].kp[1] = 0. + (10. - 0.)/3*t_part;
				params[i].kd[1] = 0. + (0.05 - 0.)/3*t_part;
				params[i].ki[1] = 0. + (2. - 0.)/3*t_part;
			}
			else if (STATE[i] == 30 || STATE[i] == 31 || STATE[i] == 32 )
			{
				if (t_part >= 0 && t_part < 1)
				{
					params[i].kp[1] = 4. + (10. - 4.)*(1. - cos(2*3.14/2*(t_part)))/2.;
				}
				else if (t_part >= 4 && t_part < 5)
				{
					params[i].kp[1] = 4. + (10. - 4.)*(1. - cos(2*3.14/2*(t_part-3.)))/2.;
				}
				else
				{
					params[i].kp[1] = 10.;
				}			
			}
			else
			{
				params[i].kp[1] = 4.;
				params[i].kd[1] = 0.05;
				params[i].ki[1] = 3.;
			}				

			consigne_km1[i] = consigne[i];	

			/* PID */
			consigne[i] = params[i].kp[1] * erreur[ i ] + params[i].kd[1] * d_erreur[ i ] + params[i].ki[1] * s_erreur[ i ] - (d_pos_actuelle[i])*F - (d2_pos_actuelle[i])*K*M  ;

			if(consigne[i] > 1)
				consigne[i] = 1;
			else if(consigne[i] < -1)
				consigne[i] = -1;

			consigne[i] = alpha*consigne_km1[i] + (1 - alpha)*consigne[i];
	
			consigne_sent[i] = (-consigne[i] + 1) * 2.5;
/* "consigne" is saturated then modified to fit in a [0, 5] range which corresponds to the capacity of the IO card */
	
			s526_DACSet(i, consigne_sent[i]);


/* "force" variable is used to manage force threshold detection in the MODE generation */
			force_inter[i] = params[i].kp[1] * erreur[ i ] + params[i].kd[1] * d_erreur[ i ] + params[i].ki[1] * s_erreur[ i ];
			if(force_inter[i] > 1)
				force_inter[i] = 1;
			else if(force_inter[i] < -1)
				force_inter[i] = -1;
			force_inter[i] = (1 - force_inter[i])*2.5;
		}

////////////////////////////////////////////////
/*
Pause management :
If the command (consigne[i]) is saturated for more than 2s, send a pause message to UI and stop the data recording.
Once "paused", if the command stops being saturated for 2s, the process starts again.
We use 2 TCP sockets to send start and end pause messages to the UI (s6 for UI1 and s7 for UI2).
2 new sockets are created each time we need to send a message, and destroyed afterwards.
*/

		if ((fabs(consigne[0]) >= 0.95 || fabs(consigne[1]) >= 0.95 ) && isPaused == 0 && ENREGISTREMENT == 1)
		{
			cptr_satu ++;
		}
		if ((fabs(consigne[0]) <= 0.95 || fabs(consigne[1]) <= 0.95 ) && cptr_satu > 100 && isPaused == 0)
		{
			cptr_satu =0;
		}
		if (cptr_satu >= 2.5*(int)FREQUENCY && isPaused == 0 && ENREGISTREMENT == 1)
		{
			printf("Entering paused status\n");
			sprintf(pauseMess, "PAUSE\n");
			ENREGISTREMENT = 0;
			cptr_satu = 0;
			isPaused = 1;
			delaiPause = rt_timer_ticks2ns(rt_timer_read());
			if ((s6=socket(AF_INET, SOCK_STREAM, 0))==-1)
				error("socket");    
			if ((s7=socket(AF_INET, SOCK_STREAM, 0))==-1)
				error("socket");

			if(connect(s6, &sock_struct_6, sizeof(sock_struct_6)) == -1)
				error("connect()");
			if(connect(s7, &sock_struct_7, sizeof(sock_struct_7)) == -1)
				error("connect()");
			printf("Connected\n");
			firstConnect = 0;

			if (send(s6, pauseMess, strlen(pauseMess), 0)==-1)
				error("send()");
			printf("Sent 1: %s\n", pauseMess);
			if((n = recv(s6, bufferRcv, sizeof bufferRcv -1, 0)) == -1)
				error("recv()");
			bufferRcv[n]='\0';
			printf("Received1: %s\n", bufferRcv);

			if (send(s7, pauseMess, strlen(pauseMess), 0)==-1)
				error("send()");
			printf("Sent 2: %s\n", pauseMess);
			if((n = recv(s7, bufferRcv, sizeof bufferRcv -1, 0)) == -1)
				error("recv()"); 
			bufferRcv[n]='\0';
			printf("Received2: %s\n", bufferRcv);
		}

			if (isPaused == 1 && (fabs(consigne[0]) <= 0.95 || fabs(consigne[1]) <= 0.95 ))
			{
			cptr_satu ++;
			}
		if (cptr_satu >= 2*(int)FREQUENCY && isPaused == 1)
		{
			printf("Exiting paused status\n");
			sprintf(pauseMess, "UNPAUSE\n");
			cptr_satu = 0;
			isPaused = 0;
			delaiPause = rt_timer_ticks2ns(rt_timer_read()) - delaiPause;
			totalPause += delaiPause;
			if (send(s6, pauseMess, strlen(pauseMess), 0)==-1)
				error("send()");
			if((n = recv(s6, bufferRcv, sizeof bufferRcv -1, 0)) == -1)
				error("recv()");
			bufferRcv[n]='\0';
			if (send(s7, pauseMess, strlen(pauseMess), 0)==-1)
				error("send()");
			if((n = recv(s7, bufferRcv, sizeof bufferRcv -1, 0)) == -1)
				error("recv()"); 
			bufferRcv[n]='\0';
			ENREGISTREMENT = 1;
			close(s6);
			close(s7);
		}
		    
/////////////////////////////////////////////
	
		if(first)
		{
			s526_DACSet(2,5); // enable epos 1
			s526_DACSet(3,5); // enable epos 2
			first = 0;
		}
	
////////////////////////////////////////////

		if(ENREGISTREMENT == 1)
		{
	    	//Envoi des donnees dans le buffer
			t=(rt_timer_ticks2ns(rt_timer_read()) - t0 - totalPause)/1000000000;//time since start of recording in sec
			sprintf(str_tmp, "%f", t);
			strcpy(mess, str_tmp);
	  
			strcat(mess, "\t");
			sprintf(str_tmp, "%i", pos_actuelle[0]);
			strcat(mess, str_tmp);
	  
			strcat(mess, "\t");
			sprintf(str_tmp, "%f", (consigne_sent[0]-2.5)*1.5/2.5);
			strcat(mess, str_tmp);
	  
			strcat(mess, "\t");
			sprintf(str_tmp, "%i", pos_actuelle[1]);
			strcat(mess, str_tmp);
		  
			strcat(mess, "\t");
			sprintf(str_tmp, "%f", (consigne_sent[1]-2.5)*1.5/2.5);
			strcat(mess, str_tmp);

			strcat(mess, "\t");
			sprintf(str_tmp, "%i", pos_virtuelle[0]);
			strcat(mess, str_tmp);

			strcat(mess, "\t");
			sprintf(str_tmp, "%i", pos_virtuelle[1]);
			strcat(mess, str_tmp);
		  
			rt_buffer_write(&buffer, &mess, MESS_SIZE, TM_INFINITE);
		}

/////////////////////////////////////////////
/* Envoi des donnees dans les sockets */
	
		/* Reset buffers */
		bzero(buf1, BUFLEN);
		bzero(buf2, BUFLEN);
		bzero(buf4, BUFLEN);
		bzero(buf5, BUFLEN);

		/* Writing on buffers */	
		sprintf(buf1, "%i\n", pos_actuelle[0]);
		sprintf(buf2, "%i\n", pos_virtuelle[0]);
		sprintf(buf4, "%i\n", pos_virtuelle[1]);
		sprintf(buf5, "%i\n", pos_actuelle[1]);
	
		//envoi des donnes buffers dans les sockets
		if (sendto(s1, buf1, BUFLEN, 0, &sock_struct_1, slen1)==-1)
			error("sento(1)");
		if (sendto(s2, buf2, BUFLEN, 0, &sock_struct_2, slen2)==-1)
			error("sento(2)");
		if (sendto(s4, buf4, BUFLEN, 0, &sock_struct_4, slen4)==-1)
			error("sento(4)");
		if (sendto(s5, buf5, BUFLEN, 0, &sock_struct_5, slen5)==-1)
			error("sento(5)");
	
	
////////////////////////////////////////

		/* garanty the periodicity of the task */
		if(rt_task_wait_period(NULL) != 0)
			printf("Task Wait Period Error\r\n");
	}
}




/* Closes all tasks, deletes the buffer, and resets the motors command before closing */
void Hpc_end(int sig)
{	
	ENREGISTREMENT = 0;
	rt_task_delete(&Hpc_Main_task);
	rt_task_delete(&Recording_signal);
	rt_task_delete(&Receive_position1);
	rt_task_delete(&Receive_position2);

	s526_DACReset(0);
	s526_DACReset(1);
	s526_DACReset(2);
	s526_DACReset(3);

	rt_buffer_delete(&buffer);
	rt_task_delete(&Stock_data);
    //fclose(fp);

	printf("bye...\r\n");
}


/* Create the threads and launch the programm */
int main (int argc, char *argv[])
{

	iopl(3);
    
	signal(SIGINT, Hpc_end);
	signal(SIGTERM, Hpc_end);


    /* IO Card initialisation and configuration */
	s526_init();

	s526_DACReset(0);
	s526_DACReset(1);
	s526_DACReset(2);
	s526_DACReset(3);

	s526_CounterConfig(0);
	s526_CounterConfig(1);
	s526_CounterConfig(2);
	s526_CounterConfig(3);

	Homing();

    /* Xenomai: process memory locked */
	mlockall( MCL_CURRENT | MCL_FUTURE );

	printf("starting..\r\n");

	if (rt_buffer_create(&buffer, "BUFFER", BUFF_SIZE, B_FIFO))
	{
		printf("Error while creating BUFFER\r\n");
	}

	/* Création du thread secondaire Stock_data */
	if (rt_task_create(&Stock_data, "STOCK_DATA", 0, 59, T_JOINABLE))
	{
		printf("Error while creating STOCK_DATA\r\n");
	}

	if(rt_task_start(&Stock_data, &Stock_data_thread,NULL))
	{
		printf("Error while starting STOCK_DATA\r\n");
	}

    /* Création du thread secondaire Recording_signal */
	if (rt_task_create(&Recording_signal, "RECORDING_SIGNAL", 0, HPC_MAIN_TASK_PRIO, T_JOINABLE))
	{
		printf("Error while creating RECORDING_SIGNAL\r\n");
	}

	if(rt_task_start(&Recording_signal, &Recording_signal_thread,NULL))
	{
		printf("Error while starting RECORDING_SIGNAL\r\n");
	}


    /* Création du thread secondaire Receive position1 */
	if (rt_task_create(&Receive_position1, "RECEIVE_POSITION1", 0, HPC_MAIN_TASK_PRIO-1, T_JOINABLE))
	{
		printf("Error while creating RECEIVE_POSITION1\r\n");
	}

	if(rt_task_start(&Receive_position1, &Receive_position_thread1,NULL))
	{
		printf("Error while starting RECEIVE_POSITION1\r\n");
	}


    /* Création du thread secondaire Receive position2 */
	if (rt_task_create(&Receive_position2, "RECEIVE_POSITION2", 0, HPC_MAIN_TASK_PRIO-1, T_JOINABLE))
	{
		printf("Error while creating RECEIVE_POSITION2\r\n");
	}

	if(rt_task_start(&Receive_position2, &Receive_position_thread2,NULL))
	{
		printf("Error while starting RECEIVE_POSITION2\r\n");
	}


    /* Création du thread principal Hpc_main_task */
	if (rt_task_create(&Hpc_Main_task, "HPC_MAIN_TASK",HPC_MAIN_TASK_STKSZ, HPC_MAIN_TASK_PRIO-2,T_JOINABLE /*HPC_MAIN_TASK_MODE*/))
	{
		printf("Error while creating Hpc_main_task\r\n");
	}

	if(rt_task_start(&Hpc_Main_task, &Hpc_main_thread,NULL))
	{
		printf("Error while starting Hpc_main_task\r\n");
	}

	pause();
	return 0;
}

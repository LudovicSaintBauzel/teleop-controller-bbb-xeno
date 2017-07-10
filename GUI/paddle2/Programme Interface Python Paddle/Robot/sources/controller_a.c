#include "xeno_hpc.h"

#include <sys/types.h> 
#include <sys/socket.h> 
#include <stdio.h> /* for fprintf */ 
#include <string.h> /* for memcpy */ 
#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <time.h>

#define BUFLEN 1024
#define PORT1 5005
#define PORT2 5006
#define PORT3 5007
#define PORT4 5008
#define PORT5 5009
#define TCP_PORT1 5010
#define TCP_PORT2 5011
#define SRV_IP "10.0.0.1"

#define FREQUENCY 2000.0 /*Hz*/
#define PERIOD 1.0/FREQUENCY


S526_STRUCT io_card;
//M2=1; M1=0

RT_BUFFER buffer;
RT_TASK Hpc_Main_task;
RT_TASK Stock_data;
RT_TASK Recording_signal;
FILE *fp;


char data[100];
int MESS_SIZE = sizeof(data);
int BUFF_SIZE = 1024;

int ENREGISTREMENT = 0;
double t0 = 0.0;
double totalPause = 0.;

//////////////////////////////////////
void diep(char *s)
{
	perror(s);
	exit(1);
}
//////////////////////////////////////
//Thread permettant de stocker les data dans un fichier data/data.txt
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

// Thread listening to the socket sock3, waiting for recording signal to be sent from the UI
void Recording_signal_thread (void* unused)
{

	struct sockaddr_in sock_struct_3;

	int s3,j=0, slen3 = sizeof(sock_struct_3), r=59;
	char buf3[BUFLEN];

	double value_received_1=0;
	s3=socket(AF_INET, SOCK_DGRAM, 0);

	bzero(&sock_struct_3, sizeof(sock_struct_3));

	sock_struct_3.sin_family = AF_INET;
	sock_struct_3.sin_addr.s_addr=htonl(INADDR_ANY);
	sock_struct_3.sin_port = htons(PORT3);
	bind(s3, (struct sockaddr *) &sock_struct_3, sizeof(sock_struct_3));

	while(1)
	{
		if (recvfrom(s3, buf3, BUFLEN, MSG_PEEK, &sock_struct_3, &slen3)==-1)
		{
			printf("fail");
			diep("recvfrom()");
		}
		strcpy(buf3,"");
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
			fclose(fp);
			printf("Recording stopping ...\n");
		}
		else
		{
			printf("Wrong message received\n");
		}
	}
}


// Main thread, control of the paddles and communication with the UI
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
	double kp = 10.;//15
	double kd = 0.05;//0.2
	double ki = 3.;//5
	double F = 0.00;
	double M = 0.315;
	double K = 0.0052; 
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

	double value_sent_1=0;
	double value_sent_2=0;


//Creation des sockets INET
	if ((s1=socket(AF_INET, SOCK_DGRAM, 0))==-1)
		diep("socket");
	if ((s2=socket(AF_INET, SOCK_DGRAM, 0))==-1)
		diep("socket");
	if ((s4=socket(AF_INET, SOCK_DGRAM, 0))==-1)
		diep("socket");
	if ((s5=socket(AF_INET, SOCK_DGRAM, 0))==-1)
		diep("socket");


//Creation des structures contenants les infos des sockets (adresse, port, taille....)
	memset((char *) &sock_struct_1, 0, sizeof(sock_struct_1));
	sock_struct_1.sin_family = AF_INET;
	sock_struct_1.sin_port = htons(PORT1);
	if (inet_aton(SRV_IP, &sock_struct_1.sin_addr)==0){
		fprintf(stderr, "inet_aton() failed\n");
	exit(1);
	}

	memset((char *) &sock_struct_2, 0, sizeof(sock_struct_2));
	sock_struct_2.sin_family = AF_INET;
	sock_struct_2.sin_port = htons(PORT2);
	if (inet_aton(SRV_IP, &sock_struct_2.sin_addr)==0){
		fprintf(stderr, "inet_aton() failed\n");
	exit(1);
	}

	memset((char *) &sock_struct_4, 0, sizeof(sock_struct_4));
	sock_struct_4.sin_family = AF_INET;
	sock_struct_4.sin_port = htons(PORT4);
	if (inet_aton(SRV_IP, &sock_struct_4.sin_addr)==0){
		fprintf(stderr, "inet_aton() failed\n");
	exit(1);
	}

	memset((char *) &sock_struct_5, 0, sizeof(sock_struct_5));
	sock_struct_5.sin_family = AF_INET;
	sock_struct_5.sin_port = htons(PORT5);
	if (inet_aton(SRV_IP, &sock_struct_5.sin_addr)==0){
		fprintf(stderr, "inet_aton() failed\n");
	exit(1);
	}

	memset((char *) &sock_struct_6, 0, sizeof(sock_struct_6));
	sock_struct_6.sin_family = AF_INET;
	sock_struct_6.sin_port = htons(TCP_PORT1);
	if (inet_aton(SRV_IP, &sock_struct_6.sin_addr)==0){
		fprintf(stderr, "inet_aton() failed\n");
	exit(1);
	}

	memset((char *) &sock_struct_7, 0, sizeof(sock_struct_7));
	sock_struct_7.sin_family = AF_INET;
	sock_struct_7.sin_port = htons(TCP_PORT2);
	if (inet_aton(SRV_IP, &sock_struct_7.sin_addr)==0){
		fprintf(stderr, "inet_aton() failed\n");
	exit(1);
	}

///////////////////////////////////////////



//Boucle principale (controle PID + envoi des donnees)
	while(1)
	{

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
	
			d_pos_actuelle[i] = (2 * (double)pos_actuelle[i]/4000. + 1 * pos_actuelle_km1[i] + 0 * pos_actuelle_km2[i] - 1 * pos_actuelle_km3[i] - 2 * pos_actuelle_km4[i] )/(10.* PERIOD);

			d2_pos_actuelle[i] = (2 * d_pos_actuelle[i] + 1 * d_pos_actuelle_km1[i] + 0 * d_pos_actuelle_km2[i] - 1 * d_pos_actuelle_km3[i] - 2 * d_pos_actuelle_km4[i] )/(10.* PERIOD);

			d2_pos_actuelle[i] = (d2_pos_actuelle[i] + d2_pos_actuelle_km1[i] + d2_pos_actuelle_km2[i] + d2_pos_actuelle_km3[i])/4.;
/*
//Limitation of the force used to simulate the virtual mass
			if (d2_pos_actuelle[i] >= 0.5/M)
				d2_pos_actuelle[i]=0.5/M;
			if (d2_pos_actuelle[i] <= -0.5/M)
				d2_pos_actuelle[i] = -0.5/M;
*/


			//d_pos_actuelle[i] = ((double)pos_actuelle[i]/4000. - pos_actuelle_km4[i])/( 4*PERIOD );
			//d2_pos_actuelle[i] = (d_pos_actuelle[i] - d_pos_actuelle_km4[i])/( 4*PERIOD );

	
			pos_cible[ 0 ] = pos_actuelle [ 1 ];	
			pos_cible[ 1 ] = pos_actuelle [ 0 ];

	
			erreur_km4[ i ] = erreur_km3[ i ];
			erreur_km3[ i ] = erreur_km2[ i ];
			erreur_km2[ i ] = erreur_km1[ i ];
			erreur_km1[ i ] = erreur[ i ];
	
	
			erreur[ i ] = (double)( pos_cible[i] - pos_actuelle[i] )/4000.0;
			//d_erreur[ i ] = ( erreur[i] - erreur_km1[i] )/0.0005; 
			d_erreur[ i ] = ( 2 * erreur[i] + 1 * erreur_km1[i] + 0 * erreur_km2[i] - 1 * erreur_km3[ i ] - 2 * erreur_km4[ i ] )/( 10. * PERIOD);
			s_erreur[i] += erreur[i]*PERIOD;
			if (s_erreur[i] >= 0.1)
				s_erreur[i] = 0.1;
			if (s_erreur[i] <= -0.1)
				s_erreur[i] = -0.1;
	
//			consigne_km5[i] = consigne_km4[i];
//			consigne_km4[i] = consigne_km3[i];
//			consigne_km3[i] = consigne_km2[i];
//			consigne_km2[i] = consigne_km1[i];	
			consigne_km1[i] = consigne[i];	

			consigne[i] =kp * erreur[ i ] + kd * d_erreur[ i ] + ki * s_erreur[ i ] - (d_pos_actuelle[i])*F - (d2_pos_actuelle[i])*K*M  ;	

	
			if(consigne[i] > 1)
				consigne[i] = 1;
			else if(consigne[i] < -1)
				consigne[i] = -1;

			consigne[i] = alpha*consigne_km1[i] + (1 - alpha)*consigne[i];
//			consigne[i] = (consigne[i] + consigne_km1[i] + consigne_km2[i]/* + consigne_km3[i] + consigne_km4[i] + consigne_km5[i]*/)/3.;	
	
			consigne_sent[i] = (-consigne[i] + 1) * 2.5;
	
			s526_DACSet(i, consigne_sent[i]);
		}

		//printf("%f\n", consigne[0]);
////////////////////////////////////////////////////////////////////////////////////////////////////
/*
Pause management :
If the command (consigne[i]) is saturated for more than 2s, send a pause message to UI and stop the data recording.
Once "paused", if the command stops being saturated for 2s, the process starts again.
*/
		//printf("%i\n", cptr_satu);
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
				diep("socket");    
			if ((s7=socket(AF_INET, SOCK_STREAM, 0))==-1)
				diep("socket");

			if(connect(s6, &sock_struct_6, sizeof(sock_struct_6)) == -1)
				diep("connect()");
			if(connect(s7, &sock_struct_7, sizeof(sock_struct_7)) == -1)
				diep("connect()");
			printf("Connected\n");
			firstConnect = 0;

			if (send(s6, pauseMess, strlen(pauseMess), 0)==-1)
				diep("send()");
			printf("Sent 1: %s\n", pauseMess);
			if((n = recv(s6, bufferRcv, sizeof bufferRcv -1, 0)) == -1)
				diep("recv()");
			bufferRcv[n]='\0';
			printf("Received1: %s\n", bufferRcv);

			if (send(s7, pauseMess, strlen(pauseMess), 0)==-1)
				diep("send()");
			printf("Sent 2: %s\n", pauseMess);
			if((n = recv(s7, bufferRcv, sizeof bufferRcv -1, 0)) == -1)
				diep("recv()"); 
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
				diep("send()");
			if((n = recv(s6, bufferRcv, sizeof bufferRcv -1, 0)) == -1)
				diep("recv()");
			bufferRcv[n]='\0';
			if (send(s7, pauseMess, strlen(pauseMess), 0)==-1)
				diep("send()");
			if((n = recv(s7, bufferRcv, sizeof bufferRcv -1, 0)) == -1)
				diep("recv()"); 
			bufferRcv[n]='\0';
			ENREGISTREMENT = 1;
			close(s6);
			close(s7);
		}
		    

/////////////////////////////////
	
		if(first)
		{
			s526_DACSet(2,5); // enable epos 1
			s526_DACSet(3,5); // enable epos 2
			first = 0;
		}
	
/////////////////////////////////////////////////////////////////////////////////////////:

		if(ENREGISTREMENT == 1)
		{
	    	//Envoi des donnees dans le buffer
			t=(rt_timer_ticks2ns(rt_timer_read()) - t0 - totalPause)/1000000000;//time since start in sec
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
		  
			rt_buffer_write(&buffer, &mess, MESS_SIZE, TM_INFINITE);
		}
//////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////
		//Envoi des donnees dans les sockets
	
		for (j=0; j<BUFLEN; j++) //remise à zéro des buffers
		{
			buf1[j]=0,
			buf2[j]=0;
			buf4[j]=0;
			buf5[j]=0; 
		}
	
		value_sent_1 = pos_actuelle[0];
		value_sent_2 = pos_actuelle[1];
	
	
		//Ecriture des buffers
		sprintf(buf1, "%f\n", value_sent_1);
		sprintf(buf2, "%f\n", value_sent_2);
		sprintf(buf4, "%f\n", value_sent_1);
		sprintf(buf5, "%f\n", value_sent_2);
	
		//envoi des donnes buffers dans les sockets
		if (sendto(s1, buf1, BUFLEN, 0, &sock_struct_1, slen1)==-1)
			diep("sento()");
		if (sendto(s2, buf2, BUFLEN, 0, &sock_struct_2, slen2)==-1)
			diep("sento()");
		if (sendto(s4, buf4, BUFLEN, 0, &sock_struct_4, slen4)==-1)
			diep("sento()");
		if (sendto(s5, buf5, BUFLEN, 0, &sock_struct_5, slen5)==-1)
			diep("sento()");
	
	
///////////////////////////////////////////////////////////////////////////////////


		if(rt_task_wait_period(NULL) != 0)
			printf("Task Wait Period Error\r\n");
		}
	}

void Homing()
{	
	double t, t0;
	t0 = rt_timer_ticks2ns(rt_timer_read());
	t = rt_timer_ticks2ns(rt_timer_read());

	while (t - t0 < 1000000000)
	{
		s526_DACSet(0, 3);
		s526_DACSet(1, 3);
		t = rt_timer_ticks2ns(rt_timer_read());
	}	
}

	
void Hpc_end(int sig)
{	
	ENREGISTREMENT = 0;
	rt_task_delete(&Hpc_Main_task);
	rt_task_delete(&Recording_signal);

	s526_DACReset(0);
	s526_DACReset(1);
	s526_DACReset(2);
	s526_DACReset(3);

	rt_buffer_delete(&buffer);
	rt_task_delete(&Stock_data);
    //fclose(fp);

	printf("bye...\r\n");
}

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

/* Moves the paddles to the starting point before starting */
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

    /* Création du thread principal Hpc_main_task */
	if (rt_task_create(&Hpc_Main_task, "HPC_MAIN_TASK",HPC_MAIN_TASK_STKSZ, HPC_MAIN_TASK_PRIO-1,T_JOINABLE /*HPC_MAIN_TASK_MODE*/))
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

#include "xeno_hpc.h"

#include <sys/types.h> 
#include <sys/socket.h> 
#include <stdio.h> /* for fprintf */ 
#include <string.h> /* for memcpy */ 
#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <time.h>
#include <rtdm/rtdm.h>
#include <linux/types.h>  
#include <linux/spi/spidev.h>  
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))  

#define BUFLEN 256
#define PORT1 5005
#define PORT2 5006
#define PORT3 5007
#define PORT4 5008
#define PORT5 5009
#define TCP_PORT1 5010
#define TCP_PORT2 5011
#define TCP_PORT3 5012
#define TCP_PORT4 5013
#define SRV_IP "192.168.0.1"
#define TCP_IP "192.168.0.2"

double FREQUENCY = 5000.0; /*Hz*/
double PERIOD;
#define PERIOD_FSENSORS 1.0/80.
#define PERIOD_SEND_DATA 1.0/100.
#define PERIOD_SPI 1.0/1000.

#define PWM_PERIOD 10000
#define PWM_NEUTRAL 4970
#define WEIGHT_SCALE 18.2
#define ENCODER_PRECISION 4*4096

#define DEVICE_NAME_1 "xeno_encoder1"
#define DEVICE_NAME_2 "xeno_encoder2"
#define DEVICE_NAME_3 "xeno_force_sensor1"
#define DEVICE_NAME_4 "xeno_force_sensor2"
#define DEVICE_NAME_5 "xeno_pwm"

#define PWMSS0_REGISTER 0x48300000
#define PWMSS2_REGISTER 0x48304000


/* Xenomai : Declaration of the differents real-time tasks and buffer */
RT_BUFFER buffer;
RT_TASK Hpc_Main_task;
RT_TASK Stock_data;
RT_TASK Recording_signal;
RT_TASK Debug;
RT_TASK Contact;
RT_TASK Force_sensors;
RT_TASK Receive_position1;
RT_TASK Receive_position2;
RT_TASK Encoder_read;
RT_TASK Send_data;
RT_TASK SPI;
FILE *fp;
int fd;
static volatile uint32_t *eqep0;
static volatile uint32_t *eqep1;

float ARGS[20];
int sizeARGS = 0;
const int DEBUG = 1;
static int done = 0;

int COND_EXP = 1; // 0 = Alone/HFO ; 1 = HFOP ; 2 = KRP/HRP


/* Variables used for internal communication (writing data on the recording file) */
char data[100];
int MESS_SIZE = 100;
int BUFF_SIZE = 1024;
int ENREGISTREMENT = 0;
int BUFFER_TOTAL_LINES = 0;

/* Variables used for time management */
double t0 = 0.0;
double t0center = 0.0;
double totalPause = 0.;
double CENTERING_DURATION = 1.0;

/* Variables used for trajectory generation */
int STATE[2] = {100, 100};  /* STATE = type of part being played */
double t0part[2] = {0.0};
double MILIEU = 0.082;
double INTER = 0.0125; 
double DROITE;
double GAUCHE;
double OFFSET_ENCODER;
//double SENSITIVITY = 3.0;

/* RTDM Devices */
int dev_motor1, dev_motor2, dev_fsensor1, dev_fsensor2, dev_contact1, dev_pwm;

/* Global variables for data handling */
float dbg_1[2], dbg_2[2], dbg_3[2];
int POS_SEM =0, FOR_SEM = 0, SPI_SEM = 0;
double pos_actuelle[2] = {0};
double pos_virtuelle[2] = {0};
double pos_start_centering[2] = {0};
int goal[2] = {0};
float f_sensor_value[2];
float f_sensor_offset1=0, f_sensor_offset2=0;
int contact_on[2] = {0};
float SPI_value[4]; // 0: f_sensor_1; 1 : f_sensor_2; 2: contact1; 3 : contact2;
float contact_threshold_init = 0.65;

/* SPI */
static uint32_t mode = 0;  
static uint8_t bits = 8;  
static uint32_t speed = 1000000;  
static uint16_t delay;  
static const char *device =  "/dev/spidev1.0";// "/sys/devices/ocp.3/48030000.spi/spi_master/spi1/spi1.0/spidev/spidev1.0/dev";

//////////////////////////////////////
void diep(char *s)
{
	perror(s);
	exit(1);
}
double signum(double x)
{
	if (x < 0) return -1;
	if (x > 0) return 1;
	return 0;
}

double MAXnum(double x, double y)
{
	if (x <= y) return y;
	if (x > y) return x;
}

double MINnum(double x, double y)
{
	if (x <= y) return x;
	if (x > y) return y;
}

double fsaturation(double x, double limh, double limb)
{
	if (x > limh) return limh;
	else if (x < limb) return limb;
	else return x;
}

void load_motor_drivers()
{
	system("insmod ./drivers/pwm.ko");
}

void load_fsensor_drivers()
{
	system("insmod ./drivers/force_sensor1.ko");
	system("insmod ./drivers/force_sensor2.ko");
}
	

void unload_drivers()
{
	system("rmmod pwm");
	system("rmmod force_sensor1");
	system("rmmod force_sensor2");
}


//DEBUG Thread, allows to print info without overloading CPU (set DEBUG to 1 to use)
void Debug_thread (void* unused)
{

	if( rt_task_set_periodic( NULL, TM_NOW, rt_timer_ns2ticks(1000000000*0.2)))  //5 Hz
		printf("erreur (task_set_periodic) \n");
		
	int i = 0, test;
	float b;
	
	while(!done)
	{
		for(i=0;i<2;i++)
		{
			b = (fabs(f_sensor_value[0]) + fabs(f_sensor_value[1]))/2.;
			//~ printf("Test = %d\tContact 2 = %f \tContact 1 = %f\n", test, contact_value[1], contact_value[0]);
			printf(" %i : t= %f\tpos actuelle = %f\tforce = %f\tconsigne = %f\tcontact = %i\n", i, dbg_2[i], dbg_1[i], f_sensor_value[i], dbg_3[i], contact_on[i]);
		}
		
		if(rt_task_wait_period(NULL) != 0)
			printf("Task Wait Period Error.... \n");
	}
}

int transfer(int fd, int write)  
 {  
	int ret;
	uint8_t byte_r = 0x0;
	uint8_t byte_w1 = 0x9E;
	uint8_t byte_w2 = 0xDE;
	uint8_t byte_w3 = 0xAE;
	uint8_t byte_w4 = 0xEE;
	uint8_t tx_w1[] = {  
		   byte_w1,
	}; 
	uint8_t tx_w2[] = {  
		   byte_w2,
	}; 
	uint8_t tx_w3[] = {  
		   byte_w3,
	}; 
	uint8_t tx_w4[] = {  
		   byte_w4,
	}; 	
	uint8_t tx_r[] = {  
		   byte_r, byte_r,
	}; 
	uint8_t rx_r[ARRAY_SIZE(tx_r)] = {0, }; 
	uint8_t rx_w[ARRAY_SIZE(tx_w1)] = {0, }; 
	struct spi_ioc_transfer tr = {  
	   .tx_buf = (unsigned long)tx_r,  
	   .rx_buf = (unsigned long)rx_r,  
	   .len = ARRAY_SIZE(tx_r),  
	   .delay_usecs = delay,  
	   .speed_hz = speed,  
	   .bits_per_word = bits,  
	}; 
	if(write==0){
		tr.tx_buf = (unsigned long)tx_r;
		tr.rx_buf = (unsigned long)rx_r;	
		tr.len = ARRAY_SIZE(tx_r);
	}
	else if(write ==1){
		tr.tx_buf = (unsigned long)tx_w1;
		tr.rx_buf = (unsigned long)rx_w;
		tr.len = ARRAY_SIZE(tx_w1);
	}
	else if(write ==2){
		tr.tx_buf = (unsigned long)tx_w2;
		tr.rx_buf = (unsigned long)rx_w;
		tr.len = ARRAY_SIZE(tx_w2);
	}
	else if(write ==3){
		tr.tx_buf = (unsigned long)tx_w3;
		tr.rx_buf = (unsigned long)rx_w;
		tr.len = ARRAY_SIZE(tx_w3);
	}
	else if(write ==4){
		tr.tx_buf = (unsigned long)tx_w4;
		tr.rx_buf = (unsigned long)rx_w;
		tr.len = ARRAY_SIZE(tx_w4);
	}	
	else{
		return -1;
	}  
	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);  
	if (ret < 1)  
	   perror("can't send spi message");  
	//~ for (ret = 0; ret < ARRAY_SIZE(tx); ret++) {  
	   //~ if (!(ret % 6))  
			//~ puts("");  
	   //~ printf("%.2X ", rx[ret]);  
	//~ }
	if (write == 0)
		return (rx_r[0]*256+rx_r[1])/8;
	else
		return 0;
 }

/* SPI read on adc device */
void SPI_thread (void *unused)
{
	if( rt_task_set_periodic( NULL, TM_NOW, rt_timer_ns2ticks(1000000000*PERIOD_SPI)))  //periode en nanos
		printf("erreur (task_set_periodic) \n");
	int ret = 0;  
	int fd;
	double t1, t2 , t0loc;
	int value1, value2, value3, value4;
	fd = open(device, O_RDWR);  
	if (fd < 0)  
	   perror("can't open device");  
	/*  
	* spi mode  
	*/  
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);  
	if (ret == -1)  
	   perror("can't set spi mode w");  
	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);  
	if (ret == -1)  
	   perror("can't get spi mode r");  
	/*  
	* bits per word  
	*/  
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);  
	if (ret == -1)  
	   perror("can't set bits per word");  
	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);  
	if (ret == -1)  
	   perror("can't get bits per word");  
	/*  
	* max speed hz  
	*/  
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);  
	if (ret == -1)  
	   perror("can't set max speed hz");  
	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);  
	if (ret == -1)  
	   perror("can't get max speed hz");  

	t0loc=rt_timer_ticks2ns(rt_timer_read());

	while(!done){
		//~ t1=rt_timer_ticks2ns(rt_timer_read());
		SPI_SEM = 1;
		transfer(fd,1);
		//~ rt_task_sleep(10000); // Pause de 7.5µs necessaire, mais le temps d'execution de transfer() est suffisant.
		SPI_value[0] = transfer(fd,0);
		transfer(fd, 2);
		SPI_value[1] = transfer(fd, 0);
		transfer(fd, 3);
		SPI_value[2] = transfer(fd, 0);
		transfer(fd, 4);
		SPI_value[3] = transfer(fd, 0);		
		SPI_SEM = 0;
		
		//~ dbg_3[0] = SPI_value[0];
		//~ dbg_3[1] = SPI_value[1];
		
		//~ if(rt_timer_ticks2ns(rt_timer_read()) - t0loc > 100000000){
			//~ dbg_2[1] = 0;
			//~ t0loc=rt_timer_ticks2ns(rt_timer_read());
		//~ }
//~ 
		//~ t2 = (rt_timer_ticks2ns(rt_timer_read()) - t1);
		//~ dbg_2[1] = MAXnum( t2 , dbg_2[1]);
		//~ dbg_2[0] = t2;
		
		if(rt_task_wait_period(NULL) != 0)
			printf("Task Wait Period Error SPI \n");
	}
} 

/* Thread to check if fingers are in contact with the handles */
void Contact_thread (void* unused)
{

	if( rt_task_set_periodic( NULL, TM_NOW, rt_timer_ns2ticks(1000000000*0.1)))  //10 Hz
		printf("erreur (task_set_periodic) \n");

	char c[10];
	int i;
	float t0loc, tloc, t1;
	float old_c_on_val[2], contact_thresh[2] = {contact_threshold_init}, contact_value[2];;
	while(!done)
	{
		old_c_on_val[0] = contact_on[0];
		old_c_on_val[1] = contact_on[1];
		
		contact_value[0] = SPI_value[2]/4096.*1.8;
		contact_value[1] = SPI_value[3]/4096.*1.8;
					
		tloc = rt_timer_ticks2ns(rt_timer_read())/1000000000.0;
		
		for(i=0; i<2;i++){
			/*Comparaison au seuil de tension de contact*/
			if(contact_value[i] > contact_thresh[i]){
				contact_on[i] = 1;
			}
			else{
				contact_on[i] = 0;
			}
			
			/*Mise a jour du seuil pour detecter les chutes plus facilement */
			if(old_c_on_val[i] < contact_on[i]){
				contact_thresh[i] = 0.7*contact_value[i];
				contact_thresh[i] = fsaturation(contact_thresh[i], 1.2, contact_threshold_init);
			}
			else if (old_c_on_val[i] > contact_on[i]){
				t0loc = rt_timer_ticks2ns(rt_timer_read())/1000000000.0;
			}
			
			/* Reinitialisation du seuil au bout d'un certain temps sans contact*/
			if(contact_on[i] == 0 && tloc-t0loc > 3){
				contact_thresh[i] = contact_threshold_init;
			}
		}
		
		if(rt_task_wait_period(NULL) != 0)
			printf("Task Wait Period Error.... \n");
	}
}

////////////////////////////////////////////////////////////
//Reset base value of the force sensors
void F_sensor_reset(float *moyenne1, float *moyenne2, int n)
{
	int i;
	for (i=0;i<n;i++){
		*moyenne1 += SPI_value[0];
		*moyenne2 += SPI_value[1];
		usleep(PERIOD_SPI*1000000);
	}
	*moyenne1 /= n;
	*moyenne2 /= n;
}


//////////////////////////////////////////////////////////////////
// Thread lisant les capteurs d'efforts (drivers force_sensor_driver1 et 2)
void Force_sensors_thread (void *unused)
{
	if( rt_task_set_periodic( NULL, TM_NOW, rt_timer_ns2ticks(1000000000*PERIOD_SPI)))  //periode en nanos
		printf("erreur (task_set_periodic) \n");
	double value1, value2, limit = 0;
	double gravity_comp = 0;
	double f_sensor_value_km1[2] = {0};
	double cutoff_freq = 50;	
	double alpha ;
	alpha = PERIOD/(1/(2*3.14*cutoff_freq) + PERIOD);
	size_t size;
	F_sensor_reset(&f_sensor_offset1, &f_sensor_offset2, 50);
	printf("%f\t%f\n", f_sensor_offset1, f_sensor_offset2);

	while(!done)
	{
		f_sensor_value_km1[0] = f_sensor_value[0];
		f_sensor_value_km1[1] = f_sensor_value[1];
		int force_sensor_value = 100;

		gravity_comp = 0.03*9.81*sin((MILIEU)*2*3.14);
		FOR_SEM = 1;
		f_sensor_value[0] = (SPI_value[0] - f_sensor_offset1)/4096*WEIGHT_SCALE*9.81;
		f_sensor_value[1] = (SPI_value[1] - f_sensor_offset2)/4096*WEIGHT_SCALE*9.81;
		f_sensor_value[0] = (f_sensor_value[0] - gravity_comp);
		f_sensor_value[1] = (f_sensor_value[1] - gravity_comp);
		f_sensor_value[0] = alpha*f_sensor_value[0] + (1 - alpha)*f_sensor_value_km1[0];
		f_sensor_value[1] = alpha*f_sensor_value[1] + (1 - alpha)*f_sensor_value_km1[1];
		FOR_SEM = 0;
		
		//printf("gr_comp 0 : %f\t f_0 : %f\t gr_comp 1 : %f\t f_1 : %f\n", gravity_comp[0], f_sensor_value[0], gravity_comp[1], f_sensor_value[1]);		
					
		if(rt_task_wait_period(NULL) != 0)
			printf("Task Wait Period Error FORCE SENSORS \n");
	}
}


//////////////////////////////////////////////////////////////////
// Thread sending position and force data to UI through sockets 1 and 2
void Send_data_thread (void *unused)
{
	if( rt_task_set_periodic( NULL, TM_NOW, rt_timer_ns2ticks(1000000000*PERIOD_SEND_DATA)))  //periode en nanos
		printf("erreur (task_set_periodic) \n");
//////////////////////////////////////////
/*
Creation of differents sockets for communication with the UI:
sock 1 : UDP socket used to send position of paddle 1 to UI 
sock 2 : UDP socket used to send position of paddle 2 to UI 
*/
	struct sockaddr_in sock_struct_1;
	struct sockaddr_in sock_struct_2;

	int s1, s2, j, slen1=sizeof(sock_struct_1), slen2 = sizeof(sock_struct_2);
	char buf1[BUFLEN], buf2[BUFLEN];

	double value_sent_1=0;
	double value_sent_2=0;


//Creation des sockets INET
	if ((s1=socket(AF_INET, SOCK_DGRAM, 0))==-1)
		diep("socket");
	if ((s2=socket(AF_INET, SOCK_DGRAM, 0))==-1)
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
			
	while(!done)
	{
		/*Data writing semaphores*/
		while(POS_SEM || FOR_SEM){;}
	
		/* Reset buffers */
		bzero(buf1, BUFLEN);
		bzero(buf2, BUFLEN);

		sprintf(buf1, "%f\t%f\t%f\n", pos_actuelle[0], pos_virtuelle[0], f_sensor_value[0]);
		sprintf(buf2, "%f\t%f\t%f\n", pos_actuelle[1], pos_virtuelle[1], f_sensor_value[1]);

		//envoi des donnees buffers dans les sockets
		if (sendto(s1, buf1, BUFLEN, 0, &sock_struct_1, slen1)==-1)
			error("sento(1)");
		if (sendto(s2, buf2, BUFLEN, 0, &sock_struct_2, slen2)==-1)
			error("sento(2)");
			
		if(rt_task_wait_period(NULL) != 0)
			printf("Task Wait Period Error SEND DATA \n");		
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// Thread listening to the socket sock3, waiting for recording signal to be sent from the UI
void Recording_signal_thread (void* unused)
{

	struct sockaddr_in sock_struct_3;

	int s3,j=0, r=59;
	socklen_t slen3 = sizeof(sock_struct_3);
	char buf3[BUFLEN];

	double value_received_1=0;
	s3=socket(AF_INET, SOCK_DGRAM, 0);

	memset((char *) &sock_struct_3, 0, sizeof(sock_struct_3));
	sock_struct_3.sin_family = AF_INET;
	inet_aton(TCP_IP, &sock_struct_3.sin_addr);
	sock_struct_3.sin_port = htons(PORT3);
	bind(s3, (struct sockaddr *) &sock_struct_3, sizeof(sock_struct_3));
	
	while(!done)
	{
		bzero(buf3, BUFLEN);
		r = recvfrom(s3, buf3, BUFLEN, 0, (struct sockaddr *) &sock_struct_3, &slen3);
		if (r<0)
		{
			printf("fail");
			diep("recvfrom()");
		}
		else
		{
			printf("%s\n",buf3);
			if (strcmp(buf3, "START")==0)
			{
				//fp = fopen("./data/data.txt","w+");
				t0=rt_timer_ticks2ns(rt_timer_read());
				ENREGISTREMENT = 1;
				totalPause=0;
				BUFFER_TOTAL_LINES = 0;
				rt_buffer_clear(&buffer);
				printf("Recording starting ...\n");
			}
			else if (strcmp(buf3, "STOP") == 0)
			{
				ENREGISTREMENT = 0;
				STATE[0] = STATE[1] = 200;
				printf("Recording stopping , DATA NOT SAVED...\n");	
				rt_buffer_clear(&buffer);		
			}
			else if (strcmp(buf3, "STOPANDSAVE") == 0)
			{
				ENREGISTREMENT = 0;
				STATE[0] = STATE[1] = 200;
				//rt_buffer_bind(&buffer, "BUFFER", TM_INFINITE);
				fp = fopen("./data/data.txt","w+");
				while( BUFFER_TOTAL_LINES > 0)
				{
					rt_buffer_read(&buffer, &data, MESS_SIZE, TM_INFINITE);
					fprintf(fp, "%s\n", data);
					BUFFER_TOTAL_LINES --;
				}				
				fclose(fp);
				rt_buffer_clear(&buffer);
				//rt_buffer_unbind(&buffer);
				printf("...Recording stopping ...\n");
				
			}
			else if (strcmp(buf3, "CENTER") == 0)
			{
				pos_start_centering[0] = pos_actuelle[0];
				pos_start_centering[1] = pos_actuelle[1];				
				goal[0] = goal[1] = 0;
				printf("Centering handles...\n");
				t0center = rt_timer_ticks2ns(rt_timer_read());
				COND_EXP = 10;
			} 	
			else if (strcmp(buf3, "0") == 0 )
			{
				COND_EXP = 0;
				printf("Condition Expe : Alone\n");
			}
			else if (strcmp(buf3, "1") == 0)
			{
				COND_EXP = 1;
				printf("Condition Expe : HFOP\n");
			}
			else if (strcmp(buf3, "2") == 0)
			{
				COND_EXP = 2;
				printf("Condition Expe : HRP\n");
			}
			else if (strcmp(buf3, "3") == 0)
			{
				COND_EXP = 3;
				printf("Condition Expe : HFO\n");
			}
			else if (strcmp(buf3, "4") == 0)
			{
				COND_EXP = 4;
				printf("Condition Expe : KRP\n");
			}
			else
			{
				printf("Wrong message received\n");
			}
		}
	}
}


///////////////////////////////////////////////////////////////////////////////////
/* Thread used to receive the current part of the UI being played, the thread change the variable STATE depending on the part */
void Receive_position_thread1 (void* unused)
{
	struct sockaddr_in serv_addr, cli_addr;

	int s8, newS8, slen8 = sizeof(serv_addr), r=59, end = 0, optval = 1;
	char buf8[BUFLEN];
	socklen_t clilen;
	clilen = sizeof(cli_addr);

	bzero(&serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	inet_aton(TCP_IP, &serv_addr.sin_addr);
	serv_addr.sin_port = htons(TCP_PORT3);

//Version TCP 
	s8=socket(AF_INET, SOCK_STREAM, 0);
	if (s8 <0){ perror("erreur socket 8 :");}
	setsockopt(s8, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
	setsockopt(s8, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	r = bind(s8, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	if (r<0) perror("ERROR ON BINDING");
	while(!done)
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
		else if (strcmp(buf8, "END_PATH")==0)
		{
			COND_EXP = 1;
			t0part[1] = rt_timer_ticks2ns(rt_timer_read());
		}
	}
	printf("tcho !\n");
	close(s8);
}


///////////////////////////////////////////////////////////////////////////////////
/* Thread used to receive the current part of the UI being played, the thread change the variable STATE depending on the part */
void Receive_position_thread2 (void* unused)
{
	struct sockaddr_in serv_addr, cli_addr;

	int s9, newS9, slen9 = sizeof(serv_addr), r=59, end = 0,optval = 1;
	char buf9[BUFLEN];
	socklen_t clilen;
	clilen = sizeof(cli_addr);

	bzero(&serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	inet_aton(TCP_IP, &serv_addr.sin_addr);
	serv_addr.sin_port = htons(TCP_PORT4);


//Version TCP 
	s9=socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(s9, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
	setsockopt(s9, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	r = bind(s9, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	if (r<0) error("ERROR ON BINDING");
	while(!done)
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
		else if (strcmp(buf9, "END_PATH")==0)
		{
			COND_EXP = 1;
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
void genTrajParams(struct TrajParams *params, double pos_virtuelle_i, int MODE, int chg, int i)
{
	double a1 = 0.;
	double a2 = 0.;
	double u1 = 0.;
	double u2 = 0.;
	double r1 = 0.;
	double r2 = 0.;
	double t1 = 0.;
	double t2 = 0.;
	if (chg == 0)
	{
		u1 = ((double) rand())/RAND_MAX;
		u2 = ((double) rand())/RAND_MAX;
		r1 = sqrt(-2*log(u1))*cos(2*3.14159*u2);
		r2 = sqrt(-2*log(u1))*sin(2*3.14159*u2);
		if (MODE != 32)
		{
			a1 = (r1)*0.10 + 0.85;
			a2 = a1 + 0.2 + r2*0.05;
		}
		else
		{
			a1 = (r1)*0.10 + 0.95;
			a2 = a2 + 0.2 + r2*0.05;
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
		(*(params + i)).stPos = pos_virtuelle_i; 
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
void generateMode(double pos_actuelle_0, double pos_actuelle_km1_0, int *MODE, int *p, double *timer, struct TrajParams params, int i)
{
	double t = (rt_timer_ticks2ns(rt_timer_read()) -t0part[i] )/1000000000.;
	double threshold = INTER/4.;
	double force = f_sensor_value[i];
	double t_max = 2;
	double force_thr = 5;
	double t_thr = 0.;

	if (STATE[i] == 32) {force_thr = 1; t_thr = 0.3;}
	else if (STATE[i] != 32 && *(MODE + i) == 1) {force_thr = 1.5; t_thr = 0.4;}
	else {force_thr = 2; t_thr = 0.5;}

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
		if (t > 0.3 && t < params.t_thr_int && pos_actuelle_0 - OFFSET_ENCODER < -threshold  && *(MODE + i) == 0)
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
		if (t > 0.3 && t < params.t_thr_int && pos_actuelle_0 - OFFSET_ENCODER > threshold  && *(MODE + i) == 0)
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
		if (t > 0.3 && t < params.t_thr_int && pos_actuelle_0 - OFFSET_ENCODER > threshold  && *(MODE + i) == 0)
		{
			*(MODE + i) = 2;
		}
		else if (t > 0.3 && t < params.t_thr_int && pos_actuelle_0 - OFFSET_ENCODER < -threshold  && *(MODE + i) == 0)
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
double generateTarget(double pos_actuelle, int MODE, struct TrajParams params, int i)
{
	double y = 0.;
	double a = 0.;
	double t = 0.;
	double y1 = params.stPos;
	double y2 = params.endPos;
	double t1 = params.stTime;
	double t2 = params.endTime;
	double pos_cible = 0.;
	double t3 = params.stT_Or-0.1;
	double t4 = params.endT_Or-0.1;
	t = (rt_timer_ticks2ns(rt_timer_read()) -t0part[i] )/1000000000.;
	if (STATE[i] == 100)
	{
		pos_cible = pos_actuelle;
	}
	if (STATE[i] == 200)
	{
		if (t < 3.)
		{
			pos_cible = MILIEU;
		}
		else
		{
			pos_cible = pos_actuelle;
		}
	}
	if (STATE[i] == 0)
	{
		pos_cible = MILIEU;
	}
	else if (STATE[i] == 1 || STATE[i] == 11)
	{				
		pos_cible = ( MILIEU - INTER*(-1+cos(2*3.14159*t*1))/2);
	}
	else if (STATE[i] == 2 || STATE[i] == 21)
	{
		pos_cible =( MILIEU + INTER*(-1+cos(2*3.14159*t*1))/2);
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
		pos_cible = a;
	}
/*			else if (STATE[i] == 32)
	{
		if ((t < 1) || (t > 4))
		{
			a = MILIEU;
			y = (1 - a/MILIEU)/SENSITIVITY*OFFSET_ENCODER+OFFSET_ENCODER;
			pos_cible = (int) y;
		}
		else
		{
			pos_cible = pos_actuelle;
		}

	}*/
	return(pos_cible);
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Main thread, control of the paddles and communication with the UI
void Hpc_main_thread (void* unused)
{

	if( rt_task_set_periodic( NULL, TM_NOW, rt_timer_ns2ticks(1000000000*PERIOD)))  //periode en nanos
		printf("erreur (task_set_periodic) \n");
    
	int i = 0;
	double k=0;
    
    FILE *fpAIN0, *fpPWM0;
	ssize_t size;
	int encoder_value_1 =0;
	int encoder_value_2 =0;
	
	char mess[100];
	char str_tmp[100];
	int n;
	double t;

	double pos_cible[2] = {0};
	int qep1, qep2;
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
	int consigne_sent[2] = {0};
	double erreur[ 2 ] = {0};
	double d_erreur[ 2 ] = {0};
	double d_erreur_km1[2] = {0};
	double s_erreur[ 2 ] = {0};
	double erreur_km1[ 2 ] = {0};
	double erreur_km3[ 2 ] = {0};
	double erreur_km2[ 2 ] = {0};
	double erreur_km4[ 2 ] = {0};
	int testInt1 = 0;
	int testInt2 = 0;
	double cutoff_freq = 100;	
	double alpha ;
	alpha = PERIOD/(1/(2*3.14*cutoff_freq) + PERIOD);
	int previous_state[2] = {0};
	struct TrajParams params[2];
	float Mv = 0;
	float Cv = 0;
	params[0].kp[0] = params[1].kp[0] = 200;
	params[0].kd[0] = params[1].kd[0] = 1;
	params[0].ki[0] = params[1].ki[0] = 0;
	params[0].kp[1] = params[1].kp[1] = 200.;
	params[0].kd[1] = params[1].kd[1] = 1;
	params[0].ki[1] = params[1].ki[1] = 0.;			
	float kp_rp = 110.;
	float kd_rp = 1;
	float ki_rp = 0.;
	float ratio_part = 0.75;
	int MODE[2] = {0};  // MODE = 0 --> Follow initial target    // MODE = 1 --> Go for the other side
	int previous_mode[2] = {0};
	int p[2] = {0};
	double timer[2] = {0.};
	double t_part;
	float lmh = 0.89; //limite moteur haute
	float lmb = 1 - lmh; // limite moteur basse
	float r = 0.018;//m
	float R = 0.100;//m
	float lp = 0.082;//m
	float km = 0.123; //mNm/A
	double cons_max = 0;
	double pos_init[2] = {0};
	float kpmin = 200, kpmax = 1000, fint, fintmax=2*12, fintmin = 2*0.5, fmax;
	float kdmin = 1.5, kdmax = 5;
	float Kf = 0.9;
	if (sizeARGS > 1){Mv = ARGS[1];}
	if (sizeARGS > 2){Kf = ARGS[2];}
	if (sizeARGS > 3){params[0].kp[0] = params[1].kp[0] = ARGS[3];}
	if (sizeARGS > 4){params[0].kd[0] = params[1].kd[0] = ARGS[4];}
	if (sizeARGS > 5){params[0].ki[0] = params[1].ki[0] = ARGS[5];}
	if (sizeARGS > 7){kpmax = ARGS[7];}
	if (sizeARGS > 8){kpmin = ARGS[8];}	
	if (sizeARGS > 9){kdmax = ARGS[9];}
	if (sizeARGS > 10){kdmin = ARGS[10];}

	double t1, t2;
	
	double Ak=0.3, Ak1=0.3, fk=0.5, fk1=0.5, tok=0.1, tok1=0.1, p1=0, p2=1;



	//~ t0=rt_timer_ticks2ns(rt_timer_read());
//Boucle principale (controle PID + envoi des donnees)
	while(!done)
	{
		t1 = rt_timer_ticks2ns(rt_timer_read());
		for(i = 0; i <2 ; i++)
		{
			pos_actuelle_km4[i] = pos_actuelle_km3[i];
			pos_actuelle_km3[i] = pos_actuelle_km2[i];
			pos_actuelle_km2[i] = pos_actuelle_km1[i];
			pos_actuelle_km1[i] = pos_actuelle[i];
	    
			d_pos_actuelle_km4[i] = d_pos_actuelle_km3[i];
			d_pos_actuelle_km3[i] = d_pos_actuelle_km2[i];
			d_pos_actuelle_km2[i] = d_pos_actuelle_km1[i];
			d_pos_actuelle_km1[i] = d_pos_actuelle[i];

			d2_pos_actuelle_km4[i] = d2_pos_actuelle_km3[i];
			d2_pos_actuelle_km3[i] = d2_pos_actuelle_km2[i];
			d2_pos_actuelle_km2[i] = d2_pos_actuelle_km1[i];
			d2_pos_actuelle_km1[i] = d2_pos_actuelle[i];
		}

	/* lecture des positions dans les variables globables actualisees par le thread Encoder_read_thread */
		POS_SEM = 1;
		qep1 = *(int *)(eqep0+(0x180>>2));
		qep2 = *(int *)(eqep1+(0x180>>2));
		pos_actuelle[0] = (float)qep1 /((float)ENCODER_PRECISION);
		pos_actuelle[1] = (float)qep2 /((float)ENCODER_PRECISION);
		POS_SEM = 0;
	
		for(i = 0; i <2 ; i++)
		{	
/* Calculate the velocities and accelerations of the paddles */	
			d_pos_actuelle[i] = (2 * pos_actuelle[i] + 1 * pos_actuelle_km1[i] + 0 * pos_actuelle_km2[i] - 1 * pos_actuelle_km3[i] - 2 * pos_actuelle_km4[i] )/(10.* PERIOD);
			d2_pos_actuelle[i] = (2 * d_pos_actuelle[i] + 1 * d_pos_actuelle_km1[i] + 0 * d_pos_actuelle_km2[i] - 1 * d_pos_actuelle_km3[i] - 2 * d_pos_actuelle_km4[i] )/(10.* PERIOD);


///////////////////////////////////////
			if (COND_EXP == 2 || COND_EXP == 4)
			{
	/* Generate a new trajectory upon changing STATE or MODE */
	
				if (previous_state[i] != STATE[i])
				{
					if(STATE[i] == 0){
						pos_init[i] = pos_actuelle[i];
					}
					genTrajParams(params, pos_virtuelle[i], MODE[i], 0, i);
				}
				previous_state[i] = STATE[i];

				if (previous_mode[i] != MODE[i])
				{
					genTrajParams(params, pos_virtuelle[i], MODE[i], 1, i);
				}
				previous_mode[i] = MODE[i];
				generateMode(pos_actuelle[i], pos_actuelle_km1[i], MODE, p, timer, params[i], i);


	/* Define the PID target for each paddle */		
				pos_virtuelle[ i ] = generateTarget(pos_actuelle[ i ], MODE[i], params[i], i);
				
				if (STATE[i] == 100 || STATE[i] == 200)
				{
					pos_cible[i] = pos_actuelle[1-i];
				}
				else if (STATE[i] == 0)
				{
					if ((t-4)<0.75){
						pos_cible[i] = pos_init[i] + (MILIEU - pos_init[i])*(t-4)/0.75;//( 10*pow( (double) (t-4)/(3) , 3) - 15*pow( (double)  (t-4)/(3) , 4) + 6*pow( (double)  (t-4)/(3) , 5) );
					}
					else{
						pos_cible[i] = MILIEU;
					}
				}
				else
				{	
					pos_cible[ i ] = pos_virtuelle [ i ];
				}
				
	/* We use different PID parameters wether the paddles are free to move or there is a trajectory to follow */
				t_part = (rt_timer_ticks2ns(rt_timer_read()) - t0part[i])/1000000000.;
				if (STATE[i] == 100) 
				{
					params[i].kp[1] = kp_rp;
					params[i].kd[1] = kd_rp;
					params[i].ki[1] = ki_rp;
				}
				else if(STATE[i] == 200)
				{
					params[i].kp[1] = kp_rp;
					params[i].kd[1] = kd_rp;
					params[i].ki[1] = ki_rp;
				}
				else if (STATE[i] == 0)
				{
					//params[i].kp[1] = 0. + (kp_rp - kp_rp/2)/3*t_part;
					//params[i].kd[1] = 0. + (kd_rp - kp_rp/2)/3*t_part;
					//params[i].ki[1] = 0. + (ki_rp - kp_rp/2)/3*t_part;
				}
				else if (STATE[i] == 30 || STATE[i] == 31 || STATE[i] == 32 )
				{
					if (t_part >= 0 && t_part < 1)
					{
						params[i].kp[1] = kp_rp*ratio_part + (kp_rp - kp_rp/3)*(1. - cos(2*3.14/2*(t_part)))/2.;
					}
					else if (t_part >= 4 && t_part < 5)
					{
						params[i].kp[1] = kp_rp*ratio_part + (kp_rp - kp_rp/3)*(1. - cos(2*3.14/2*(t_part-3.)))/2.;
					}
					else
					{
						params[i].kp[1] = kp_rp;
					}			
				}
				else
				{
					params[i].kp[1] = kp_rp*ratio_part;
					params[i].kd[1] = kd_rp*ratio_part;
					params[i].ki[1] = ki_rp*ratio_part;
				}
			
			}
///////////////CENTERING/////////////////
			else if (COND_EXP == 10){
				if(fabs(pos_actuelle[i]-MILIEU) > 0.001 && !goal[i]){
					pos_cible[i] = pos_start_centering[i] + (MILIEU - pos_start_centering[i])/(CENTERING_DURATION)*(rt_timer_ticks2ns(rt_timer_read()) - t0center)/1000000000;
					params[i].kp[0] = 50;
					params[i].kd[0] = 0.4;
					//~ pos_cible[i] = pos_cible[i] + signum(MILIEU - pos_actuelle[i])*0.5*PERIOD;
				}
				else{
					goal[i] = 1;
					pos_cible[i] = MILIEU;
					params[i].kp[0] = 100;
					params[i].kd[0] = 0.8;
				}
			}
/////////////////////////////////////////////					
			else if(COND_EXP == 1){
				pos_cible[0] = pos_actuelle[1];
				pos_cible[1] = pos_actuelle[0];			
			}
			else {
				pos_cible[0] = pos_actuelle[0];
				pos_cible[1] = pos_actuelle[1];			
			}
///////////////////////////////////////////////


			erreur_km4[ i ] = erreur_km3[ i ];
			erreur_km3[ i ] = erreur_km2[ i ];
			erreur_km2[ i ] = erreur_km1[ i ];
			erreur_km1[ i ] = erreur[ i ];
			d_erreur_km1[i] = d_erreur[i];
	
	
			erreur[ i ] = ( pos_cible[i] - pos_actuelle[i] );
			//d_erreur[ i ] = ( erreur[i] - erreur_km1[i] )/0.0005; 
			d_erreur[ i ] = ( 25./12 * erreur[i] - 4 * erreur_km1[i] + 3 * erreur_km2[i] - 4./3 * erreur_km3[ i ] + 1./4 * erreur_km4[ i ])/( 1. * PERIOD);
			d_erreur[i] = alpha*d_erreur[i] + (1-alpha)*d_erreur_km1[i];
			s_erreur[i] += erreur[i]*PERIOD;
			s_erreur[i] = fsaturation(s_erreur[i], 0.1, -0.1);
	
//			consigne_km5[i] = consigne_km4[i];
//			consigne_km4[i] = consigne_km3[i];
//			consigne_km3[i] = consigne_km2[i];
//			consigne_km2[i] = consigne_km1[i];	
			consigne_km1[i] = consigne[i];	



/* Creation of the consigne value according to PID */

			if(COND_EXP == 1)
			{	
				fint = fabs(f_sensor_value[0] - f_sensor_value[1]);
				if(1)//contact_on[i] && contact_on[1-i])
				{
					if (fint > fintmin)
					{
						params[i].kp[0] = kpmin + (kpmax-kpmin)/(fintmax-fintmin)*(fint-fintmin);
						params[i].kp[0] = fsaturation(params[i].kp[0], 1500, 50);
						params[i].kd[0] = kdmin + (kdmax-kdmin)/(fintmax-fintmin)*(fint-fintmin);
						params[i].kd[0] = fsaturation(params[i].kd[0], 7, 1);
					}
					else
					{
						params[i].kp[0] = kpmin;
						params[i].kd[0] = kdmin;
					}
				}
				else
				{
					Kf = 0;
					params[i].kp[0] = 50;
					params[i].kd[0] = 0.5;					
				}
				consigne[i] = params[i].kp[0] * erreur[ i ] + params[i].kd[0] * d_erreur[ i ] + params[i].ki[0] * s_erreur[ i ] - (d_pos_actuelle[i])*Cv - (d2_pos_actuelle[i])*Mv  ;
				consigne[i] += Kf*f_sensor_value[1-i]*(lp)*(1./km);
				
			}
			else
			{
				consigne[i] = params[i].kp[1] * erreur[ i ] + params[i].kd[1] * d_erreur[ i ] + params[i].ki[1] * s_erreur[ i ] - (d_pos_actuelle[i])*Cv - (d2_pos_actuelle[i])*Mv  ;
			}
			
/*Filter if needed */
			//consigne[i] = alpha*consigne[i] + (1 - alpha)*consigne_km1[i];

			if (fabs(d_pos_actuelle[i]) > 1.5){
				consigne[i] =0;
			}
			
			
			//if(i==1){consigne[i] = 0;}
			//else if (ENREGISTREMENT==1){
				//fk=1;
				//fk1=fk;
				//Ak1=Ak;
				//tok1=tok;
				//if( (k/FREQUENCY - tok1) <= 1/fk1){
					//Ak = Ak1;
					//fk = fk1;
					//tok = tok1;
				//}
				//else{
					//srand(time(NULL));
					//Ak = 1.0*rand()/RAND_MAX;
					//fk = 9.0*rand()/RAND_MAX+1;
					//tok = k/FREQUENCY;
				//}
				////consigne[i]= Ak*sin(2*3.14159*fk*k/FREQUENCY);
				//consigne[i]= 3*sin(2*3.14159*t*fk*(int)(k/FREQUENCY));
				//k++;
			//}
			//else{
				//k=0;
				//consigne[i]=0;
			//}
					
			cons_max = 10; 
			consigne_sent[i] = (-consigne[i]/cons_max*(0.8*PWM_PERIOD/2.) + PWM_NEUTRAL);
			consigne_sent[i] = fsaturation(consigne_sent[i], lmh*PWM_PERIOD, lmb*PWM_PERIOD);

			//Envoie de la consigne via PWM					
			if(i==0){	    
				testInt1 = (int) consigne_sent[0];
				rt_dev_ioctl(dev_pwm, 0, testInt1);
			}
			if(i==1){
				testInt2 = (int) consigne_sent[1];
				rt_dev_ioctl(dev_pwm, 1, testInt2);				
			}
			
			//~ t2 = rt_timer_ticks2ns(rt_timer_read()) - t1;
			dbg_1[i] = pos_actuelle[i];
			dbg_2[0] = t;
			dbg_2[1] = k;
			dbg_3[i] = consigne[i];

		
		}// Fin boucle for commande
			
/////////////////////////////////////////////////////////////////////////////////////////:

		if(ENREGISTREMENT == 1)
		{
	    	//Envoi des donnees dans le buffer
			t=(rt_timer_ticks2ns(rt_timer_read()) - t0 - totalPause)/1000000000;//time since start in sec
			sprintf(str_tmp, "%.9g", t);
			strcpy(mess, str_tmp);
	  
			strcat(mess, "\t");
			sprintf(str_tmp, "%f", pos_actuelle[0]);
			strcat(mess, str_tmp);
	  
			strcat(mess, "\t");
			sprintf(str_tmp, "%i", consigne_sent[0]);
			strcat(mess, str_tmp);
			
			strcat(mess, "\t");
			sprintf(str_tmp, "%f", f_sensor_value[0]);
			strcat(mess, str_tmp);
	  
			strcat(mess, "\t");
			sprintf(str_tmp, "%f", pos_actuelle[1]);
			strcat(mess, str_tmp);
		  
			strcat(mess, "\t");
			sprintf(str_tmp, "%i", consigne_sent[1]);
			strcat(mess, str_tmp);

			strcat(mess, "\t");
			sprintf(str_tmp, "%f", f_sensor_value[1]);
			strcat(mess, str_tmp);
			
			strcat(mess, "\t");
			sprintf(str_tmp, "%f", pos_virtuelle[0]);
			strcat(mess, str_tmp);

			strcat(mess, "\t");
			sprintf(str_tmp, "%f", pos_virtuelle[1]);
			strcat(mess, str_tmp);
					  
			rt_buffer_write(&buffer, &mess, MESS_SIZE, TM_INFINITE);
			BUFFER_TOTAL_LINES++;
		}
//////////////////////////////////////////////////////////////////////////////////////////		
		//~ if(rt_timer_ticks2ns(rt_timer_read()) - t0 > 2000000000){
			//~ dbg_2[1] = 0;
			//~ t0=rt_timer_ticks2ns(rt_timer_read());
		//~ }
//~ 
		//~ t2 = (rt_timer_ticks2ns(rt_timer_read()) - t1)/1000000000;
		//~ dbg_2[1] = MAXnum( t2 , dbg_2[1]);
		//~ dbg_2[0] = t2;
///////////////////////////////////////////////////////////////////////////////////
	
		if(rt_task_wait_period(NULL) != 0)
		{
			printf("Task Wait Period Error.... \n");
			for(i=0; i<2;i++)
			{
				printf("Error %i : pos actuelle = %f\tpos cible = %f\tConsigne = %f\tConsigne sent = %i\tForce = %f\n", i, pos_actuelle[i], pos_cible[i],consigne[i],  consigne_sent[i], f_sensor_value[i]);
			}
		}
		
	}
}

/////////////////////////////////////////////////////////////
void Hpc_end(int sig)
{	
	ENREGISTREMENT = 0;
	done = 1;
	usleep(10000);
	
	//killSockets();

    //Fermeture des devices
	rt_dev_close(dev_pwm);	
	//~ rt_dev_close(dev_fsensor1);
	//~ rt_dev_close(dev_fsensor2);

	//Unmapping des registres de lectures des encodeurs
	munmap((void*)eqep0,getpagesize());
	munmap((void*)eqep1,getpagesize());
	close(fd);
	
	usleep(10000);
	
	//disable ESCONs and unload_drivers();
	system("./close_all.sh");
	usleep(10000);

	//Deleting Xenomai objects
	rt_buffer_delete(&buffer);
	rt_task_delete(&Stock_data);
	
    POS_SEM = 0;
    FOR_SEM = 0;
    rt_task_delete(&Send_data);
	rt_task_delete(&Hpc_Main_task);
	rt_task_delete(&SPI);
	rt_task_delete(&Recording_signal);
	rt_task_delete(&Force_sensors);
	rt_task_delete(&Receive_position1);
	rt_task_delete(&Receive_position2);
	if(DEBUG == 1){	rt_task_delete(&Debug);}
	rt_task_delete(&Contact);
	
	usleep(10000);
	
	printf("Bye...\r\n");
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
int BBB_Init()
{
	printf("Salut\n");
	// Initialisation des slots PWM, des eQEP et des pins Enable ESCON
	system("./init.sh");
}

/////////////////////////////////////////////////////////////////////////////////////////////

int Homing()
{
	FILE *fpPWM, *fpg;
	int homing_current = (int) (0.515*PWM_PERIOD);
	int neutral_current = PWM_NEUTRAL;
	int homing_current_o = (int) (0.515*PWM_PERIOD*100);
	int neutral_current_o = PWM_NEUTRAL*100;	
	int reset_signal = 59595959;
	
	printf("Start homing ....\n");
	
	rt_dev_ioctl(dev_pwm, 0, homing_current);
	rt_dev_ioctl(dev_pwm, 1, homing_current);	
	

	fpg = fopen("/sys/class/gpio/gpio65/direction" , "w");
	fprintf(fpg, "%s", "high");
	fclose(fpg);
	usleep(10000);
	fpg = fopen("/sys/class/gpio/gpio60/direction" , "w");
	fprintf(fpg, "%s", "high");
	fclose(fpg);
	usleep(1000000);	
	
	rt_dev_ioctl(dev_pwm, 0, neutral_current);
	rt_dev_ioctl(dev_pwm, 1, neutral_current);
		
	//~ fpg = fopen("/sys/class/gpio/gpio65/direction" , "w");
	//~ fprintf(fpg, "%s", "low");
	//~ fclose(fpg);
	//~ usleep(10000);
	//~ fpg = fopen("/sys/class/gpio/gpio60/direction" , "w");
	//~ fprintf(fpg, "%s", "low");
	//~ fclose(fpg);
	//~ usleep(10000);
	//~ 
	//~ /* Mise a 0 capteurs de force */
	//~ rt_dev_write(dev_fsensor1, (void *) &reset_signal, sizeof(int));
	//~ rt_dev_write(dev_fsensor2, (void *) &reset_signal, sizeof(int));
	//~ F_sensor_reset(&f_sensor_offset1, &f_sensor_offset2, 50);
	//~ usleep(500000);
	//~ 
	//~ fpg = fopen("/sys/class/gpio/gpio65/direction" , "w");
	//~ fprintf(fpg, "%s", "high");
	//~ fclose(fpg);
	//~ usleep(10000);
	//~ fpg = fopen("/sys/class/gpio/gpio60/direction" , "w");
	//~ fprintf(fpg, "%s", "high");
	//~ fclose(fpg);
	
	/* Mise a 0 eQEPs */
	system("echo 0 > /sys/devices/ocp.3/48300000.epwmss/48300180.eqep/position");
	system("echo 0 > /sys/devices/ocp.3/48304000.epwmss/48304180.eqep/position");	
	
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////

int main (int argc, char *argv[])
{
	DROITE = MILIEU - INTER;
	GAUCHE = MILIEU + INTER;
	OFFSET_ENCODER = MILIEU;

	int k;
	if (argc >1){
    	if (argc==2 && (strcmp(argv[1], "help")==0 || strcmp(argv[1], "-h")==0)){ printf("Ordre des arguments : Mv (def = 0.005) , Cv (0), kp (15), kd (0.2), ki (0), freq (1000), kpmax, kpmin, kdmax, kdmin\n"); return 0;}
    	sizeARGS = argc;
    	for (k=0;k<argc;k++){
    		ARGS[k] = atof(argv[k]);
    	}
    	if (sizeARGS > 6){
			FREQUENCY = ARGS[6];
		}
    }
    PERIOD = 1/FREQUENCY;
	
	iopl(3);
    
	signal(SIGINT, Hpc_end);
	signal(SIGTERM, Hpc_end);
	
	unload_drivers();	
	
    /* Xenomai: process memory locked */
    mlockall( MCL_CURRENT | MCL_FUTURE );
   
    /* Initialisation des slots BBB */
	BBB_Init();	
    
	/* Mapping the registers for encoder position (pos1 -> PWMSS0-eQEP / pos2 -> PWMSS2 - eQEP) */
	if ((fd = open ("/dev/mem", O_RDWR | O_SYNC) ) < 0) {   
		printf("Unable to open /dev/mem\n");             
		return -1;       
	}  
	eqep0 = mmap(0, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, fd, PWMSS0_REGISTER);
	if (eqep0 < 0){
		printf("Mmap failed.\n");
		return -1;
	}
	eqep1 = mmap(0, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, fd, PWMSS2_REGISTER);
	if (eqep0 < 0){
		printf("Mmap failed.\n");
		return -1;
	}
	
	/* Loading the PWM driver  */
	load_motor_drivers();  
	/* Device des pwm*/
	dev_pwm = rt_dev_open(DEVICE_NAME_5, 0);
	if (dev_pwm < 0) {
		printf("ERROR : can't open device %s (%s)\n",
				DEVICE_NAME_5, strerror(-dev_pwm));
		fflush(stdout);
		exit(1);
	}	

	//Création du thread Contact 
	if (rt_task_create(&Contact, "CONTACT", 0, 2, T_JOINABLE))
		printf("Error while creating CONTACT\r\n");
	if(rt_task_start(&Contact, &Contact_thread,NULL))
		printf("Error while starting CONTACT\r\n");		
	//Création du thread SPI
	if (rt_task_create(&SPI, "SPI", 0, HPC_MAIN_TASK_PRIO , T_JOINABLE))
		printf("Error while creating SPI\r\n");
	if(rt_task_start(&SPI, &SPI_thread,NULL))
		printf("Error while starting SPI\r\n");	
			
	/* Put the handles in stop position */
	Homing();
		
	printf("Starting...\r\n");

	if (rt_buffer_create(&buffer, "BUFFER", BUFF_SIZE*200000, B_FIFO))
		printf("Error while creating BUFFER\r\n");

/* Communication tasks */
    /* Création du thread secondaire Recording_signal */
	if (rt_task_create(&Recording_signal, "RECORDING_SIGNAL", 0, HPC_MAIN_TASK_PRIO-1, T_JOINABLE))
		printf("Error while creating RECORDING_SIGNAL\r\n");
	if(rt_task_start(&Recording_signal, &Recording_signal_thread,NULL))
		printf("Error while starting RECORDING_SIGNAL\r\n");

    /* Création du thread secondaire Receive position1 */
	if (rt_task_create(&Receive_position1, "RECEIVE_POSITION1", 0, HPC_MAIN_TASK_PRIO-2, T_JOINABLE))
		printf("Error while creating RECEIVE_POSITION1\r\n");
	if(rt_task_start(&Receive_position1, &Receive_position_thread1,NULL))
		printf("Error while starting RECEIVE_POSITION1\r\n");

    /* Création du thread secondaire Receive position2 */
	if (rt_task_create(&Receive_position2, "RECEIVE_POSITION2", 0, HPC_MAIN_TASK_PRIO-3, T_JOINABLE))
		printf("Error while creating RECEIVE_POSITION2\r\n");
	if(rt_task_start(&Receive_position2, &Receive_position_thread2,NULL))
		printf("Error while starting RECEIVE_POSITION2\r\n");
		
    /* Création du thread secondaire Send Data */
	if (rt_task_create(&Send_data, "SEND DATA", 0, HPC_MAIN_TASK_PRIO-12, T_JOINABLE))
		printf("Error while creating SEND DATA\r\n");
	if(rt_task_start(&Send_data, &Send_data_thread,NULL))
		printf("Error while starting SEND DATA\r\n");	

/* Sensors tasks */
    /* Création du thread secondaire Force_sensors */
	if (rt_task_create(&Force_sensors, "FORCE SENSORS", 0, HPC_MAIN_TASK_PRIO-10, T_JOINABLE))
		printf("Error while creating RECORDING_SIGNAL\r\n");
	if(rt_task_start(&Force_sensors, &Force_sensors_thread,NULL))
		printf("Error while starting FORCE SENSORS\r\n");

			
    /* Création du thread principal Hpc_main_task */
	if (rt_task_create(&Hpc_Main_task, "HPC_MAIN_TASK",HPC_MAIN_TASK_STKSZ, HPC_MAIN_TASK_PRIO-5,T_JOINABLE /*HPC_MAIN_TASK_MODE*/))
		printf("Error while creating Hpc_main_task\r\n");

	if(rt_task_start(&Hpc_Main_task, &Hpc_main_thread,NULL))
		printf("Error while starting Hpc_main_task\r\n");


	if(DEBUG == 1){
		//Création du thread Debug 
		if (rt_task_create(&Debug, "DEBUG", 0, 1, T_JOINABLE))
			printf("Error while creating DEBUG\r\n");

		if(rt_task_start(&Debug, &Debug_thread,NULL))
			printf("Error while starting DEBUG\r\n");
	}

					

	pause();
	return 0;
}

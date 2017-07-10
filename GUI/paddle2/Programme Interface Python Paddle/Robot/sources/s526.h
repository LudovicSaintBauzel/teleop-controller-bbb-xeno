#ifndef S526_H_
#define S526_H_

#include <stdio.h>
#include <stdlib.h> //Pour le exit
#include<sys/io.h>
#include <math.h>
#include <unistd.h> //Pour le usleep

#define NB_ADC  8 //!< Number of ADC on the board

#define BASE_ADRESS 0x300

typedef struct{

	int adress;
	double ADC_calib;
	double DAC_calib_a[ 4 ];
	double DAC_calib_b[ 4 ];

} S526_STRUCT;


extern S526_STRUCT io_card;


void cpy(void *dest, void * src, int deb, int n);
/************* Initialisation*********************/
void s526_init();


/************Conversion numérique analogique (sortie)******************/
int s526_DACReset(int channel);
int s526_DACSet(int channel, double value);
int s526_DACSetAll(double value0, double value1, double value2, double value3);



/**********Conversion analogique numérique (entrées)***************/
int s526_ADCRefresh(int channel, float *value);
int s526_ADCRefreshAll(double *tab);


/*****************COMPTEURS*******************************************/
int s526_CounterConfig(int channel);
int s526_CounterReset(int channel);
int s526_CounterPreload(int channel, int value);
int s526_CounterRefresh(int channel, int *value);
int s526_CounterRefreshAll();



#endif

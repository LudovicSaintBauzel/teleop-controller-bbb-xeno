#include"s526.h"


S526_STRUCT io_card;

void cpy(void *dest, void * src, int deb, int n)
{
    char *p_dest=(char*)dest;
    char *p_src=(char*)src;

    while(deb--)
        p_dest++;

    while(n--)
        *p_dest++=*p_src++;
}


/******************** IO CARD INITIALIZATION *********************/
void s526_init(  )
{
	int i, retval, value1;
	
	
	io_card.adress = BASE_ADRESS;
	
	//Autorisation acces au port
	retval = ioperm( io_card.adress, 64, 1 );
	
	if (0 != retval) {
	    perror("ioperm error.");
	    printf("(You must be root)\n\n");
	    printf("Impossible to initialize s526 board.\nBye bye!\n\n");
	    exit(-1);
	}
	else
    {
        printf("--------------- s526 card ---------------\nBase adress : %d\n", io_card.adress);
        
		//Calibration parameters
        printf("\n--------Calibration-------\n");
		
		value1 = 0;
        
		//For ADC (auto) :
        io_card.ADC_calib = 0.0;
        
        for(i=0; i<4; i++)
        {
            outw(((0x20+i)*0x8)+0x5,io_card.adress+0x34); //Command 0b101
            usleep(1000);
            value1=inw(io_card.adress+0x32);
            cpy(&io_card.ADC_calib, &value1, 2*i, 2);
        }
        
		printf("ADC calibration value : %lf\n", io_card.ADC_calib);

        //For DAC (manual) :
        io_card.DAC_calib_a[0]=3229.0;
        io_card.DAC_calib_a[1]=3229.0;
        io_card.DAC_calib_a[2]=3229.0;
        io_card.DAC_calib_a[3]=3229.0;
        io_card.DAC_calib_b[0]=65535/2.0+15;
        io_card.DAC_calib_b[1]=65535/2.0+98;
        io_card.DAC_calib_b[2]=65535/2.0+98;
        io_card.DAC_calib_b[3]=65535/2.0+170;
      
        printf("--------------------------\n\n");
        //Enable ADC interrupt :
        outw(0x0004,io_card.adress+0x0C); //0b0000000000000100
        printf("ADC interrupt enabled.\n");
        printf("\n-----------------------------------------\n\n");
    }
}


/*-----------------------------------------------------------------------------------------------------------------*/
/*#################################################  DAC METHODS  #################################################*/
/*-----------------------------------------------------------------------------------------------------------------*/
//! Reset the DAC (output to 0V)
//! \param channel : channel number to reset
int s526_DACReset(int channel)
{
	outw(0x0008,io_card.adress+0x04); //Reset & selection channel0
	return 0;
}
//! Set a value to the corresponding DAC through buffer (2 instructions)
//! \param channel : channel number to set
//! \param value : value to set in volt, range:-10.0/+10.0
int s526_DACSet(int channel, double value)
{
	switch(channel)
	{
		case 0:
	    outw(0x8000,io_card.adress+0x04); //Choix channel0
	    	break;
		case 1:
            outw(0x8002,io_card.adress+0x04); //Choix channel1
		break;
		case 2:
            outw(0x8004,io_card.adress+0x04); //Choix channel2
		break;
		case 3:
            outw(0x8006,io_card.adress+0x04); //Choix channel3
		break;
		default:
			printf("s526_DAC_set : Bad channel number.\n");
			return -1;
		break;
	}
	if (value>=10.0) value=10.0;
	if (value<=-10.0) value=-10.0; 
		   
	 value=io_card.DAC_calib_a[channel]*value+io_card.DAC_calib_b[channel];//Conversion de la valeur en volt a l'aide des parametres de calibration

	if (!(( !(value < 0) && !(value > 0) && !(value == 0) ) || ( fabs( value ) == 1.0 / 0.0 ))) 
		{
		  outw((unsigned short int)value,io_card.adress+0x08); //Ecriture valeur dans le buffer
		  outw(0x8001,io_card.adress+0x04); //Ecriture buffers sur les channels
		}
	


	return 0;
}

//! Set all the DAC channel
//! \param value0 : value (in volt) to set on the channel 0
//! \param value1 : value (in volt) to set on the channel 1
//! \param value2 : value (in volt) to set on the channel 2
//! \param value3 : value (in volt) to set on the channel 3
int s526_DACSetAll(double value0, double value1, double value2, double value3)
{

	outw(0x8000,io_card.adress+0x04); //Choix channel 0
	value0=io_card.DAC_calib_a[0]*value0+io_card.DAC_calib_b[0]; //Conversion
	outw(value0,io_card.adress+0x08); //Ecriture valeur dans le buffer0

	outw(0x8002,io_card.adress+0x04); //Choix channel 1
	value1=io_card.DAC_calib_a[1]*value1+io_card.DAC_calib_b[1];
	outw(value1,io_card.adress+0x08); //Ecriture valeur dans le buffer1

	outw(0x8004,io_card.adress+0x04); //Choix channel 2
	value2=io_card.DAC_calib_a[2]*value2+io_card.DAC_calib_b[2];
	outw(value2,io_card.adress+0x08); //Ecriture valeur dans le buffer2

	outw(0x8006,io_card.adress+0x04); //Choix channel 3
	value3=io_card.DAC_calib_a[3]*value3+io_card.DAC_calib_b[3];
	outw(value3,io_card.adress+0x08); //Ecriture valeur dans le buffer3


	outw(0x8001,io_card.adress+0x04); //Ecriture buffers sur les channels

	return 0;
}
/*#################################################################################################################*/

/*-----------------------------------------------------------------------------------------------------------------*/
/*#################################################  ADC METHODS  #################################################*/
/*-----------------------------------------------------------------------------------------------------------------*/
//! Fill the ADC_value with the current value (from card) and return it
//! \param channel : channel number to set
//! \param value : the return value (in volt)
int ADCRefresh(int channel, float *value)
{
	unsigned short int int_bit=0;
	short int buffer, ref10V, ref0V;
	double val;
	//Choix channel
	switch(channel)
	{
		case 0:
			outw(0xE021,io_card.adress+0x06); //Choix channel 0 0b1110000000100001
        break;
		case 1:
            outw(0xE043,io_card.adress+0x06); //Choix channel 1 0b1110000001000011
		break;
		case 2:
            outw(0xE085,io_card.adress+0x06); //Choix channel 2 0b1110000010000101
		break;
		case 3:
            outw(0xE107,io_card.adress+0x06); //Choix channel 3 0b1110000100000111
		break;
		case 4:
			outw(0xE209,io_card.adress+0x06); //Choix channel 4 0b1110001000001001
		break;
		case 5:
            outw(0xE40B,io_card.adress+0x06); //Choix channel 5 0b1110010000001011
		break;
		case 6:
            outw(0xE80D,io_card.adress+0x06); //Choix channel 6 0b1110100000001101
		break;
		case 7:
            outw(0xF00F,io_card.adress+0x06); //Choix channel 7 0b1111000000001111
		break;
		default:
			printf("s526_ADC_refresh : Bad channel number.\n");
			return -1;
		break;
	}

	//Attente mise a jour du buffer
	
	while(int_bit!=4)
	{
	    int_bit=inw(io_card.adress+0x0E); //Lecture registre des interruptions
	    int_bit=int_bit & 0x0004; //Recuperation du bit d'interruption de l'ADC 0b0000000000000100
	}
	outw(0x0004,io_card.adress+0x0E); //Reset du bit d'interruption 0b0000000000000100


    //Récupération et conversion valeur
    
    buffer=inw(io_card.adress+0x08); //Lecture valeur du buffer

    outw(0xE010,io_card.adress+0x06); //Choix channel reference 0 (+10V) 0b1110000000010000
    ref10V=inw(io_card.adress+0x08); //Lecture valeur buffer

    outw(0xE012,io_card.adress+0x06); //Choix channel reference 1 (0V) 0b1110000000010010
    ref0V=inw(io_card.adress+0x08); //Lecture valeur buffer

    val =io_card.ADC_calib*((buffer-ref0V)/(double)(ref10V-ref0V)); //Conversion �  l'aide des valeurs de référence


    //Affecte la valeur de retour s'il y'en a une
    if(value!=NULL)
    {
        (*value)=val;
    }

	return 0;
}


//! Fill the ADC_values with the new values from card (only the 6 first channels)
int s526_ADCRefreshAll(double *tab)
{
    	short int buffer[NB_ADC];
	unsigned short int int_bit=0;
	short int ref10V, ref0V;
   	int v,i;
	double tab2[NB_ADC],tab3[NB_ADC],tab4[NB_ADC];

for(v=0; v<3; v++)
{
    outw(0xFFE1,io_card.adress+0x06); //Choix buffer channel0 et capture de toutes les voix 0b1111111111100001
    
	while(int_bit!=4) //Attente mise a jour du buffer
	{
	    int_bit=inw(io_card.adress+0x0E); //Lecture registre des interruptions
	    int_bit=int_bit & 0x0004; //Recuperation du bit d'interruption de l'ADC 0b0000000000000100
	}
	outw(0x0004,io_card.adress+0x0E); //Reset du bit d'interruption 0b0000000000000100
    buffer[0]=inw(io_card.adress+0x08); //Récupération buffer channel 0

    outw(0x0042,io_card.adress+0x06); //Choix buffer channel1 0b0000000001000010
    buffer[1]=inw(io_card.adress+0x08); //Récupération buffer channel 1

    outw(0x0084,io_card.adress+0x06); //Choix buffer channel2 0b0000000010000100
    buffer[2]=inw(io_card.adress+0x08); //Récupération buffer channel 2

    outw(0x0106,io_card.adress+0x06); //Choix buffer channel3 0b0000000100000110
    buffer[3]=inw(io_card.adress+0x08); //Récupération buffer channel 3

    outw(0x0208,io_card.adress+0x06); //Choix buffer channel4 0b0000001000001000
    buffer[4]=inw(io_card.adress+0x08); //Récupération buffer channel 4

    outw(0x040A,io_card.adress+0x06); //Choix buffer channel5 0b0000010000001010
    buffer[5]=inw(io_card.adress+0x08); //Récupération buffer channel 5

    /*outw(0x080C,io_card.adress+0x06); //Choix buffer channel6 0b0000100000001100
    buffer[6]=inw(io_card.adress+0x08); //Récupération buffer channel 6
    outw(0x100D,io_card.adress+0x06); //Choix buffer channel7 0b0001000000001110
    buffer[7]=inw(io_card.adress+0x08); //Récupération buffer channel 7*/
    
    outw(0xE010,io_card.adress+0x06); //Choix channel reference 0 (+10V) 0b1110000000010000
    ref10V=inw(io_card.adress+0x08); //Lecture valeur buffer
    outw(0xE012,io_card.adress+0x06); //Choix channel reference 1 (0V) 0b1110000000010010
    ref0V=inw(io_card.adress+0x08); //Lecture valeur buffer




    for(i=0; i<6; i++)
    {

        if (v==0) tab2[i]=io_card.ADC_calib*((buffer[i]-ref0V)/(double)(ref10V-ref0V)); //Conversion �  l'aide des valeurs de référence
        if (v==1) tab3[i]=io_card.ADC_calib*((buffer[i]-ref0V)/(double)(ref10V-ref0V)); //Conversion �  l'aide des valeurs de référence
        if (v==2) tab4[i]=io_card.ADC_calib*((buffer[i]-ref0V)/(double)(ref10V-ref0V)); //Conversion �  l'aide des valeurs de référence

    }
int_bit=0;
}

for(i=0; i<6; i++)
{
    *(tab+i)=(tab2[i]+tab3[i]+tab4[i])/3.0; //Conversion �  l'aide des valeurs de référence
}

return 0;

}

/*-----------------------------------------------------------------------------------------------------------------*/
/*###############################################  COUNTERS METHODS  ##############################################*/
/*-----------------------------------------------------------------------------------------------------------------*/
//! Default configuration for encoder (enable, in quadrature...)
//! \param channel : channel number
int s526_CounterConfig(int channel)
{
	switch(channel)
	{
		case 0:
			outw(0x0680,io_card.adress+0x16); //0b0000011010000000
			outw(0x8000,io_card.adress+0x18); // 0b1000000000000000
		break;
		case 1:
			outw(0x0680,io_card.adress+0x1E); //0b0000011010000000
			outw(0x8000,io_card.adress+0x20); //0b1000000000000000
		break;
		case 2:
			outw(0x0680,io_card.adress+0x26);
			outw(0x8000,io_card.adress+0x28);
		break;
		case 3:
			outw(0x0680,io_card.adress+0x2E);
			outw(0x8000,io_card.adress+0x30);
		break;
		default:
			printf("s526_Counter_config: Bad channel number.\n");
			return -1;
		break;
	}

	return 0;
}

//! Set the encoder channel to 0
//! \param channel : channel number
int s526_CounterReset(int channel)
{
    switch(channel)
	{
		case 0:
			outw(0x8000,io_card.adress+0x18); //0b1000000000000000
		break;
		case 1:
			outw(0x8000,io_card.adress+0x20); //0b1000000000000000
		break;
		case 2:
			outw(0x8000,io_card.adress+0x28);
		break;
		case 3:
			outw(0x8000,io_card.adress+0x30);
		break;
		default:
			printf("s526_Counter_reset: Bad channel number.\n");
			return -1;
		break;
	}

	return 0;
}

int s526_CounterPreload(int channel, int value)
{
	int value_l, value_h;

	if(value>65535)
	{
		value_h=(int)(value/65536);
		value_l=value-(value_h*65536);
	}
	else
	{
		value_h=0;
		value_l=value;
	}

	switch(channel)
	{
		case 0:
			outw(value_h,io_card.adress+0x14);
			outw(value_l,io_card.adress+0x12);
		break;
		case 1:
			outw(value_h,io_card.adress+0x1A);
			outw(value_l,io_card.adress+0x1C);
		break;
		case 2:
			outw(value_h,io_card.adress+0x22);
			outw(value_l,io_card.adress+0x24);
		break;
		case 3:
			outw(value_h,io_card.adress+0x2A);
			outw(value_l,io_card.adress+0x2C);
		break;
		default:
			printf("s526_Counter_preload: Bad channel number.\n");
			return -1;
		break;
	}

	return 0;
}

//! Fill the encoder_value corresponding to the channel number with the current value (from card) and return it
//! \param channel : Encoder channel number
//! \param value : Pointer to fill the new value with
int s526_CounterRefresh(int channel, int *value)
{
	int value_l, value_h;

	switch(channel)
	  {
	  case 0:
	    value_l=inw(io_card.adress+0x12);
	    value_h=inw(io_card.adress+0x14);
	    //encoder_value[0]=value_h*65536+value_l;
	    break;
	  case 1:
	    value_l=inw(io_card.adress+0x1A);
	    value_h=inw(io_card.adress+0x1C);
            //encoder_value[1]=value_h*65536+value_l;
	    break;
	  case 2:
            value_l=inw(io_card.adress+0x22);
	    value_h=inw(io_card.adress+0x24);
	    //encoder_value[2]=value_h*65536+value_l;
	    break;
	  case 3:
            value_l=inw(io_card.adress+0x2A);
	    value_h=inw(io_card.adress+0x2C);
	    //encoder_value[3]=value_h*65536+value_l;
	    break;
	  default:
	    printf("s526_Counter_get: Bad channel number.\n");
	    return -1;
	    break;
	  }



	*value=value_h*65536+value_l;
	return 0;
}





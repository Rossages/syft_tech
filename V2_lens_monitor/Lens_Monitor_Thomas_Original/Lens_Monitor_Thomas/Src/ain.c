
#include "stm32f7xx_hal.h"
#include "global.h"


// adjust the number of MUX Channels per ADC here, its different for every board, total 15 measurements
#define NO_OF_ADC1_CHANNELS 0
#define NO_OF_ADC2_CHANNELS 7
#define NO_OF_ADC3_CHANNELS 4
//Total Number of ADCs
#define NO_OF_ADCs 3

// define an array that will hold all values of the 12bit ADCs
uint32_t  AIN[NO_OF_ADC1_CHANNELS+NO_OF_ADC2_CHANNELS+NO_OF_ADC3_CHANNELS];

//current ADC configuration

/**ADC2 GPIO Configuration
 *       Name           Index Actual Voltage          Multiplier

PC0     ------> ADC2_IN10 2       +48V			          ((47+2.2)/2.2)*2.5
PC2     ------> ADC2_IN12 3	      -12V					  (100/10)*(-1)*2.5
PC3     ------> ADC2_IN13 4       -15V					  (100/10)*(-1)*2.5
PA6     ------> ADC2_IN6  0    PULSE CIRCUIT VOLATGE	  ((100+10)/10)*2.5
PA7     ------> ADC2_IN7  1    PULSE CIRCUIT CURRENT	  (1/2)*100*2.5 // current mirror ratio is 1:2 (mA)
PC4     ------> ADC2_IN14 5	    +12V					  ((100+10)/10)*2.5
PC5     ------> ADC2_IN15 6     +15V					  ((100+10)/10)*2.5
*/
/**ADC3 GPIO Configuration

PF3     ------> ADC3_IN9  9      VDD						((10+10)/10)*2.5
PF4     ------> ADC3_IN14 10     TS4 Chamber				(30+(1.3-ADC/4095*2.5))/8.2mv/C // verify!
PF9     ------> ADC3_IN7  7      6V8						((10+4.7)/4.7)*2.5
PF10    ------> ADC3_IN8  8      5V							((10+4.7)/4.7)*2.5



    */

/*
 * The above channels in ascending order
 *
   0       PULSE CIRCUIT VOLATGE	 -->    ((100+10)/10)*2.5
   1       PULSE CIRCUIT CURRENT 	 -->    (1/2)*100*2.5 // as current mirror ratio is 1:2 (mA)
   2       +48V			             -->    ((47+2.2)/2.2)*2.5
   3	   -12V					     -->    (100/10)*(-1)*2.5
   4       -15V					     -->    (100/10)*(-1)*2.5
   5	    +12V					 -->    ((100+10)/10)*2.5
   6       +15V					     -->    ((100+10)/10)*2.5
   7       6V8			             -->	((10+4.7)/4.7)*2.5
   8       5V			             -->	((10+4.7)/4.7)*2.5
   9       VDD			             -->	((10+10)/10)*2.5
   10      TempSensor TS4 Chamber	 -->	30+(1.3-(ADC/4095)*2.5)*(1000/8.2)
 */
void readAins(void)
{


	for (int jj=1;jj<=NO_OF_ADCs;jj++)
	{

	 switch(jj)
	 {
		         case 1: //ADC1 (N/A)
		        //	 HAL_ADC_Start_DMA(&hadc1,(uint32_t *) &AIN,NO_OF_ADC1_CHANNELS);//
					break;
		         case 2: //ADC2, start AIN index from the last previous index of ADC1
		        	 HAL_ADC_Start_DMA(&hadc2,(uint32_t *) &AIN[NO_OF_ADC1_CHANNELS],NO_OF_ADC2_CHANNELS); //
		        	 break;
		         case 3: //ADC3,start AIN index from the last previous index of the ADC2
		        	 HAL_ADC_Start_DMA(&hadc3,(uint32_t *) &AIN[NO_OF_ADC1_CHANNELS+NO_OF_ADC2_CHANNELS],NO_OF_ADC3_CHANNELS); //
		        	 break;
	 }

	}

}

// get the analogue value of any channel max value for 12 bit is 4096

int ain_get( int index ){

	if(index >= (NO_OF_ADC1_CHANNELS+NO_OF_ADC2_CHANNELS+NO_OF_ADC3_CHANNELS))return 0;

	return AIN[index];

}

// read all ADC channels and update the AIN array
StatusCode_t update_ain() {
   	readAins();

   	return STATUS_OK;
}

// reset the DMA every time a full transfer is complete

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc){
	// HAL_ADC_Stop_DMA(&hadc1); (N/A)
	 HAL_ADC_Stop_DMA(&hadc2);
	 HAL_ADC_Stop_DMA(&hadc3);

}

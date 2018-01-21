/*
 * adc_external_ADS8638.c
 *
 *  Created on: 16/06/2017
 *      Author: Esam.Alzqhoul
 */

/*
 * This is a driver to read 12bit ADC voltages from the Texas Instrument TI ADS8638 ADCs
 * Each chip has 8 channels and and has a built in temperature sensor
 *
 * General Comments:
 * - commands are executed upon a low on the CS pin
 * - setting the PD pin to active low will power down the chip
 * - it can also be powered down with an SPI command (optional)
 */

/*
 * channels 0-15 , 16 is nA, 17 is pA voltages for all lenses
 * channels 18-32 currents on all lenses, 33 is nA, 34 is pA
 *
 * temperature is
 * C = (Output Code – CREF)/mREF, mREF_2.5 = 0.47 and CREF_2.5 = 3777.2
 *
 *
 *Note: SPI configurations are in spi.c, clock speed 48MHz/4
 */


#include "global.h"
#include "types.h"

#define MAX_NUMBER_ADC_ADS8638_CHANNELS 8 // total number of channels on each ADS8638 ADC
#define MAX_NUMBER_ADC_ADS8638_CHIPS 5 // total number of ADS8638 ADCs

#define ADS8638_INTERNAL_VREF_ON 0xE
#define ADS8638_REG_AUX_CONFIG   0x06
#define ADS8638_REG_MANUAL       0x04
#define ADS8638_SPI_WRITE_FLAG   0x1
#define UNUSED_RANGE 0

// assign an index to each ads8638 chip asper the lens controller schematics
#define ADS_CHIP0 0
#define ADS_CHIP1 1
#define ADS_CHIP2 2
#define ADS_CHIP3 3
#define ADS_CHIP4 4


// SPI specific defines
#define SPI_TIME_OUT 1 // 1 ms time out
#define SPI_NUMBER_BYTES_RECEIVED 2

// A structure to hold values and ranges for all channels on an ADC chip
struct Adc8638Info
{
	uint16_t ADC_TI_VALUE[MAX_NUMBER_ADC_ADS8638_CHANNELS]; // this holds all DAC values
	uint16_t ADC_TI_RANGE[MAX_NUMBER_ADC_ADS8638_CHANNELS]; // this hold all DAC ranges (voltage spans), range can only be changed on the DAC chip, on STM32F7 its always 0-2.5V
	// DEV: below converts a read integer to a value. this could be quite useful for testing
	float ADC_ACTUAL_VOLTAGE[MAX_NUMBER_ADC_ADS8638_CHANNELS]; // this hold all DAC ranges (voltage spans), range can only be changed on the DAC chip, on STM32F7 its always 0-2.5V

};


// create multiple ADC chips
struct Adc8638Info adcChips[MAX_NUMBER_ADC_ADS8638_CHIPS];

// a structure that holds information about available ranges for the ads8638, selecting a wrong range will return a "BAD Range" error
struct  AdcRangeInfo{
 	uint8_t AdcRange;
 	float AdcRangeMin;
 	float AdcRangeMax;
 	char* description;
 };


// A list that contains all the ranges on the ADS86388.
struct AdcRangeInfo AdcRangeList[] = {

        {    0x0, UNUSED_RANGE, UNUSED_RANGE," Ranges as selected through the configuration registers (address 10h to 13h, page 0)     " },
		{    0x1, -10 , 10  ," Range is set to ±10V     " }                                                               ,
		{    0x2, -5  , 5   ," Range is set to ±5V     " }                                                                ,
		{    0x3, -2.5, 2.5 ," Range is set to ±2.5V     " }                                                              ,
		{    0x4,UNUSED_RANGE , UNUSED_RANGE," Reserved do not use this setting "    }                                                    ,
		{    0x5,0    ,10   ," Range is set to 0V to 10V     " }                                                          ,
		{    0x6,0    ,5    ," Range is set to 0V to 5V     " }                                                           ,
		{    0x7,UNUSED_RANGE , UNUSED_RANGE," Powers down the device     " }                                                             ,

};

// gain multipliers, used to get the actual ADC value, useful for testing and access is made available to the house keeper.
struct  AdcGainInfo{
 	int adcNumber;
 	int channelNumber;
 	float Gain;

 };

// gain multipliers for calculating the actual value read back from all ADS8638 channels as per the lens controller schematics. See lens_control.c as well for more details on the map
struct AdcGainInfo AdcGainList[] = {

		{1,        0  ,        (10+0.68)/0.68            }       ,
		{1,        1  ,        (10+0.68)/0.68            }       ,
		{1,        2  ,        (10+0.68)/0.68            }       ,
		{1,        3  ,        (10+0.68)/0.68            }       ,
		{1,        4  ,        (10+0.68)/0.68            }       ,
		{1,        5  ,        (10+0.68)/0.68            }       ,
		{1,        6  ,        (10+0.68)/0.68            }       ,
		{1,        7  ,        (10+0.68)/0.68            }       ,
		{2,        0  ,        (10+0.68)/0.68            }       ,
		{2,        1  ,        (10+0.68)/0.68            }       ,
		{2,        2  ,        (10+0.68)/0.68            }       ,
		{2,        3  ,        (10+0.68)/0.68            }       ,
		{2,        4  ,        (10+0.68)/0.68            }       ,
		{2,        5  ,        (10+0.68)/0.68            }       ,
		{2,        6  ,        (10+0.68)/0.68            }       ,
		{2,        7  ,        (10+0.68)/0.68            }       ,
		{3,        0  ,        (10+0.68)/0.68            }       ,
		{3,        1  ,         (1000)                   }       ,
		{3,        2  ,         (1000)                   }       ,
		{3,        3  ,         (1000)                   }       ,
		{3,        4  ,         (1000)                   }       ,
		{3,        5  ,         (1000)                   }       ,
		{3,        6  ,         (1000)                   }       ,
		{3,        7  ,         (1000)                   }       ,
		{4,        0  ,         (1000)                   }       ,
		{4,        1  ,         (1000)                   }       ,
		{4,        2  ,         (1000)                   }       ,
		{4,        3  ,         (1000)                   }       ,
		{4,        4  ,         (1000)                   }       ,
		{4,        5  ,         (1000)                   }       ,
		{4,        6  ,         (1000)                   }       ,
		{4,        7  ,         (1000)                   }       ,
		{0,        0  ,         (100)                 }       ,
		{0,        1  ,         (200)                 }       ,
		{0,        2  ,        (470*4+10)/10          }       ,
		{0,        3  ,        (470*4+10)/10          }       ,
		{0,        4  ,        1                      }       ,
		{0,        5  ,        1                      }       ,
		{0,        6  ,        1                      }       ,
		{0,        7  ,        1                      }       ,

};


// set a channel and range to read from
char* adc_setChannelandRange(uint8_t adcNumber,uint8_t channelNumber, uint8_t range, uint8_t tempretureSelector)
{
	uint8_t cmdLSB = 0;
	// see page 40 of the data sheet. ranges 4 and 7 are reserved
	if(range==4 && range==7)return "BAD DAC RANGE";
	// form the LSB of the command

	cmdLSB = (channelNumber << 4) | (range << 1) | tempretureSelector;


	if(adcNumber >= MAX_NUMBER_ADC_ADS8638_CHIPS)return "BAD DAC NUMBER";
	// code for this command
	uint8_t  cmdMSB = ADS8638_REG_MANUAL;

	// form a command and send it
	// adc_setRegisterValue(adcNumber, channelNumber, ADS8638_INTERNAL_VREF_ON, ADS8638_REG_AUX_CONFIG);

	adc_setRegisterValue(adcNumber, channelNumber, cmdLSB, cmdMSB);


	// adc_setCmdGeneric(adcNumber, commandWord);


    // update ADC range
	// Note: use this kind of syntax always ---> [channelNumber % MAX_NUMBER_ADC_ADS8638_CHANNELS] to avoid out of range indices
	if(range!=0)adcChips[adcNumber].ADC_TI_RANGE[channelNumber % MAX_NUMBER_ADC_ADS8638_CHANNELS] = range;

	return "Ok\n";
}


// set the range on a channel, (see data sheet page 44)
char* adc_setRange(uint8_t adcNumber, uint8_t channelNumber, uint8_t range)
{
	uint8_t cmdMSB = 0;
	uint8_t cmdLSB = 0;

	// set the cmdMSB which contains address of different channels, (see data sheet page 44)
	switch(channelNumber)
	{
	case 0:cmdMSB = 0x10;break;
	case 1:cmdMSB = 0x10;break;
	case 2:cmdMSB = 0x11;break;
	case 3:cmdMSB = 0x11;break;
	case 4:cmdMSB = 0x12;break;
	case 5:cmdMSB = 0x12;break;
	case 6:cmdMSB = 0x13;break;
	case 7:cmdMSB = 0x13;break;
	default:break;
	}

	// form the LSB part of the command. if the channel is an even channel the range value should be stored in first 4 bits of cmdLSB. If odd in the last 4 bits of cmdLSB
	if((channelNumber % 2) == 0) //if Even
	{
		cmdLSB = (range << 4) + (adcChips[adcNumber].ADC_TI_RANGE[abs(channelNumber+1) % MAX_NUMBER_ADC_ADS8638_CHANNELS] );
	}
	else // if ODD
	{
		cmdLSB = range + (adcChips[adcNumber].ADC_TI_RANGE[abs(channelNumber-1) % MAX_NUMBER_ADC_ADS8638_CHANNELS] << 4);

	}

	// range 0 and 7 values are reserved
	if(range!=0 && range!=7)
		{
		adc_setRegisterValue(adcNumber, channelNumber, cmdLSB,cmdMSB );
		adcChips[adcNumber].ADC_TI_RANGE[channelNumber % MAX_NUMBER_ADC_ADS8638_CHANNELS] = range;
		}

	return "Ok\n";
}

// initialize the all ADS8638 chips. Init can be changed for different controllers. The reference map for the ADCs is shown in lens_controller.c
void adc8638_init(void)
{
	//page 42 of the data sheet. Default values to enable the following:
	/*
	 * power down functionality, AL_PD = 1
	 * use an internal reference voltage 2.5V, Vref_Internal_Enable =1
	 * Enable Temperature sensor block, Temp_Sensor_Enable =1
	 */
	uint8_t AL_PD_Enable = 1, Vref_Internal_Enable = 1 ,Temp_Sensor_Enable = 1;

	uint8_t cmdMSB = ADS8638_REG_AUX_CONFIG , cmdLSB = 0;

	// form the LSB part of the command

	cmdLSB = (Temp_Sensor_Enable << 1) + (Vref_Internal_Enable << 2) + (AL_PD_Enable << 3);

	// scan through ADCs
  	for( int jj = 0; jj < MAX_NUMBER_ADC_ADS8638_CHIPS; jj++ )
   	{
  	// power up all ADCs

  	  	adc_powerUp(jj,1);
     // Scan through ADC channels
   	 for( int ii = 0; ii < MAX_NUMBER_ADC_ADS8638_CHANNELS; ii++ )
   	 {
   		 //  enable (power down, internal reference and temperature sensor) on all ADC chips

   		adc_setRegisterValue(jj, ii, cmdLSB, cmdMSB);

     	// set range on ADC 3 and 4 to 1 (±2.5V)
   		if((jj == ADS_CHIP3) || (jj == ADS_CHIP4))
   		{
		 	adc_setRange(jj, ii, AdcRangeList[3].AdcRange);

   		}
   		else //  and ±10V on other ADCs
   		{
   			adc_setRange(jj, ii, AdcRangeList[1].AdcRange);
   		}

   	 }

   	}

// set channels 0, 1 range on ADC0 (ADC_CHIP0) to ±2.5V
		adc_setRange(0, 1, AdcRangeList[3].AdcRange);
		adc_setRange(0, 2, AdcRangeList[3].AdcRange);
		adc_setRange(0, 7, AdcRangeList[5].AdcRange);

}

// get an adc value from an ADS8638 channel
int adc_getChannelValue(uint8_t adcNumber,uint8_t channelNumber)
{
	return adcChips[adcNumber].ADC_TI_VALUE[channelNumber % MAX_NUMBER_ADC_ADS8638_CHANNELS];
}


//Future: get actual values
float adc_getChannelActualValue(uint8_t adcNumber,uint8_t channelNumber)
{
float actualValue =0;
	adcChips[adcNumber].ADC_ACTUAL_VOLTAGE[channelNumber % MAX_NUMBER_ADC_ADS8638_CHANNELS] = adc_IntegerChannelValuetoVolts(adcNumber,channelNumber, adc_getGain(adcNumber,channelNumber));
	actualValue = adcChips[adcNumber].ADC_ACTUAL_VOLTAGE[channelNumber % MAX_NUMBER_ADC_ADS8638_CHANNELS];
return actualValue;
}

// get the gain value for a specific channel as per the lens controller schamtics
float adc_getGain(uint8_t adcNumber,uint8_t channelNumber)
{
	float gainValue = 0;
	for(uint8_t ii=0 ;ii<(sizeof(AdcGainList)/sizeof(AdcGainList[0]));ii++)
		{
			if((AdcGainList[ii].adcNumber == adcNumber) && (AdcGainList[ii].channelNumber == channelNumber) )
			{
				gainValue = AdcGainList[ii].Gain;
				break;
			}

		}
	return gainValue;

}

// get the current set  channel range
int adc_getChannelRange(uint8_t adcNumber,uint8_t channelNumber)
{
	return adcChips[adcNumber].ADC_TI_RANGE[channelNumber % MAX_NUMBER_ADC_ADS8638_CHANNELS];
}

// Set/reset the corresponding chip select pin on an ADC
StatusCode_t adc_nCs_set(uint8_t adcNumber, int value)
{

	GPIO_PinState PinState;

    if(adcNumber > MAX_NUMBER_ADC_ADS8638_CHIPS)return STATUS_FAIL;
	if(value != 0 && value != 1) return STATUS_FAIL;

	switch(value)
	{
	case 0:PinState = GPIO_PIN_RESET;break;
	case 1:PinState = GPIO_PIN_SET;break;
	}
	// add more cases for as many ADCs on the board
	switch(adcNumber)
	{
	case 0:HAL_GPIO_WritePin(GPIOG, nCS_ADC0_Pin, PinState);break;
	case 1:HAL_GPIO_WritePin(GPIOG, nCS_ADC1_Pin, PinState);break;
	case 2:HAL_GPIO_WritePin(GPIOG, nCS_ADC2_Pin, PinState);break;
	case 3:HAL_GPIO_WritePin(GPIOG, nCS_ADC3_Pin, PinState);break;
	case 4:HAL_GPIO_WritePin(GPIOG, nCS_ADC4_Pin, PinState);break;
	default:break;
	}

	return STATUS_OK;
}

// power up and down an ADC chip
StatusCode_t adc_powerUp(uint8_t adcNumber, int value)
{
	GPIO_PinState PinState;

    if(adcNumber > MAX_NUMBER_ADC_ADS8638_CHIPS)return STATUS_FAIL;
	if(value != 0 && value != 1) return STATUS_FAIL;

	switch(value)
	{
	case 0:PinState = GPIO_PIN_RESET;break;
	case 1:PinState = GPIO_PIN_SET;break;
	}

	// add more cases for as many ADCs on the board
	switch(adcNumber)
	{
	case 0:HAL_GPIO_WritePin(GPIOG, ADC_EN_Pin, PinState);break;
	case 1:HAL_GPIO_WritePin(GPIOG, ADC_EN_Pin, PinState);break;
	case 2:HAL_GPIO_WritePin(GPIOG, ADC_EN_Pin, PinState);break;
	case 3:HAL_GPIO_WritePin(GPIOG, ADC_EN_Pin, PinState);break;
	case 4:HAL_GPIO_WritePin(GPIOG, ADC_EN_Pin, PinState);break;
	default:break;
	}

	return STATUS_OK;
}



// write a value to register (transmit a command on the SPI bus to the selected chip).
char* adc_setRegisterValue(uint8_t adcNumber, uint8_t channelNumber, uint8_t cmdLSB, uint8_t cmdMSB)
{
	 // two bytes Array the holds the 32 bit command for the DAC
      uint8_t data_array[2]={0};

  	if(adcNumber >= MAX_NUMBER_ADC_ADS8638_CHIPS)return "BAD DAC NUMBER";

  	// power up the chip
  	if(adc_powerUp(adcNumber,1)== STATUS_FAIL)
  	{
  		return " failed to power up the ADS8638 chip";
  	}


	// pull the corresponding chip select pin nCS to low to start transmission on the selected ADC and then pull High to execute the command

  	// reset nCs on all chips
  	for(int ii = 0;ii<MAX_NUMBER_ADC_ADS8638_CHIPS; ii++)adc_nCs_set(ii,1);

  	    // pull chip select low
  		if(adc_nCs_set(adcNumber,0) == STATUS_FAIL)
  		{
  			return " failed to enable CS on ADS8638";
  		}


		data_array[0] = cmdMSB << 1;                    // MS Byte
		data_array[1] = cmdLSB;

		// Send the bytes command to the ADC
		HAL_SPI_Transmit(&hspi6,&data_array,2,10);

  	    // pull chip select High

		if(adc_nCs_set(adcNumber,1)== STATUS_FAIL)
		{
			return " failed to enable CS on ADS8638";
		}

		// we only want to receive data from read channel, this is command address 0x04 (data sheet page 40)
		if(cmdMSB == ADS8638_REG_MANUAL)adc_SPI_receive(adcNumber,channelNumber);

      	return "Ok\n";
}

// receive from SPI on a selected chip
char* adc_SPI_receive(uint8_t adcNumber, uint8_t channelNumber)
{
	 // Four bytes Array the holds the 32 bit command for the DAC
      uint8_t data_array[SPI_NUMBER_BYTES_RECEIVED]={0};
      volatile uint16_t adc_readValue = 0;
      uint8_t b0, b1, channelReadAddress;

  	if(adcNumber >= MAX_NUMBER_ADC_ADS8638_CHIPS)return "BAD DAC NUMBER";

  	// frame n+1
		if (adc_nCs_set(adcNumber,0) == STATUS_FAIL)
		{
			return " failed to enable CS on ADS8638";
		}
		HAL_SPI_Receive(&hspi6, &data_array, SPI_NUMBER_BYTES_RECEIVED, SPI_TIME_OUT);
		if (adc_nCs_set(adcNumber,1) == STATUS_FAIL)
		{
			return " failed to enable CS on ADS8638";
		}

    // frame n+2
//		adc_nCs_set(adcNumber,0);
//		HAL_SPI_Receive(&hspi6,&data_array,SPI_NUMBER_BYTES_TO_BE_RECEIVED,SPI_TIME_OUT);
//		adc_nCs_set(adcNumber,1);

		b0= data_array[0];
		b1= data_array[1];

		// Extract channel address 4 MSB
		channelReadAddress = (b0 & 0xF0) >> 4;

		// update ADC value
		// exclude the 4 MSB as they are the address ones and store it in 16 bit integer
		adc_readValue = b0 & 0xF;
		// shift the 16 bit integer value to left by 8 digits so it becomes the MSByte
		adc_readValue = adc_readValue << 8;
		// Add the LSByte
		adc_readValue = adc_readValue | b1;

//		adcChips[adcNumber].ADC_TI_VALUE[channelNumber % MAX_NUMBER_ADC_ADS8638_CHANNELS] = adc_readValue;
		// use the address in the header, it is safer!
		adcChips[adcNumber].ADC_TI_VALUE[channelReadAddress % MAX_NUMBER_ADC_ADS8638_CHANNELS] = adc_readValue;

      	return "Ok\n";
}


// update all channel reads on all adcs. Note that pins 4,5 and 6 on ADC 0 will read whatever routed to the muxout of DACs 0,1 and 2
StatusCode_t readAllAdcChannels(void)
{
  	for( int jj = 0; jj < MAX_NUMBER_ADC_ADS8638_CHIPS; jj++ )
   	{
   	 for( int ii = 0; ii < MAX_NUMBER_ADC_ADS8638_CHANNELS; ii++ )
   	 {
   		 //  enable: power down, internal reference and temperature sensor on all chips
   		adc_setChannelandRange(jj,ii,0,0);
// Future get actual value and store it as well
   		adc_getChannelActualValue(jj,ii);

   	 }

   	}
  	return STATUS_OK;
}

// convert value to volts. Gain is calculated from the voltage divider to voltage multiplier. This is local function not to be used by dictionary
float adc_IntegerChannelValuetoVolts(uint8_t adcNumber,uint8_t channelNumber, float gain)
{
	volatile float valueInVolts = 0;
	volatile float fullRange, rangeMin;

    int readValue =  adc_getChannelValue(adcNumber,channelNumber);

	if (gain == 0) return 0;

	// get the full range and range min to plug into the conversion formula below
	fullRange = AdcRangeList[adc_getChannelRange(adcNumber,channelNumber)].AdcRangeMax - AdcRangeList[adc_getChannelRange(adcNumber,channelNumber)].AdcRangeMin;
	rangeMin =  AdcRangeList[adc_getChannelRange(adcNumber,channelNumber)].AdcRangeMin;

	//conversion formula
	valueInVolts = ((readValue/4095.0f)*fullRange + rangeMin)*gain ; // see gain factors in lens_control.c

	return valueInVolts;
}

// convert a float value into integer. Multiply a value by 1000 and convert to integer
int floatToFixedPointInt(float value)
{
	volatile float fixedPointValueFloat = 0;

	fixedPointValueFloat = value *1000;

	return (int)fixedPointValueFloat;
}

//// FUTURE: -this is optional if DMA to be used on reception.  I do not believe this is necessary at this stage as the reading is quite fast.
//           -if interrupt on reception complete is enabled, the HAL_UART_RxCpltCallback will be called back. example is shown below
//void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi){
//
//	if (hspi->Instance == SPI6)
//	{
//
//	}
//}

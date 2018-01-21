/*
 * adc_external_ADS838.h
 *
 *  Created on: 16/06/2017
 *      Author: Esam.Alzqhoul
 */

#ifndef ADC_EXTERNAL_ADS838_H_
#define ADC_EXTERNAL_ADS838_H_

#include "types.h"

char* adc_setRange(uint8_t adcNumber, uint8_t channelNumber, uint8_t range);
char* adc_setChannelandRange(uint8_t adcNumber,uint8_t channelNumber, uint8_t value, uint8_t tempretureSelector);
StatusCode_t adc_powerUp(uint8_t dacNumber, int value);
int adc_getChannelValue(uint8_t adcNumber,uint8_t channelNumber);
int adc_getChannelRange(uint8_t adcNumber,uint8_t channelNumber);

void adc8638_init(void);
StatusCode_t readAllAdcChannels(void);

StatusCode_t adc_nCs_set(uint8_t dacNumber, int value);
char* adc_SPI_receive(uint8_t adcNumber, uint8_t channelNumber);
char* adc_setRegisterValue(uint8_t dacNumber, uint8_t channelNumber, uint8_t cmdLSB, uint8_t cmdMSB); // this can be used as a generic command former for the ADS8638, see how it is used in adc_setRange as an example

// Future: useful to read actual values
float adc_getChannelActualValue(uint8_t adcNumber,uint8_t channelNumber);
float adc_IntegerChannelValuetoVolts(uint8_t adcNumber,uint8_t channelNumber, float gain);
int   floatToFixedPointInt(float value);
float adc_getGain(uint8_t adcNumber,uint8_t channelNumber);


#endif /* ADC_EXTERNAL_ADS838_H_ */

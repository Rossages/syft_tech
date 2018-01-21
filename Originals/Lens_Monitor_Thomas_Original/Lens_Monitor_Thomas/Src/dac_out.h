/*
 * dac_out.h
 *
 *  Created on: 1/05/2017
 *      Author: Esam.Alzqhoul
 */

#ifndef DAC_OUT_H_
#define DAC_OUT_H_


//char* dac_setExternalRefVoltage( uint32_t value); // not available in current design
//typedef enum
//{
//  PIN_LOW = 0,
//  PIN_HIGH =!LOW
//}PinState_t;

//Basic
char* dac_setCmdGeneric(uint8_t dacNumber, uint8_t channelNumber, uint8_t commandWord, uint32_t value);

char* dac_setAout(uint8_t dacNumber,uint8_t channelNumber, uint32_t value);
char* dac_setAoutNoUpdate(uint8_t dacNumber,uint8_t channelNumber, uint32_t value);
char* dac_setRange(uint8_t dacNumber,uint8_t channelNumber, uint32_t value);
char* dac_setAoutAllChannels(uint8_t dacNumber, uint32_t value);
char* dac_setRangeAllChannels(uint8_t dacNumber,uint32_t value);
char* dac_setMuxOut(uint8_t dacNumber, uint16_t MuxOutChannel);
char* dac_UpdateChannel(uint8_t dacNumber,uint8_t channelNumber);
char* dac_UpdateAllChannels( uint8_t dacNumber);

// Advanced
char* dac_globalToggleRegisters( uint8_t dacNumber, uint32_t value);
char* dac_toggleRegisters(uint8_t dacNumber, uint8_t channelNumber,  uint32_t value);
char* dac_powerDownChannel(uint8_t dacNumber,uint8_t channelNumber);
char* dac_powerDownChip(uint8_t dacNumber);
char* dac_setThermalProtection(uint8_t dacNumber, uint32_t value);


// STM DACs
char* dac_setSetPoint1(uint16_t value);
//char* dac_setSetPoint2(uint16_t value);
void voltageSTMDacRamp(  uint8_t channelNumber,  int value);


//get
int dac_getAout( uint8_t dacNumber, uint8_t channelNumber );
int dac_getTemperature(uint8_t dacNumber);
int dac_getRange(uint8_t dacNumber, uint8_t channelNumber );
int dac_getSetPoint1(void);
//int dac_getSetPoint2(void);
void dac_init_aout(void);

// Chip Specific
char* SPI_transmit(uint8_t dacNumber, uint8_t combinedCmdAddr, uint16_t value);
char* dac_nCs_set(uint8_t dacNumber, int value);
void nLDAC_set(uint8_t dacNumber);
void voltageLtcDacRamp(uint8_t dacNumber, uint8_t channelNumber,  int  value, uint8_t  commandWord);

// generic and static functions
void dacLtc_setStepSize(uint8_t dacNumber, uint8_t channelNumber, uint16_t value);
int  dacLtc_getStepSize(uint8_t dacNumber, uint8_t channelNumber);
void dacStm_setStepSize(uint8_t channelNumber, uint16_t value);
int  dacStm_getStepSize(uint8_t channelNumber);

#endif /* DAC_OUT_H_ */

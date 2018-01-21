/*
 * dictionary2.c
 *
 *  Created on: 4/04/2017
 *      Author: Esam.Alzqhoul
 */

#include "objectdictionary.h"
#include "objectdictionaryhelper.h"
#include "global.h"

// LTC chip defines
#define DAC_ADDRESS(X) ((X >> 16) & 0xF) // Extract LTC device number from address, shift to right by 4 hex digits or 16 bits
#define DAC_CHANNEL_ADDRESS(X) (X  & 0xF) //Extract LTC DAC Channel address
#define DAC_MUXOUT_CHANNEL(X) (X  & 0xFF) //Extract  LTC DAC MUXOUT CHANNEL ID
//STM DACs defines
#define DAC_STM_CHANNEL_ADDRESS(X) (X  & 0xF) //Extract STM DAC Channel address
// ADS8638 defines
#define ADS8638_ADDRESS(X) ((X >> 16) & 0xF) // Extract ADS8638 ADC device number from address, shift to right by 4 hex digits or 16 bits
#define ADS8638_CHANNEL_ADDRESS(X) (X  & 0xF) //Extract ADS8638 Channel address

//Digital IN/OUT defines
//#define DIN_ADRRESS(X) (X  & 0xF) //Extract DIN address
//#define DOUT_ADRRESS(X) (X  & 0xF) //Extract DOUT address
#define AIN_ADDRESS(X) (X  & 0xF) //Extract AIN address

//DATASTREAM and Multibyte transfer defines
#define DATASTREAM_INDEX(X) (X & 0xF) //extract the data stream index
#define ADDRESS(X,Y) ((X<<8)|Y) //Extract the address from address and subindex .

static DictionaryEntry dictionary[] =
{
		{ 0x200000,0xFF0FF0,NULL,DATA_INT,  readDacChannel               , writeDacChannel                 ,  "	Set/Get the voltage on DAC N channel n (12 bit integer)	" }                                                 ,
		{ 0x200100,0xFF0FF0,NULL,DATA_INT,  NULL                         , writeDacChannelNoUpdate         ,  "	Send value to the DAC N channel n register (12 bit integer)" }                                              ,
		{ 0x200200,0xFF0FF0,NULL,DATA_INT,  readDacRange                 , writeDacRange                   ,  "	Set/Get voltage range (span) on DAC N channel n.	" }                                                        ,
		{ 0x200300,0xFF0FFF,NULL,DATA_INT,  NULL                         , WriteDacAllChannels             ,  "	Set the same voltage on DAC N all channels (12 bit integer)" }                                              ,
		{ 0x200400,0xFF0FFF,NULL,DATA_INT,  NULL                         , writeDacRangeAllChannels        ,  "	Set all channels on DAC N to the same voltage range (span)	" }                                              ,
		{ 0x200500,0xFF0F00,NULL,DATA_INT,  readMuxOutChannel            , writeMuxOutChannel              ,  "	Route a channel on DAC N to the Mux output pin on the LTC DAC chip." }                                      ,
		{ 0x200600,0xFF0FF0,NULL,DATA_INT,  NULL                         , WriteDacUpdateChannel           ,  "	Update the output of DAC N channel n from its register	" }                                                  ,
		{ 0x200700,0xFF0FFF,NULL,DATA_INT,  NULL                         , WriteDacUpdateAllChannels       ,  "	Update the output of DAC N all channels from their registers	" }                                            ,
		{ 0x200800,0xFF0FFF,NULL,DATA_INT,  NULL                         , WriteDacPowerDown               ,  "	Power down the DAC Chip	" }                                                                                 ,
		{ 0x200900,0xFF0FFF,NULL,DATA_INT,  readDacTemperature           , NULL                            ,  "	Get the DAC Chip temperature	" }                                                                            ,
		{ 0x200A00,0xFF0FFF,NULL,DATA_INT,  NULL                         , WriteDacDisableThermalProtection,  "	Enable (0) default or Disable (1) 160C thermal protection, Enabled by default	" }                           ,
		{ 0x200B00,0xFF0FF0,NULL,DATA_INT,  readDacChannelStepSize       , WriteDacChannelStepSize         ,  "	write and read DAC N channel n step size	" }                                                                ,


		{ 0x300000,0xFFFFFF,NULL,DATA_INT,  readHighVoltageLensSupply    , writeSetHighVoltageLensSupply   ,  "	Set/Get the High Voltage Lens Supply. e.g. 4095--> 2.5V is ±150V" }                                        ,
		{ 0x300100,0xFF0FF0,NULL,DATA_INT,  readHvLensSupplyStepSize     , WriteHvLensSupplyStepSize       ,  "	Set/Get HV Lens Supply stepsize (STM DACs channels 1 )	" }                                                  ,

		{ 0x400000,0xFFFFFF,NULL,DATA_INT,  readDigitalInputGuage        , NULL                            ,  "	Get digital value on Guage Connector  	" }                                                                  ,
		{ 0x400100,0xFFFFFF,NULL,DATA_INT,  readPulseOn                  , writePulseOn                    ,  "	Turn on/off the pulse circuit in normal mode	" }                                                            ,
		{ 0x400200,0xFFFFFF,NULL,DATA_INT,  readPulseTestMode            , writePulseTestMode              ,  "	Turn on/off pulse circuit in test mode, if this is on, it overrides the normal mode	" }                     ,
		{ 0x400300,0xFFFFFF,NULL,DATA_INT,  readGuageDout                , writeGuageDout                  ,  "	Set/Get digital output on the Guage connector	" }                                                             ,
		{ 0x400400,0xFFFFFF,NULL,DATA_INT,  readEnable15VSupply          , writeEnable15VSupply            ,  "	Turn on/off the 15V power supply for the DACs and ADCs" }                                                   ,

		{ 0x500000,0xFF0FF0,NULL,DATA_INT,  readExternalAds8638Channel   , NULL                            ,  "	read an ADC channel on ADS8638 (12 bit adc), Mostly lens's current and voltage readings	" }                 ,
		{ 0x500100,0xFF0FF0,NULL,DATA_INT,  readAds8638ChannelActualValue, NULL                            ,  "	read an actual value from an ADS8638 channel directly as 3 decimal fixed point intger. e.g. 5.72 as 5720 " },
		{ 0x500200,0xFF0FF0,NULL,DATA_INT,  readAds8638ChannelRange      , writeAds8638ChannelRange        ,  "	Set/get channel range on ADS8638 (or voltage span)" }                                                       ,
		{ 0x500300,0xFFFFFF,NULL,DATA_INT,  NULL                         , writePowerUpDownAllAds8638Chips ,  "	Turn on/off all ADS8638 chips" }                                                                            ,

		{ 0x600000,0xFFFFFF,NULL,DATA_INT,  readPulseCount               , NULL                            ,  "	Read pulse count from detector" }                                                   ,
		{ 0x600100,0xFFFFFF,NULL,DATA_INT,  readActualCountTime          , NULL                            ,  "	Read actual pulse counting time from detector" }                                                   ,
		{ 0x600200,0xFFFFFF,NULL,DATA_INT,  readMaxPulseCount            , writeMaxPulseCount              ,  "	set/get maximum pulse count" }                                                   ,
		{ 0x600300,0xFFFFFF,NULL,DATA_INT,  readMaxCountTime             , writeMaxCountTime               ,  "	set/get max counting time" }                                                   ,

		{ 0x700000,0xFFFFF0,NULL,DATA_INT,  readAin                      , NULL                            ,  " Read an ADC channel on the STM32F7 (12 bit adc). [1-16] see AIN.c for details. " }

};

//init dictionary  to have the queue
void objectdictionary_init()
{
	//dictionary should always be static
	objectdictionaryhelper_init(dictionary, sizeof(dictionary)/sizeof(DictionaryEntry));
}


/* All read functions call backs are here
 *
 */
//DAC reads

// read a channel value on dac
int readDacChannel (uint32_t address ){
return dac_getAout( DAC_ADDRESS(address ),DAC_CHANNEL_ADDRESS(address));
}

// read a dac channel range
int readDacRange (uint32_t address ){
return dac_getRange( DAC_ADDRESS(address ),DAC_CHANNEL_ADDRESS(address));
}

// read muxout voltage
int readMuxOutChannel (uint32_t address ){
	const int muxout_pin = 9;
	dac_setMuxOut(DAC_ADDRESS(address ),DAC_MUXOUT_CHANNEL(address));
return ain_get(muxout_pin);
}

// read a  DAC temperature
int readDacTemperature (uint32_t address ){
return dac_getTemperature( DAC_ADDRESS(address ));
}

// read a DAC channel step size
int readDacChannelStepSize (uint32_t address ){
return dacLtc_getStepSize( DAC_ADDRESS(address ),DAC_CHANNEL_ADDRESS(address));
}


//read High voltage lens supply set values
int readHighVoltageLensSupply (uint32_t address ){
return lens_getVlensVoltageSupply();
}

// read high voltage step size, // lens voltage is connected to DAC_CHANNEL_1, index in our stmDac structure is 0
int readHvLensSupplyStepSize (uint32_t address ){
	const int dacChannel_1 = 0;

return dacStm_getStepSize(dacChannel_1);
}


// Digital reads

int readDigitalInputGuage (uint32_t address ){
	const int dinIndex = 0;

return din_get(dinIndex);
}

int readPulseTestMode (uint32_t address ){
	const int doutIndex = 0;
return dout_get( doutIndex);
}

int readPulseOn (uint32_t address ){
	const int doutIndex = 1;
return dout_get( doutIndex);
}

int readGuageDout (uint32_t address ){
	const int doutIndex = 2;
return dout_get( doutIndex);
}

int readEnable15VSupply (uint32_t address ){
	const int doutIndex = 3;
return dout_get( doutIndex);
}

// ADS8638 ADC reads

int readExternalAds8638Channel (uint32_t address ){
return adc_getChannelValue( ADS8638_ADDRESS(address ), ADS8638_CHANNEL_ADDRESS(address));
}

int readAds8638ChannelActualValue (uint32_t address ){
	// multiply the actual float value by 1000 and round it to the nearest integer, we only transmit integers
return floatToFixedPointInt(adc_getChannelActualValue (ADS8638_ADDRESS(address ), ADS8638_CHANNEL_ADDRESS(address)));
}

int readAds8638ChannelRange (uint32_t address ){
return adc_getChannelValue( ADS8638_ADDRESS(address ), ADS8638_CHANNEL_ADDRESS(address));
}

// TODO: pulse counter reads
int readPulseCount  (uint32_t address ){
//return pulse_getCount();
}

int readActualCountTime  (uint32_t address ){
//return pulse_getActualCountTime();
}

int readMaxPulseCount  (uint32_t address ){
//return pulse_getMaxPulseCount();
}

int readMaxCountTime  (uint32_t address ){
//return pulse_getMaxCountTime();
}

//Analogue measurements on board
int readAin  (uint32_t address ){
return ain_get( AIN_ADDRESS( address ));
}



//DAC writes
void  writeDacChannel( uint32_t address, int value ){

  dac_setAout(DAC_ADDRESS(address ),DAC_CHANNEL_ADDRESS(address),value);
}

void  writeDacChannelNoUpdate( uint32_t address, int value ){

  dac_setAoutNoUpdate(DAC_ADDRESS(address ),DAC_CHANNEL_ADDRESS(address),value);
}
void  writeDacRange( uint32_t address, int value ){

  dac_setRange(DAC_ADDRESS(address ),DAC_CHANNEL_ADDRESS(address),value);
}

void  WriteDacAllChannels( uint32_t address, int value ){

  dac_setAoutAllChannels(DAC_ADDRESS(address ),value);
}
void  writeDacRangeAllChannels( uint32_t address, int value ){

  dac_setRangeAllChannels(DAC_ADDRESS(address ),value);
}
void  writeMuxOutChannel( uint32_t address, int value ){

  dac_setMuxOut(DAC_ADDRESS(address ),DAC_MUXOUT_CHANNEL(address));
}

void  WriteDacUpdateChannel( uint32_t address, int value ){

  dac_UpdateChannel(DAC_ADDRESS(address ),DAC_CHANNEL_ADDRESS(address));
}
void  WriteDacUpdateAllChannels( uint32_t address, int value ){

  dac_UpdateAllChannels(DAC_ADDRESS(address ));
}
void  WriteDacPowerDown( uint32_t address, int value ){

  dac_UpdateChannel(DAC_ADDRESS(address ),DAC_CHANNEL_ADDRESS(address));
}
void  WriteDacDisableThermalProtection( uint32_t address, int value ){

  dac_setThermalProtection(DAC_ADDRESS(address ),value);
}

void  WriteDacChannelStepSize( uint32_t address, int value ){
  dacLtc_setStepSize(DAC_ADDRESS(address ),DAC_CHANNEL_ADDRESS(address),value);
}

//high voltage lens supply writes
void  writeSetHighVoltageLensSupply( uint32_t address, int value ){

  lens_setVlensVoltageSupply(value);
}


void  WriteHvLensSupplyStepSize( uint32_t address, int value ){
  const int stmDacChannel_1 = 0;
  dacStm_setStepSize(stmDacChannel_1,value);
}

// ADS8638 ADC-12bit writes
void  writeAds8638ChannelRange( uint32_t address, int value ){
	adc_setRange(ADS8638_ADDRESS(address ),ADS8638_CHANNEL_ADDRESS(address),value);
}

void  writePowerUpDownAllAds8638Chips( uint32_t address, int value ){

adc_powerUp(ADS8638_ADDRESS(address ),value);

}
// Future: this can be used to set any register on the ADS8638
//void  writeAds8638Register( uint32_t address, int value ){
//	// extract cmdLSB and cmdMSB from  value
//	adc_setRegisterValue(ADs8638_ADDRESS(address ),ADS8638_CHANNEL_ADDRESS(address),cmdLSB,cmdMSB);
//}

// set digital outputs
void  writePulseTestMode( uint32_t address, int value ){
  const int doutIndex = 0;
  dout_setDout(doutIndex, value);
}


void  writePulseOn( uint32_t address, int value ){
	 const int doutIndex = 1;
  dout_setDout(doutIndex, value);
}

void  writeGuageDout( uint32_t address, int value ){
	 const int doutIndex = 2;
  dout_setDout(doutIndex, value);
}

void  writeEnable15VSupply( uint32_t address, int value ){
	 const int doutIndex = 3;
  dout_setDout(doutIndex, value);
}

// set pulse count parameters
void  writeMaxPulseCount( uint32_t address, int value ){
//	pulse_setMaxPulseCount(value);
}
void  writeMaxCountTime( uint32_t address, int value ){
//	pulse_setMaxPulseCountTime(value);
}

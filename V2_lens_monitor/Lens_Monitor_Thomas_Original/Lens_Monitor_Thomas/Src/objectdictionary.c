/*
 * dictionary2.c
 *
 *  Created on: 4/04/2017
 *      Author: Esam.Alzqhoul
 */

#include "objectdictionary.h"
#include "global.h"



SemaphoreHandle_t qHardwareSemaphoreHandle =NULL;
static QueueDataEntry newQueueData;
QueueDataEntry* pNewQueueDataEntry = &newQueueData;
//queue handle for RtosQueue
static QueueHandle_t queueDataHandle = NULL;

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


int counter_CAN=0;

//init dictionary  to have the queue
void objectdictionary_init()
{
	//when we reinit we just reinit everything
	//reset any old queue
	if(queueDataHandle != NULL)
	{
		//delete the queue
		xQueueReset(queueDataHandle);
		//vQueueDelete(queueDataHandle);
	}
	else
	{
	//Allot the size of queue 100 msgs should be enough
		queueDataHandle = xQueueCreate( 100, sizeof( QueueDataEntry ) );
	}



	//delete the old semaphore handle and create a new one and give after creating it.
	if(qHardwareSemaphoreHandle == NULL)
	{
	qHardwareSemaphoreHandle = xSemaphoreCreateBinary();

	}
	xSemaphoreGive(qHardwareSemaphoreHandle);




	//create timer task mutex//delete the  handle and create a new one and give after creating it.
	/*if(timerTaskMutex == NULL)
	{
		timerTaskMutex = xSemaphoreCreateMutex();
	}
	xSemaphoreGive(timerTaskMutex);*/

	//initialize the clal back to null we will set it when sdo inits.;
	pGetDataStreamCallback = NULL;

   // heater_init();
}




//this method should be triggered by clock interuppt.

void updateHardwareComponent()
{
	dequeueMsg();
//	readHardware();
//	readObjectDictionary();

}


//Dequeue all queue messages and write to hard ware
void dequeueMsg()
{
	uint8_t updateRequired = 0x00;
	//dont wait to get the semaphore
	if(xSemaphoreTake(qHardwareSemaphoreHandle,( TickType_t ) 0)==pdTRUE)
    {
	//while (uxQueueMessagesWaiting(queueDataHandle)>0)
	{
		//If we receive anything from the queue process it

		//dont wait to get the data
		if(xQueueReceive(queueDataHandle, (void*) pNewQueueDataEntry, ( TickType_t ) 0))
		{
			//TODO: use only one.
			writeObjectDictionary(pNewQueueDataEntry);
			//Update the message in the dictionary and storage hardware ///
			if (!updateRequired) {
				updateRequired = 0x01;
			}
		}

	   }
	//we took so give semaphore
	xSemaphoreGive(qHardwareSemaphoreHandle);
	}

	if(updateRequired)
	{
		//update the hardware
		//read hardware after every update
//		heater_updateValues(updateHeater);
		//updateRequired = 0x00;
	}


}
//TODO: This might actually not required! check with Gurav, as we are reading directly from structures in the device drivers
void readHardware()
{
	//just iterate the object dictionary and update it
	for(uint8_t i=0 ;i<(sizeof(dictionary)/sizeof(DictionaryEntry));i++)
	{
		switch(dictionary[i].dataType)
		{
		case DATA_INT:
		{


//			for(uint8_t component=0;component< numberOfHeaters;component++)
//			{
//			if ((dictionary[i].pDictionaryData != NULL) && (dictionary[i].handleRead != NULL)) {
//
//				DictionaryValues* pDictionaryValues =  dictionary[i].pDictionaryData;
//				pDictionaryValues[component].value  = dictionary[i].handleRead(pDictionaryValues[component].subIndex);
//			}
//			}

		}
			break;
		case DATA_STREAM:
			// blank for now test later
			break;

		}

	}
}
//void readObjectDictionary( int address ){
//
//	// Loop for each dictionary entry
//	//
//	for( int ii = 0; ii < (sizeof(dictionary)/sizeof(DictionaryEntry)); ii++ ){
//
//		// Cope with address matches a dictionary entry
//		//
//		if(( dictionary[ii].address & dictionary[ii].mask ) == (  pNewQueueData->address & dictionary[ii].mask )){
//			//TODO: add char* if you want to return something after reading
//			return dictionary[ii].handleRead(  pNewQueueData->address );
//		}
//	}
////	return "0";
//}

void writeObjectDictionary(QueueDataEntry*  pNewQueueData)
{
	//printf("InWriteObjectDictionary index %x  \r\n",pNewQueueData->index);
   // DictionaryDataType dataType = getDataType(pNewQueueData->index);
    int dictionaryAddress = ADDRESS(pNewQueueData->index,pNewQueueData->subIndex) ;
	int dictionaryIndex = getDictionaryIndex(dictionaryAddress);
   // printf("InWriteObjectDictionary dictionaryindex %d  \r\n",pNewQueueData->index);

    //write only if we have a valid dictionary index . any negatives indicate no index found
    if( dictionaryIndex >=0)
    {
		switch(dictionary[dictionaryIndex].dataType)
		{
		case DATA_INT:
		 //call write handler
		 if(dictionary[dictionaryIndex].handleWrite != NULL)
		 {
			 dictionary[dictionaryIndex].handleWrite(dictionaryAddress,pNewQueueData->dataValue);
		 }
		 break;
		case DATA_STREAM:

		 writeDataStream(dictionaryIndex,dictionaryAddress,pNewQueueData->dataValue);// here data value indicates data size
		 break;
		default:
			break;

		}
    }

}

//returns the handle to the queue
void objectdictionary_writeDataQueue(QueueDataEntry* pQueueData)
{
	//Todo Locks ?
	//wait till we get the handle
	if (xSemaphoreTake(qHardwareSemaphoreHandle,0xFFFF) == pdTRUE)
	{
		xQueueSend(queueDataHandle, (void* ) pQueueData, (TickType_t ) 0); //NO Block if queue is full- though ideally shouldn't happen
		xSemaphoreGive(qHardwareSemaphoreHandle);
	}
}


//TODO: Makesure subindex is actually the subindex and not the subindex value
int objectdictionary_getDataValue(uint16_t addressIndex, uint8_t subIndex) {
	int retDataVal = -1;

	uint32_t address = ADDRESS(addressIndex,subIndex);
	int dIndex = getDictionaryIndex(address);
	//write only if we have a valid dictionary index . any negatives indicate no index found
	if( dIndex >=0)
	{
		switch(dictionary[dIndex].dataType)
		{
		case DATA_INT:
			if(dictionary[dIndex].handleRead!=NULL)
			{
			 retDataVal =dictionary[dIndex].handleRead(address);
			}
			break;
		case DATA_STREAM:
			//here we just  return the data size
			if (dictionary[dIndex].pDictionaryData != NULL)
			{
				DictionaryDataStream* pDictionaryDataStream = dictionary[dIndex].pDictionaryData;
				retDataVal = pDictionaryDataStream[DATASTREAM_INDEX(address)].dataSize;
			}
			break;

		}
	}

	return retDataVal;
}


//Mthod to return the dictionary index which can be used to access the dictionary object
int getDictionaryIndex(uint32_t addressIndex)
{

	int dictionaryIndex = -1;
	for (uint8_t i = 0; i < (sizeof(dictionary) / sizeof(DictionaryEntry)); i++)
	{
		if ((addressIndex & dictionary[i].mask)	== (dictionary[i].address & dictionary[i].mask))
		{
			dictionaryIndex = i;
			break;
		}
	}
	return dictionaryIndex;

}



//this method will return call the call back to get the data from stored queue
void writeDataStream(int dictionaryIndex ,int address,int dataSize)
{


	DictionaryDataStream* pDictionaryDataStream = (DictionaryDataStream*)dictionary[dictionaryIndex].pDictionaryData;

	if (pDictionaryDataStream != NULL)
	{
		//get the index if there are any
		pDictionaryDataStream[DATASTREAM_INDEX(address)].dataSize = dataSize;

		//printf("writeDataStream - get the data via callback \r\n");

		//Callback function to get the data at the pointer location
		if (pGetDataStreamCallback != NULL)
		{
			//TODO:We may want to synchronize here becuase the pointer data may be used by hardware or other thread.
			pGetDataStreamCallback(pDictionaryDataStream);
			//TODO:
		}
	}	//end ifdictionarydatastream not null
	//print test of data
	//printf("DataStreamNew %s \r\n", testStringData);
}


/**
 * Method to update the data stream call back so that we can call it when funtion gets updated.
 */
void objectdictionary_updateGetDataStreamCallback(void(*pGetDataCallback)(DictionaryDataStream* pDictionaryDataStream))
{
	//check we are not assigning to null
	if (pGetDataCallback != NULL) //todo: log eror
	{
		pGetDataStreamCallback = pGetDataCallback;
	}
}

//Method to get the data type at a given index.
DictionaryDataType objectdictionary_getDataType(uint16_t addressIndex,uint8_t subIndex) {
	DictionaryDataType dataType = DATA_TYPE_UNDEFINED;

	int dIndex = getDictionaryIndex(ADDRESS(addressIndex,subIndex));
	//write only if we have a valid dictionary index . any negatives indicate no index found
	if( dIndex >=0)
	{
		dataType= dictionary[dIndex].dataType;

	}

	return dataType;
}

//Method to get the data struct ptr for dictionarydataStream on index and subIndex
void* objectdictionary_getDataStreamPtr(uint16_t addressIndex,uint8_t subIndex)
{
	DictionaryDataStream* pDataStream = NULL;
	int address = ADDRESS(addressIndex,subIndex);
	int dIndex = getDictionaryIndex(address);
		//write only if we have a valid dictionary index . any negatives indicate no index found
	if (dIndex >= 0)
	{
		if (dictionary[dIndex].dataType == DATA_STREAM)
		{
			DictionaryDataStream* pDictionaryDataStream =
					dictionary[dIndex].pDictionaryData;
			//null check
			if (pDictionaryDataStream != NULL)
			{
				pDataStream = &pDictionaryDataStream[DATASTREAM_INDEX(address)];
			}//end null check
		}//end if

	}//end if dindex
	return pDataStream;
}

//Readwrite handlers for data stream we may not need them at all
void readTestDataStream( uint8_t subIndex, void* pData )
{
	//todo: nothing for now
}

void writeTestDataStream( uint8_t subIndex, void* pData )
{
 //todo: nothing for now
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

/*
 * dictionary2.h
 *
 *  Created on: 4/04/2017
 *      Author: Esam.Alzqhoul
 */
/*
 * objectdictionary.h
 *
 *  Created on: 23/01/2017
 *      Author: gaurav.vyas
 */

#ifndef APPLICATION_USER_OBJECTDICTIONARY_H_
#define APPLICATION_USER_OBJECTDICTIONARY_H_
#include "stddef.h"
#include "stm32f7xx_hal.h"
#include "FreeRtos.h"
#include "queue.h"
#include "semphr.h"


//test MultiByte transfer of Data
typedef struct{
	int dataSize;
	void* pData;
	int dataCapacity;

}DictionaryDataStream;

static char testStringData[512]={};
static DictionaryDataStream testDataStream[]= {{0,0,&testStringData,512}};
/////End the multibyte transfer of data

void(*pGetDataStreamCallback)(DictionaryDataStream* pDictionaryDataStream);


typedef enum
{
	DATA_TYPE_UNDEFINED,
	DATA_INT,
	DATA_STREAM
}DictionaryDataType;



//DictionaryValues
typedef struct{
	uint8_t subIndex;
	uint32_t value;
}DictionaryValues;

//QueueData
typedef struct{
	uint16_t index;
	uint8_t subIndex;
	uint32_t dataValue;
}QueueDataEntry;



//this is to store the value for canopen layer
typedef struct{
	uint32_t address;
	uint32_t mask;
	//int dataValue;
	void* pDictionaryData;
	DictionaryDataType dataType;
	int (*handleRead)( uint32_t address );
	void(*handleWrite)( uint32_t address, uint32_t value );
//	uint8_t rwhardware;  //value to indicate whether hardware can be written or read. 0 indicates its is software only value.
	char* desc;

}DictionaryEntry;


//List of all read dictionary functions
int readDacChannel (uint32_t address );
int readDacRange (uint32_t address );
int readMuxOutChannel (uint32_t address );
int readDacTemperature (uint32_t address );
int readDacChannelStepSize (uint32_t address );

//read High voltage lens supply set values
int readHighVoltageLensSupply (uint32_t address );
int readHvLensSupplyStepSize (uint32_t address );

// Digital reads

int readDigitalInputGuage (uint32_t address );
int readPulseTestMode (uint32_t address );
int readPulseOn (uint32_t address );
int readGuageDout (uint32_t address );
int readEnable15VSupply (uint32_t address );
// ADS8638 ADC reads
int readExternalAds8638Channel (uint32_t address );
int readAds8638ChannelActualValue (uint32_t address );
int readAds8638ChannelRange (uint32_t address );

// read pulse counts
int readPulseCount  (uint32_t address );
int readActualCountTime  (uint32_t address );
int readMaxPulseCount  (uint32_t address );
int readMaxCountTime  (uint32_t address );
//Analogue measurements on board
int readAin  (uint32_t address );


//DAC writes
void  writeDacChannel( uint32_t address, int value );
void  writeDacChannelNoUpdate( uint32_t address, int value );
void  writeDacRange( uint32_t address, int value );
void  WriteDacAllChannels( uint32_t address, int value );
void  writeDacRangeAllChannels( uint32_t address, int value );
void  writeMuxOutChannel( uint32_t address, int value );
void  WriteDacUpdateChannel( uint32_t address, int value );
void  WriteDacUpdateAllChannels( uint32_t address, int value );
void  WriteDacPowerDown( uint32_t address, int value );
void  WriteDacDisableThermalProtection( uint32_t address, int value );
void  WriteDacChannelStepSize( uint32_t address, int value );

// High voltage lens supply
void  writeSetHighVoltageLensSupply( uint32_t address, int value );
void  WriteHvLensSupplyStepSize( uint32_t address, int value );

// ADS8638 ADC-12bit writes
void  writeAds8638ChannelRange( uint32_t address, int value );
void  writePowerUpDownAllAds8638Chips( uint32_t address, int value );

// set digital outputs
void  writePulseTestMode( uint32_t address, int value );
void  writePulseOn( uint32_t address, int value );
void  writeGuageDout( uint32_t address, int value );
void  writeEnable15VSupply( uint32_t address, int value );

// set pulse count parameters
void  writeMaxPulseCount( uint32_t address, int value );
void  writeMaxCountTime( uint32_t address, int value );


//general dictionary
void objectdictionary_init();
void objectdictionary_writeDataQueue(QueueDataEntry* pQueueData);
int objectdictionary_getDataValue(uint16_t address,uint8_t subIndex);
void updateHardwareComponent();
void dequeMsg();
void readHardware();

void readTestDataStream( uint8_t subIndex, void* pData );
void writeTestDataStream( uint8_t subIndex, void* pData );

//New methods for testing the multibyte transfer
int getDictionaryIndex(uint32_t addressIndex);

//Method to get the data stream from SDO module block data queue;
void objectdictionary_updateGetDataStreamCallback(void(*pGetDataCallback)(DictionaryDataStream* pDictionaryDataStream));

//return data type of a dictionary index
DictionaryDataType objectdictionary_getDataType(uint16_t addressIndex,uint8_t subIndex);

//read write handlers
//void writeDataInt(int dictionaryIndex ,uint8_t subIndex,int dataSize);
void writeDataStream(int dictionaryIndex ,int address,int dataSize);

//method to write object dictionary
void writeObjectDictionary(QueueDataEntry*  pNewQueueData);

//method to dequeue msgs and update hardware and dictionary.
void dequeueMsg(void);
//DictionaryValues* OD_getviciPosition();


#endif /* DICTIONARY2_H_ */


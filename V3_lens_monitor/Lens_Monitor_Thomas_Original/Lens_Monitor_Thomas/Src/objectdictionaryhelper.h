/*
 * objectdictionaryhelper.h
 *
 *  Created on: 29/09/2017
 *      Author: gaurav.vyas
 */

#ifndef APPLICATION_USER_OBJECTDICTIONARYHELPER_H_
#define APPLICATION_USER_OBJECTDICTIONARYHELPER_H_
#include "stddef.h"
#include "stm32f7xx_hal.h"
#include "FreeRtos.h"
#include "queue.h"
#include "semphr.h"
#include "stdbool.h"

#define ABORT_CODE_ADDRESS_NOT_FOUND 0x06020000
#define ABORT_CODE_SUBINDEX_NOT_FOUND 0x06090011
#define ABORT_CODE_ADDRESS_READ_ONLY 0x06010002

//this enum defines type of data stream for eg string or just bytes or file types .
//This differentiation is to give flexibility of implementation for strings so a padding and quotes could be accomodated for transfer in case of string.
//I think we don't need quotes for strings but Housekeeping controller wants a clear demarcation for string.
typedef struct
{
	int minStringLength;
	char delimiter; //double quote
	char padChar; // space padding
	int delimiterCount; // delimiter to demarcate a string

}StringElements;

typedef enum
{
	DATA_STREAM_TYPE_UNDEFINED,
	DATA_STREAM_TYPE_QUOTED_STRING
}DataStreamType;

//test MultiByte transfer of Data
typedef struct{
	int dataSize;
	void* pData;
	int dataCapacity;
	DataStreamType dataStreamType;
}DictionaryDataStream;

//const int DATA_BUFFER_SIZE=512;
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
	void (*handleWrite)( uint32_t address, uint32_t value );
	//uint8_t rwhardware;  //value to indicate whether hardware can be written or read. 0 indicates its is software only value.
	char* desc;

}DictionaryEntry;


void objectdictionaryhelper_init(DictionaryEntry* pStaticDictionary,int staticDictionarySize);


//New methods for testing the multibyte transfer
int getDictionaryIndex(uint32_t addressIndex);


//return data type of a dictionary index
DictionaryDataType objectdictionaryhelper_getDataType(uint16_t addressIndex,uint8_t subIndex);

//method to write object dictionary
void writeObjectDictionary(QueueDataEntry*  pNewQueueData);

//method to dequeue msgs and update hardware and dictionary.
void objectdictionaryhelper_dequeueWriteMsg();

//Method return pointer to datastream data type
void* objectdictionaryhelper_getDataStreamPtr(uint16_t addressIndex,uint8_t subIndex);

//Method checks address errors for Object dictionary of this node.
//Returns error code if index or subindex is missing or attempt to write read only adress for this object dictionary.
int objectdictionaryhelper_checkAddress(int index,uint8_t subIndex,bool isWrite);

//method returns the const struct of string elements
const StringElements* objectdictionaryhelper_getStringElements(void);


void objectdictionaryhelper_writeDataQueue(QueueDataEntry* pQueueData);

int objectdictionaryhelper_getDataValue(uint16_t address,uint8_t subIndex);


#endif /* APPLICATION_USER_OBJECTDICTIONARYHELPER_H_ */

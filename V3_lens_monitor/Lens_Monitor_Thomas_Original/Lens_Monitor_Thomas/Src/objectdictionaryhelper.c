/*
 * objectdictionaryhelper.c
 *
 *  Created on: 29/09/2017
 *      Author: gaurav.vyas
 */
#include "objectdictionaryhelper.h"
//#define HEATER_INDEX(X) ( X & 0xF)
#define ADDRESS(X,Y) ((X<<8)|Y) //Extract the address from address and subindex .

//Can Layer  should always transfer strings with minimum of 5 character.
const StringElements stringElements={ 5,//minStringLength
		                              0x22, //double quote char delimiter
									  0x20, // char padChar =space padding
									  2};//int delimiterCount

const int WRITE_DICTIONARY_QUEUE_LENGTH = 100;




//const uint8_t numberOfHeaters = 3;
SemaphoreHandle_t qHardwareSemaphoreHandle =NULL;
//SemaphoreHandle_t timerTaskMutex = NULL;
static QueueDataEntry newQueueData;
QueueDataEntry* pNewQueueDataEntry = &newQueueData;
//queue handle for RtosQueue
static QueueHandle_t queueDataHandle = NULL;

//This is the pointer to static dictionary in file objectdictionary.c
DictionaryEntry* pDictionary = NULL;
int dictionarySize = 0;

//init dictionary  to have the queue
void objectdictionaryhelper_init(DictionaryEntry* pStaticDictionary,int staticDictionarySize)
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
		queueDataHandle = xQueueCreate( WRITE_DICTIONARY_QUEUE_LENGTH, sizeof( QueueDataEntry ) );
	}

	//get the pointer to static dictionary
	pDictionary = pStaticDictionary;
	dictionarySize = staticDictionarySize;


	//delete the old semaphore handle and create a new one and give after creating it.
	if (qHardwareSemaphoreHandle == NULL)
	{
		qHardwareSemaphoreHandle = xSemaphoreCreateBinary();

	}
	xSemaphoreGive(qHardwareSemaphoreHandle);

}


//Dequeue all queue messages and write to hard ware
void objectdictionaryhelper_dequeueWriteMsg()
{
	uint8_t updateRequired = 0x00;
	uint8_t updateHeater = 0x00;
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
				//updateHeater  = HEATER_INDEX(ADDRESS(pNewQueueDataEntry->index,pNewQueueDataEntry->subIndex));
			}
		}

	  }
	//we took so give semaphore
	xSemaphoreGive(qHardwareSemaphoreHandle);
	}

	if(updateRequired)
	{
		//update the hardware
		//heater_updateValues(updateHeater);
	}


}



void writeObjectDictionary(QueueDataEntry*  pNewQueueData)
{
	int dictionaryAddress = ADDRESS(pNewQueueData->index,pNewQueueData->subIndex) ;
	int dictionaryIndex = getDictionaryIndex(dictionaryAddress);

    //write only if we have a valid dictionary index . any negatives indicate no index found
    if( dictionaryIndex >=0)
    {
		switch(pDictionary[dictionaryIndex].dataType)
		{
		case DATA_INT:
		 //call write handler
		 if(pDictionary[dictionaryIndex].handleWrite != NULL)
		 {
			 pDictionary[dictionaryIndex].handleWrite(dictionaryAddress,pNewQueueData->dataValue);
		 }
		 break;
		case DATA_STREAM:
			//don't do anything unless needed by hardware layer.
			if (pDictionary[dictionaryIndex].handleWrite != NULL)
			{
				pDictionary[dictionaryIndex].handleWrite(dictionaryAddress,pNewQueueData->dataValue);
			}
		 break;
		default:
			break;

		}
    }

}


//returns the handle to the queue
void objectdictionaryhelper_writeDataQueue(QueueDataEntry* pQueueData)
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
int objectdictionaryhelper_getDataValue(uint16_t addressIndex, uint8_t subIndex) {
	int retDataVal = -1;

	uint32_t address = ADDRESS(addressIndex,subIndex);
	int dIndex = getDictionaryIndex(address);
	//write only if we have a valid dictionary index . any negatives indicates no index found
	if( dIndex >=0)
	{
		switch(pDictionary[dIndex].dataType)
		{
		case DATA_INT:
			if(pDictionary[dIndex].handleRead!=NULL)
			{
			 retDataVal =pDictionary[dIndex].handleRead(address);
			}
			break;
		case DATA_STREAM:
			//here we just  return the data size
			if (pDictionary[dIndex].pDictionaryData != NULL)
			{
				//execute read handler if specified
				if (pDictionary[dIndex].handleRead != NULL)
				{
					retDataVal = pDictionary[dIndex].handleRead(address);
				}
				DictionaryDataStream* pDictionaryDataStream = pDictionary[dIndex].pDictionaryData;
				retDataVal=pDictionaryDataStream->dataSize;
			    //For string we demarcate by quotes so data size = datasize+2 and if data size is lesser than 3 bytes
				if(pDictionaryDataStream->dataStreamType==DATA_STREAM_TYPE_QUOTED_STRING)
				{
					retDataVal = ((retDataVal+ stringElements.delimiterCount)>=stringElements.minStringLength) ?
							       retDataVal+stringElements.delimiterCount:
								   stringElements.minStringLength;
				}
				else
				{
					//for anything other than string we need to have 5 bytes minimum, NULL char is used for padding
					retDataVal = retDataVal<stringElements.minStringLength? stringElements.minStringLength : retDataVal;
				}
			}
			break;

		}
	}

	return retDataVal;
}


//Method to return the dictionary index which can be used to access the dictionary object
int getDictionaryIndex(uint32_t addressIndex)
{

	int dictionaryIndex = -1;
	for (uint8_t i = 0; i < dictionarySize; i++)
	{
		if ((addressIndex & pDictionary[i].mask)	== (pDictionary[i].address & pDictionary[i].mask))
		{
			dictionaryIndex = i;
			break;
		}
	}
	return dictionaryIndex;

}


//Method to get the data type at a given index.
DictionaryDataType objectdictionaryhelper_getDataType(uint16_t addressIndex,uint8_t subIndex) {
	DictionaryDataType dataType = DATA_TYPE_UNDEFINED;

	int dIndex = getDictionaryIndex(ADDRESS(addressIndex,subIndex));
	//write only if we have a valid dictionary index . any negatives indicate no index found
	if( dIndex >=0)
	{
		dataType= pDictionary[dIndex].dataType;

	}

	return dataType;
}

//Method to get the data struct ptr for dictionarydataStream on index and subIndex
void* objectdictionaryhelper_getDataStreamPtr(uint16_t addressIndex,uint8_t subIndex)
{
	DictionaryDataStream* pDataStream = NULL;
	int address = ADDRESS(addressIndex,subIndex);
	int dIndex = getDictionaryIndex(address);
		//write only if we have a valid dictionary index . any negatives indicate no index found
	if (dIndex >= 0)
	{
		if (pDictionary[dIndex].dataType == DATA_STREAM)
		{
			DictionaryDataStream* pDictionaryDataStream =
					pDictionary[dIndex].pDictionaryData;
			//null check
			if (pDictionaryDataStream != NULL)
			{
				pDataStream = pDictionaryDataStream;
			}//end null check
		}//end if

	}//end if dindex
	return pDataStream;
}


//Method checks address errors for Object dictionary of this node.
//Returns error code if index or subindex is missing or attempt to write read only adress for this object dictionary.
int objectdictionaryhelper_checkAddress(int index,uint8_t subIndex,bool isWrite)
{
	//abort code 0 indicates no  address error to SDO layer
	int abortCode = 0;
	int address = ADDRESS(index,subIndex);
	/*if address index doesn't exist then abortcode=0x06020000 Object doesn't exist in Object dictionary
	if subindex not found then  abortcode =  0x06090011 Object doesn't exist in object dictionary
	if attempt to wite to read only  then abortCode= 0x06010002
	*/
	//TODO: subIndex not found abort code implementation needs to be revisited for more than one type of peripherals for e.g. one dictionary might have 3 relays and 4 rs485 channels
	int dictionaryIndex =getDictionaryIndex(address);

	if (dictionaryIndex<0)// index not forund
	{
		abortCode = ABORT_CODE_ADDRESS_NOT_FOUND;
	}/*We will implement that in a Hardware as they would know the mask for every address and subindex
	else if(HEATER_INDEX(address) >= numberOfHeaters) //subindex not found
	{
		abortCode =  ABORT_CODE_SUBINDEX_NOT_FOUND;
	}*/
	else if(isWrite)
	{
		//check if there is write handler is null , means it is read only address
		if(pDictionary[dictionaryIndex].handleWrite == NULL)
		{
			abortCode = ABORT_CODE_ADDRESS_READ_ONLY;
		}
	}

 return abortCode;
}


//method returns the const struct of string elements
const StringElements* objectdictionaryhelper_getStringElements(void)
{
	return &stringElements;
}

//check whether subindex exist in the dictionary

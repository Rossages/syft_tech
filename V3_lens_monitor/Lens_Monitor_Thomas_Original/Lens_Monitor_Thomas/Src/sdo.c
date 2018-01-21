/*
 * sdo.c
 *
 *  Created on: 15/12/2016
 *      Author: gaurav.vyas
 */
#include "sdo.h"
#include "canbus.h"
#include "string.h"
// modify in initialization
 static QueueHandle_t blkDataQHandle = NULL;
uint16_t sdoRxBufferIndex = 0;
uint16_t sdoTxBufferIndex = 0;
static QueueDataEntry queueDataEntry;
//lets keep a block size of 10
uint8_t sdoBlockSize=127;
//Transfer buffer
const int BLOCK_MSG_QUEUE_LENGTH = 292; //(292 *7= 2044 bytes)

//This block of data is used for transferring the string for block transfer
static char dataBuffer[512];
static DictionaryDataStream transferStreamBuffer[]= {{0,&dataBuffer,sizeof(dataBuffer),DATA_STREAM_TYPE_UNDEFINED}};
/**
 * Method to initilize all pointers and assign receive callbacks.
 */
void sdo_init(uint8_t nodeId,
		      SdoModule* pSdoObject,
		      CanOpenDriverModule* pCanOpenDriverRx,
			  uint16_t canOpenDriverRxIndex,
			  CanOpenDriverModule* pCanOpenDriverTx,
			  uint16_t canOpenDriverTxIndex

		 )
{
	sdoRxBufferIndex = canOpenDriverRxIndex;
	sdoTxBufferIndex = canOpenDriverTxIndex;

	//nit the pSdoObject first
	pSdoObject->nodeId = nodeId;
	uint32_t cobIdClientToServer = CAN_OPEN_SDO_CLIENT_TO_SERVER+nodeId;
	uint32_t cobIdServerToClient = CAN_OPEN_SDO_SERVER_TO_CLIENT+nodeId;
	//initialzing so keep the sdo state to be idle.
	pSdoObject->sdoCurrentState = SDO_ST_IDLE; //This is not used at the moment
	pSdoObject->canRxNew = false;
	pSdoObject->pFunctSignal = NULL;

	pSdoObject->pDataToTransfer = NULL;
	pSdoObject->pDictioanryDataStream = NULL;
	pSdoObject->sequenceNumber = 0;
	pSdoObject->pBlockStartByte = NULL;

	//init the block buffer
	memset(dataBuffer,0,sizeof(dataBuffer));

	//rx buffer init
	canbus_initRxBuffer(cobIdClientToServer,pCanOpenDriverRx, canOpenDriverRxIndex, pSdoObject, receiveSdo);

	//tx buffer init
	pSdoObject->pCanOpenDriverTxObject = pCanOpenDriverTx;
	pSdoObject->pCanOpenTxBuffer =canbus_initTxBuffer(cobIdServerToClient,pCanOpenDriverTx,canOpenDriverTxIndex,8);

	//make sure we set the
	//pSdoObject->pCanOpenDriverTxObject->pCanOpenTxBuffer = pSdoObject->pCanOpenTxBuffer

	//Initialize the block data queue, just rest the queue and  create a new only if we are doing it for first time.
	if (blkDataQHandle != NULL)
	{
		xQueueReset(blkDataQHandle);
		//vQueueDelete(blkDataQHandle);
	}
	else
	{
		blkDataQHandle = xQueueCreate(BLOCK_MSG_QUEUE_LENGTH, sizeof(uint8_t) * 7); //292*7= 2044 ~ approx 2048
	}

}



/**
 * Method to read data at object dictionary address
 * This method reads and Transmits data value for data size 4 bytes or shorter.
 * For data size > 4 bytes it returns the data size.
 */
void readObjectDictionary(SdoModule* pSdoObject)
{
	 uint16_t addressIndex = pSdoObject->canRxData[2];
	 addressIndex = addressIndex << 8 | pSdoObject->canRxData[1];
	 uint8_t subIndex = pSdoObject->canRxData[3];



		 uint32_t value = objectdictionaryhelper_getDataValue(addressIndex, subIndex);

	//send response to sdoClient //its expedite data in all cases 010 SCS_INITIATE_UPLOAD 00010 expedite bit set
	pSdoObject->pCanOpenTxBuffer->data[0] = SCS_UPLOAD_INITIATE << 5 | 0x02;
	if (objectdictionaryhelper_getDataType(addressIndex, subIndex) == DATA_STREAM)
	{
		// if this is block upload request we send the upload block response
		pSdoObject->pCanOpenTxBuffer->data[0] = SCS_UPLOAD_BLOCK << 5 | 0x02;
		//check the string type and
	}

	pSdoObject->pCanOpenTxBuffer->data[1] = pSdoObject->canRxData[1];
	pSdoObject->pCanOpenTxBuffer->data[2] = pSdoObject->canRxData[2];
	pSdoObject->pCanOpenTxBuffer->data[3] = pSdoObject->canRxData[3];

	//Copy the value to array
	pSdoObject->pCanOpenTxBuffer->data[7] = (value >> 24) & 0xFF;
	pSdoObject->pCanOpenTxBuffer->data[6] = (value >> 16) & 0xFF;
	pSdoObject->pCanOpenTxBuffer->data[5] = (value >> 8) & 0xFF;
	pSdoObject->pCanOpenTxBuffer->data[4] = value & 0xFF;

	//Make sure we clear new data flag
	pSdoObject->canRxNew = false;
	//send
	sendSdo(pSdoObject);

}


/**
 * Method to write the Object dictionary data queue and send the sdo response
 *
 */
void writeDataQueue(SdoModule* pSdoObject)
{

 uint16_t addressIndex = pSdoObject->canRxData[2];
 addressIndex = addressIndex << 8 | pSdoObject->canRxData[1];
 uint8_t subIndex = pSdoObject->canRxData[3];

 //check for how many bytes are actually data //8-nodata to 7  is not data implement support for that


 uint32_t value  = pSdoObject->canRxData[7]<<24|pSdoObject->canRxData[6]<<16|pSdoObject->canRxData[5]<<8|pSdoObject->canRxData[4];



    writeOdDataQueue(addressIndex,subIndex,value);

	//send response to sdoClient
	pSdoObject->pCanOpenTxBuffer->data[0] =0x60;
	pSdoObject->pCanOpenTxBuffer->data[1] = pSdoObject->canRxData[1];
	pSdoObject->pCanOpenTxBuffer->data[2] = pSdoObject->canRxData[2];
	pSdoObject->pCanOpenTxBuffer->data[3] = pSdoObject->canRxData[3];
	//Everything else is zero because we don't care
	memset(&(pSdoObject->pCanOpenTxBuffer->data[4]),0,4);

	//Make sure we clear new data flag
	pSdoObject->canRxNew = false;
	sendSdo(pSdoObject);
}

/**
 * receive a SDO CAN frame
 */
void receiveSdo(void *object, CanRxMsgTypeDef *pMessage)
{

	//Type cast SDo module should only receive the sdo object make sure in interuppt method
	SdoModule* pSdoObject = (SdoModule*)object;
    //ToDO Look at DLC
	memcpy((void*)&(pSdoObject->canRxData),(void*)&(pMessage->Data),8);
    pSdoObject->canRxNew =true;
}

/**
 * send a SDO CAN frame
 */
void sendSdo(SdoModule *pSdoModule)
{
	//printf for test only
	// printf("sendSdo pSdoObject %x",pSdoModule->canRxData[1]);
	canbus_transmit(pSdoModule->pCanOpenDriverTxObject,sdoTxBufferIndex);

}


//Expedite transfer in 8 byte message
		/*
		 *
		 *                                  Index and subIndex Object dictionary       Data/Value
		 *                                |-------------------------------------|     |---------|
		 * BYTES->           1             2               3              4            5 6 7 8
			                 ||
			                 \/
		  *BITS->  1  2   3      4           5     6                    7          8
		          |________|   |___|        |_______|                 |____|     |___|
		 *           CCS       Reserved      No of Bytes              Expedite    Where data size
		                                  that dont contain data      Transfer    is Set.
			 */
	//TODO: Check for CCS and if there is something new to process .For timebeing just process with check for IDLe
	/*For now we are only concerned about expedited transfers so listing request and response here
	                                      SDO Client(1st Byte)                            SDO Server (1st Byte)
	InitiateSDODownload(Expedite)            (00100010) 0x22        Request====>
	(Write Object Dictionary)
	                                                                  <====Response           (01100000) 0x60
	 ----------------------------------------------------------------------------------------------------------
	 InitiateSDOUPload(Expedite)            (01000000)0x40            Request====>
	 (ReadObjectDictionary)

	                                                    			<====Response   		(01000010)0x42

	--------------------------------------------------------------------------------------------------------
	*/
/**
 * process a sdpMessage -
 *  Download means write to object dictionary
 * Upload means read the object dictionary.
 */
void sdo_process(SdoModule* pSdoObject)
{


		//TODO: this could be more flexible with checking the first three bits CCS and SCS
		if (pSdoObject->canRxNew == true)
		{

			//first three bits
			uint8_t ccs = pSdoObject->canRxData[0]>>5;

			//IF ccs ==  abort just abort the whole transfer. and reset the state to idle
			if((pSdoObject->canRxData[0] & 0xFF) == 0x80)//if((ccs == CCS_ABORT ) 10000000 = ccs_abort fisrt three bits are 100 ( 4)
			{
				//Empty the blockQueue.
				//when asked to abort abort the block transfer  clear the buffers and set the state to IDLE
				pSdoObject->sdoCurrentState = SDO_ST_IDLE;

				//reset the sdo module object pointer
				pSdoObject->pDataToTransfer = NULL;
				pSdoObject->pDictioanryDataStream = NULL;
				pSdoObject->sequenceNumber = 0;
				pSdoObject->pBlockStartByte = NULL;

				//reset the queue we dont wait for data nymore
				xQueueReset(blkDataQHandle);

			}
			else
			{


			//Look for CCS only if state is idle
			if(pSdoObject->sdoCurrentState == SDO_ST_IDLE)
			{
				//Check for addressing error
				int abortCode = objectdictionaryhelper_checkAddress((pSdoObject->canRxData[2]<<8)|(pSdoObject->canRxData[1]),pSdoObject->canRxData[3],
						((ccs==CCS_DOWNLOAD_INITIATE) || (ccs==CCS_DOWNLOAD_BLOCK)) ? true:false);
				if(!abortCode)
				{
				//check for addressing error
				switch (ccs)
				{
				case CCS_SEGMENT_DOWNLOAD:
					//NOT supported
					break;
				case CCS_DOWNLOAD_INITIATE:
					//We only support expedite Write via this command  and NO Segment transfers
					if ((pSdoObject->canRxData[0] & 0x02) == 0x02) //Expedite transfer "e" bit is set.
							{
						writeDataQueue(pSdoObject);
					}
					break;
				case CCS_UPLOAD_INITIATE:
					//send the data if 4 bytes otherwise send the data size with  block response. Master will send the Upload_BLOCK COMMAND for handshake
					readObjectDictionary(pSdoObject);
					break;
				case CCS_UPLOAD_SEGMENT:
					//Not supported
					break;
				//case CCS_ABORT:
					//break;
				case CCS_UPLOAD_BLOCK:
					//last 00 meaning just request to initiate upload and we need to send size of data bytes
					if((pSdoObject->canRxData[0] & 0x03) == 0x00)
					{
						//make sure we have data stream pointer to be able to change the state
						DictionaryDataStream* pDictionaryDataStreamSource = (DictionaryDataStream*)objectdictionaryhelper_getDataStreamPtr((pSdoObject->canRxData[2]<<8)|(pSdoObject->canRxData[1]),pSdoObject->canRxData[3]);
						if(pDictionaryDataStreamSource!=NULL)
						{
							//Dictionary design will take care of sending the data size first time.
							readObjectDictionary(pSdoObject);
							pSdoObject->pDictioanryDataStream = getDataStreamPtr(pDictionaryDataStreamSource, (DictionaryDataStream*)&transferStreamBuffer);
							pSdoObject->pDataToTransfer = pSdoObject->pDictioanryDataStream->pData;
							pSdoObject->blockSize = pSdoObject->canRxData[4];
							pSdoObject->sequenceNumber = 0;
							//Change the SDO state to initiate mode
							pSdoObject->sdoCurrentState = SDO_ST_UPLOAD_BL_INITIATE;
						}
						else
						{
							//this is to get  all integer writes
							//Dictionary design will take care of sending the data size first time.
							readObjectDictionary(pSdoObject);
						}



					}


					break;
				case CCS_DOWNLOAD_BLOCK:
				{
					//Change the state here
					pSdoObject->sdoCurrentState = SDO_ST_DOWNLOAD_BL_INITIATE;
					//send response SCS_DOWNLOAD_BLOCK _  _ SErvr CRC support _ 0
					//101--SC-0
					uint8_t scs= SCS_DOWNLOAD_BLOCK<<5;
					pSdoObject->pCanOpenTxBuffer->data[0] =scs; //no CRC for now
					pSdoObject->pCanOpenTxBuffer->data[1] = pSdoObject->canRxData[1];
					pSdoObject->pCanOpenTxBuffer->data[2] = pSdoObject->canRxData[2];
					pSdoObject->pCanOpenTxBuffer->data[3] = pSdoObject->canRxData[3];
					pSdoObject->pCanOpenTxBuffer->data[4] = sdoBlockSize; // this is block size and cannot be more than 127
					//store whole message in to buffer
					writeBlockDataQueue(pSdoObject); //store this one for addresses and data size
					sendSdo(pSdoObject);
				}
					break;
				default:
					break;
				}
				}
				//we have a address eror
				else
				{
					//send response to sdoClient
						pSdoObject->pCanOpenTxBuffer->data[0] =0x80;// abort sdo
						pSdoObject->pCanOpenTxBuffer->data[1] = pSdoObject->canRxData[1];
						pSdoObject->pCanOpenTxBuffer->data[2] = pSdoObject->canRxData[2];
						pSdoObject->pCanOpenTxBuffer->data[3] = pSdoObject->canRxData[3];
						pSdoObject->pCanOpenTxBuffer->data[7] = (abortCode >> 24) & 0xFF;
						pSdoObject->pCanOpenTxBuffer->data[6] = (abortCode >> 16) & 0xFF;
						pSdoObject->pCanOpenTxBuffer->data[5] = (abortCode>> 8) & 0xFF;
						pSdoObject->pCanOpenTxBuffer->data[4] = abortCode & 0xFF;
						sendSdo(pSdoObject);
				}
			}
			else if((pSdoObject->sdoCurrentState == SDO_ST_DOWNLOAD_BL_INITIATE) || (pSdoObject->sdoCurrentState == SDO_ST_DOWNLOAD_BL_SUBBLOCK))
			{
				//store whole message in to buffer
				writeBlockDataQueue(pSdoObject);
				//Check if we have reached the block size server send a response message again

				//Check if this is the last data segment or blocksize reached (sequence number  7 bits so 0<seq number<128)
				if(((pSdoObject->canRxData[0]>>7)== 0x01)||((pSdoObject->canRxData[0]&0x7F)==sdoBlockSize))
				{

					//No more segments to download or reached bloack size.
					//Check for error  and send the 1st byte as sequence number of last segment


					//Server should send the response message
					//SCS_DOWNLOAD_BLOCK---10 ,Byte1 sequence number of last segment
					//101----10
					uint8_t scs = SCS_DOWNLOAD_BLOCK<<5 |0x02;
					pSdoObject->pCanOpenTxBuffer->data[0]= scs;
					pSdoObject->pCanOpenTxBuffer->data[1] = pSdoObject->canRxData[0]&0x7F; //sequence number of last segment //check for erros
					pSdoObject->pCanOpenTxBuffer->data[2] = sdoBlockSize;
					if((pSdoObject->canRxData[0]&0x7F)==sdoBlockSize)//reached block size
					{
						pSdoObject->sdoCurrentState = SDO_ST_DOWNLOAD_BL_SUBBLOCK;
					}

					//no more data
					if ((pSdoObject->canRxData[0]>>7)== 0x01) //no more data
					{
						pSdoObject->sdoCurrentState = SDO_ST_DOWNLOAD_BL_END;
					}

					sendSdo(pSdoObject);

				}
			}
			else if(pSdoObject->sdoCurrentState == SDO_ST_DOWNLOAD_BL_END)
			{
				//check ccs
				if(ccs==CCS_DOWNLOAD_BLOCK)
				{
					//remove from the  number of bytes that is not data from the back of the queue
					//send response to complete the transfer
					//SCS_DOWNLOAD_BLOCK 101----1
					pSdoObject->pCanOpenTxBuffer->data[0] = SCS_DOWNLOAD_BLOCK<<5|0x01;

					sendSdo(pSdoObject); //ignore the CRCs for now.
					//cHECK the CRCs
				}

				//set the state back to idle
				pSdoObject->sdoCurrentState = SDO_ST_IDLE;
				//get data from block transfer queue
				getDataStreamFromBlockDataQueue();

			}
			//handle upload initiate and change the states to
			else if(pSdoObject->sdoCurrentState == SDO_ST_UPLOAD_BL_INITIATE)
			{
				//change the state
				if ((pSdoObject->canRxData[0] & 0x03) == 0x03)
				{

					pSdoObject->sdoCurrentState = SDO_ST_UPLOAD_BL_INITIATE_2;
				}
			}
			else if (pSdoObject->sdoCurrentState == SDO_ST_UPLOAD_BL_SUBBLOCK)
			{
				if ((pSdoObject->canRxData[0] & 0x02) == 0x02)
				{
					pSdoObject->sdoCurrentState = SDO_ST_UPLOAD_BL_INITIATE_2; //indicate to continue the transfer
					//check for last sequence number if matches continue with new block else re transmit whole block
					if (pSdoObject->canRxData[1] != pSdoObject->sequenceNumber)
					{
						//resend the whole block
						//rewind the pointer by block size
						pSdoObject->pDataToTransfer = pSdoObject->pBlockStartByte;
						//start the transfer from the beginning
						pSdoObject->sequenceNumber = 0;

					}
					//set the new block size
					pSdoObject->blockSize = pSdoObject->canRxData[2];

				}
			}
			else if(pSdoObject->sdoCurrentState == SDO_ST_UPLOAD_BL_END)
			{
				if ((pSdoObject->canRxData[0] & 0x01) == 0x01)
				{
					//perform CRC canrxdata[1] +[2]
					//reset the  state if needed

					//set the state back to idle
					pSdoObject->sdoCurrentState = SDO_ST_IDLE;

					//reset the sdo module object pointer
					pSdoObject->pDataToTransfer = NULL;
					pSdoObject->pDictioanryDataStream = NULL;
					pSdoObject->sequenceNumber = 0;
					pSdoObject->pBlockStartByte = NULL;
				}
				else if ((pSdoObject->canRxData[0] & 0x02) == 0x02)
				{
					//check for last sequence
					if (pSdoObject->sequenceNumber == pSdoObject->canRxData[1])
					{
						//sequence number received correctly-- Data received correctly- indicate end transfer
						//1st byte was calculated and set when the end was detected
						sendSdo(pSdoObject);
						// dont change the state to idle we wait for one more frame to goto idle
					}
					else
					{
						//Sequence number didn't match send the block again
						pSdoObject->sdoCurrentState = SDO_ST_UPLOAD_BL_INITIATE_2; //indicate to continue the transfer

						//resend the whole block
						pSdoObject->pDataToTransfer = pSdoObject->pBlockStartByte;
						//start the transfer from the beginning
						pSdoObject->sequenceNumber = 0;

					}

				}

			}

		}//end else not abort


		pSdoObject->canRxNew = false;

	}//end if CanRx new true

		//Always check for upload, it is state driven we will only send if we are set to a particular state
		sdoUploadBlock(pSdoObject);
		//we got the data here so reset
		memset(&(pSdoObject->canRxData),0,8);

}

/*
 * Method to write data bytes to a queue while doing block download.
 *
 */
void writeBlockDataQueue(SdoModule* pSdoModule)
{
	uint8_t blkData[7] = {};
	//uint8_t blkDataTest[7]={};
	uint8_t* pData = &(pSdoModule->canRxData);
	pData++; //only store data
	memcpy((void*)&blkData,(void*)pData,7);
	if (xQueueSend(blkDataQHandle,(void*)&blkData,( TickType_t ) 0) != pdPASS) //no block if queue full
	{
		//todo Log error
	}


}


//This is the call back method called from object dictionary when indicated to get the data from SDO . only for block transfers.
void getDataStreamFromBlockDataQueue()
{
	uint8_t blkData[7] = {};
	int dataSize = 0;
	DictionaryDataStream* pDataStreamBuffer = (DictionaryDataStream*)&transferStreamBuffer;

	const StringElements* pStringElements = objectdictionaryhelper_getStringElements();
	//receive the first message and know the size to read
	if(xQueueReceive(blkDataQHandle,(void*)&blkData,( TickType_t ) 0)!=pdPASS)
	{
		//TODO: Log error and remove return to something else. method should always return at the end
		return;
	}
	//Get the address
	DictionaryDataStream* pDictionaryDataStream = objectdictionaryhelper_getDataStreamPtr(blkData[1]<<8|blkData[0],blkData[2]);
	//first message contains the size of data (address->0,1,2, Data--> 3,4,5,6)
	dataSize = (blkData[6]<<24|blkData[5]<<16|blkData[4]<<8|blkData[3])>512 ?
			    512:
				(blkData[6]<<24|blkData[5]<<16|blkData[4]<<8|blkData[3]);

	//queue it  t execute write handler
	writeOdDataQueue(blkData[1]<<8|blkData[0],blkData[2], dataSize);

	//before we start getting the data we set the reciving buffer
	memset((void*)pDataStreamBuffer->pData,0,pDataStreamBuffer->dataCapacity);

	//check the max size
	uint8_t* pData = (uint8_t*)pDataStreamBuffer->pData;
	uint8_t* pDataStart = (uint8_t*)pDataStreamBuffer->pData;
	//copy data from
	while(1)
	{
		if(xQueueReceive(blkDataQHandle,(void*)&blkData,( TickType_t ) 0)== pdPASS)
		{

	    if(dataSize> ((pData+7)-pDataStart))
	    {
	    	memcpy((void*)pData,(void*)&blkData,7);
	        pData+=7;
	    }
	    else if(dataSize <= ((pData+7)-pDataStart))
	    {
	    	int cpySize = (pDataStart+dataSize)-pData;//;  dataSize - (((pData)-pDataStart)- dataSize);//?((pData+7)-pDataStart)%7:7;
	    	//pDataStart = pData+7;
	    	memcpy((void*)pData,(void*)&blkData,cpySize);
	    	//set the datasize and complete
	    	pDataStreamBuffer->dataSize = dataSize;
	    	break;
	     //all data copied no need to increment the pointer.
	    }
		}
		else
		{
			//TOdO: log error
			//printf("failed to receive from queue");
		}
	}

	//before we start copying new data just reset the stuff

	//check for quotes if so it is a string type?
	if(pDictionaryDataStream->dataStreamType == DATA_STREAM_TYPE_QUOTED_STRING)
	{

		//got to detect the end of string , we also want to accomodate the null chars
		char* pEndOfString = pDataStreamBuffer->pData + pDataStreamBuffer->dataSize -1;
		while(pEndOfString != pDataStreamBuffer->pData)
		{
			if(*pEndOfString==pStringElements->delimiter)
			{
				break;
			}
			pEndOfString--;
		}

		//calculate actual payload.
		pDictionaryDataStream->dataSize = ((void*)pEndOfString - pDataStreamBuffer->pData) -1;// double quotes are included in received data size and quotes won't count in stored data size.
		if (pDictionaryDataStream->dataSize < 0)
		{
			pDictionaryDataStream->dataSize = pDataStreamBuffer->dataSize;
		}
		memcpy(pDictionaryDataStream->pData,pDataStreamBuffer->pData+1,pDictionaryDataStream->dataSize);
	}
	else
	{

		char* pEndOfData = pDataStreamBuffer->pData + pDataStreamBuffer->dataSize -1;
		//calculate actual payload in min size data that is remove any NULL paddings from data lesser than
		if(pDataStreamBuffer->dataSize == pStringElements->minStringLength)
		{
		while((pEndOfData != pDataStreamBuffer->pData)&&(*pEndOfData==0x00)) //(
			{
			  pEndOfData--;
			}
		}
		pDictionaryDataStream->dataSize = (((void*)pEndOfData - pDataStreamBuffer->pData) +1) < 0? 0 : ((void*)pEndOfData - pDataStreamBuffer->pData)+1 ;
		memcpy(pDictionaryDataStream->pData,pDataStreamBuffer->pData,pDictionaryDataStream->dataSize);
	}
	//

}



/**
 * Method to just write the object dictionary data queue so the change gets propogated to hardware as well as OD
 */
void writeOdDataQueue(uint16_t addressIndex,uint8_t subIndex, uint32_t value)
{
	queueDataEntry.dataValue = value;
	   queueDataEntry.index = addressIndex;
	   queueDataEntry.subIndex = subIndex;
	   objectdictionaryhelper_writeDataQueue(&queueDataEntry);

}


//Method to do a block upload
void sdoUploadBlock(SdoModule* pSdoModule )
{
	const int frameDataBytes = 7;
	//Check the SDO state for initiate we have to start from beginning so reset everything.
    //keep sending the data only if we are indicated to do so.
	if(pSdoModule->sdoCurrentState == SDO_ST_UPLOAD_BL_INITIATE_2)
	{
		//set SDO initiateuploadBlock
		//pSdoModule->sdoCurrentState = SDO_ST_UPLOAD_BL_INITIATE;
		//get objest dictionart datastream pointer // we already get that in sdo object when upload request began.

		//set sequence number to 1 //increment
		pSdoModule->sequenceNumber ++;

		//Check the sequence and set the state
		if (pSdoModule->sequenceNumber == pSdoModule->blockSize)
		{
			pSdoModule->sdoCurrentState = SDO_ST_UPLOAD_BL_SUBBLOCK;
		}

		//calculate bytesToTransfer = dataSize -(pDataPointer-pDataStreamPointer)
		int bytesToTransfer = pSdoModule->pDictioanryDataStream->dataSize - (pSdoModule->pDataToTransfer - pSdoModule->pDictioanryDataStream->pData );
		int bytesToCopy = 0;
		//assign sequnce number to firstbyte sdoTxBuffer[0] = sequencenumber
		pSdoModule->pCanOpenTxBuffer->data[0] = pSdoModule->sequenceNumber;
		//determine bytes to copy in this segment and set the state as well
		if (bytesToTransfer<= frameDataBytes)
		{
			pSdoModule->pCanOpenTxBuffer->data[0] |= 0x01<<frameDataBytes ;
		    bytesToCopy = bytesToTransfer;
		    pSdoModule->sdoCurrentState = SDO_ST_UPLOAD_BL_END;
		}
		else
		{
			bytesToCopy = frameDataBytes;
		}
		//increment the buffer, pTxBuffer can max carry only 7 bytes of data
		//save the block start byte
		pSdoModule->pBlockStartByte = pSdoModule->pDataToTransfer;
		void* pTxBuffer = &(pSdoModule->pCanOpenTxBuffer->data[1]) ;
		memset(pTxBuffer,0,frameDataBytes);
		memcpy(pTxBuffer, pSdoModule->pDataToTransfer,bytesToCopy);
		pSdoModule->pDataToTransfer += bytesToCopy;

		//sendSDO here
		sendSdo(pSdoModule);

		//To send the end sdo if we are at the end of transfer we need to save the bytes to copy so it gets transferred correctly.
		if(pSdoModule->sdoCurrentState == SDO_ST_UPLOAD_BL_END)
		{

			memset(pTxBuffer,0,frameDataBytes);
			// n =  number of bytes that is not data = 7-bytesToCopy
			//110 --- 01
			pSdoModule->pCanOpenTxBuffer->data[0] = (SCS_UPLOAD_BLOCK <<5)|(frameDataBytes-bytesToCopy)<<2|(0x01);
			//pSdoModule->pCanOpenTxBuffer->data[0] = (SCS_UPLOAD_BLOCK <<5)|(notDataBytes)|(0x01);

		}

	}//end if




}


DictionaryDataStream* getDataStreamPtr(DictionaryDataStream* pDataStreamSource, DictionaryDataStream* pDataTransferBuffer)
{
	const StringElements* pStringElements = objectdictionaryhelper_getStringElements();
	if (pDataStreamSource != NULL)
	{
		switch (pDataStreamSource->dataStreamType)
		{
		case DATA_STREAM_TYPE_QUOTED_STRING:
		{
			//set the maxdata size correctly
			int maxDataSize = (pDataStreamSource->dataSize > (pDataTransferBuffer->dataCapacity-pStringElements->delimiterCount )) ? (pDataTransferBuffer->dataCapacity-pStringElements->delimiterCount ):pDataStreamSource->dataSize ;
			//Add padding if needed , add double quotes and Copy string to local array and transfer
			memset(pDataTransferBuffer->pData, pStringElements->padChar, pDataTransferBuffer->dataCapacity);
			memcpy(pDataTransferBuffer->pData + 1, pDataStreamSource->pData,
					maxDataSize);
			//add delimiter
			memset(pDataTransferBuffer->pData, pStringElements->delimiter, 1);

			//detect the end of string , we should always have 5 byte long string to support house keeper. (We don't need it but unfortunately housekeeper needs this support.)
			//house kepper string format eg  5 bytes with padded spaces [""   ]
			void* pEndOfString = pDataTransferBuffer->pData + maxDataSize + 1;

			//mark end of string with delimiter
			memset(pEndOfString, pStringElements->delimiter, 1);

			//accomodate padding.
			pEndOfString = (maxDataSize <= (pStringElements->minStringLength - pStringElements->delimiterCount)) ?
							      pDataTransferBuffer->pData + pStringElements->minStringLength - 1 :
							      pDataTransferBuffer->pData + pDataStreamSource->dataSize + 1;


			//set the datastreamtype and size
			pDataTransferBuffer->dataSize = (pEndOfString - pDataTransferBuffer->pData)+1;
			pDataTransferBuffer->dataStreamType = pDataStreamSource->dataStreamType;

		}
			break;

		//default and undefined - copy whole buffer.
		case DATA_STREAM_TYPE_UNDEFINED:
		default:
			//we don't care just transfer the data bytes but null padding if lesser than 5 bytes
			memset((void*)pDataTransferBuffer->pData, 0, pDataTransferBuffer->dataCapacity);
			pDataTransferBuffer->dataStreamType=pDataStreamSource->dataStreamType;
			//set max bounds
			pDataTransferBuffer->dataSize = (pDataStreamSource->dataSize > pDataTransferBuffer->dataCapacity) ? pDataTransferBuffer->dataCapacity:pDataStreamSource->dataSize ;

			//copy the data
			memcpy(pDataTransferBuffer->pData, pDataStreamSource->pData,pDataTransferBuffer->dataSize);

			//set min bounds data size to take care of shorter byte stream
			pDataTransferBuffer->dataSize = (pDataTransferBuffer->dataSize<=pStringElements->minStringLength) ? pStringElements->minStringLength : pDataTransferBuffer->dataSize;



			break;

		}
	}

	return pDataTransferBuffer;
}



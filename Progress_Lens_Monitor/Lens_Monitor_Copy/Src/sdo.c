/*
 * sdo.c
 *
 *  Created on: 15/12/2016
 *      Author: gaurav.vyas
 */
#include "sdo.h"
#include "objectdictionary.h"
#include "global.h"

// modify in initialization
 static QueueHandle_t blkDataQHandle = NULL;
uint16_t sdoRxBufferIndex = 0;
uint16_t sdoTxBufferIndex = 0;
static QueueDataEntry queueDataEntry;
//lets keep a block size of 10
uint8_t sdoBlockSize=10;
//Test dictionary***************************************************
//this is a temp object dictionary for testing the protocol
/*static ObjectDictionary objectDictionary[]=
{
		{0x0000,0x00,0},
		{0x0001,0x02,2},
		{0x0001,0x03,3},
		{0x1000,0x00,1000}
};*/
//test dictionary****************************************************

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
		blkDataQHandle = xQueueCreate(292, sizeof(uint8_t) * 7); //292*7= 2044 ~ approx 2048
	}
	//set the call back in the object dictionary; Pass the function pointer for call back(register the call back)
	objectdictionary_updateGetDataStreamCallback(getDataStreamFromBlockQueue); //getdataStreamFromBlockQueue is method to get the data from blockqueue


}



/**
 * Method to read object dictionary
 */
void readObjectDictionary(SdoModule* pSdoObject)
{

	uint16_t addressIndex = pSdoObject->canRxData[2];
	 addressIndex = addressIndex << 8 | pSdoObject->canRxData[1];
	 uint8_t subIndex = pSdoObject->canRxData[3];
	 //Make sure this is correct
	 //uint32_t value  = 0;//pSdoObject->canRxData[7]<<24|pSdoObject->canRxData[6]<<16|pSdoObject->canRxData[5]<<8|pSdoObject->canRxData[5];

	 uint32_t value  = objectdictionary_getDataValue(addressIndex, subIndex);
	 {


		//send response to sdoClient //its expedite data in all cases 010 SCS_INITIATE_UPLOAD 00010 expedite bit set
		 pSdoObject->pCanOpenTxBuffer->data[0] = SCS_UPLOAD_INITIATE<<5|0x02;
		if(objectdictionary_getDataType(addressIndex,subIndex) == DATA_STREAM)
		{
			//SCS 3 bits = SCS_UPLOAD_INITIATE
			//4th bit reserveed = 0
			//5th and 6th bit  = if n =3 then 8-3 =5 to 7 no data ( number of bytes that donot contain data such that 8-n to 7 is not data)
			//7th expedite bit shouldn't be set = 0
			//8th bit =s  should be set = 1
			pSdoObject->pCanOpenTxBuffer->data[0] = SCS_UPLOAD_INITIATE<<5|0x00<<2|0x01;
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


}

/**
 * Method to write the object dictionary
 *
 */
void writeDataQueue(SdoModule* pSdoObject)
{

 uint16_t addressIndex = pSdoObject->canRxData[2];
 addressIndex = addressIndex << 8 | pSdoObject->canRxData[1];
 uint8_t subIndex = pSdoObject->canRxData[3];

 //check for how many bytes are actually data
 uint8_t noData = pSdoObject->canRxData[0] & 0x0C; //8-nodata to 7  is not data implement support for that


 uint32_t value  = pSdoObject->canRxData[7]<<24|pSdoObject->canRxData[6]<<16|pSdoObject->canRxData[5]<<8|pSdoObject->canRxData[4];


  /* queueDataEntry.dataValue = value;
   queueDataEntry.index = addressIndex;
   queueDataEntry.subIndex = subIndex;
   objectdictionary_writeDataQueue(&queueDataEntry);*/
  writeOdDataQueue(addressIndex,subIndex,value);

	//send response to sdoClient
	pSdoObject->pCanOpenTxBuffer->data[0] =0x60;
	pSdoObject->pCanOpenTxBuffer->data[1] = pSdoObject->canRxData[1];
	pSdoObject->pCanOpenTxBuffer->data[2] = pSdoObject->canRxData[2];
	pSdoObject->pCanOpenTxBuffer->data[3] = pSdoObject->canRxData[3];
	//Everything else is zero because we don't care

	memset(&(pSdoObject->pCanOpenTxBuffer->data[4]),0,4);
/*for(uint8_t i=4;i<8;i++)
	{
		pSdoObject->pCanOpenTxBuffer->data[i]=0x00;
	}*/

	//fortest only
	//printf("WriteDataQueue pSdoObject %x",pSdoObject->canRxData[1]);

	//Make sure we clear new data flag
	pSdoObject->canRxNew = false;
	sendSdo(pSdoObject);

}

/**
 * receive a SDO CAN frame
 */
void receiveSdo(void *object, CanRxMsgTypeDef *pMessage){

	//Type cast SDo module should only receive the sdo object make sure in interuppt method
	SdoModule* pSdoObject = (SdoModule*)object;
	//if((message->DLC == 8U) && (!SDO->CANrxNew)) //TODO:check for message length and overflow
	 //TODO:Check for SDO state
	//Just thinking about expedited transfer now
	//copy message
	//printf("receiveSdo before Copy %x\r\n",pMessage->Data[1]);
	//check the state and store to the block buffer
	//if(pSdoObject->sdoCurrentState !=)
//ToDO Look at DLC
	memcpy((void*)&(pSdoObject->canRxData),(void*)&(pMessage->Data),8);
     /*for(int i=0; i<8;i++)
     {
    	 pSdoObject->canRxData[i] = pMessage->Data[i];
     }*/

     pSdoObject->canRxNew =true;
     // printf fortesting only
     //printf("receiveSdo pSdoObject %x\r\n",pSdoObject->canRxData[1]);
     //optional signal to other call back
       //pSdoObject->pFunctSignal();
    // processSdo(pSdoObject);
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

/**
 * process a sdpMessage -
 *  Download means write to object dictionary
 * Upload means read the object dictionary.
 */
void sdo_process(SdoModule* pSdoObject)
{
	//we will only do expedite transfer for now Specifier Byte
			//so a 8 byte message
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
	Only interested in two cases at the moment
	*/
	//if(pSdoObject->sdoCurrentState != SDO_ST_IDLE)

		//switch state
		//caseSDO_ST_DOWNLOAD_INITIATE
		///if((pSdoObject->canRxData[0] & 0x02U) != 0U)
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
				//printf("abort message received \r\n");

					//reset the sdo module object pointer
				pSdoObject->pDataToTransfer = NULL;
				pSdoObject->pDictioanryDataStream = NULL;
				pSdoObject->sequenceNumber = 0;

				//reset the queue we dont wait for data nymore
				xQueueReset(blkDataQHandle);

			}
			else
			{
			//Look for CCS only if state is idle
			if(pSdoObject->sdoCurrentState == SDO_ST_IDLE)
			{
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
					//this is where you start uploading a block. so change the state
					if(pSdoObject->canRxData[0] & 0x03 == 0x00)
					{
						//Initiate re transmits the whole data
						pSdoObject->sdoCurrentState = SDO_ST_UPLOAD_BL_INITIATE;
					}

					break;
				case CCS_DOWNLOAD_BLOCK:
					//Change the state here
					pSdoObject->sdoCurrentState = SDO_ST_DOWNLOAD_BL_INITIATE;
					//send response SCS_DOWNLOAD_BLOCK _  _ SErvr CRC support _ 0
					//101--SC-0
					uint8_t scs= SCS_DOWNLOAD_BLOCK<<5;
					pSdoObject->pCanOpenTxBuffer->data[0] =scs; //no CRC for now
					pSdoObject->pCanOpenTxBuffer->data[4] = sdoBlockSize; // this is block size and cannot be more than 127
					//store whole message in to buffer
					writeBlkDataQueue(pSdoObject); //store this one for addresses and data size
					sendSdo(pSdoObject);
					break;
				default:
					break;
				}
			}
			else if((pSdoObject->sdoCurrentState == SDO_ST_DOWNLOAD_BL_INITIATE) || (pSdoObject->sdoCurrentState == SDO_ST_DOWNLOAD_BL_SUBBLOCK))
			{
				//store whole message in to buffer
				writeBlkDataQueue(pSdoObject);
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
					if((pSdoObject->canRxData[0]&0x7F)==sdoBlockSize)//reached block size
					{
						pSdoObject->sdoCurrentState = SDO_ST_DOWNLOAD_BL_SUBBLOCK;
					}

					//no more data
					if ((pSdoObject->canRxData[0]>>7)==0x01) //no more data
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
				//Write object Dictionary
				//peek the queue and write the object dictionary
				uint8_t blkData[7] ={};
				if(xQueuePeek(blkDataQHandle,(void*)&blkData,( TickType_t ) 0) == pdPASS)
				{

				  writeOdDataQueue(blkData[1]<<8|blkData[0],blkData[2], blkData[6]<<24|blkData[5]<<16|blkData[4]<<8|blkData[3]);
				}



			}
			else if(pSdoObject->sdoCurrentState == SDO_ST_UPLOAD_BL_SUBBLOCK)
			{
				if (pSdoObject->canRxData[0] & 0x02 == 0x02)
				{
					pSdoObject->sdoCurrentState = SDO_ST_UPLOAD_BL_INITIATE_2; //indicate to continue the transfer
					//check for last sequence number if matches continue with new block else re transmit whole data
					if (pSdoObject->canRxData[1] != pSdoObject->sequenceNumber)
					{
						//start the transfer from the beginning
						pSdoObject->sdoCurrentState = SDO_ST_UPLOAD_BL_INITIATE;
					}

				}
			}
			else if(pSdoObject->sdoCurrentState == SDO_ST_UPLOAD_BL_END)
			{
				if (pSdoObject->canRxData[0] & 0x01 == 0x01)
				{
					//perform CRC canrxdata[1] +[2]
					//reset the  state if needed

					//set the state back to idle
					pSdoObject->sdoCurrentState = SDO_ST_IDLE;

					//reset the sdo module object pointer
					pSdoObject->pDataToTransfer = NULL;
					pSdoObject->pDictioanryDataStream = NULL;
					pSdoObject->sequenceNumber = 0;
				}

			}



		}//end else not abort

			//we got the data here so reset
			memset(&(pSdoObject->canRxData),0,8);
		pSdoObject->canRxNew = false;

	}//end if CanRx new true

		//Always check for upload, it is state driven we will only send if we are set to a particular state
		sdoUploadBlock(pSdoObject);

}

void writeBlkDataQueue(SdoModule* pSdoModule)
{
	uint8_t blkData[7] = {};
	//uint8_t blkDataTest[7]={};
	uint8_t* pData = &(pSdoModule->canRxData);
	pData++;
	memcpy((void*)&blkData,(void*)pData,7);
	//push all 8 byted to block queue
	//or(uint8_t i=0;i<8;i++)
	{
		//printf("blk data before %x %x %x %x\r\n",*blkData,*(blkData+1),*(blkData+2),*(blkData+3));
		if(xQueueSend(blkDataQHandle,(void*)&blkData,( TickType_t ) 0)!=pdPASS) //no block if queue full
		{
			//printf("error writing block data queue \r\n");
		}

	}
}


//This is the call back method called from object dictionary when indicated to get the data from SDO . only for block transfers.
void getDataStreamFromBlockQueue(DictionaryDataStream* pDataStream)
{
	uint8_t blkData[7] = {};
	int dataSize = 0;
	//receive the first message and know the size to read
	if(xQueueReceive(blkDataQHandle,(void*)&blkData,( TickType_t ) 0)!=pdPASS)
	{
		//printf("error receiving")
	}
	//first message contains the size of (address->0,1,2, Data--> 3,4,5,6)
	dataSize = blkData[6]<<24|blkData[5]<<16|blkData[4]<<8|blkData[3];

	//printf("Callback BlockDataSize %d \r\n",dataSize);
	//check the max size
	uint8_t* pData = (uint8_t*)pDataStream->pData;
	uint8_t* pDataStart = (uint8_t*)pDataStream->pData;
	//copy data from
	while(1 )
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
	    	break;
	     //all data copied no need to increment the pointer.
	    }
		}
		else
		{
			//printf("failed to receive from queue");
		}
	}

}



/**
 * Method to just write the object dictionary data queue so the change gets propogated to hardware as well as OD
 */
void writeOdDataQueue(uint16_t addressIndex,uint8_t subIndex, uint32_t value)
{
	queueDataEntry.dataValue = value;
	   queueDataEntry.index = addressIndex;
	   queueDataEntry.subIndex = subIndex;
	   objectdictionary_writeDataQueue(&queueDataEntry);

}


//Method to do a block upload
void sdoUploadBlock(SdoModule* pSdoModule )
{
	//Check the SDO state for initiate we have to start from beginning so reset everything.
	if(pSdoModule->sdoCurrentState == SDO_ST_UPLOAD_BL_INITIATE )
	{
		pSdoModule->pDictioanryDataStream = objectdictionary_getDataStreamPtr(pSdoModule->canRxData[2]<<8|pSdoModule->canRxData[1],pSdoModule->canRxData[3]);
	    pSdoModule->pDataToTransfer = pSdoModule->pDictioanryDataStream->pData;
	    pSdoModule->sequenceNumber = 0;
	    pSdoModule->sdoCurrentState = SDO_ST_UPLOAD_BL_INITIATE_2;
	}
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
		if (bytesToTransfer<=7)
		{
			pSdoModule->pCanOpenTxBuffer->data[0] |= 0x01<<7 ;
		    bytesToCopy = bytesToTransfer;
		    pSdoModule->sdoCurrentState = SDO_ST_UPLOAD_BL_END;
		}
		else
		{
			bytesToCopy = 7;
		}
		//increment the buffer pTxBuffer 7 bytes of data
		void* pTxBuffer = &(pSdoModule->pCanOpenTxBuffer->data[1]) ;
		memcpy(pTxBuffer, pSdoModule->pDataToTransfer,bytesToCopy);
		pSdoModule->pDataToTransfer += bytesToCopy;

		//sendSDO
		sendSdo(pSdoModule);

		//send the end sdo if we are at the end of transfer
		if(pSdoModule->sdoCurrentState == SDO_ST_UPLOAD_BL_END)
		{
			//     n =  number of bytes that is not data  8-n to 7
			//110 --- 01
			pSdoModule->pCanOpenTxBuffer->data[0] = SCS_UPLOAD_BLOCK <<5|0x01;

		}

	}//end if



}


///Local method to get the object dictionary entry ---TestOnly
/*
//We may neeed hash tables but a simple search will do for now.
ObjectDictionary* getObjectDictionaryEntry(uint16_t addressIndex, uint8_t subIndex)
{
	uint16_t objectDictionarySize = sizeof(objectDictionary)/sizeof(ObjectDictionary);

	//loop to get the current entry
	 for(uint8_t i=0;i<objectDictionarySize;i++)
	 {

		 if((objectDictionary[i].addressIndex == addressIndex)&&(objectDictionary[i].subIndex == subIndex))
		 {
			 return (ObjectDictionary*)(objectDictionary+i);
		 }
	 }

    //we are here return null
	 return NULL;
}*/

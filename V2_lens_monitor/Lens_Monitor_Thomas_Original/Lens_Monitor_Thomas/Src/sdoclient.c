
/*
 * sdoclient.c
 *
 *  Created on: 13/01/2017
 *      Author: gaurav.vyas
 */

#include "sdoclient.h"

// modify in initialization
uint16_t sdoClientRxBufferIndex = 0;
uint16_t sdoClientTxBufferIndex = 0;
static SdoServerNode sdoServerNode[NUMBER_OF_NODES];
//uint8_t maxCount  = 10 ;
//this is a temp object dictionary for testing the protocol

void sdoClient_init(uint8_t* pNodeIds,
		      SdoClientModule* pSdoClientObject,
		      CanOpenDriverModule* pCanOpenDriverRx,
			  uint16_t canOpenDriverRxIndex,
			  CanOpenDriverModule* pCanOpenDriverTx,
			  uint16_t canOpenDriverTxIndex,
			  uint8_t numberOfSdoServerNodes

		 )
{
	sdoClientRxBufferIndex = canOpenDriverRxIndex;
	sdoClientTxBufferIndex = canOpenDriverTxIndex;

	//nit the pSdoObject first
	//pSdoClientObject->nodeId = nodeId;
	//uint32_t cobIdClientToServer = CAN_OPEN_SDO_CLIENT_TO_SERVER+nodeId;
	//uint32_t cobIdServerToClient = CAN_OPEN_SDO_SERVER_TO_CLIENT+nodeId;
	//initialzing so keep the sdo state to be idle.
	pSdoClientObject->pFunctSignal = NULL;
	pSdoClientObject->numberOfSdoNodes=numberOfSdoServerNodes;
	pSdoClientObject->pCanOpenDriverRx = pCanOpenDriverRx;
	pSdoClientObject->canOpenRxSdoClientStartIndex =canOpenDriverRxIndex;
	pSdoClientObject->pCanOpenDriverTx = pCanOpenDriverTx;
	pSdoClientObject->canOpenTxSdoClientStartIndex = canOpenDriverTxIndex;
	pSdoClientObject->pSdoServerNode = &sdoServerNode;
				//rx buffer init
				for(uint8_t index =0;index<pSdoClientObject->numberOfSdoNodes;index++)
					configSdoNode(pSdoClientObject,index,pNodeIds); //

	//rx buffer init
	//canbus_initRxBuffer(cobIdClientToServer,pCanOpenDriverRx, canOpenDriverRxIndex, pSdoObject, receiveSdo);

	//tx buffer init
	//pSdoObject->pCanOpenDriverTxObject = pCanOpenDriverTx;
	//pSdoObject->pCanOpenTxBuffer =canbus_initTxBuffer(cobIdServerToClient,pCanOpenDriverTx,canOpenDriverTxIndex,8);

	//make sure we set the
	//pSdoObject->pCanOpenDriverTxObject->pCanOpenTxBuffer = pSdoObject->pCanOpenTxBuffer

}



/**
 * Config the node, assuming that we start at node 0 and and everything else is fixed.
 */
void configSdoNode(SdoClientModule* pSdoClientObject,uint8_t index,uint8_t* pNodeIds)
{
	//donot assume that index is a node id , node id may be different number then index.
	pSdoClientObject->pSdoServerNode[index].nodeId = pNodeIds[index];
	uint32_t cobIdClientToServer = CAN_OPEN_SDO_CLIENT_TO_SERVER+pSdoClientObject->pSdoServerNode[index].nodeId;
	uint32_t cobIdServerToClient = CAN_OPEN_SDO_SERVER_TO_CLIENT+pSdoClientObject->pSdoServerNode[index].nodeId;
	canbus_initRxBuffer(cobIdServerToClient,pSdoClientObject->pCanOpenDriverRx, pSdoClientObject->canOpenRxSdoClientStartIndex+index, pSdoClientObject->pSdoServerNode[index], receiveSdoClient);

/*       	_____________________8 Byte DATA ____
* 			|	                                 |
* COBID         0             1          234567
* |____|     |____|         |_____|     |_______|
* 0x00       Command         nodeID    Don't Care
*/
	//tx buffer init only 2 bytes of data
	pSdoClientObject->pSdoServerNode[index].pCanOpenTxBuffer =canbus_initTxBuffer(cobIdClientToServer,pSdoClientObject->pCanOpenDriverTx,pSdoClientObject->canOpenTxSdoClientStartIndex+index,2);//number of data bytes should be 2 for Master


}

/**
 * receive a SDO CAN frame
 */
void receiveSdoClient(void *object, CanRxMsgTypeDef *pMessage){

	//Type cast SDo module should only receive the sdo object make sure in interuppt method
	SdoServerNode* pSdoServerNode = (SdoServerNode*)object;
	//if((pmessage->DLC == 8U) && (!pSdoServerNode->CANrxNew)) //TODO:check for message length and overflow


	//copy message
     for(int i=0; i<8;i++)
     {
    	 pSdoServerNode->canRxData[i] = pMessage->Data[i];
     }

     pSdoServerNode->canRxNew =TRUE;

}

/**
 * send a SDO CAN frame
 */
void sendSdoClient(SdoModule *pSdoModule)
{

	canbus_transmit(pSdoModule->pCanOpenDriverTxObject,sdoClientTxBufferIndex);

}

/**
 * process a sdpMessage -
 *  Download means write to object dictionary
 * Upload means read the object dictionary.
 */
void sdoClient_process(SdoClientModule* pSdoClientModule)
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
		 *           SCS       Reserved      No of Bytes              Expedite    Where data size
		                                  that dont contain data      Transfer    is Set.
			 */
	//TODO: Check for SCS and if there is something new to process .For timebeing just process with check for IDLe
	/*For now we are only concerned about expedited transfers so listing 1st byte request and response here
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
	//Todo check for master state if(nmtMasterState==preoperational)
		for(uint8_t i=0;i<pSdoClientModule->numberOfSdoNodes;i++)
		{
			if(pSdoClientModule->pSdoServerNode[i].canRxNew == TRUE )
			{

				//TODO - inform the application? or below code
				if(pSdoClientModule->pSdoServerNode[i].canRxData[0]==0x60) //object dictionary written  response
				{

				}
				else if(pSdoClientModule->pSdoServerNode[i].canRxData[0]==0x42) // 4 byte value retrieved from object dictionary
				{
					//send the value to application


				}

				pSdoClientModule->pSdoServerNode[i].canRxNew = FALSE;
			}

			/*if(pSdoClientModule->pSdoServerNode[i].canTxNew == true)
			{

			}*/


		}


}



/**
 * method to send the canopen send message.
 *
 * 			_____________________8 Byte DATA ____
 * 			|	                                 |
 * COBID      0       12        3          4567
 * |____|    |__|   |_____|    |__|       |_____|
 * 0x600      SCS    Index   sub-index     Data value
 * +nodeid
 * SCS stands for server command specifier for now just expedite
 */
void sdoClient_sendCommand(SdoClientModule *pSdoClientModule,uint8_t nodeId,uint8_t serverCommandSpecifier,uint16_t addressIndex,uint8_t subIndex,uint32_t dataValue)
{
	//Detect the nod update the buffer and send
	// never assume that nodeId and index are same.. allow user of this stack to define that.
	for(uint8_t nodeIndex =0; nodeIndex<pSdoClientModule->numberOfSdoNodes;nodeIndex++)
	{
		if(pSdoClientModule->pSdoServerNode[nodeIndex].nodeId == nodeId)
		{
			//only and always 8 bytes of data for expedite transfers
			pSdoClientModule->pSdoServerNode[nodeIndex].pCanOpenTxBuffer->data[0] = serverCommandSpecifier;
			pSdoClientModule->pSdoServerNode[nodeIndex].pCanOpenTxBuffer->data[1]= addressIndex & 0xFF;
			pSdoClientModule->pSdoServerNode[nodeIndex].pCanOpenTxBuffer->data[2] = (addressIndex >>8) & 0xFF;
			pSdoClientModule->pSdoServerNode[nodeIndex].pCanOpenTxBuffer->data[3] = subIndex;
			//32 byte value
			pSdoClientModule->pSdoServerNode[nodeIndex].pCanOpenTxBuffer->data[7] = (dataValue >> 24) & 0xFF;
			pSdoClientModule->pSdoServerNode[nodeIndex].pCanOpenTxBuffer->data[6] = (dataValue >> 16) & 0xFF;
			pSdoClientModule->pSdoServerNode[nodeIndex].pCanOpenTxBuffer->data[5] = (dataValue >> 8) & 0xFF;
			pSdoClientModule->pSdoServerNode[nodeIndex].pCanOpenTxBuffer->data[4] = dataValue & 0xFF;


			canbus_transmit(pSdoClientModule->pCanOpenDriverTx,pSdoClientModule->canOpenTxSdoClientStartIndex+nodeIndex);
			break;
		}
	}

}




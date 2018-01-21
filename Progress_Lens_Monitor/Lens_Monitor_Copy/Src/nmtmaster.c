/*
 * nmtmaster.c
 *
 *  Created on: 11/01/2017
 *      Author: gaurav.vyas
 */
#include "nmtmaster.h"
uint16_t nmtMasterRxBufferIndex = 0;
uint16_t nmtMasterTxBufferIndex = 0;
uint8_t maxProcessCount  = 10 ; //this is the max process count that can execute without a heartbeat before deciding that there is error on the node.
static NmtNode nmtNode[NUMBER_OF_NODES];

/**
 * Initialize nmtheartbeat object
 */
void nmtmaster_init(uint8_t* pNodeIds,
		NmtMasterModule* pNmtMaster,
		      CanOpenDriverModule* pCanOpenDriverRx,
			  uint16_t canOpenDriverRxIndex,
			  CanOpenDriverModule* pCanOpenDriverTx,
			  uint16_t canOpenDriverTxIndex,
			  uint8_t numberOfMonitoredNodes

		 )
{
	//assign transmit index
		nmtMasterRxBufferIndex = canOpenDriverRxIndex;
		nmtMasterTxBufferIndex = canOpenDriverTxIndex;

		//
		//pNmtMaster->nodeId = nodeId;
		   //never add node ID for NMT messages , its always 0x00, data[1] is node id

			pNmtMaster->pFunctSignal = NULL;
			pNmtMaster->numberOfMonitoredNodes=numberOfMonitoredNodes;
            pNmtMaster->pCanOpenDriverRx = pCanOpenDriverRx;
            pNmtMaster->canOpenRxNmtStartIndex =canOpenDriverRxIndex;
            pNmtMaster->pCanOpenDriverTx = pCanOpenDriverTx;
            pNmtMaster->canOpenTxNmtStartIndex = canOpenDriverTxIndex;
            pNmtMaster->pMonitoredNodes = &nmtNode;
			//rx buffer init
			for(uint8_t nodeIndex =0;nodeIndex<pNmtMaster->numberOfMonitoredNodes;nodeIndex++)
			{
				configMonitoredNode(pNmtMaster,nodeIndex,maxProcessCount,pNodeIds); // keep 1000 milliseconds for now.
			}



}

/**
 * Config the node, assuming that we start at node 0 and and everything else is fixed.
 */
void configMonitoredNode(NmtMasterModule* pNmtMaster,uint8_t nodeIndex,uint8_t maxProcessCount,uint8_t* pNodeIds)
{
	//donot assume that index is a node id , node id may be different number then index.
	pNmtMaster->pMonitoredNodes[nodeIndex].nodeId = pNodeIds[nodeIndex];

	uint32_t cobIdMasterToSlave = CAN_OPEN_NMT_MASTER_TO_SLAVE;
	uint32_t cobIdSlaveToMaster = CAN_OPEN_NMT_HEARTBEAT_SLAVE_TO_MASTER+pNmtMaster->pMonitoredNodes[nodeIndex].nodeId;
	canbus_initRxBuffer(cobIdSlaveToMaster,pNmtMaster->pCanOpenDriverRx, pNmtMaster->canOpenRxNmtStartIndex+nodeIndex, pNmtMaster->pMonitoredNodes[nodeIndex], receiveNmtMaster);

/*       	_____________________8 Byte DATA ____
* 			|	                                 |
* COBID         0             1          234567
* |____|     |____|         |_____|     |_______|
* 0x00       Command         nodeID    Don't Care
*/
	//tx buffer init only 2 bytes of data
   pNmtMaster->pMonitoredNodes[nodeIndex].pCanOpenTxBuffer =canbus_initTxBuffer(cobIdMasterToSlave,pNmtMaster->pCanOpenDriverTx,pNmtMaster->canOpenTxNmtStartIndex+nodeIndex,2);//number of data bytes should be 2 for Master

   pNmtMaster->pMonitoredNodes[nodeIndex].maxProcessCount = maxProcessCount;

}


/**
 * receive a heartbeat from the producer
 */
void receiveNmtMaster(void *object, CanRxMsgTypeDef *pMessage)
{
	/*              	  _________8 Byte DATA ____
	* received frm slave  |	                      |
	* COBID                 0             1234567
	* |____|              |____|         |________|
	* 0x700+nodeId         state          Don't Care
	*/
	NmtNode* pMonitoredNmtNode = (NmtNode*)object;
	    //get the node id this hearbeat is from.
		uint8_t nodeId = pMessage->StdId & 0xFF;

		//set the operational state based on command
		if ((pMessage->DLC == 1)&& (nodeId == pMonitoredNmtNode->nodeId))
		{
			///Store the state and set the Canrx new
			pMonitoredNmtNode->nmtState = pMessage->Data[0];
			pMonitoredNmtNode->canRxNew = TRUE;

		}


}


/**
 * process a nmtMessage
 */
void nmtmaster_process(NmtMasterModule *pNmtMasterModule,uint8_t nmtMasterState)
{
	//Todo check for master state if(nmtMasterState==preoperational)
	for(uint8_t i=0;i<pNmtMasterModule->numberOfMonitoredNodes;i++)
	{
		if(pNmtMasterModule->pMonitoredNodes[i].canRxNew == TRUE )
		{
			pNmtMasterModule->pMonitoredNodes[i].monStarted = TRUE;
			pNmtMasterModule->pMonitoredNodes[i].processCount =0; //reset the process counter
			pNmtMasterModule->pMonitoredNodes[i].canRxNew = FALSE;
		}

		//Check for timeouttimer to detect error.
		if(pNmtMasterModule->pMonitoredNodes[i].processCount >= pNmtMasterModule->pMonitoredNodes[i].maxProcessCount)
		{
			//raise error about no heartbeat
			pNmtMasterModule->pMonitoredNodes[i].nmtState = NMT_INITIALIZING; //bootup may be
		}
		else
		{
			//Just increment the process count
			pNmtMasterModule->pMonitoredNodes[i].processCount++;

		}
	}


}

/**
 * method to send the canopen send message.
 *
 * 			_____________________8 Byte DATA ____
 * 			|	                                 |
 * COBID         0             1          234567
 * |____|     |____|         |_____|     |_______|
 * 0x00       Command         nodeID    Don't Care
 *
 * Command shouldbe as defined in NmtCommand in canopendefines  values 1,2,128,129,130 only
 */
void nmtmaster_sendCommand(NmtMasterModule *pNmtMasterModule,uint8_t command,uint8_t nodeId )
{
	//Detect the nod update the buffer and send
	// never assume that nodeId and index are same.. allow user of this stack to define that.
	for(uint8_t nodeIndex =0; nodeIndex<pNmtMasterModule->numberOfMonitoredNodes;nodeIndex++)
	{
		if(pNmtMasterModule->pMonitoredNodes[nodeIndex].nodeId == nodeId)
		{
			pNmtMasterModule->pMonitoredNodes[nodeIndex].pCanOpenTxBuffer->data[0] = command;
			pNmtMasterModule->pMonitoredNodes[nodeIndex].pCanOpenTxBuffer->data[1] = nodeId;
			canbus_transmit(pNmtMasterModule->pCanOpenDriverTx,pNmtMasterModule->canOpenTxNmtStartIndex+nodeIndex);
			break;
		}
	}

}



/**
 * Method to get the latest node state available with master
 * Only intended for Master use or Application usage
 */

NmtState getNmtNodeStateFromMaster(NmtMasterModule *pNmtMasterModule,uint8_t nodeId)
{
	NmtState nodeState = NMT_INITIALIZING;
	for(uint8_t nodeIndex =0; nodeIndex<pNmtMasterModule->numberOfMonitoredNodes;nodeIndex++)
	{
		if(pNmtMasterModule->pMonitoredNodes[nodeIndex].nodeId == nodeId)
		{
			nodeState = pNmtMasterModule->pMonitoredNodes[nodeIndex].nmtState;
			break;
		}
	}

	return nodeState;
}

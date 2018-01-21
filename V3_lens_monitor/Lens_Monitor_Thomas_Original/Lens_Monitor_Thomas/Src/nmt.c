/*
 * nmt.c
 *
 *  Created on: 28/12/2016
 *      Author: gaurav.vyas
 */
#include "nmt.h"
uint16_t nmtRxBufferIndex = 0;
uint16_t nmtTxBufferIndex = 0;

/**
 * receive a NMT CAN frame
 *   COBID  Data Bytes
 * 	   		     0         1      2,3,4,5,6,7
 *  |_____|  |_______| |______|  |___________|
 *   0x00    NMT cmd    Node ID   Dont care
 *
 *   When node id is 0 all the nodes on network should switch the message
 */
void receiveNmt(void *object, CanRxMsgTypeDef *pMessage)
{

	NmtHeartbeatModule* pNmtHbModule = (NmtHeartbeatModule*)object;
	uint8_t nodeId = pMessage->Data[1];

	//set the operational state based on command at some point check for DLC at the moment  DLC check doesn't work out well with house keeper
	//if ((pMessage->DLC == 2)&& ((nodeId == 0) || (nodeId == pNmtHbModule->nodeId)))
	if((nodeId == 0) || (nodeId == pNmtHbModule->nodeId))
	{
		uint8_t command = pMessage->Data[0];
		switch(command)
		{
		case NMT_ENTER_OPERATIONAL:
			pNmtHbModule->operatingState = NMT_OPERATIONAL;
			break;
		case    NMT_ENTER_STOPPED:
			pNmtHbModule->operatingState = NMT_STOPPED;
			break;
		case    NMT_ENTER_PRE_OPERATIONAL:
			pNmtHbModule->operatingState = NMT_PRE_OPERATIONAL;
			break;
		case    NMT_RESET_NODE:
			pNmtHbModule->resetCommand = RESET_APP;
			break;

		case    NMT_RESET_COMMUNICATION:
			pNmtHbModule->resetCommand = RESET_COMM;
		    break;
		}

	}

}


/**
 * initialize nmt module object
 *
 */
void nmt_init(uint8_t nodeId,
		NmtHeartbeatModule* pNmtHbObject,
		      CanOpenDriverModule* pCanOpenDriverRx,
			  uint16_t canOpenDriverRxIndex,
			  CanOpenDriverModule* pCanOpenDriverTx,
			  uint16_t canOpenDriverTxIndex

		 )
{
	//assign transmit index
	nmtRxBufferIndex = canOpenDriverRxIndex;
	nmtTxBufferIndex = canOpenDriverTxIndex;

	//init the pSdoObject first
	pNmtHbObject->nodeId = nodeId;
	//never add node ID for NMT messages , its always 0x00, data[1] is node id
	uint32_t cobIdMasterToSlave = CAN_OPEN_NMT_MASTER_TO_SLAVE;
	uint32_t cobIdSlaveToMaster = CAN_OPEN_NMT_HEARTBEAT_SLAVE_TO_MASTER+ nodeId;

	//initialzing so keep the nmt state to be idle.
	pNmtHbObject->operatingState = NMT_INITIALIZING;
	pNmtHbObject->resetCommand = RESET_NOT;
	pNmtHbObject->newState = false;
	pNmtHbObject->pFunctSignal = NULL;

	//rx buffer init
	canbus_initRxBuffer(cobIdMasterToSlave, pCanOpenDriverRx,
			canOpenDriverRxIndex, pNmtHbObject, receiveNmt);

	//tx buffer init
	pNmtHbObject->pCanOpenDriverTxObject = pCanOpenDriverTx;
	pNmtHbObject->pCanOpenTxBuffer = canbus_initTxBuffer(cobIdSlaveToMaster,
			pCanOpenDriverTx, canOpenDriverTxIndex, 1); //number of data bytes should be 1

}


/*Process NMT object for  heartbeat generation or any other required operation for NMT
 * we got to somehow do it periodically may be put this on a thread in CanOpen
 * */
NmtResetCode nmt_process(NmtHeartbeatModule* pNmtHbObject)
{

	/*//TODO: Make sure we send it periodically and not very frequently*/

	//Make sure we return this, this will be used by higher layer to reset all comm or even application.
	return pNmtHbObject->resetCommand;

}


/**
 * send a SDO CAN frame
 */
void sendNmtHeartbeat(NmtHeartbeatModule* pNmtHbObject)
{
	//todo:sync here this may be used by heart beat generator timer  and/or main threads for seding heart beat.
	canbus_transmit(pNmtHbObject->pCanOpenDriverTxObject,nmtTxBufferIndex);
	//sync here
}



/*
 * Method to generate heartbeat ans send to can driver
 *             	  		_________8 Byte DATA ____
 * received frm slave  |	                      |
 * COBID                 0             1234567
 * |____|              |____|         |________|
 * 0x700+nodeId         state          Don't Care
 */
void nmt_generateHeartbeat(NmtHeartbeatModule* pNmtHbObject)
{
	//Kepping heartbeat functionality separate to put it on software timer.
	//check the state , only if not stopped we send heart beat
	//if(pNmtHbObject->operatingState != NMT_STOPPED)
	{
		pNmtHbObject->pCanOpenTxBuffer->data[0] = pNmtHbObject->operatingState;
		sendNmtHeartbeat(pNmtHbObject);
	}

}

/*
 * Just update the state we will send this using heartbeat
 */
void nmt_updateOperatingState(NmtHeartbeatModule* pNmtHbObject,NmtState nmtState)
{
	//TODO: Start synchronization
	pNmtHbObject->operatingState = nmtState;
	//TODO: Synchronization
}



/*
 * Just update the reset command  we will need this while resetting the app or communication
 */
void nmt_updateResetCommand(NmtHeartbeatModule* pNmtHbObject,NmtResetCode nmtResetCode)
{
	pNmtHbObject->resetCommand = nmtResetCode;
}

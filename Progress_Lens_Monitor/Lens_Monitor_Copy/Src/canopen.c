/*
 * CanOpen.c
 *
 *  Created on: 15/12/2016
 *      Author: gaurav.vyas
 */

#include "canopen.h"
#include "stddef.h"


//Global defines
//#define NUMBER_OF_NODES 1 //total number of nodes excluding master

//define the indexes for messages in Canopen buffer
#define START_INDEX  0
//Always add index above this line and make it a startindex
#define NMT_MASTER_INDEX START_INDEX
#define SDO_SERVER_INDEX NMT_MASTER_INDEX + NUMBER_OF_NODES
#define SDO_CLIENT_INDEX (SDO_SERVER_INDEX + 1)
#define NMT_SLAVE_INDEX (SDO_CLIENT_INDEX + 1)
//Do not modify high index
#define HIGH_INDEX NMT_SLAVE_INDEX
//total number of received CAN messages
#define CAN_OPEN_NO_RX_MSGS (1+HIGH_INDEX)
//total tx msgs
#define CAN_OPEN_NO_TX_MSGS (1+HIGH_INDEX)

static CanOpenModule canOpenObject;
CanOpenModule *pCanOpen=NULL;
static CanOpenRx *pCanOpenRxArray;
static CanOpenTx *pCanOpenTxArray;
static CanOpenDriverModule canOpenDriverObject;
static SdoModule sdoObject;
static NmtMasterModule nmtMasterObject;
static NmtHeartbeatModule nmtHbObject;
static SdoClientModule sdoClientObject;

static CanOpenRx canOpenRxArray[CAN_OPEN_NO_RX_MSGS];
static CanOpenTx canOpenTxArray[CAN_OPEN_NO_TX_MSGS];
//TODO: need better way to assign node ids. this is just for testing .nodeIds array
static uint8_t nodeIds[NUMBER_OF_NODES];

//test variables*******************************************************************
/*
uint8_t masterCommand = 0x80;// go to  preoperational //This variable is for testing only
uint8_t  dataValue = 0;//this variable is for testing only
uint8_t scs = 0x22;//this variable is for testing only*/
//end test variables*****************************************************************
/**
 * Method to intitialize the CAN HAL libraries and appropriate underlying registers
 */
void canopen_initCan(uint8_t nodeId)
{
	//set default if out of range
	if(nodeId<1 || nodeId>127) nodeId = 0x10;

	pCanOpen =&canOpenObject;
	pCanOpen->pCanDriverObject = &canOpenDriverObject;
	pCanOpenRxArray = canOpenRxArray;
	pCanOpenTxArray  = canOpenTxArray;
	pCanOpen->pSdoObject = &sdoObject;
	pCanOpen->pNmtMasterObject = &nmtMasterObject;
	pCanOpen->pSdoClientObject = &sdoClientObject;
	pCanOpen->pNmtHbObject = &nmtHbObject;
	pCanOpen->nodeID = nodeId;

	//to initialize dictionary and hardware.
	objectdictionary_init();
	//CanbusInit
	canbus_initialize(pCanOpen->pCanDriverObject,canOpenRxArray,CAN_OPEN_NO_RX_MSGS,canOpenTxArray,CAN_OPEN_NO_TX_MSGS,nodeId);
	//sdoinit - this is just the server
	sdo_init(nodeId,
			pCanOpen->pSdoObject,
			      pCanOpen->pCanDriverObject,
				  SDO_SERVER_INDEX,
				  pCanOpen->pCanDriverObject,
				  SDO_SERVER_INDEX);

	//TODO: implement PDO .
	nmt_init(nodeId,pCanOpen->pNmtHbObject,
		      pCanOpen->pCanDriverObject,
			  NMT_SLAVE_INDEX,
			  pCanOpen->pCanDriverObject,
			  NMT_SLAVE_INDEX);


	//After init completed successfully change the state to nmtstate preoperational
	//all initialization above this line
			canbus_enableInterrupts();
//we are done initializing so set thestate to preoperational
			canopen_updateOperatingState(NMT_PRE_OPERATIONAL);

}

//This mehtod is to be used with master node.
void canopenMaster_initCan()
{
	initNodeIds();
	pCanOpen =&canOpenObject;
		pCanOpen->pCanDriverObject = &canOpenDriverObject;
		pCanOpenRxArray = canOpenRxArray;
		pCanOpenTxArray  = canOpenTxArray;
		pCanOpen->pSdoObject = &sdoObject;
		pCanOpen->pNmtMasterObject = &nmtMasterObject;
		pCanOpen->pSdoClientObject = &sdoClientObject;
		pCanOpen->pNmtHbObject = &nmtHbObject;
		pCanOpen->nodeID = 0;// nodeId; //Doesn't apply for master TODO: think of something else.

		//CanbusInit
		canbus_initialize(pCanOpen->pCanDriverObject,canOpenRxArray,CAN_OPEN_NO_RX_MSGS,canOpenTxArray,CAN_OPEN_NO_TX_MSGS);
	//After init completed successfully change the state to nmtstate preoperational

		nmtmaster_init(nodeIds,pCanOpen->pNmtMasterObject,
				      pCanOpen->pCanDriverObject,
					  NMT_MASTER_INDEX,
					  pCanOpen->pCanDriverObject,
					  NMT_MASTER_INDEX,
					  NUMBER_OF_NODES); // using number of nodes as number of monitored nodes.

		sdoClient_init(nodeIds,pCanOpen->pSdoClientObject,
			      pCanOpen->pCanDriverObject,
				  SDO_CLIENT_INDEX,
				  pCanOpen->pCanDriverObject,
				  SDO_CLIENT_INDEX,
				  NUMBER_OF_NODES);




}

/**
 * Method to assign all node ids .
 */
void initNodeIds()
{
	//for(int i=0;i<NUMBER_OF_NODES;i++)
	{
		///just one node at the moment
		nodeIds[0] = 3; //TODO: correct for proper master
	}
}

/**
 * processCanOpen - method to do the appropriate processing of the can messages
 */
NmtResetCode canopen_process()
{

	//should we check for state before processing.
	NmtResetCode resetCode = RESET_NOT;

	//we are here meaning we are opearatig so set thestate to operational
	canopen_updateOperatingState(NMT_OPERATIONAL);
	//check for new messages
	canopen_rxCan();

	//May be process SDO here.
	sdo_process(pCanOpen->pSdoObject);
	//NMT process
	resetCode = nmt_process(pCanOpen->pNmtHbObject);

	//pdo Process

//	if(hcan1.State==HAL_CAN_STATE_ERROR)
//	{
////    				MX_CAN1_Init();
//		hcan1.State=HAL_CAN_STATE_READY;
//		HAL_CAN_Receive_IT(&hcan1,CAN_FIFO0);
//	}

	return resetCode;
}



/**
 * processCanOpen - method to do the appropriate processing of the can messages
 */
NmtResetCode canopenMaster_process()
{

	//should we check for state before processing.
	NmtResetCode resetCode = RESET_NOT;
	//check for new messages
	canopen_rxCan();

	//May be process SDO here.
	sdoClient_process(pCanOpen->pSdoClientObject);
	//NMT process
	 nmtmaster_process(pCanOpen->pNmtMasterObject);

	 //Test code********************************************************************
	/*uint16_t addressIndex = 0x00;
	uint8_t subIndex = 0x00;
	if (dataValue % 2 == 0) {
		sdoClient_sendCommand(pCanOpen->pSdoClientObject, 3, scs, addressIndex,
				subIndex, dataValue);
		if(scs==0x22)
				scs=0x40;
			else
				scs=0x22;
	} else {
		nmtmaster_sendCommand(pCanOpen->pNmtMasterObject, masterCommand, 3);

		if (masterCommand == 0x80) {
			masterCommand = 0x01;
		} else if (masterCommand == 0x01) {
			masterCommand = 0x02;
		} else if (masterCommand == 0x02) {
			masterCommand = 0x80;
		}
	}
	dataValue++;

	if(dataValue > 127)
		dataValue=0;*/
	//End Test Code******************************************************************
	 //pdo Process

	return resetCode;
}
/**
 *We receive the message here to check periodically. To make it interuppt driven canbus receive may be called on receive interupt ?
 */
void canopen_rxCan()
{
	//Get the message
	//use when polling
	//canbus_receive(pCanOpen->pCanDriverObject);

	//use when using can receive interuppts
	canbus_receive_It(pCanOpen->pCanDriverObject);
}

void canopen_generateHeartbeat()
{
	nmt_generateHeartbeat(pCanOpen->pNmtHbObject);
}


void canopen_updateOperatingState(NmtState state)
{
	nmt_updateOperatingState(pCanOpen->pNmtHbObject,state);
}

/*
 * Method to enable communication and receive interrupts inturn
 */
void canopen_enableComm(void)
{
	//Ideally may be  we should also reset all data queues but first we have to find out the leak.


	canopen_updateOperatingState(NMT_PRE_OPERATIONAL);
	nmt_updateResetCommand(pCanOpen->pNmtHbObject, RESET_NOT);
	canbus_enableInterrupts();
}

/**
 * not used for now.
 *
 */
void canopen_txCan()
{

}

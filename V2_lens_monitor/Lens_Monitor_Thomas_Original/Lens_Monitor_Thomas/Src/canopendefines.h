/*
 * canopendefines.h
 *
 *  Created on: 19/12/2016
 *      Author: gaurav.vyas
 */

#ifndef APPLICATION_USER_CANOPENDEFINES_H_
#define APPLICATION_USER_CANOPENDEFINES_H_
#include "stddef.h"
#include "global.h"
//#include "can.h"
#include "objectdictionary.h"

#include "stm32f7xx_hal.h"
#define NUMBER_OF_NODES 1

CAN_HandleTypeDef hcan1;
CAN_HandleTypeDef hcan2;
/* Client command specifier */

typedef enum {
	CCS_SEGMENT_DOWNLOAD = 0,
	CCS_DOWNLOAD_INITIATE = 1,
	CCS_UPLOAD_INITIATE = 2,
	CCS_UPLOAD_SEGMENT = 3,
	CCS_ABORT = 4,
	CCS_UPLOAD_BLOCK = 5,
	CCS_DOWNLOAD_BLOCK = 6

} ClientCommandSpecifier, CCS;

/* Server Command Specifier */
typedef enum {
	SCS_UPLOAD_INITIATE = 2,
	SCS_UPLOAD_SEGMENT = 0,
	SCS_DOWNLOAD_INITIATED = 3,
	SCS_DOWNLOAD_SEGMENT = 1,
	SCS_ABORT = 4,
	SCS_DOWNLOAD_BLOCK = 5,
	SCS_UPLOAD_BLOCK = 6
} ServerCommandSpecifier, SCS;




typedef enum
{
	CAN_OPEN_NMT = 0x000,
	CAN_OPEN_SDO_SERVER_TO_CLIENT = 0x580, /*SDO response from server (+nodeId)*/
	CAN_OPEN_SDO_CLIENT_TO_SERVER = 0x600, /*SDO request from client (+nodeId)*/
	CAN_OPEN_NMT_MASTER_TO_SLAVE = 0x00,/*Master to slave  always 0 - never add node ID */
	CAN_OPEN_NMT_HEARTBEAT_SLAVE_TO_MASTER = 0x700 /* HEARTBEAT slave to master (+nodeId) */

}CanOpenIdentifier;



typedef struct
{
	uint32_t stdId;
	void* protocolObject; /*this is protocol object like sdoModule, PDO,emergency, NMT, heartbeat*/
	void* (*pFunct)(void* object,CanRxMsgTypeDef* rxMessage);

}CanOpenRx;

typedef struct
{
	uint32_t stdId;
	uint8_t DLC;
	uint8_t data[8];

}CanOpenTx;


typedef struct{
	CAN_HandleTypeDef* pCanHandle;
	CanOpenRx* pCanOpenRxBuffer;
	uint16_t rxSize;
	CanOpenTx* pCanOpenTxBuffer;
	uint16_t txSize;

}CanOpenDriverModule;
/*this  canOpenModule will be static throughout the layer */


/*
typedef struct{
	uint16_t addressIndex;
	uint8_t subIndex;
	uint32_t value;

}ObjectDictionary;*/

//All sdo defined go here
//const uint8_t sdoBufferSize=32; //lets keep it 32 bytes for now
typedef enum {
	FALSE=0,
	TRUE

} BOOL;
/**
 * Internal states of the SDO server state machine
 *  Download means write to object dictionary
 *  Upload means read from object dictionary
 */
typedef enum {
	SDO_ST_IDLE = 0x00U,
	SDO_ST_DOWNLOAD_INITIATE = 0x11U,
	SDO_ST_DOWNLOAD_SEGMENTED = 0x12U,
	SDO_ST_DOWNLOAD_BL_INITIATE = 0x14U,
	SDO_ST_DOWNLOAD_BL_SUBBLOCK = 0x15U,
	SDO_ST_DOWNLOAD_BL_SUB_RESP = 0x16U,
	SDO_ST_DOWNLOAD_BL_END = 0x17U,
	SDO_ST_UPLOAD_INITIATE = 0x21U,
	SDO_ST_UPLOAD_SEGMENTED = 0x22U,
	SDO_ST_UPLOAD_BL_INITIATE = 0x24U,
	SDO_ST_UPLOAD_BL_INITIATE_2 = 0x25U,
	SDO_ST_UPLOAD_BL_SUBBLOCK = 0x26U,
	SDO_ST_UPLOAD_BL_END = 0x27U
} SdoState;


/**
 * SDO struct
 */

typedef struct{
	uint8_t canRxData[8]; //Buffer to receive the RX data and handling the data
	//uint8_t dataBuffer[sdoBufferSize];     //this could be used in future commented for now
	uint8_t nodeId; //Nodeid of controller on the can netwrok only Max 127
	SdoState sdoCurrentState; //Intended for future use when we start using segmented and block sdo transfer
	uint16_t crc; // intended for future use when we start using CRC
	BOOL canRxNew; //This must be set to true when RxData is updated.
	CanOpenDriverModule *pCanOpenDriverTxObject; //this will be the SDO response object for each request.
	CanOpenTx *pCanOpenTxBuffer;//this will be transmission buffer from CanOpenDriverModule here for easy data handling and keep it aligned with other modules as well in future.
	DictionaryDataStream* pDictioanryDataStream; // used in block upload
	void* pDataToTransfer; // this is data pointer that would be used to track the num of bytes reamining to be transferred in sdo upload
	uint8_t sequenceNumber; //this is used for nblock transfers to keep count of of an upload or download.
	uint8_t blockSize; //this is the agreed bloack size between two blocks;
	void (*pFunctSignal)(void); //Just for signalling Rtos or anyother call back . optional for now.

}SdoModule;

//This is a NMT consumer and heartbeat producer object
typedef struct{
	uint8_t nodeId; //Nodeid of controller on the can netwrok only Max 127
	uint8_t resetCommand; //used at Canopen level for reseting the node
	uint8_t operatingState; //used for maintaining the state .
	uint16_t crc; // intended for future use when we start using CRC
	BOOL newState; //This must be set to true when operating state is updated.
	CanOpenDriverModule *pCanOpenDriverTxObject; //this will be the SDO response object for each request.
	CanOpenTx *pCanOpenTxBuffer;//this will be transmission buffer from CanOpenDriverModule here for easy data handling and keep it aligned with other modules as well in future.
	void (*pFunctSignal)(void); //Just for signalling Rtos or any other call back . optional for now.

}NmtHeartbeatModule;

/**
 * Internal network state of the CANopen node
 */
typedef enum{
    NMT_INITIALIZING             = 0,    ///initializing
    NMT_PRE_OPERATIONAL          = 127,  // pre-operational state */
    NMT_OPERATIONAL              = 5,    // operational state */
    NMT_STOPPED                  = 4     //stopped */
}NmtState; //this is always uint8_t


/**
 * Commands from NMT master.
 */
typedef enum{
    NMT_ENTER_OPERATIONAL        = 1,    //Start device */
    NMT_ENTER_STOPPED            = 2,    // Stop device */
    NMT_ENTER_PRE_OPERATIONAL    = 128,  // Put device into pre-operational */
    NMT_RESET_NODE               = 129,  // Reset device */
    NMT_RESET_COMMUNICATION      = 130   // Reset CANopen communication on device */
}NmtCommand; //this is aways uint8


/**
 * Return code for nmt_process() that tells application what to do
 * reset.
 */
typedef enum{
    RESET_NOT  = 0,// Normal return, no action */
    RESET_COMM = 1,// Application must provide communication reset. */
    RESET_APP  = 2,// Application must provide complete device reset */
    RESET_QUIT = 3 // Application must quit, no reset of microcontroller (command is not requested by the stack.) */
}NmtResetCode;

/**
 * Structure to for NMT node status used by master to for each node.
 * Only used by the master
 *
 */

typedef struct
{
	uint8_t nodeId;
	uint8_t nmtState; //NMT state of the remote node
	BOOL monStarted; //true after reception of first heartbeat message
	uint8_t processCount; //process count since lastheartbeat
	uint8_t maxProcessCount; //heartbeat should be received within this many process counts
	BOOL canRxNew; //true if new hearbeat message received from the CAN bus
	CanOpenTx *pCanOpenTxBuffer;//this will be transmission buffer from CanOpenDriverModule here for easy data handling and keep it aligned with other modules as well in future.

}NmtNode;

/**
 *
 *
 */

typedef struct
{
	NmtNode* pMonitoredNodes; //pointer to array of nodes.
	uint8_t numberOfMonitoredNodes; //number of nodes monitored by this master
	CanOpenDriverModule* pCanOpenDriverRx; //canopen driver module to receive the can message
	uint16_t canOpenRxNmtStartIndex; //Where the NMt index start
	CanOpenDriverModule* pCanOpenDriverTx; //canopen driver module to receive the can message
	uint16_t canOpenTxNmtStartIndex; //Where the NMt index start
	void (*pFunctSignal)(void); //Just for signalling Rtos or any other call back . optional for now.
}NmtMasterModule;

/**
 * Struct used by client to store sdo server node info
 */
typedef struct
{
	uint8_t canRxData[8];
	uint8_t nodeId;
	CanOpenTx* pCanOpenTxBuffer;
	BOOL canRxNew;
	BOOL canTxNew;

}SdoServerNode;


typedef struct
{
	SdoServerNode* pSdoServerNode;
	uint8_t numberOfSdoNodes;
	CanOpenDriverModule* pCanOpenDriverRx;
	uint16_t canOpenRxSdoClientStartIndex;
	uint16_t canOpenTxSdoClientStartIndex;
	CanOpenDriverModule* pCanOpenDriverTx;
	void (*pFunctSignal)(void); //Just for signalling Rtos or any other call back . optional for now.
}SdoClientModule;

/////////Define all struct above this line for inclusion in CanOpenModule
typedef struct{
	uint8_t nodeID;
	SdoModule *pSdoObject; /* to handle sdo communication*/
	CanOpenDriverModule* pCanDriverObject;/* This is sort of driver layer between canOpen and can 2.0*/
	NmtHeartbeatModule* pNmtHbObject;
	NmtMasterModule* pNmtMasterObject;
	SdoClientModule* pSdoClientObject;
	//All future Can Modules go here
	  /**
	   * 1.PDO
	   * 2.NMT
	   * 3.HeartBeat
	   */

}CanOpenModule;

#endif /* APPLICATION_USER_CANOPENDEFINES_H_ */

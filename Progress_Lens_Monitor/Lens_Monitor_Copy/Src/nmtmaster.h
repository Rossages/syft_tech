/*
 * nmtmaster.h
 *
 *  Created on: 11/01/2017
 *      Author: gaurav.vyas
 */

#ifndef APPLICATION_USER_NMTMASTER_H_
#define APPLICATION_USER_NMTMASTER_H_
#include "canopendefines.h"
/**
 * Initialize nmtheartbeat object
 */
void nmtmaster_init(uint8_t* nodeIds,
		NmtMasterModule* pNmtMaster,
		      CanOpenDriverModule* pCanOpenDriverRx,
			  uint16_t canOpenDriverRxIndex,
			  CanOpenDriverModule* pCanOpenDriverTx,
			  uint16_t canOpenDriverTxIndex,
			  uint8_t numberOfMonitoredNodes

		 );

/**
 * Config the node
 */
void configMonitoredNode(NmtMasterModule* pNmtMaster,uint8_t index,uint8_t maxProcessCount,uint8_t* nodeIds);


/**
 * receive a NMT CAN frame
 */
void receiveNmtMaster(void *object, CanRxMsgTypeDef *pMessage);


/**
 * process a sdpMessage
 */
void nmtmaster_process(NmtMasterModule *pNmtHbModule,uint8_t nmtMasterState);

void nmtmaster_sendCommand(NmtMasterModule *pNmtMasterModule,uint8_t command,uint8_t nodeId );

#endif /* APPLICATION_USER_NMTMASTER_H_ */

/*
 * nmt.h
 *
 *  Created on: 28/12/2016
 *      Author: gaurav.vyas
 */

#ifndef APPLICATION_USER_NMT_H_
#define APPLICATION_USER_NMT_H_
#include "canopendefines.h"

/**
 * Initialize nmtheartbeat object
 */
void nmt_init(uint8_t nodeId,
		NmtHeartbeatModule* pSdo,
		      CanOpenDriverModule* pCanOpenDriverRx,
			  uint16_t canOpenDriverRxIndex,
			  CanOpenDriverModule* pCanOpenDriverTx,
			  uint16_t canOpenDriverTxIndex

		 );


/**
 * receive a NMT CAN frame
 */
void receiveNmt(void *object, CanRxMsgTypeDef *pMessage);

/**
 * send a SDO CAN frame
 */
void sendNmtHeartbeat();

/**
 * process a sdpMessage
 */
NmtResetCode nmt_process(NmtHeartbeatModule *pNmtHbModule);



/**
 * Initialize call back  - this is optional at the moment
 */
void initializeCallback(
		NmtHeartbeatModule *pNmtHbModule,
		void (*pFunctSignal)(void));

/**
 * Method to generate the heart beat for the node. Basically sends a tX to NMT master
 */
void nmt_generateHeartbeat(NmtHeartbeatModule* pNmtHbObject);



/*
 * Just update the state we will send this using heartbeat
 */
void nmt_updateOperatingState(NmtHeartbeatModule* pNmtHbObject,NmtState nmtState);


/*
 *
 *just update the reset code for the node
 */

void nmt_updateResetCommand(NmtHeartbeatModule* pNmtHbObject,NmtResetCode nmtResetCode);


#endif /* APPLICATION_USER_NMT_H_ */

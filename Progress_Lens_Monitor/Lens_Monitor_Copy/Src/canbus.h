/*
 * canbus.h
 *
 *  Created on: 15/12/2016
 *      Author: gaurav.vyas
 */

#ifndef APPLICATION_USER_CANBUS_H_
#define APPLICATION_USER_CANBUS_H_
//#include "can.h"
#include "canopendefines.h"
/*
 * This is intended just to be a sort of plugin wrapper around HAL Libraries
 */

/**
 * initialize HAL Can libraries
 */
void canbus_initialize(CanOpenDriverModule* pcanOpenDriver,
						CanOpenRx canOpenRx[],
						uint16_t canOpenRxSize,
						CanOpenTx canOpenTx[],
						uint16_t canOpenTxSize,
						uint8_t nodeID
						);

/**
 * transmit can Libraries
 *
 */
void canbus_transmit(CanOpenDriverModule *pCanOpenDriverTxObject,uint16_t txIndex);
void canbus_receive(CanOpenDriverModule *pCanOpenDriverRxObject);


/**
 * receive can message
 */
void canbus_initRxBuffer(uint32_t cobId,
						CanOpenDriverModule* pCanOpenDriver,
						uint16_t rxIndex,
						void* object,
		 	 	 	 	 void (*pFunct)(void *object, CanRxMsgTypeDef*message));

/**
 * canbus_initTxBuffer() - Initializes the Tx buffer for underlying protocol like sdo,pdo
 *
 *
 */
CanOpenTx* canbus_initTxBuffer(uint32_t cobId,CanOpenDriverModule* pCanOpenDriver, uint16_t txIndex, uint8_t noOfBytes);



void writeCanMessageQueue(CanRxMsgTypeDef* pCanMessage);
void initRxQueue();
void MX_CAN1_Init(void);

void canbus_enableInterrupts();

#endif /* APPLICATION_USER_CANBUS_H_ */

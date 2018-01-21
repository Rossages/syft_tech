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


//this enum contains all allowed baud rates for the can bus, up on initialization this will set  the prescalar value.
typedef enum
{
	CAN_125_KBPS_PRESCALER=24,
	CAN_250_KBPS_PRESCALER=12,
	CAN_500_KBPS_PRESCALER=6,
	CAN_1000_KBPS_PRESCALER=3

}CanBaudRatePrescaler;

/**
 * initialize HAL Can libraries
 */
void canbus_initialize(CanOpenDriverModule* pcanOpenDriver,
						CanOpenRx canOpenRx[],
						uint16_t canOpenRxSize,
						CanOpenTx canOpenTx[],
						uint16_t canOpenTxSize,
						uint8_t nodeID,
						CanBaudRatePrescaler baudRatePrescaler
						);

/**
 * transmit can Libraries
 *
 */
void canbus_transmit(CanOpenDriverModule *pCanOpenDriverTxObject,uint16_t txIndex);

//This method is used by can process thread when CAN receive interrupts are enabled.
void canbus_receive_It(CanOpenDriverModule *pCanOpenDriverRxObject);

/**
 * initialize the Rx buffers and registers the callback.
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

//Method to initialize the CAN bus  hal interfaces and peripherals. prescaler sets the baud rate  and node id is used for filters.
static void MX_CAN1_Init(CanBaudRatePrescaler prescaler, uint8_t nodeId);

void canbus_enableInterrupts();

#endif /* APPLICATION_USER_CANBUS_H_ */

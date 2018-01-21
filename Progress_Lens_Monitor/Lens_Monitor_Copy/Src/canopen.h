/*
 * CanOpen.h
 *
 *  Created on: 15/12/2016
 *      Author: gaurav.vyas
 */

#ifndef APPLICATION_USER_CANOPEN_H_
#define APPLICATION_USER_CANOPEN_H_
//All includes go here
//#include "can.h"
#include "canopendefines.h"
#include "sdo.h"





/**
 * Method to initialize whole CAN architecture including can BUS
 */
void canopen_initCan(uint8_t nodeId);

/**
 * Method to send the can message
 *
 */
 void canopen_txMessage();



 /**
 * Method to receive messages
 *  //TODO: we may need a call back here to register the appropriate moducle here.
 */
void canopen_rxMessage();

/**
 * Method to process all can open messages.
 */
NmtResetCode canopen_process();

void canopen_generateHeartbeat();

void canopen_updateOperatingState(NmtState state);

void canopenMaster_initCan();
NmtResetCode canopenMaster_process();

void initNodeIds();
void canopen_rxCan();

/*
 * method to enable receive interrupts.
 */
void canopen_enableComm(void);

#endif /* APPLICATION_USER_CANOPEN_H_ */

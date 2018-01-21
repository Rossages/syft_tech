/*
 * sdoclient.h
 *
 *  Created on: 13/01/2017
 *      Author: gaurav.vyas
 */

#ifndef APPLICATION_USER_SDOCLIENT_H_
#define APPLICATION_USER_SDOCLIENT_H_
#include "canopenDefines.h"

void sdoClient_init(uint8_t* pNodeIds,
		      SdoClientModule* pSdoClientObject,
		      CanOpenDriverModule* pCanOpenDriverRx,
			  uint16_t canOpenDriverRxIndex,
			  CanOpenDriverModule* pCanOpenDriverTx,
			  uint16_t canOpenDriverTxIndex,
			  uint8_t numberOfSdoServerNodes

		 );

void configSdoNode(SdoClientModule* pSdoClientObject,uint8_t index,uint8_t* pNodeIds);

void receiveSdoClient(void *object, CanRxMsgTypeDef *pMessage);

void sendSdoClient(SdoModule *pSdoModule);

void sdoClient_process(SdoClientModule* pSdoClientModule);

void sdoClient_sendCommand(SdoClientModule *pSdoClientModule,uint8_t nodeId,uint8_t serverCommandSpecifier,uint16_t addressIndex,uint8_t subIndex,uint32_t dataValue);

#endif /* APPLICATION_USER_SDOCLIENT_H_ */

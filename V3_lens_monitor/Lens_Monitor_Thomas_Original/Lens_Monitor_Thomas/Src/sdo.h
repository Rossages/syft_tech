/*
 * sdo.h
 *
 *  Created on: 15/12/2016
 *      Author: gaurav.vyas
 */

#ifndef APPLICATION_USER_SDO_H_
#define APPLICATION_USER_SDO_H_
#include "canopendefines.h"
#include "objectdictionary.h"
//All includes go here



/**
 * Initialize SDO object
 */
void sdo_init(uint8_t nodeId,
		SdoModule* pSdo,
		      CanOpenDriverModule* pCanOpenDriverRx,
			  uint16_t canOpenDriverRxIndex,
			  CanOpenDriverModule* pCanOpenDriverTx,
			  uint16_t canOpenDriverTxIndex

		 );
/**
 * Method to read object dictionary
 */
void readObjectDictionary();

/**
 * Method to write the object dictionary
 *
 */
void writeDataQueue(SdoModule* pSdoObject);
//void writeObjectDictionary();

/**
 * receive a SDO CAN frame
 */
void receiveSdo(void *object, CanRxMsgTypeDef *pMessage);

/**
 * send a SDO CAN frame
 */
void sendSdo();

/**
 * process a sdpMessage
 */
void sdo_process(SdoModule *sdo);



/**
 * Initialize call back  - this is optional at the moment
 */
void initializeCallback(
		SdoModule *sdo,
		void (*pFunctSignal)(void));


//Method to write the block data queue
void writeBlockDataQueue(SdoModule* pSdoModule);

//Method to get the data from block queue and update
void getDataStreamFromBlockDataQueue(void);


/**
 * Method to wite the object dictionary Data queue
 *
 */
void writeOdDataQueue(uint16_t addressIndex,uint8_t subIndex, uint32_t value);


/**
 * Method to upload the data to master.
 */
void sdoUploadBlock(SdoModule* pSdoModule );
/**
 * Method to get the object dictionary entry
 *
 */


//method to get the data in required format for transfer and return datastream pointer for transfers.
DictionaryDataStream* getDataStreamPtr(DictionaryDataStream* pDataStreamSource, DictionaryDataStream* pDataTransferBuffer);

#endif /* APPLICATION_USER_SDO_H_ */

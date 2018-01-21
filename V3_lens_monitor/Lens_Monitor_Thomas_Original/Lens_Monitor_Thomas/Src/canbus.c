/*
 * canbus.c
 *
 *  Created on: 15/12/2016
 *      Author: gaurav.vyas
 */

#include "canbus.h"
#include "string.h"
#include "FreeRtos.h"
#include "queue.h"
#include "semphr.h"
//______________________________________________
//storing interuppted can messages
//TaskHandle_t msgQTaskHandle = NULL;
//SemaphoreHandle_t semaphoreMsgTask = NULL;


//_______________________________________
CAN_FilterConfTypeDef  sFilterConfig1;
static CanTxMsgTypeDef        Tx1Message;
static CanRxMsgTypeDef        Rx1Message;
CanRxMsgTypeDef initCanRxMsg = {};
CanRxMsgTypeDef canMessageRxEntry;
CanRxMsgTypeDef* pCanMessageRxEntry =NULL;
CanRxMsgTypeDef canMessageQueueEntry ;
CanRxMsgTypeDef* pCanMessageQueueEntry =NULL;
const int canTxRxTimeoutMs = 10;

//queue handle for RtosQueue
static QueueHandle_t canMsgQueueHandle = NULL;

const int RX_MSG_QUEUE_LENGTH= 100;

/**
 * initialize HAL Can libraries
 */
void canbus_initialize(CanOpenDriverModule* pCanOpenDriver,
		CanOpenRx canOpenRx[],
		uint16_t canOpenRxSize,
		CanOpenTx canOpenTx[],
		uint16_t canOpenTxSize,
		uint8_t nodeId,
		CanBaudRatePrescaler baudRatePrescaler
		)
{
	//these are can open defines.
	pCanOpenDriver->pCanHandle= &hcan1;
	pCanOpenDriver->pCanOpenRxBuffer = canOpenRx;
	pCanOpenDriver->rxSize = canOpenRxSize;
	pCanOpenDriver->pCanOpenTxBuffer = canOpenTx;
	pCanOpenDriver->txSize = canOpenTxSize;

	//CUBE MX generated function so i kept the name :( .
	MX_CAN1_Init(baudRatePrescaler,nodeId);
	//assign buffer
	hcan1.pRxMsg = &Rx1Message;
	hcan1.pTxMsg = &Tx1Message;

	//initialize the rx queue. Synchronization ?
	initRxQueue();
}


/**
 * Enable can bus interrupts for can which was intialized.This should always be called after all intializations have been completed.
 */

void canbus_enableInterrupts()
{
	//enable interrupt reception
	HAL_CAN_Receive_IT(&hcan1,CAN_FIFO0);
}


/**
 * transmit to can Libraries
 */
void canbus_transmit(CanOpenDriverModule *pCanOpenDriverTxObject,uint16_t txIndex)
{
	//Move to transmit index
	CanOpenTx* pTxBuffer = pCanOpenDriverTxObject->pCanOpenTxBuffer+txIndex;
	pCanOpenDriverTxObject->pCanHandle->pTxMsg->StdId = pTxBuffer->stdId;
	pCanOpenDriverTxObject->pCanHandle->pTxMsg->RTR =CAN_RTR_DATA;
	pCanOpenDriverTxObject->pCanHandle->pTxMsg->IDE = CAN_ID_STD;
	pCanOpenDriverTxObject->pCanHandle->pTxMsg->DLC = pTxBuffer->DLC;
	for(uint8_t i= 0;i<pTxBuffer->DLC;i++)
	{
		pCanOpenDriverTxObject->pCanHandle->pTxMsg->Data[i] = pTxBuffer->data[i];
	}

	//HAL_CAN_TRANSMIT is synchronized  so no need to synchronize this method
	if(HAL_CAN_Transmit(pCanOpenDriverTxObject->pCanHandle, canTxRxTimeoutMs) != HAL_OK)
	{
		//Log - Error May be dictionary write
	}
	else
	{
		//clear the data buffer
		memset(pCanOpenDriverTxObject->pCanHandle->pTxMsg->Data,0,sizeof(pCanOpenDriverTxObject->pCanHandle->pTxMsg->Data));
	}

}



/**
 * canbus_rxBufferInit - registers the call back function shall be called from all sub protocols SDO, PDO,NMT heartbeat
 */
void canbus_initRxBuffer(uint32_t cobId,CanOpenDriverModule* pCanOpenDriver, uint16_t rxIndex, void* object,
															void(*pFunct)(void *object, CanRxMsgTypeDef*message))
{
	//TODO: NULL checks everywhere

	CanOpenRx* rxBuffer = pCanOpenDriver->pCanOpenRxBuffer + rxIndex;
	rxBuffer->stdId = cobId;
	rxBuffer->protocolObject = object;
	rxBuffer->pFunct = pFunct;

	//alignmnet ? may be we dont need check later on


}


/**
 * canbus_initTxBuffer() - Initializes the Tx buffer for underlying protocol like sdo,pdo
 *
 */
CanOpenTx* canbus_initTxBuffer(uint32_t cobId,CanOpenDriverModule* pCanOpenDriver, uint16_t txIndex, uint8_t noOfBytes)
{
	//TODO: NULL checks everywhere
	CanOpenTx* txBuffer = pCanOpenDriver->pCanOpenTxBuffer+txIndex;
	txBuffer->stdId = cobId;
	txBuffer->DLC = noOfBytes;

	return txBuffer;
}

// for now init it here
/* CAN1 init function */
static void MX_CAN1_Init(CanBaudRatePrescaler prescaler, uint8_t nodeId)
{

//init the CAN1 STB pin
	GPIO_InitTypeDef GPIO_InitStruct;
	/*Configure GPIO pin : CAN1_nSTB_Pin */
	GPIO_InitStruct.Pin = CAN1_nSTB_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(CAN1_nSTB_GPIO_Port, &GPIO_InitStruct);
	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(CAN1_nSTB_GPIO_Port, CAN1_nSTB_Pin, GPIO_PIN_SET);

	hcan1.Instance = CAN1;
	//Prescalar depends on SJW , BS1 and BS2  and should be chosen carefully. For current clocks  prescalar 6 = 500 kbps ,3= 1000Kbps 12 =250 kbps
	hcan1.Init.Prescaler = prescaler; //6 is 500kHz ,3 is 1000Khz,12 is 250 kHz
	hcan1.Init.Mode = CAN_MODE_NORMAL;
	hcan1.Init.SJW = CAN_SJW_1TQ;
	hcan1.Init.BS1 = CAN_BS1_13TQ;
	hcan1.Init.BS2 = CAN_BS2_2TQ;
	hcan1.Init.TTCM = DISABLE;
	hcan1.Init.ABOM = DISABLE;
	hcan1.Init.AWUM = DISABLE;
	hcan1.Init.NART = DISABLE;
	hcan1.Init.RFLM = DISABLE;
	hcan1.Init.TXFP = DISABLE;
	if (HAL_CAN_Init(&hcan1) != HAL_OK)
	{
		//TODO :Error_Handler();
	}

	//make sure filters are configured  for SDO and NMT only

	sFilterConfig1.FilterNumber = 0;
	sFilterConfig1.FilterMode = CAN_FILTERMODE_IDLIST;
	sFilterConfig1.FilterScale = CAN_FILTERSCALE_32BIT;
	sFilterConfig1.FilterIdHigh = (CAN_OPEN_SDO_CLIENT_TO_SERVER + nodeId) << 5;
	sFilterConfig1.FilterIdLow = 0x0000;
	sFilterConfig1.FilterMaskIdHigh = CAN_OPEN_NMT_MASTER_TO_SLAVE << 5;
	sFilterConfig1.FilterMaskIdLow = 0x0000;
	sFilterConfig1.FilterFIFOAssignment = 0;
	sFilterConfig1.FilterActivation = ENABLE;
	sFilterConfig1.BankNumber = 1;


	HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig1);

}

///////////////////////////interuupt functions/////////////////////////////////////////////////////////////////////////////
//this method is to be used when receive interuppts are enabled, so can _Process threads use this method to deque and store message.
void canbus_receive_It(CanOpenDriverModule *pCanOpenDriverRxObject)
{


	//Sync slows the message to great extent even without it things will work and will be even faster because the
	//Queue receive will fail  if its is being accessed by interuppt complete but we will get the message in next run so all good.
	if(xQueueReceive(canMsgQueueHandle, (void*)pCanMessageQueueEntry,( TickType_t ) 10  )==pdTRUE)
	{
		//Update the message in the dictionary and storage hardware ///
		for(uint16_t i = 0; i<pCanOpenDriverRxObject->rxSize;i++)
			{
				//Matchids and store data
				if(pCanOpenDriverRxObject->pCanOpenRxBuffer[i].stdId==pCanMessageQueueEntry->StdId)
				{
					//Call back to module
					if(pCanOpenDriverRxObject->pCanOpenRxBuffer[i].pFunct != NULL)
					{

					pCanOpenDriverRxObject->pCanOpenRxBuffer[i].pFunct(pCanOpenDriverRxObject->pCanOpenRxBuffer[i].protocolObject,pCanMessageQueueEntry);
				    break;
					}
				}
			}//end FOR

	}//end if(Qreceive)

}


//can call backs -- this method is called after the interuppt handler is finished gettting the can message.
void HAL_CAN_RxCpltCallback(CAN_HandleTypeDef* hcan)
{
  /* Prevent unused argument(s) compilation warning */

  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_CAN_RxCpltCallback could be implemented in the user file*/
	//write the message to the receive queue
	writeCanMessageQueue(hcan->pRxMsg);
	//keep resetting if something goes bad
	//just make sure whatever is holding CAN unlocks the can handle
	__HAL_UNLOCK(hcan); //hal unlock will wait for mutex state to go unlocked before setting the interrupt registers
	while (HAL_CAN_Receive_IT(hcan, CAN_FIFO0) != HAL_OK)
	{
		//keep unlocking so HAL_CAN_Receive to clear interrupt flags
		__HAL_UNLOCK(hcan);
	}

}


//write the queue
void writeCanMessageQueue(CanRxMsgTypeDef* pCanMessage)
{
	//send to queue
	static BaseType_t highPriorityTaskWoken = pdFALSE;
	if (pCanMessage != NULL)
	{

		if (xQueueSendFromISR(canMsgQueueHandle, (void* )pCanMessage,
				highPriorityTaskWoken)==pdPASS)
		{
			//someindication of data sent to queue
		}
		else
		{
			//log error -  queue full or data not sent to queue
		}

	}

}


void initRxQueue()
{

	//check if this is comm reset request we reset the queue
	if (canMsgQueueHandle != NULL)
	{
		//delete the queue
		xQueueReset(canMsgQueueHandle);
	}
	else
	{
		//init the can message queue
		canMsgQueueHandle = xQueueCreate(RX_MSG_QUEUE_LENGTH, sizeof(CanRxMsgTypeDef));
	}

	//initialize Rx queue entry structs
	canMessageRxEntry=initCanRxMsg;
	canMessageQueueEntry=initCanRxMsg;
	//pointer assignment
	pCanMessageRxEntry = &canMessageRxEntry;
	pCanMessageQueueEntry = &canMessageQueueEntry;

	//NMT reset comm message incase the comm is failed and can reception gets locked or unresponsive

}


void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
  /* Prevent unused argument(s) compilation warning */
  //UNUSED(hcan);
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_CAN_ErrorCallback could be implemented in the user file
   */
	///I think we need to init the CAN  again  and enable interuppts;
	//MX_CAN1_Init();
	//HAL_CAN_Receive_IT(&hcan1,CAN_FIFO0);

}



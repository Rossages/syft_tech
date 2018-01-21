/*
 * canbus.c
 *
 *  Created on: 15/12/2016
 *      Author: gaurav.vyas
 */

#include "canbus.h"
#include "FreeRtos.h"
#include "queue.h"
#include "semphr.h"
#include "global.h"
//______________________________________________
//storing interuppted can messages
//TaskHandle_t msgQTaskHandle = NULL;
//SemaphoreHandle_t semaphoreMsgTask = NULL;


//_______________________________________
CAN_FilterConfTypeDef  sFilterConfig1;
CAN_FilterConfTypeDef  sFilterConfig2;
static CanTxMsgTypeDef        Tx1Message;
static CanRxMsgTypeDef        Rx1Message;
//static int msgCount = 100;
CanRxMsgTypeDef initCanRxMsg = {};
CanRxMsgTypeDef canMessageRxEntry;
CanRxMsgTypeDef* pCanMessageRxEntry =NULL;
//uint8_t rxData[8] ={0,0,0,0,0,0,0,0};
//uint8_t queueData[8] ={0,0,0,0,0,0,0,0};
CanRxMsgTypeDef canMessageQueueEntry ;
CanRxMsgTypeDef* pCanMessageQueueEntry =NULL;

CanRxMsgTypeDef canResetCommQueueEntry;
CanRxMsgTypeDef* pCanResetCommQueueEntry =NULL;
uint8_t resetNodeID ;
//uint8_t *pRxData=NULL;
//uint8_t *pQueueData=NULL;

//queue handle for RtosQueue
static QueueHandle_t canMsgQueueHandle = NULL;
//SemaphoreHandle_t qSemaphoreHandle;

//static CanTxMsgTypeDef        Tx2Message;
//static CanRxMsgTypeDef        Rx2Message;
/**
 * initialize HAL Can libraries
 */
void canbus_initialize(CanOpenDriverModule* pCanOpenDriver,
		CanOpenRx canOpenRx[],
		uint16_t canOpenRxSize,
		CanOpenTx canOpenTx[],
		uint16_t canOpenTxSize,
		uint8_t nodeId
		)
{
	//these are can open defines.



	pCanOpenDriver->pCanHandle= &hcan1;
	pCanOpenDriver->pCanOpenRxBuffer = canOpenRx;
	pCanOpenDriver->rxSize = canOpenRxSize;
	pCanOpenDriver->pCanOpenTxBuffer = canOpenTx;
	pCanOpenDriver->txSize = canOpenTxSize;

	//CUBE MX function
		MX_CAN1_Init();
	//assign buffer
			hcan1.pRxMsg = &Rx1Message;
			hcan1.pTxMsg = &Tx1Message;

			//initialize the rx queue. Synchronization ?
			initRxQueue();



		/*##-2 Configure CAN1 transmission message this will be overriden by protocol*/

	  /*##-2- Configure the CAN1 Filter ###########################################*/


			//enable interrupt reception
		///HAL_CAN_Receive_IT(&hcan1,CAN_FIFO0);

			resetNodeID = nodeId;
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
	//TODO: somekind of Synchronization here
	pCanOpenDriverTxObject->pCanHandle->pTxMsg->StdId = pTxBuffer->stdId;
	pCanOpenDriverTxObject->pCanHandle->pTxMsg->RTR =CAN_RTR_DATA;
	pCanOpenDriverTxObject->pCanHandle->pTxMsg->IDE = CAN_ID_STD;
	pCanOpenDriverTxObject->pCanHandle->pTxMsg->DLC = pTxBuffer->DLC;
	for(uint8_t i= 0;i<pTxBuffer->DLC;i++)
	{
		pCanOpenDriverTxObject->pCanHandle->pTxMsg->Data[i] = pTxBuffer->data[i];
	}

	if(HAL_CAN_Transmit(pCanOpenDriverTxObject->pCanHandle, 10) != HAL_OK)
	{
		//test code Gaurav
		//HandleError

	/*	switch(hcan1.State)
		{
		case HAL_CAN_STATE_RESET:
			printf("HAL_CAN_STATE_RESET ");
			break;//!< CAN not yet initialized or disabled
		case HAL_CAN_STATE_READY:
			printf("HAL_CAN_STATE_READY");
			break;/*!< CAN initialized and ready for use
		case HAL_CAN_STATE_BUSY:
			printf("HAL_CAN_STATE_BUSY: ");
			break;/*!< CAN process is ongoing
		case HAL_CAN_STATE_BUSY_TX:
			printf("HAL_CAN_STATE_BUSY_TX ");
			break;/*!< CAN process is ongoing
		case HAL_CAN_STATE_BUSY_RX:
			printf("HAL_CAN_STATE_BUSY_RX ")
			break;/*!< CAN process is ongoing
		case HAL_CAN_STATE_BUSY_TX_RX:
			printf("HAL_CAN_STATE_BUSY_TX_RX: ");
			break;/*!< CAN process is ongoing
		case HAL_CAN_STATE_TIMEOUT:
			printf("HAL_CAN_STATE_TIMEOUT ");
			break;/*!< Timeout state
		case  HAL_CAN_STATE_ERROR:
			printf("HAL_CAN_STATE_ERROR");
			break;

		}
		printf("HAL_CAN_Transmit failed- No can ");
						printf("\r\n\r\n");*/
	}
	else
	{
		//clear the data buffer

		//test code Gaurav
	uint8_t* pMsg = pCanOpenDriverTxObject->pCanHandle->pTxMsg->Data;
			//printf("Canbus Transmit %x %x %x %x %x %x %x %x ",*pMsg,*(pMsg+1),*(pMsg+2),*(pMsg+3),*(pMsg+4),*(pMsg+5),*(pMsg+6),*(pMsg+7));
			//printf("\r\n\r\n");

			//clear the data buffer
			memset(pCanOpenDriverTxObject->pCanHandle->pTxMsg->Data,0,sizeof(pCanOpenDriverTxObject->pCanHandle->pTxMsg->Data));
	}
	//Synchronization remove
}

/**
 * This method should be used for just polling  and no interrupts-
 * receive can message only when polling
 */
void canbus_receive(CanOpenDriverModule *pCanOpenDriverRxObject)
{


	if(HAL_CAN_Receive(pCanOpenDriverRxObject->pCanHandle,CAN_FIFO0, 10) != HAL_OK)
	//if(HAL_CAN_Receive_IT(pCanOpenDriverRxObject->pCanHandle,CAN_FIFO0) != HAL_OK)
	{
			//HandleError
		/*switch(hcan1.State)
			{
			case HAL_CAN_STATE_RESET:
				printf("HAL_CAN_STATE_RESET ");
				break;!< CAN not yet initialized or disabled
			case HAL_CAN_STATE_READY:
				printf("HAL_CAN_STATE_READY");
				break;/*!< CAN initialized and ready for use
			case HAL_CAN_STATE_BUSY:
				printf("HAL_CAN_STATE_BUSY: ");
				break;/*!< CAN process is ongoing
			case HAL_CAN_STATE_BUSY_TX:
				printf("HAL_CAN_STATE_BUSY_TX ");
				break;/*!< CAN process is ongoing
			case HAL_CAN_STATE_BUSY_RX:
				printf("HAL_CAN_STATE_BUSY_RX ");
				break;/*!< CAN process is ongoing
			case HAL_CAN_STATE_BUSY_TX_RX:
				printf("HAL_CAN_STATE_BUSY_TX_RX: ");
				break;/*!< CAN process is ongoing
			case HAL_CAN_STATE_TIMEOUT:
				printf("HAL_CAN_STATE_TIMEOUT ");
				break;/*!< Timeout state
			case  HAL_CAN_STATE_ERROR:
				printf("HAL_CAN_STATE_ERROR");
				break;

			}
		printf("\r\n HAL_CAN_RECEIVE failed- No msg ");
				printf("\r\n\r\n");*/
	}
	else
	{
		uint8_t* pMsg = pCanOpenDriverRxObject->pCanHandle->pRxMsg->Data;
		//printf("Canbus Recieve %x %x %x %x %x %x %x %x ",*pMsg,*(pMsg+1),*(pMsg+2),*(pMsg+3),*(pMsg+4),*(pMsg+5),*(pMsg+6),*(pMsg+7));
		//printf("\r\n\r\n");
	//store the Data by Matching CobID
	for(uint16_t i = 0; i<pCanOpenDriverRxObject->rxSize;i++)
	{
		//Matchids and store data
		if(pCanOpenDriverRxObject->pCanOpenRxBuffer[i].stdId==pCanOpenDriverRxObject->pCanHandle->pRxMsg->StdId)
		{
			//Call back to module
			if(pCanOpenDriverRxObject->pCanOpenRxBuffer[i].pFunct != NULL)
			{
			pCanOpenDriverRxObject->pCanOpenRxBuffer[i].pFunct(pCanOpenDriverRxObject->pCanOpenRxBuffer[i].protocolObject,pCanOpenDriverRxObject->pCanHandle->pRxMsg);
		    break;
			}
		}
	}
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
void MX_CAN1_Init(void)
{

  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 6;//6 is 500kHz ,3 is 1000Khz,12 is 250 kHz
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
    Error_Handler();
    //printf("Can1 init failed");
  }

  //Init the filter for reception
  	  sFilterConfig1.FilterNumber = 0;
 	  sFilterConfig1.FilterMode = CAN_FILTERMODE_IDMASK;
 	  sFilterConfig1.FilterScale = CAN_FILTERSCALE_32BIT;
 	  sFilterConfig1.FilterIdHigh = 0x0000;
 	  sFilterConfig1.FilterIdLow = 0x0000;
 	  sFilterConfig1.FilterMaskIdHigh = 0x0000;
 	  sFilterConfig1.FilterMaskIdLow = 0x0000;
 	  sFilterConfig1.FilterFIFOAssignment = 0;
 	  sFilterConfig1.FilterActivation = ENABLE;
 	  sFilterConfig1.BankNumber = 14;
      HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig1);

}

///////////////////////////interuupt functions/////////////////////////////////////////////////////////////////////////////
//this method is to be used when receive interuppts are enabled.
void canbus_receive_It(CanOpenDriverModule *pCanOpenDriverRxObject)
{


	//Sync slows the message to great extent even without it things will work and will be even faster because the
	//Queue receive will fail  if its is being accessed by interuppt complete.
	//if( xSemaphoreTake( qSemaphoreHandle, ( TickType_t ) 10  ) == pdTRUE)
	{
		//printf("msgWaiting %d",uxQueueMessagesWaiting(canMsgQueueHandle));
	if(xQueueReceive(canMsgQueueHandle, (void*)pCanMessageQueueEntry,( TickType_t ) 10  )==pdTRUE)
	{
		//printf("\r\n canbus receive IT  qReceive %x,%x \r\n",pCanMessageQueueEntry->Data[0],pCanMessageQueueEntry->Data[1]);
				//TODO: use only one.
		//just for testing

				//Update the message in the dictionary and storage hardware ///
		for(uint16_t i = 0; i<pCanOpenDriverRxObject->rxSize;i++)
			{
				//Matchids and store data
				if(pCanOpenDriverRxObject->pCanOpenRxBuffer[i].stdId==pCanMessageQueueEntry->StdId)
				{
					//Call back to module
					if(pCanOpenDriverRxObject->pCanOpenRxBuffer[i].pFunct != NULL)
					{
						// enable printf for test only

						//printf("\r\n Msgcount %d \r\n",msgCount);
						//printf("\r\n canbus receive IT  %x,%x \r\n",pCanMessageQueueEntry->Data[0],pCanMessageQueueEntry->Data[1]);
					pCanOpenDriverRxObject->pCanOpenRxBuffer[i].pFunct(pCanOpenDriverRxObject->pCanOpenRxBuffer[i].protocolObject,pCanMessageQueueEntry);
					//printf("\r\n canbus receive IT  after CallBack %x,%x \r\n",pCanMessageQueueEntry->Data[0],pCanMessageQueueEntry->Data[1]);
				    break;
					}
				}
			}

	//	        uint8_t* pMsg = pCanOpenDriverRxObject->pCanHandle->pRxMsg->Data;
	//			printf("Canbus Recieve %x %x %x %x %x %x %x %x ",*pMsg,*(pMsg+1),*(pMsg+2),*(pMsg+3),*(pMsg+4),*(pMsg+5),*(pMsg+6),*(pMsg+7));
	//			printf("\r\n\r\n");


	}
	//we need to give it here to makesure queue gets dequeued after the burst interuupt ends
		//xSemaphoreGive( qSemaphoreHandle );

	}

}


//can call backs -- this methos is called after the interuppt handler is finished gettting the can message.
void HAL_CAN_RxCpltCallback(CAN_HandleTypeDef* hcan)
{
  /* Prevent unused argument(s) compilation warning */

  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_CAN_RxCpltCallback could be implemented in the user file*/


	//
	//printf("Can Data  %x %x %x %x %x %x %x %x\r\n ", hcan->pRxMsg->Data[0],hcan->pRxMsg->Data[1],hcan->pRxMsg->Data[2],hcan->pRxMsg->Data[3],hcan->pRxMsg->Data[4],hcan->pRxMsg->Data[5],hcan->pRxMsg->Data[6],hcan->pRxMsg->Data[7]);
	writeCanMessageQueue(hcan->pRxMsg);
	if (HAL_CAN_Receive_IT(hcan, CAN_FIFO0) != HAL_OK)
	  {
	    /* Reception Error */
		//printf("Error receiving can \r\n reset CAN communication \r\n");
		//reset the comm
		if(xQueueSendFromISR(canMsgQueueHandle, (void*)pCanResetCommQueueEntry, pdFALSE) !=pdTRUE)
		{
		//	printf("Receive queue full \r\n");
		}
		/*switch(hcan1.State)
					{
					case HAL_CAN_STATE_RESET:
						printf("HAL_CAN_STATE_RESET ");
						break;/*!< CAN not yet initialized or disabled
					case HAL_CAN_STATE_READY:
						printf("HAL_CAN_STATE_READY");
						break;/*!< CAN initialized and ready for use
					case HAL_CAN_STATE_BUSY:
						printf("HAL_CAN_STATE_BUSY: ");
						break;/*!< CAN process is ongoing
					case HAL_CAN_STATE_BUSY_TX:
						printf("HAL_CAN_STATE_BUSY_TX ");
						break;/*!< CAN process is ongoing
					case HAL_CAN_STATE_BUSY_RX:
						printf("HAL_CAN_STATE_BUSY_RX ");
						break;/*!< CAN process is ongoing
					case HAL_CAN_STATE_BUSY_TX_RX:
						printf("HAL_CAN_STATE_BUSY_TX_RX: ");
						break;/*!< CAN process is ongoing
					case HAL_CAN_STATE_TIMEOUT:
						printf("HAL_CAN_STATE_TIMEOUT ");
						break;/*!< Timeout state
					case  HAL_CAN_STATE_ERROR:
						printf("HAL_CAN_STATE_ERROR");
						break;

					}
		printf("\r\n");
	    //Error_Handler();*/
	  }
	//printf("hcan callback exit  %x\r\n ", hcan->pRxMsg->Data[1]);
}


//write the queue
void writeCanMessageQueue(CanRxMsgTypeDef* pCanMessage)
{
	//copy values and not pointer - this isnot needed as queue copies by value and faster.
	/*pCanMessageRxEntry->DLC = pCanMessage->DLC;
	pCanMessageRxEntry->ExtId = pCanMessage->ExtId;
	pCanMessageRxEntry->FIFONumber = pCanMessage->FIFONumber;
	pCanMessageRxEntry->FMI = pCanMessage->FMI;
	pCanMessageRxEntry->IDE = pCanMessage->IDE;
	pCanMessageRxEntry->RTR = pCanMessage->RTR;
	pCanMessageRxEntry->StdId = pCanMessage->StdId;
	//just copy whole 8 bytes at the moment
	for(int i=0; i<8;i++)
	{
		pCanMessageRxEntry->Data[i] = pCanMessage->Data[i];
	}*/


	//call XsemaphoretakefromISR
	//send to queue
	static BaseType_t highPriorityTaskWoken = pdFALSE;
	//xSemaphoreTake(qSemaphoreHandle,( TickType_t ) 100 );
	if(pCanMessage != NULL)
	{
		//printf("hcan interuppt data sent to queue  %x\r\n ", pCanMessage->Data[1]);
	//if(xSemaphoreTakeFromISR(qSemaphoreHandle,&highPriorityTaskWoken )==pdPASS)
	{
	if(xQueueSendFromISR(canMsgQueueHandle, (void*)pCanMessage, highPriorityTaskWoken)==pdPASS)
	{
		//printf("hcan interuppt data sent to queue  %x\r\n ", pCanMessage->Data[1]);
	}
	else
	{
	//	printf("writeCanMessageQueue Receive queue full ---\r\n");
	}
	//xQueueSend(canMsgQueueHandle, &pCanMessageRxEntry,( TickType_t ) 10 );
	//xSemaphoreGiveFromISR(qSemaphoreHandle,&highPriorityTaskWoken);
	//portYIELD_FROM_ISR( highPriorityTaskWoken );
	}


	}//call XSemaphoregivefromISR

	//receive

}


void initRxQueue()
{

	//check if this is comm reset we reset the queue
	if(canMsgQueueHandle != NULL)
	{
		//delete the queue
		 xQueueReset(canMsgQueueHandle);
		//vQueueDelete(canMsgQueueHandle);
	}
	else
	{
	//init the can message queue
	canMsgQueueHandle = xQueueCreate( 100, sizeof( CanRxMsgTypeDef ) );
	}
	//initialize Rx queue entry structs
	canMessageRxEntry=initCanRxMsg;
	canMessageQueueEntry=initCanRxMsg;
	canResetCommQueueEntry=initCanRxMsg;

	//pointer assignment
	pCanMessageRxEntry = &canMessageRxEntry;
	pCanMessageQueueEntry = &canMessageQueueEntry;
	pCanResetCommQueueEntry = &canResetCommQueueEntry;

	//NMT reset comm message incase the comm is failed and can reception gets locked or unresponsive
	canResetCommQueueEntry.StdId = 0;
	canResetCommQueueEntry.DLC = 2;
	canResetCommQueueEntry.Data[0] = NMT_RESET_COMMUNICATION;
	canResetCommQueueEntry.Data[1] = resetNodeID;



	//BaseType_t queueTaskReturn = xTaskCreate (queueInterruptMsg,"CanOpenProcess",configMINIMAL_STACK_SIZE,(void*)"queInteruptMsg",6,&msgQTaskHandle);
	//semaphoreMsgTask = xSemaphoreCreateBinary();

	//qSemaphoreHandle= xSemaphoreCreateBinary();

	//test only code
	//if(xQueueSend(canMsgQueueHandle, &pCanMessageRxEntry, ( TickType_t ) 10))
	//	printf("send to queue success");
	//xSemaphoreGive(qSemaphoreHandle);

	//pQueueData = &queueData;
	//pRxData = &rxData;
	//canMessageQueueEntry.Data = {0,0,0,0,0,0,0,0};
	//canMessageRxEntry.Data = {0,0,0,0,0,0,0,0};

}


void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
  /* Prevent unused argument(s) compilation warning */
  //UNUSED(hcan);
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_CAN_ErrorCallback could be implemented in the user file
   */
	///I think we need to init the CAN  again  and enable interuppts;
//	MX_CAN1_Init();
//	HAL_CAN_Receive_IT(&hcan1,CAN_FIFO0);

}

//void (queueInterupptMsg)


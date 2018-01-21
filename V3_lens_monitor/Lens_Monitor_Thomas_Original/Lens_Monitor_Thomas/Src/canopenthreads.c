/*
 * canopenthreads.c
 *
 *  Created on: 29/09/2017
 *      Author: gaurav.vyas
 */

#include "canopenthreads.h"
//Initialize control parameters

//by default we go to 3 because master will be node id 1 and assuming that there might be another controller with node id 2 already.
TaskHandle_t xCanProcessThreadHandle = NULL; //TODO: use this to detect the thread running state .
int canNodeId = 3;
int heartbeatMs = 1000;
int canProcessWaitMs = 2; //5 millisecond by default
int dequeueWriteMs = 2; //4 millisecond by default //timers are at low priority
const int maxDequeueDurationMs = 20;
CanBaudRatePrescaler canBaudRatePrescaler = CAN_500_KBPS_PRESCALER; // default to 500KBPS
const int timerStartDelayMs = 500;  // This is delay in Ms when any process on timer is started the first time. this to ensure all hardware peripherals have initialized on startup.

/*Create  can Threads*/
void canopenthreads_createCanThreads(int nodeId, int processThreadPriority,int processWaitMs,int dequeueWriteTimerMs, int heartbeatTimerMs,CanBaudRatePrescaler baudRatePrescaler)
{
	//set the CAN thread and timer performance tuners
	canNodeId = nodeId<0 ? canNodeId: nodeId;
	processThreadPriority = processThreadPriority >configMAX_PRIORITIES ?configMAX_PRIORITIES: processThreadPriority;
	canProcessWaitMs = processWaitMs<canProcessWaitMs ?canProcessWaitMs : processWaitMs;
	heartbeatMs = heartbeatTimerMs>0? heartbeatTimerMs:heartbeatMs;
	//set dequeue max min bounds
	dequeueWriteMs = (dequeueWriteTimerMs<=0)? dequeueWriteMs: dequeueWriteTimerMs;
	dequeueWriteMs = (dequeueWriteMs > maxDequeueDurationMs)? maxDequeueDurationMs:dequeueWriteMs;
	canBaudRatePrescaler=baudRatePrescaler;

	//Create the can process thread
	BaseType_t xCanOpen1Returned = xTaskCreate (canOpenProcessThread,"CanOpenProcess",configMINIMAL_STACK_SIZE,(void*)"CanOpen Thread",processThreadPriority,&xCanProcessThreadHandle);
	if(xCanOpen1Returned==pdFAIL)
	{
	 //TODO:Log error main process thread can't be created
	}
}




//Method to generate heartbeat will be called on  FReeRtos timer.
void generateHeartbeatTimer(TimerHandle_t xTimer)
{
	//We dot want to use handle
	canopen_generateHeartbeat();
}

//Method to generate heartbeat will be called on timer.
void dequeueWriteMsgTimer(TimerHandle_t xTimer)
{
	//We don't want to use handle
	//Dequeue all queue messages and execute any write handlers. done it separately to avoid any reception delays.
	objectdictionaryhelper_dequeueWriteMsg();
	//TODO: move this to thread
}


//thread to check the can open messages.
void canOpenProcessThread(void* pParam)
{
	//in FreeRtosConfig.h
	//if you set configTICK_RATE_HZ to 1000 (1KHz), then a tick is 1ms (one one thousandth of a second).
	//If you set configTICK_RATE_HZ to 100 (100Hz), then a tick is 10ms (one one hundredth of a second).
	///Make sure  configUSE_TIMERS is set to 1 in FreeRtosConfig.h for using timers.             1
	//pdMS_TO_TICKS convert the millisecond to ticks

	//this is the heartbeat timer
	TimerHandle_t timerHandleHb;
	timerHandleHb = xTimerCreate("HeartbeatTimer", pdMS_TO_TICKS(heartbeatMs),
			pdTRUE, (void *) 0, generateHeartbeatTimer); //keep timerIDs to zero as we are using separate timer funtions.


	//This is the dequeue write timer
	TimerHandle_t timerHandleDequeueWrite;
	timerHandleDequeueWrite = xTimerCreate("DequeueWriteTimer",
			pdMS_TO_TICKS(dequeueWriteMs), pdTRUE, (void *) 0,
			dequeueWriteMsgTimer);//keep timerIDs to zero as we are using separate timer funtions.


	NmtResetCode reset = RESET_NOT;

	//initialize the Can node
	canopen_initCan(canNodeId,canBaudRatePrescaler);

	//start the thread for procesing the messages
	while(1)
	{
		NmtResetCode reset = RESET_NOT;
	while (reset != RESET_APP) {
		canopen_enableComm();
		//canopen_initCan(3);
		xTimerStart(timerHandleHb,pdMS_TO_TICKS( timerStartDelayMs ));
		xTimerStart(timerHandleDequeueWrite,pdMS_TO_TICKS( timerStartDelayMs));
		//makesure we reset after initialization
		reset = RESET_NOT;
		//Loop till there is no reset command when there is one and not for app reset all the comms and initialize coms again.
		while (reset == RESET_NOT) {
			reset = canopen_process();
			//we don't ned delay the moment as this is not polliing hardware
			vTaskDelay(pdMS_TO_TICKS(canProcessWaitMs));
		}
		//Stop the timers at this point we are resetting comm or exitine may be
		 xTimerStop(timerHandleDequeueWrite,pdMS_TO_TICKS( 0 ));
		 xTimerStop(timerHandleHb,pdMS_TO_TICKS( 0 ));
		canopen_updateOperatingState(NMT_STOPPED);
	}//end while !reset app
	}//end while 1
}

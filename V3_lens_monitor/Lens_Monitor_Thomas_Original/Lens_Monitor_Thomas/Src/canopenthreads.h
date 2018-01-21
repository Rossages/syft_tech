/*
 * canopenthreads.h
 *
 *  Created on: 29/09/2017
 *      Author: gaurav.vyas
 */

#ifndef APPLICATION_CANOPENTHREADS_H_
#define APPLICATION_CANOPENTHREADS_H_


#include "canopen.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "canopen.h"
#include "objectdictionary.h"
//this method shall start all can threads and timers for application
void canopenthreads_createCanThreads(int NodeId, int processThreadPriority,int processWaitMs,int dequeueWriteTimerMs, int heartbeatTimerMs,CanBaudRatePrescaler baudRatePrescaler);

//this is the main method which executes on the thread
void canOpenProcessThread(void* pParam);

//This method executes on heartbeat timer
void generateHeartbeatTimer(TimerHandle_t xTimer);

//this method queues all writes and executes on timer- this should be a thread at a later stage in prototype 3
void dequeueWriteMsgTimer(TimerHandle_t xTimer);

#endif /* APPLICATION_CANOPENTHREADS_H_ */

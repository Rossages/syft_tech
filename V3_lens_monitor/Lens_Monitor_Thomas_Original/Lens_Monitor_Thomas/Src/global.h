/*
 * global.h
 *
 *  Created on: 16/12/2016
 *      Author: Esam.Alzqhoul
 */

#ifndef GLOBAL_H_
#define GLOBAL_H_

// generated by CUBEMX
#include "main.h"
#include "stm32f7xx_hal.h"
#include "cmsis_os.h"
#include "adc.h"
#include "can.h"
#include "dac.h"
#include "dma.h"
#include "fatfs.h"
#include "iwdg.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"

//extras

#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"
#include <stdio.h>
#include <stdlib.h>
#include "ain.h"
#include "dac_out.h"
#include "digital_in_out.h"
#include "lens_control.h"
#include "adc_external_ADS838.h"

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
extern uint32_t AIN[];
/* Private variables end ---------------------------------------------------------*/


#endif /* GLOBAL_H_ */

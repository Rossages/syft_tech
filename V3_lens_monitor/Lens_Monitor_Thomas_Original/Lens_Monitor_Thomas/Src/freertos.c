/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  *
  * Copyright (c) 2017 STMicroelectronics International N.V. 
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "global.h"
#include <stdbool.h>
#include "objectdictionary.h"
#include "canopen.h"
#include "types.h"
#include "lens_control.h"

#include "lenstestvalues.h"
/* USER CODE BEGIN Includes */     

/* USER CODE END Includes */

// Value used to identify automatic lens control (rather than static)
#define AUTOMATIC_CONTROL   (0)
#define SEC_TO_MS			(1000)

#define FIRMWARE_VERSION	(3)

//Use this values for performance of Can threads and timers.
int nodeId = 6; // node Id for controller
int processThreadPriority = (configMAX_PRIORITIES - 1) ;
int processWaitMs = 2; // this is wait period in millis for main can process thread which does all process and dictionary reads
int dequeueWriteTimerMs = 2; //Only used for dictionary writes to avoid any synchronization delays or operation delays in handle wrtites
int heartbeatTimerMs = 2500;
CanBaudRatePrescaler baudRatePrescaler = CAN_500_KBPS_PRESCALER;


/* Variables -----------------------------------------------------------------*/
//osThreadId defaultTaskHandle;
osSemaphoreId Generic_Sem0Handle;
osSemaphoreId Generic_Sem1Handle;

/* USER CODE BEGIN Variables */

TaskHandle_t xTx1Handle = NULL;
TaskHandle_t xRx1Handle = NULL;
TaskHandle_t xTimer1Handle = NULL;
TaskHandle_t xTx2Handle = NULL;
TaskHandle_t xRx2Handle = NULL;
TaskHandle_t xTimer2Handle = NULL;

TaskHandle_t xTx3Handle = NULL;
TaskHandle_t xRx3Handle = NULL;
TaskHandle_t xTimer3Handle = NULL;
TaskHandle_t xTimerPollingHandle = NULL;

TaskHandle_t xCanOpen1Handle = NULL;
TaskHandle_t xHandle0;
TaskHandle_t xHandle1;
TaskHandle_t xHandle2;
TaskHandle_t xHandle3;

/* USER CODE END Variables */

/* Function prototypes -------------------------------------------------------*/
//void StartDefaultTask(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */
void CreateCANThreads(void);
void canOpenProcessThread(void* pParam);
void generateHeartbeat(TimerHandle_t xTimer);
void updateHardware(TimerHandle_t xTimer);
void delay_ms(int ms);


uint8_t  dataByte = 0x00;
uint32_t simulatedCobId = 0x703;
uint8_t  odTestValue = 0;

// Lens voltage and current readings exposed for stm studio reading //**************
uint32_t lensVoltageIndex = 0;
uint32_t restartLensVoltages = 0;
uint32_t StartLensRamp = 0; // will there be a ramping function?
uint32_t SetStaticLensVotlages = 0; // **** I have set this so that the static lens function always runs.
                                   //I have added a cheeky delay and increment at the end of the 'main' loop ****

int32_t US_Lens1_SetpointV = 0;
int32_t US_Lens2_SetpointV = 0;
int32_t US_Lens3_SetpointV = 0;
int32_t US_Lens4_SetpointV = 0;
int32_t US_Lens5_SetpointV = 0;
int32_t US_Lens6_SetpointV = 0;
int32_t US_Prefilt_SetpointV = 0;

float US_Lens1_ActualV = 0;
float US_Lens2_ActualV = 0;
float US_Lens3_ActualV = 0;
float US_Lens4_ActualV = 0;
float US_Lens5_ActualV = 0;
float US_Lens6_ActualV = 0;
float US_Prefilt_ActualV = 0;

float US_Lens1_ActualA = 0;
float US_Lens2_ActualA = 0;
float US_Lens3_ActualA = 0;
float US_Lens4_ActualA = 0;
float US_Lens5_ActualA = 0;
float US_Lens6_ActualA = 0;
float US_Prefilt_ActualA = 0;

float DS_Lens1_ActualV = 0;
float DS_Lens2_ActualV = 0;
float DS_Lens3_ActualV = 0;
float DS_Lens4_ActualV = 0;
float DS_Lens5_ActualV = 0;
float DS_Prefilt_ActualV = 0;
float DS_IGB_ActualV = 0;

float DS_Lens1_ActualA = 0;
float DS_Lens2_ActualA = 0;
float DS_Lens3_ActualA = 0;
float DS_Lens4_ActualA = 0;
float DS_Lens5_ActualA = 0;
float DS_Prefilt_ActualA = 0;
float DS_IGB_ActualA = 0;

int32_t US_Lens1_StaticV = AUTOMATIC_CONTROL;
int32_t US_Lens2_StaticV = AUTOMATIC_CONTROL;
int32_t US_Lens3_StaticV = AUTOMATIC_CONTROL;
int32_t US_Lens4_StaticV = AUTOMATIC_CONTROL;
int32_t US_Lens5_StaticV = AUTOMATIC_CONTROL;
int32_t US_Lens6_StaticV = AUTOMATIC_CONTROL;
int32_t US_Prefilt_StaticV = AUTOMATIC_CONTROL;

int32_t last_time = 0;
bool scan_Started = 0; // when false the machine is only turned on. SCan hasnt started.

// Lens voltage changer timer handle
TimerHandle_t lensVoltageChangerTimerHandle = NULL;


/* USER CODE BEGIN Variables */
//osThreadId defaultTaskHandle;
//osThreadId defaultTaskHandle2;
/* USER CODE END Variables */

/* Function prototypes -------------------------------------------------------*/
//void StartDefaultTask(void const * argument);

extern void MX_FATFS_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */
void LensVoltageChange_Callback(TimerHandle_t xTimerHandle);
void copy_lensV();


// DEV variables
 int mode_x=0;
 volatile float testValue   = 0;
 volatile float testValue2  = 0;
 volatile float testValue3  = 0;
 volatile int channel_on_fly = 4, channel_on_fly2 = 5, channel_on_fly3= 6;
 // DEV variables end

/*float GetActualCurrent(float voltage)
{
    // because the ADC can read voltages, we can look the voltage drop over the 2*100K resistors on the Ion Optics schematics.
    // using V = IR we can work out the current through that branch.

    float current_meas;

    if (voltage < 0){
        voltage = voltage*(-1); // will give the ABS value.
    }

    // V = IR -> I = V/R where V is the voltage drop from source to measuring point.

    current_meas = (150.0 - voltage)/200000.0; // Vdrop/200k ohm

    return current_meas;
}*/

/* USER CODE BEGIN FunctionPrototypes */
void main_thread(void *p){

	// DEV variable
	volatile float val_disp[11]={0};
	// DEV variable ends

	// Apply the first lens set-point
	if(LENS_VALUES_NUM_ARRAY_ITEMS != 0)
	{
	    lensVoltageIndex = 0;

	    US_Lens1_SetpointV = lens1Voltages[lensVoltageIndex];
	    US_Lens2_SetpointV = lens2Voltages[lensVoltageIndex];
	    US_Lens3_SetpointV = lens3Voltages[lensVoltageIndex];
	    US_Lens4_SetpointV = lens4Voltages[lensVoltageIndex];
	    US_Lens5_SetpointV = lens5Voltages[lensVoltageIndex];
	    US_Lens6_SetpointV = lens6Voltages[lensVoltageIndex];
	    US_Prefilt_SetpointV = prefiltVoltages[lensVoltageIndex];


	    LensControl_SetLensVoltage(US_LENS1_ENUM,   US_Lens1_SetpointV);
	    LensControl_SetLensVoltage(US_LENS2_ENUM,   US_Lens2_SetpointV);
	    LensControl_SetLensVoltage(US_LENS3_ENUM,   US_Lens3_SetpointV);
	    LensControl_SetLensVoltage(US_LENS4_ENUM,   US_Lens4_SetpointV);
	    LensControl_SetLensVoltage(US_LENS5_ENUM,   US_Lens5_SetpointV);
	    LensControl_SetLensVoltage(US_LENS6_ENUM,   US_Lens6_SetpointV);
	    LensControl_SetLensVoltage(US_PREFILT_ENUM, US_Prefilt_SetpointV);
	}


	while(1){
		 /* USER CODE END WHILE */
		// updates
		// Do not to worry about the suspicious semicolon, this will disappear once some action is added after the if statement

		din_update();

		// these three don't actually log any errors, They will in future.
		if (update_ain() != STATUS_OK);/* log an error in the future*/
		if (dout_update() != STATUS_OK);/* log an error in the future*/
		if (readAllAdcChannels() != STATUS_OK);/* log an error in the future*/

		/**** Down stream measurements ****/
		// 5 down stream lens + prefilter lens
		// 6 Upstream lenses + prefilter lens

		// ADC 2 - DS lens voltage measurement

		DS_Prefilt_ActualV = adc_getChannelActualValue(2,0);
		DS_Lens1_ActualV = adc_getChannelActualValue(2,2);
		DS_Lens2_ActualV = adc_getChannelActualValue(2,3);
		DS_Lens3_ActualV = adc_getChannelActualValue(2,4);
		DS_Lens4_ActualV = adc_getChannelActualValue(2,5);
		DS_Lens5_ActualV = adc_getChannelActualValue(2,6);
		DS_IGB_ActualV = adc_getChannelActualValue(1,7); // this is actually used to measure US_PF if US is connected to DS.

		// ADC 4 - DS lens current measurement
		// I = (|150| - |I_Lm|)/200k
		// I think is still measuring the voltage and not actually the current. There is no conversion to current. V = IR, I = Vdrop/R
		// function got made to convert to Amps.

		DS_Prefilt_ActualA = adc_getChannelActualValue(4,1);
		DS_Lens1_ActualA = adc_getChannelActualValue(4,3);
		DS_Lens2_ActualA = adc_getChannelActualValue(4,4);
		DS_Lens3_ActualA = adc_getChannelActualValue(4,5);
		DS_Lens4_ActualA = adc_getChannelActualValue(4,6);
		DS_Lens5_ActualA = adc_getChannelActualValue(4,7);
		DS_IGB_ActualA = adc_getChannelActualValue(4,0); // this is actually used to measure US_PF if US is connected to DS.




        // ADC3 - US lens current measurements
        US_Lens1_ActualA = adc_getChannelActualValue(3, 1);
        US_Lens2_ActualA = adc_getChannelActualValue(3, 2);
        US_Lens3_ActualA = adc_getChannelActualValue(3, 3);
        US_Lens4_ActualA = adc_getChannelActualValue(3, 4);
        US_Lens5_ActualA = adc_getChannelActualValue(3, 5);
        US_Lens6_ActualA = adc_getChannelActualValue(3, 7);
        US_Prefilt_ActualA = adc_getChannelActualValue(3, 6);


		//printf("%f \n", DS_Lens1_ActualV);

		/* every time the V200 starts up the lens 1 starts at 20V.
		When an ion is required the lens voltage is changed. Every time it starts at 20V :) */

/*
		while (19.0 <= DS_Lens1_ActualV && DS_Lens1_ActualV >= 21.0){ // because the accuracy isnt perfect we look at the range of ~20V +-1V
		    if (!scan_Started){ // if scan_Started = FALSE
		    //copy_lensV(); // this will copy all of the voltages read from DS and copy then to the US.
		    scan_Started = FALSE; // ensure
		    DS_Lens1_ActualV = adc_getChannelActualValue(2,2);
		    }
		}
		scan_Started = TRUE;
*/
/*
		// If the start lens voltages flag is set then start the lens voltage adjustment timer
		if (StartLensRamp == 1) // this is set on line 106 - set in this file.
		{
			SetStaticLensVotlages = 0;
			// Start the timer for lens ramping
		    if( xTimerIsTimerActive( lensVoltageChangerTimerHandle ) == pdFALSE )
			 {
		    	  xTimerStart(lensVoltageChangerTimerHandle, 0); // Do not wait to start timer
			 }
		}
		else if(StartLensRamp == 0)
		{
		    // Stop the timer to stop lens ramp
			 if( xTimerIsTimerActive( lensVoltageChangerTimerHandle ) != pdFALSE )
			 {
				xTimerStop(lensVoltageChangerTimerHandle, 0);
			 }
		}
*/

		// Check static lens setting flag
		// SetStaticLensVotlages has been set to 1 in the init variables.

		if(SetStaticLensVotlages == 1 ) // && scan_Started // Scan has started. ie. Lens 1 voltage is no longer 20V.
		{
			StartLensRamp = 0; // just to be safe.

			// Stop the ramping timer
			 if( xTimerIsTimerActive( lensVoltageChangerTimerHandle ) != pdFALSE )
			 {
				xTimerStop(lensVoltageChangerTimerHandle, 0);
			 }

			/* Setting The US lenses to the down stream input values. */
			// Lens one
			//US_Lens1_SetpointV = US_Lens1_StaticV;
			//LensControl_SetLensVoltage(US_LENS1_ENUM, US_Lens1_SetpointV);
            LensControl_SetLensVoltage(US_LENS1_ENUM,   DS_Lens1_ActualV); // Mirroring the Input from DS onto US as output.


			// Lens two
			//US_Lens2_SetpointV = US_Lens2_StaticV;
			//LensControl_SetLensVoltage(US_LENS2_ENUM,   US_Lens2_SetpointV);
            LensControl_SetLensVoltage(US_LENS2_ENUM,   DS_Lens2_ActualV);


			// Lens three
			//US_Lens3_SetpointV = US_Lens3_StaticV;
			//LensControl_SetLensVoltage(US_LENS3_ENUM,   US_Lens3_SetpointV);
            LensControl_SetLensVoltage(US_LENS3_ENUM,   DS_Lens3_ActualV);


			// Lens four
			//US_Lens4_SetpointV = US_Lens4_StaticV;
			//LensControl_SetLensVoltage(US_LENS4_ENUM,   US_Lens4_SetpointV);
            LensControl_SetLensVoltage(US_LENS4_ENUM,   DS_Lens4_ActualV);


			// Lens five
			//US_Lens5_SetpointV = US_Lens5_StaticV;
			//LensControl_SetLensVoltage(US_LENS5_ENUM,   US_Lens5_SetpointV);
            LensControl_SetLensVoltage(US_LENS5_ENUM,   DS_Lens5_ActualV);


			// Lens 6 does not get controlled from the D9 connector can't directly be mirrored with out Modification.
			// Lens six
			//US_Lens6_SetpointV = US_Lens6_StaticV;
			//LensControl_SetLensVoltage(US_LENS6_ENUM,   US_Lens6_SetpointV);
            // There is no DS_Lens6 - so we will not be able to mirror this guy


			// Lens pre-filter
			//US_Prefilt_SetpointV = US_Prefilt_StaticV;
			//LensControl_SetLensVoltage(US_PREFILT_ENUM, US_Prefilt_SetpointV);
            LensControl_SetLensVoltage(US_PREFILT_ENUM,   DS_IGB_ActualV); // this is because there is no lens6. The IGB maps to Prefilt pin on board.

		}

		// DEV test begins

		// pulse amp control
//		dout_setDout(1,1); // pulse testmode on
//		dout_setDout(0,1);// pulse on



		// test ADS8638 different functions
//	     testValue = adc_getChannelValue(3,2);//thats nA
//	     testValue = adc_IntegerChannelValuetoVolts(0,4,1);
//		 testValue = floatToFixedPointInt(adc_getChannelActualValue(0,channel_on_fly));//thats nA
//	     testValue2 =floatToFixedPointInt(adc_getChannelActualValue(0,channel_on_fly2));
//		 testValue3 =floatToFixedPointInt(adc_getChannelActualValue(0,channel_on_fly3));
//	     adc_setChannelandRange(0,5,0,0);


		// read DAC temperature from the ADS8638 chip 0 channels 4, 5 and 6
	     testValue = dac_getTemperature(2);
	     testValue2 = dac_getTemperature(1);
//	     testValue3 = dac_getTemperature(0);
//       calculate and ADS8638 chip temperature
//	     testValue = (testValue - 3777.2f)/0.47f;

//	     testValue = floatToFixedPointInt(adc_getChannelActualValue(0,3));//thats nA
//	     testValue2 = floatToFixedPointInt(adc_getChannelActualValue(0,4));//thats nA
	     testValue3 = floatToFixedPointInt(adc_getChannelActualValue(0,5));//thats nA


// STM ADC on board values check
		val_disp[0]=(AIN[0]/4095.0f)*(110/10)*2.5;//pulse voltage

		val_disp[1]=(AIN[1]/4095.0f)*2.5/(2)*100; //pulse current

		val_disp[2]=(AIN[2]/4095.0f)*((47+2.2)/2.2)*2.5; //+48V

		val_disp[3]=(AIN[3]/4095.0f)*(100/10)*2.5*(-1); //-12V

		val_disp[4]=(AIN[4]/4095.0f)*(100/10)*2.5*(-1); //-15V

		val_disp[5]=(AIN[5]/4095.0f)*(110/10)*2.5; //+15V

		val_disp[6]=(AIN[6]/4095.0f)*(110/10)*2.5;//+12V

		val_disp[7]=(AIN[7]/4095.0f)*((10+4.7)/4.7)*2.5; //6V8

		val_disp[8]=(AIN[8]/4095.0f)*((10+4.7)/4.7)*2.5; //5V

		val_disp[9]=(AIN[9]/4095.0f)*((10+10)/10)*2.5; //VDD

		val_disp[10]=30+(1.3-(AIN[10]/4095.0f)*2.5)*(1000/8.2); //TS4 30�C + (1.3V �ADCVoltage in volts)/(8.2mV/�C)

		// ADC 1 & 3 may need to be commented out for tidy code.
		// If DS (input from CF hub) is mapped correctly to the US then ADC 1 & ADC3 not needed


		/* Upstream measurements */

/*
		//these will not be needed yet. The Current will need to be measured later on. (Voltage - Nope.)

        // ADC1 - US lens voltage readings
        US_Lens1_ActualV = adc_getChannelActualValue(1, 0);
        US_Lens2_ActualV = adc_getChannelActualValue(1, 1);
        US_Lens3_ActualV = adc_getChannelActualValue(1, 2);
        US_Lens4_ActualV = adc_getChannelActualValue(1, 3);
        US_Lens5_ActualV = adc_getChannelActualValue(1, 4);
        US_Lens6_ActualV = adc_getChannelActualValue(1, 6);
        US_Prefilt_ActualV = adc_getChannelActualValue(1, 5);
*/


		// this can be used to increment though the set voltages in lenstestvalues.h
		// having a delay between each loop is good practice. - when incrementing index delay of a second is good.
        vTaskDelay(pdMS_TO_TICKS(1000)); // .01 second delay.
        //lensVoltageIndex++;

      }

}

void copy_lensV(){

    /* Setting The US lenses to the down stream input values. */

    LensControl_SetLensVoltage(US_LENS1_ENUM,   DS_Lens1_ActualV); // Mirroring the Input from DS onto US as output.
    LensControl_SetLensVoltage(US_LENS2_ENUM,   DS_Lens2_ActualV);
    LensControl_SetLensVoltage(US_LENS3_ENUM,   DS_Lens3_ActualV);
    LensControl_SetLensVoltage(US_LENS4_ENUM,   DS_Lens4_ActualV);
    LensControl_SetLensVoltage(US_LENS5_ENUM,   DS_Lens4_ActualV);
    LensControl_SetLensVoltage(US_PREFILT_ENUM,   DS_Prefilt_ActualV);

}

void delay_ms(int ms)
{
    // used to stall between change in v on lenses
    int32_t current;

    //last_time = current;

    /*while ((current - last_time) <= ms){
        current = pdMS_TO_TICKS();
    }*/
    vTaskDelay(pdMS_TO_TICKS(ms));

    // this will cycle through the voltage array of 20 length
}

void LensVoltageChange_Callback(TimerHandle_t xTimerHandle)
{
    lensVoltageIndex++;

    // Apply the first lens set-point
    if(lensVoltageIndex >= LENS_VALUES_NUM_ARRAY_ITEMS)
    {
        lensVoltageIndex = 0;
    }
    else if (restartLensVoltages)
    {
        lensVoltageIndex = 0;
        restartLensVoltages = 0;
    }

    /* Update the set-point variables for display in stm studio */
    // Check to see if the static lens voltage value is valid - if it is use that, otherwise default to automatic values.

    // Lens one
    if (LensControl_isValidSetpoint(lens1Voltages[lensVoltageIndex])){
        US_Lens1_SetpointV = lens1Voltages[lensVoltageIndex];
    }else{
        US_Lens1_SetpointV = US_Lens1_StaticV;
    }
    // Lens two
    if (LensControl_isValidSetpoint(lens2Voltages[lensVoltageIndex])){
        US_Lens2_SetpointV = lens2Voltages[lensVoltageIndex];
    }else{
        US_Lens2_SetpointV = US_Lens2_StaticV;
    }
    // Lens three
    if (LensControl_isValidSetpoint(lens3Voltages[lensVoltageIndex])){
        US_Lens3_SetpointV = lens3Voltages[lensVoltageIndex];
    }else{
        US_Lens3_SetpointV = US_Lens3_StaticV;
    }
    // Lens four
    if (LensControl_isValidSetpoint(lens4Voltages[lensVoltageIndex])){
        US_Lens4_SetpointV = lens4Voltages[lensVoltageIndex];
    }else{
        US_Lens4_SetpointV = US_Lens4_StaticV;
    }
    // Lens five
    if (LensControl_isValidSetpoint(lens5Voltages[lensVoltageIndex])){
        US_Lens5_SetpointV = lens5Voltages[lensVoltageIndex];
    }else{
        US_Lens5_SetpointV = US_Lens5_StaticV;
    }
    // Lens six
    if (LensControl_isValidSetpoint(lens6Voltages[lensVoltageIndex])){
        US_Lens6_SetpointV = lens6Voltages[lensVoltageIndex];
    }else{
        US_Lens6_SetpointV = US_Lens6_StaticV;
    }
    // Lens pre-filter
    if (LensControl_isValidSetpoint(prefiltVoltages[lensVoltageIndex])){
        US_Prefilt_SetpointV = prefiltVoltages[lensVoltageIndex];
    }else{
        US_Prefilt_SetpointV = US_Prefilt_StaticV;
    }

    // Set the lens voltages to their new setpoints

    //Ross' Note: This has been left as is at the moment because this sets voltages of US lenses. This will be needed.
    LensControl_SetLensVoltage(US_LENS1_ENUM,   US_Lens1_SetpointV); //*********************
    LensControl_SetLensVoltage(US_LENS2_ENUM,   US_Lens2_SetpointV);
    LensControl_SetLensVoltage(US_LENS3_ENUM,   US_Lens3_SetpointV);
    LensControl_SetLensVoltage(US_LENS4_ENUM,   US_Lens4_SetpointV);
    LensControl_SetLensVoltage(US_LENS5_ENUM,   US_Lens5_SetpointV);
    LensControl_SetLensVoltage(US_LENS6_ENUM,   US_Lens6_SetpointV);
    LensControl_SetLensVoltage(US_PREFILT_ENUM, US_Prefilt_SetpointV);

    delay_ms(2000); ; //***** I Added this in
}


/* USER CODE END FunctionPrototypes */

/* Hook prototypes */

/* Init FreeRTOS */

void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

	// initialize all  Douts, DACs and ADCs
	if (update_ain() != STATUS_OK);/* log an error in the future*/
	if (lens_dacsInit() != STATUS_OK);/* log an error in the future*/
	if (lens_adcInit() != STATUS_OK);/* log an error in the future*/


  /* USER CODE END Init */

	// DEV begins

   //  dout_setDout(3,1); // turn on the 15V circuit

//     dac_setRangeAllChannels(0,0x3);
//     dac_setRangeAllChannels(1,0x3);
//     dac_setRangeAllChannels(2,0x3);
	uint8_t dataB[2]="AB";
	uint8_t dataBRx[2]="";




//       lensTest(2500);
       lens_setVlensVoltageSupply(4095);

	    dac_setMuxOut(0,0x10);
	    dac_setMuxOut(1,0x10);
	    dac_setMuxOut(2,0x10);

	   // adc_setRange(0,7,3);

//    set range and voltage on a DAC and MUX it out for testing
//    dac_setRange(2,6,1);
//    dac_setAout(2,6,3000);
//    dac_setMuxOut(2,0x16);

	//DEV ends

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */

  // FUTURE:  two spare semaphores (not used)
  /* definition and creation of Generic_Sem0 */
  osSemaphoreDef(Generic_Sem0);
  Generic_Sem0Handle = osSemaphoreCreate(osSemaphore(Generic_Sem0), 1);

  /* definition and creation of Generic_Sem1 */
  osSemaphoreDef(Generic_Sem1);
  Generic_Sem1Handle = osSemaphoreCreate(osSemaphore(Generic_Sem1), 1);


  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
//  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
//  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  xTaskCreate(main_thread,(signed char*)"Main Thread",512,NULL,3,NULL);
//  xTaskCreate (canOpenProcessThread,"CanOpenProcess",configMINIMAL_STACK_SIZE,NULL,4,NULL);

  // Create and start the free-RTOS timer to callback at set intervals
  lensVoltageChangerTimerHandle = xTimerCreate ("LensVoltageChanger", pdMS_TO_TICKS((TIME_BETWEEN_VALUES_SEC * SEC_TO_MS)), pdTRUE, ( void * ) 0, LensVoltageChange_Callback);

  canopenthreads_createCanThreads(nodeId,  processThreadPriority, processWaitMs, dequeueWriteTimerMs,  heartbeatTimerMs,baudRatePrescaler);

  osKernelStart();

  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */
}


////////////////////////////////////////////////////////////Gurav end
/* USER CODE BEGIN Application */
     
/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

/**
  ******************************************************************************
  * File Name          : main.h
  * Description        : This file contains the common defines of the application
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H
  /* Includes ------------------------------------------------------------------*/

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private define ------------------------------------------------------------*/

#define OSC_EN_Pin GPIO_PIN_2
#define OSC_EN_GPIO_Port GPIOF
#define ADC3_IN9_VDD_Pin GPIO_PIN_3
#define ADC3_IN9_VDD_GPIO_Port GPIOF
#define ADC3_IN14_TempS4_Chamber_Pin GPIO_PIN_4
#define ADC3_IN14_TempS4_Chamber_GPIO_Port GPIOF
#define ADC3_IN7_6V8_Pin GPIO_PIN_9
#define ADC3_IN7_6V8_GPIO_Port GPIOF
#define ADC3_IN8_5V_Pin GPIO_PIN_10
#define ADC3_IN8_5V_GPIO_Port GPIOF
#define ADC2_IN10_48V_Pin GPIO_PIN_0
#define ADC2_IN10_48V_GPIO_Port GPIOC
#define ADC2_IN12_NEG_12_Pin GPIO_PIN_2
#define ADC2_IN12_NEG_12_GPIO_Port GPIOC
#define ADC2_IN13_NEG_15_Pin GPIO_PIN_3
#define ADC2_IN13_NEG_15_GPIO_Port GPIOC
#define DAC1_OUT_VLENS_SET_Pin GPIO_PIN_4
#define DAC1_OUT_VLENS_SET_GPIO_Port GPIOA
#define ADC2_IN6_PULSE_V_Pin GPIO_PIN_6
#define ADC2_IN6_PULSE_V_GPIO_Port GPIOA
#define ADC2_IN6_PULSE_I_Pin GPIO_PIN_7
#define ADC2_IN6_PULSE_I_GPIO_Port GPIOA
#define ADC2_IN14_12V_Pin GPIO_PIN_4
#define ADC2_IN14_12V_GPIO_Port GPIOC
#define ADC2_IN15_15V_Pin GPIO_PIN_5
#define ADC2_IN15_15V_GPIO_Port GPIOC
#define DAC_nCS1_Pin GPIO_PIN_0
#define DAC_nCS1_GPIO_Port GPIOG
#define DAC_nCS2_Pin GPIO_PIN_1
#define DAC_nCS2_GPIO_Port GPIOG
#define DAC_nCS0_Pin GPIO_PIN_11
#define DAC_nCS0_GPIO_Port GPIOE
#define SPI4_nLDAC_Pin GPIO_PIN_15
#define SPI4_nLDAC_GPIO_Port GPIOE
#define DIO_B12_TEST_MODE_Pin GPIO_PIN_12
#define DIO_B12_TEST_MODE_GPIO_Port GPIOB
#define DIO_B13_PULSE_ON_Pin GPIO_PIN_13
#define DIO_B13_PULSE_ON_GPIO_Port GPIOB
#define DIO_B14_DOUT_GUAGE_Pin GPIO_PIN_14
#define DIO_B14_DOUT_GUAGE_GPIO_Port GPIOB
#define DIO_B15_DIN_GUAGE_Pin GPIO_PIN_15
#define DIO_B15_DIN_GUAGE_GPIO_Port GPIOB
#define nCS_ADC1_Pin GPIO_PIN_4
#define nCS_ADC1_GPIO_Port GPIOG
#define nCS_ADC2_Pin GPIO_PIN_5
#define nCS_ADC2_GPIO_Port GPIOG
#define nCS_ADC3_Pin GPIO_PIN_6
#define nCS_ADC3_GPIO_Port GPIOG
#define nCS_ADC4_Pin GPIO_PIN_7
#define nCS_ADC4_GPIO_Port GPIOG
#define nCS_ADC0_Pin GPIO_PIN_8
#define nCS_ADC0_GPIO_Port GPIOG
#define EN_15V_Pin GPIO_PIN_12
#define EN_15V_GPIO_Port GPIOA
#define CAN1_nSTB_Pin GPIO_PIN_2
#define CAN1_nSTB_GPIO_Port GPIOD
#define ADC_EN_Pin GPIO_PIN_15
#define ADC_EN_GPIO_Port GPIOG
#define CAN2_nSTB_Pin GPIO_PIN_7
#define CAN2_nSTB_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

/**
  * @}
  */ 

/**
  * @}
*/ 

#endif /* __MAIN_H */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

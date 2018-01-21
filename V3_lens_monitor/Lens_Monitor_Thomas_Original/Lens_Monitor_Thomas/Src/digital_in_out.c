/*
 * digital_in_out.c
 *
 *  Created on: 4/05/2017
 *      Author: Esam.Alzqhoul
 */

#include "global.h"
#define NUMBER_OF_DINS 1
#define NUMBER_OF_DOUTS 4

/* below are defined are copied from the main.h file, added them here for reference only
 DOUTS
#define DIO_B12_TEST_MODE_Pin GPIO_PIN_12
#define DIO_B12_TEST_MODE_GPIO_Port GPIOB

#define DIO_B13_PULSE_ON_Pin GPIO_PIN_13
#define DIO_B13_PULSE_ON_GPIO_Port GPIOB

#define DIO_B14_DOUT_GUAGE_Pin GPIO_PIN_14
#define DIO_B14_DOUT_GUAGE_GPIO_Port GPIOB

#define EN_15V_Pin GPIO_PIN_12
#define EN_15V_GPIO_Port GPIOA

 DINs
#define DIO_B15_DIN_GUAGE_Pin GPIO_PIN_15
#define DIO_B15_DIN_GUAGE_GPIO_Port GPIOB
*/

static volatile int DINS[ NUMBER_OF_DINS ];
static volatile int DOUTS[ NUMBER_OF_DOUTS ];

// Digital inputs

int din_get ( int index )
{
	if(index >= NUMBER_OF_DINS)return 0;

	return DINS[ index ];
}

// Read required digital inputs
void din_update( void )
{
	// read all the digital pins
	// Digital input on the guage connector.
	// Important Note: this DIN read is not physically connected (see schematics of Lens Controller) to the actual guage but rather for future use if the connector to be used for something else
	DINS[0]= HAL_GPIO_ReadPin(GPIOB, DIO_B15_DIN_GUAGE_Pin);

}

// Digital outputs

/*
 */

// initialize all douts to zero for this controller
StatusCode_t dout_init( void )
{
	// switch OFF the "test mode" on the pulse circuit
	HAL_GPIO_WritePin(GPIOB, DIO_B12_TEST_MODE_Pin,GPIO_PIN_RESET);
	DOUTS[0]= HAL_GPIO_ReadPin(GPIOB, DIO_B12_TEST_MODE_Pin);

	// switch OFF the "pulse mode" on the pulse circuit , reversed logic here (ACTIVE LOW) (see schematics )
	HAL_GPIO_WritePin(GPIOB, DIO_B13_PULSE_ON_Pin,GPIO_PIN_SET);
	DOUTS[1]= HAL_GPIO_ReadPin(GPIOB, DIO_B13_PULSE_ON_Pin);

	// switch OFF the "DIO_B14_DOUT_GUAGE_Pin" on the guage RJ45 connector.
	// Important Note: this DOUT is not connected to the actual guage, but rather for future use if the connector to be used for something else

	HAL_GPIO_WritePin(GPIOB, DIO_B14_DOUT_GUAGE_Pin,GPIO_PIN_RESET); //Active low
	DOUTS[2]= HAL_GPIO_ReadPin(GPIOB, DIO_B14_DOUT_GUAGE_Pin);

	// turn off the 15V power supply
	HAL_GPIO_WritePin(GPIOA, EN_15V_Pin,GPIO_PIN_RESET);
	DOUTS[3]= HAL_GPIO_ReadPin(GPIOA, EN_15V_Pin);

	return STATUS_OK;

}

// set DOUTS test mode, pulse mode or DIO_B14_DOUT_GUAGE_Pin
char* dout_setDout( int index, int value )
{
	GPIO_PinState PinState;

    if(index > NUMBER_OF_DOUTS)return "BAD INDEX";
	if(value != 0 && value != 1) return "Value Must Be Binary";

	switch(value)
	{
	case 0:PinState = GPIO_PIN_RESET;break;
	case 1:PinState = GPIO_PIN_SET;break;
	}

	switch(index)
	{
	case 0:HAL_GPIO_WritePin(GPIOB, DIO_B12_TEST_MODE_Pin,PinState);break;
	case 1:HAL_GPIO_WritePin(GPIOB, DIO_B13_PULSE_ON_Pin,!PinState);break; // Active low
	case 2:HAL_GPIO_WritePin(GPIOB, DIO_B14_DOUT_GUAGE_Pin,!PinState);break;// optional at this stage  but it is Active low
	case 3:HAL_GPIO_WritePin(GPIOA, EN_15V_Pin,PinState);break;// optional at this stage
	}
	//read and update all douts now
	DOUTS[0]= HAL_GPIO_ReadPin(GPIOB, DIO_B12_TEST_MODE_Pin);
	DOUTS[1]= !HAL_GPIO_ReadPin(GPIOB, DIO_B13_PULSE_ON_Pin); // reversed logic
	DOUTS[2]= !HAL_GPIO_ReadPin(GPIOB, DIO_B14_DOUT_GUAGE_Pin);// reversed logic
	DOUTS[3]= HAL_GPIO_ReadPin(GPIOA, EN_15V_Pin);


	return "Ok\n";
}

// get a Dout pin state
int dout_get ( int index )
{
	if(index >= NUMBER_OF_DOUTS)return 0;

	switch(index)
	{
	case 0: DOUTS[0]= HAL_GPIO_ReadPin(GPIOB, DIO_B12_TEST_MODE_Pin);break;
	case 1 :DOUTS[1]= HAL_GPIO_ReadPin(GPIOB, DIO_B13_PULSE_ON_Pin);break;
	case 2: DOUTS[2]= HAL_GPIO_ReadPin(GPIOB, DIO_B15_DIN_GUAGE_Pin);break;
	case 3: DOUTS[3]= HAL_GPIO_ReadPin(GPIOA, EN_15V_Pin);break;
	}

	return DOUTS[ index ];
}

// read all DOUTS status. this update is optional as we update DOUTS anyway when we set them to 0 or 1
StatusCode_t dout_update( void )
{

	DOUTS[0]= HAL_GPIO_ReadPin(GPIOB, DIO_B12_TEST_MODE_Pin);
	DOUTS[1]= HAL_GPIO_ReadPin(GPIOB, DIO_B13_PULSE_ON_Pin);
	DOUTS[2]= HAL_GPIO_ReadPin(GPIOB, DIO_B15_DIN_GUAGE_Pin);
	DOUTS[3]= HAL_GPIO_ReadPin(GPIOA, EN_15V_Pin);

	return STATUS_OK;

}

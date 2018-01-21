/*
 * dac_out.c
 *
 *  Created on: 1/05/2017
 *      Author: Esam.Alzqhoul
 */

/*General notes on this driver
 * This is an SPI driver for the DAC LTC2664-12bit chip to provide all the necessary set/get commands to adjust the voltage settings of  QUAD PSU. The MICROWAVE,HV PSU AND ION GUIDE are controlled using the STM32F7 12bit DAC
 * Commands are 32 bits and first 8 bits are do not care, so actual command is a 24bits
 * Commands are always numbers and not Chars
 *
 * Note: -one difference between the LTC2664-12bit and LTC2664-16bit drivers is that the value for dac voltages is shifter to the left by 4 binary digits in the
 *        case of the 12bit driver.
 *       - always make sure to change the chip select and LDAC pins in this driver (down at the bottom) depending on the board design
 *       - following the second point, look for anything with HAL and make sure the pins are matching the schematics
 *		 - SPI configurations are in spi.c, clock speed 48MHz/4
 *
 */

/*
 * terminology
 * write:  write range or value to input registers
 * update: copies input register into DAC register and powers up the DAC
 * set: contains write and update commands to configure and change value on the DAC output
 */


/* channels available for LTC2664, 12bits and their corresponding code
 * Binary   Hex  DAC Channel
   0 0 0 0  0x0   DAC 0
   0 0 0 1  0x1   DAC 1
   0 0 1 0  0x2   DAC 2
   0 0 1 1  0x3   DAC 3
   0 1 0 0  0x4   DAC 4
   0 1 0 1  0x5   DAC 5
   0 1 1 0  0x6   DAC 6
   0 1 1 1  0x7   DAC 7
 */

/*
 * ranges and span selection
   MSP2 MSP1 MSP0 Hex
   0    0    0    0x0    0V to 5v
   0    0    1    0x1    0V to 10V
   0    1    0    0x2    �5V
   0    1    1    0x3    �10V
   1    0    0    0x4    �2.5V
 *
  As with the double-buffered code registers, update operations
  copy the span input registers to the associated span
  DAC registers.
 */


/*
 * Five registers
 *   one shift register contains the 32 bit word from the SDI, two DAC and input reg for code data, two DAC and input reg for span range
 *   Types of registers:
 * - Input registers: are holding registers and do not affect the output of the DAC. Two registers A(default) and B(only used in toggle)
 *   Write operation shift data from SPI pin to these registers
 * - DAC register:Input register value is copied into DAC register. This directly controls the output if the chip
 *   update operation copies input register into DAC register (both range and value) and power up the DAC as well
 * - A channel update can come from a serial update command, an LDAC negative pulse, or a toggle operation.
 *
 */

/*
 * side note: Cool feature, One instruction to control multiple DAC units at once!!
 * 	The SDO output can be used to facilitate control of multiple
	serial devices from a single 3-wire serial port (i.e., SCK,
	SDI and CS/LD). Such a daisy-chain series is configured
	by connecting the SDO of each upstream device to the SDI
	of the next device in the chain. However, In use, CS/LD is first taken low.
 */

/*Powering down channels
 * power-down any unused DacsLtcValue to reduce power consumption  by using command 0100b
 * Normal operation resumes by executing any set command
   which includes a DAC update
 */

/*
 *  Note:attention need to be paid to HAL functions that calls different peripherals which might be different for another board
 *  In such cases, all we need is just to change the PIN or peripheral number. For example SPI1 to SPI4 ...etc
 *
 */


# include "global.h"

// on this board total is 6 DAC channels as follows:
// 4 DacsLtcValue LTC2664
// 2 on the STM32F7

#define LTC_DAC_CHANNELS 8 // change this to 4 for RF and DC
#define STM_DAC_CHANNELS 1 // This is not required for the Lens Controller

// only 1 LTC DAC Chip on this circuit, change this to Total number of DACs on the Lens controller 3. Current design all DACS on same SPI bus so this driver should work with changes in the comments
#define MAX_LTC_DAC_CHIPS 3

// 1ms as per the freeRTOS ticks setting, which is 1 tick for 1ms at the moment. To make it faster change "#define configTICK_RATE_HZ" in FreeRtosConfig.h from 1000 to 10000 this will make it a 100Us for a tick
#define STEP_TIME 1
#define STEP_SIZE_LTC_DAC 0
#define STEP_SIZE_STM_DAC 0


// read from register A on the DAC by default. You may never need to change that. This value can be either 0--> register A or 1---> register B
#define DEFAULT_TOGGLE_REGISTER 0


#define STM_DAC_CHANNEL1 0 // This is not required for the Lens Controller
#define STM_DAC_CHANNEL2 1 // This is not required for the Lens Controller

// define zero reference values
#define BIPOLAR_RANGES_ZERO_REF 2048
#define UNIPOLAR_RANGES_ZERO_REF 0

// define an STM DAC array to hold values for the STM (on chip DACs)
struct STMdacInfo
{
	uint16_t DacStmValue[STM_DAC_CHANNELS]; // this holds all STM DAC values
	uint16_t dacStepSize[STM_DAC_CHANNELS];
};

// create multiple LTC chips
struct STMdacInfo stmDac;


// ltcDacChips will hold value and range for each of its channels
struct LTCdacInfo
{
	uint16_t DacsLtcValue[LTC_DAC_CHANNELS]; // this holds all DAC values
	uint16_t DacsLtcRange[LTC_DAC_CHANNELS]; // this hold all DAC ranges (voltage spans), range can only be changed on the DAC chip, on STM32F7 its always 0-2.5V
	uint16_t DacsDefaultRegVALUE[LTC_DAC_CHANNELS];
	uint16_t dacStepSize[LTC_DAC_CHANNELS];
};

// initialize all DAC step sizes to zero
// create multiple LTC chips
struct LTCdacInfo ltcDacChips[MAX_LTC_DAC_CHIPS];

// command list reference
 struct dacCmdCodeInfo{
 	uint8_t cmdCode;
 	char* description;
 };

 // MuxOut list of codes
 struct  MuxOutInfo{
  	uint8_t MuxPinOutput;
  	char* description;
  };

 struct  DacRangeInfo{
  	uint8_t DacRange;
  	int DacRangeMin;
  	int DacRangeMax;
  	char* description;
  };

 // - if a new version of this chip is released with a modified software, make sure all the affected commands codes are changed throughout this driver
 // - commands for the 12bit chip are similar to the 16bit ones

 //  List of command and brief description,  this List is for our reference only! and summarizes the data sheet commands, not used as part of coding
 struct dacCmdCodeInfo cmdCodeDataList[] = {
		 { 0x0, "Write Code to DAC n" }                                                                                   ,
//		 { 0x8, "Write Code to All DACS_LTC" }                                                                                ,
		 { 0x6, "Write Span to DAC n and change its range" }                                                              ,
		 { 0xE, "Write Span to All DacsLtcValue and change their range" }                                                         ,
		 { 0x1, "Update and Power Up DAC n***" }                                                                          ,
		 { 0x9, "Update and Power Up All DacsLtcValue***" }                                                                       ,
		 { 0x3, "Write Code to DAC n, Update DAC n (Power Up)" }                                                          ,
//		 { 0x2, "Write Code to DAC  n, Update All DACS_LTC (Power Up)" }                                                      ,
		 { 0xA, "Write Code to All DACS_LTC, Update All DACS_LTC (Power Up)" }                                                    ,
		 { 0x4, "Power Down DAC n, this is ignored if LDAC is low***" }                                                   ,
		 { 0x5, "Power Down Chip (All DacsLtcValue, Mux and Reference), ignored if LDAC is low***" }                              ,
		 { 0xB, "Analog Mux Enable to measure internal voltages or ones connected to MSP" }                               ,
		 { 0xC, "Toggle Select, updates DAC n from register A on rising edge and from register B on falling edge" }       , // see note below
		 { 0xD, "Global Toggle, to update all DacsLtcValue from their A or B registers on rising and falling edges respectively" },
//		 { 0x7, "Config" }                                                                                                ,
//		 { 0xF, "Reserved code,  No Operation" }                                                                          ,
 };

 // *** these commands will ignore the value, as they will be treated as dont care bits


 // This list is  different for the Lens controller update it from the LTC2666 for the lens controller page 19
 struct MuxOutInfo MuxOutList[] = {
			{    0x00, " Disabled (Hi-Z)            " } ,
			{    0x10 , " VOUT0  "    }                 ,
			{    0x11 , " VOUT1     " }                 ,
			{    0x12 , " VOUT2     " }                 ,
			{    0x13 , " VOUT3     " }                 ,
			{    0x14 , " VOUT4     " }                 ,
			{    0x15 , " VOUT5     " }                 ,
			{    0x16 , " VOUT6     " }                 ,
			{    0x17 , " VOUT7     " }                 ,
			{    0x18 , " REFLO     " }                 ,
			{    0x19 , " REF     " }                   ,
			{    0x1A , " Temperature Monitor     " }   , // Formula to calculate temp TJ = 25�C + (1.4V �VMUXOUT)/(3.7mV/�C)
			{    0x1B , " V+  +12V    " }               ,
			{    0x1C , " V�  -12V  " }                 ,

 };

 // A list that contains all the ranges on the LTC2666-12bit 8 channels.
 struct DacRangeInfo DacRangeList[] = {

        {    0x0, 0   , 5   ," Range is set to 0-5V      " },
 		{    0x1, 0   , 10  ," Range is set to 0-10V     " },
 		{    0x2, -5  , 5   ," Range is set to �5V       " },
 		{    0x3, -10 , 10  ," Range is set to �10V      " },
 		{    0x4,-2.5 , 2.5 ," Range is set to �2.5V     " },


 };



 // this set command can be used to form commands for all those in the cmdCodeDataList above except the commented ones.
char* dac_setCmdGeneric(uint8_t dacNumber, uint8_t channelNumber, uint8_t commandWord, uint32_t value)
{
//	DacsLtcValue[channelNumber % LTC_DAC_CHANNELS] = value;
	if(dacNumber >= MAX_LTC_DAC_CHIPS)return "BAD DAC NUMBER";

	uint16_t combinedCmdAddr;

    uint8_t data_array[4]={0};

	// shift addressWord to left by 4 bits then add to commandWord
	//
	combinedCmdAddr=(commandWord << 4)+ channelNumber;

    SPI_transmit(dacNumber, combinedCmdAddr,value);

	return "Ok\n";
}


// IM PRETTY SURE THIS WILL BE UNIVERSAL FOR EACH DAC.
// set value and update out put immediately
char* dac_setAout(uint8_t dacNumber,uint8_t channelNumber, uint32_t value)
{
	// this is needed for the 12 bit DAC only
	uint32_t value_shifted = value << 4;

	if(dacNumber >= MAX_LTC_DAC_CHIPS)return "BAD DAC NUMBER";
	// code for this command
	uint8_t  commandWord = 0x3;

	// form a command and send it
	// ramp up/down to the desired voltage
	voltageLtcDacRamp(dacNumber,channelNumber,value_shifted,commandWord);
    //	comment voltageLtcDacRamp and use the line below for no ramping
    //	dac_setCmdGeneric(dacNumber,channelNumber,commandWord,value);

    // update DAC values
	ltcDacChips[dacNumber].DacsLtcValue[channelNumber % LTC_DAC_CHANNELS] = value_shifted;

	return "Ok\n";
}



//set value but do not display output, this will send a value to the DAC register but does not reflect it on the output
char* dac_setAoutNoUpdate(uint8_t dacNumber,uint8_t channelNumber, uint32_t value)
{
	if(dacNumber >= MAX_LTC_DAC_CHIPS)return "BAD DAC NUMBER";

	// code for this command
	uint8_t  commandWord = 0x0;

	// this is needed for the 12 bit DAC only
	uint32_t value_shifted = value << 4;

	// form a command and send it
    dac_setCmdGeneric(dacNumber,channelNumber,commandWord,value_shifted);

    // update DAC register value
	ltcDacChips[dacNumber].DacsDefaultRegVALUE[channelNumber % LTC_DAC_CHANNELS] = value_shifted;
	return "Ok\n";
}



// set value on all channels and update the ouptuts
char* dac_setAoutAllChannels(uint8_t dacNumber,uint32_t value)
{
	if(dacNumber >= MAX_LTC_DAC_CHIPS)return "BAD DAC NUMBER";

	// code for this command
	uint8_t  commandWord = 0xA;

	// this is needed for the 12 bit DAC only
	uint32_t value_shifted = value << 4;

	// form a command and send it, channel number is ignored set it to 0
    	dac_setCmdGeneric(dacNumber,0,commandWord,value_shifted);

    // now update DAC values
	for( int ii = 0; ii < LTC_DAC_CHANNELS; ltcDacChips[dacNumber].DacsLtcValue[ii++] = value_shifted );

	return "Ok\n";
}

// get current set value
int dac_getAout( uint8_t dacNumber,uint8_t channelNumber )
{
	if(dacNumber >= MAX_LTC_DAC_CHIPS)return 0;

	return ltcDacChips[dacNumber].DacsLtcValue[ channelNumber % LTC_DAC_CHANNELS ];
}



// set channel voltage span (range)
char* dac_setRange(uint8_t dacNumber,uint8_t channelNumber, uint32_t value)
{
	/*
	 * ranges and span selection
	   MSP2 MSP1 MSP0 Hex
	   0    0    0    0x0    0V to 5v
	   0    0    1    0x1    0V to 10V
	   0    1    0    0x2    �5V
	   0    1    1    0x3    �10V
	   1    0    0    0x4    �2.5V
	 *
	  As with the double-buffered code registers, update operations
	  copy the span input registers to the associated span
	  DAC registers.
	 */

	if(dacNumber >= MAX_LTC_DAC_CHIPS)return "BAD DAC NUMBER";

	uint8_t  commandWord = 0x6;
	// update the range value
	ltcDacChips[dacNumber].DacsLtcRange[channelNumber % LTC_DAC_CHANNELS] = value;
	// form a command and send it
	dac_setCmdGeneric(dacNumber, channelNumber,commandWord,value);

//	// update the DAC with the new range un-comment below if you like
//	commandWord = 0x1;
//	dac_setCmdGeneric(channelNumber,commandWord,value);

	return "Ok\n";
}



// set all channels to the same voltage span (range)
char* dac_setRangeAllChannels(uint8_t dacNumber, uint32_t value)
{

	/*
	 * ranges and span selection
	   MSP2 MSP1 MSP0 Hex
	   0    0    0    0x0    0V to 5v
	   0    0    1    0x1    0V to 10V
	   0    1    0    0x2    �5V
	   0    1    1    0x3    �10V
	   1    0    0    0x4    �2.5V
	 *
	  As with the double-buffered code registers, update operations
	  copy the span input registers to the associated span
	  DAC registers.
	 */
	if(dacNumber >= MAX_LTC_DAC_CHIPS)return "BAD DAC NUMBER";

	uint8_t  commandWord = 0xE;
	// set all channels to the new range
	for( int ii = 0; ii < LTC_DAC_CHANNELS; ltcDacChips[dacNumber].DacsLtcRange[ii++] = value );

	// form a command and send,	channelNumber here is do not care so we set it to 0
	dac_setCmdGeneric(dacNumber,0,commandWord,value);

//	update the DAC with the new range
//	commandWord = 0x1;
//	dac_setCmdGeneric(channelNumber,commandWord,value);

	return "Ok\n";
}


// get current range
int dac_getRange( uint8_t dacNumber,uint8_t channelNumber )
{
	if(dacNumber >= MAX_LTC_DAC_CHIPS)return 0;

	return ltcDacChips[dacNumber].DacsLtcRange[ channelNumber % LTC_DAC_CHANNELS ];
}



// update a channel and power it up
char* dac_UpdateChannel(uint8_t dacNumber, uint8_t channelNumber)
{
	if(dacNumber >= MAX_LTC_DAC_CHIPS)return "BAD DAC NUMBER";

	uint8_t  commandWord = 0x1;

	// ramp up/down to the desired voltage, value here is the register value
	voltageLtcDacRamp(dacNumber,channelNumber,ltcDacChips[dacNumber].DacsDefaultRegVALUE[channelNumber % LTC_DAC_CHANNELS],commandWord);
    //	comment voltageLtcDacRamp and use the line below for no ramping
	//  value is ignored set it to 0
	//	dac_setCmdGeneric(dacNumber,channelNumber,commandWord,0);

	// since we updated the channel we can copy the register value to the actual value
	ltcDacChips[dacNumber].DacsLtcValue[channelNumber % LTC_DAC_CHANNELS] = ltcDacChips[dacNumber].DacsDefaultRegVALUE[channelNumber % LTC_DAC_CHANNELS];
	return "Ok\n";
}



// Update and Power Up All channels on the lTC chip
// Important Note: it is not possible to ramp up or down when using update all as we do not have the previllage to access each channel
// seperately and compare the register value to the previous value
char* dac_UpdateAllChannels(uint8_t dacNumber)
{
	uint8_t  commandWord = 0x9;

	if(dacNumber >= MAX_LTC_DAC_CHIPS)return "BAD DAC NUMBER";

    // channel number and value are ignored set it to 0
		dac_setCmdGeneric(dacNumber,0,commandWord,0);

    // copy all register value to the actual value

	for( int ii = 0; ii < LTC_DAC_CHANNELS; ltcDacChips[dacNumber].DacsLtcValue[ii++] = ltcDacChips[dacNumber].DacsDefaultRegVALUE[ii % LTC_DAC_CHANNELS] );

	return "Ok\n";
}



// Power down DAC channel N, to power send a set command to any channel
char* dac_powerDownChannel(uint8_t dacNumber, uint8_t channelNumber)
{

	uint8_t  commandWord = 0x4;

	if(dacNumber >= MAX_LTC_DAC_CHIPS)return "BAD DAC NUMBER";

// value is ignored here set it to 0
	dac_setCmdGeneric(dacNumber,channelNumber,commandWord,0);

	return "Ok\n";
}



// Power down the whole chip, to power send a set command to any channel
// if multiple DacsLtcValue in future we will need to do something here instead of void
char* dac_powerDownChip(uint8_t dacNumber)
{
	uint8_t  commandWord = 0x5;

	if(dacNumber >= MAX_LTC_DAC_CHIPS)return "BAD DAC NUMBER";

// value and channel number ignored, set them to zero or any value
	dac_setCmdGeneric(dacNumber,0,commandWord,0);

	return "Ok\n";
}



// disable/enable the thermal protection
char* dac_setThermalProtection(uint8_t dacNumber,uint32_t value)
{
	uint16_t combinedCmdAddr;
	uint8_t  commandWord = 0x7;
	uint16_t value_shifted;

	if(dacNumber >= MAX_LTC_DAC_CHIPS)return "BAD DAC NUMBER";

	// value is a binary number here 0-->  means thermal protection at 160 C enabled(default) (see TS bit in config  data sheet)
	//                               1-->  means thermal protection at 160 C disabled (see TS bit in config  data sheet)
	// shift addressWord to left by 4 bits so they become MSBs
	//
	if(value != 0 && value != 1) return "Value Must Be Binary";

	// shift value by one bit first
	value_shifted = value << 1;

	combinedCmdAddr=(commandWord << 4);

	// transmit to chip
    SPI_transmit(dacNumber,combinedCmdAddr,value_shifted);

	return "Ok\n";
}



char* dac_setMuxOut(uint8_t dacNumber, uint16_t MuxOutChannel)
{
	//channelNumber not used here just, it is kept here just to keep structure uniform for dictionary
	uint16_t combinedCmdAddr;
	uint16_t value=MuxOutChannel; // this is ignored anyway in this command, see LTC2664 datasheet for MuxOut
	uint8_t commandWord=0xB;
	bool channelFound=0;

	if(dacNumber >= MAX_LTC_DAC_CHIPS)return "BAD DAC NUMBER";

	// check if the passed MuxOutChannel is a readable channel
	for(uint8_t ii=0 ;ii<(sizeof(MuxOutList)/sizeof(MuxOutList[0]));ii++)
		{
			if(MuxOutChannel==MuxOutList[ii].MuxPinOutput) channelFound=1;

		}

	if(channelFound == 0) return " BAD MUXOUT CHANNEL";
	// shift addressWord to left by 4 bits then add to commandWord
	//
	combinedCmdAddr=(commandWord << 4);


	// transmit to chip
    SPI_transmit(dacNumber,combinedCmdAddr,value);

	return "Ok\n";
}



// get temperature of the chip.
// Note: - This is different to the way we read temp in RF and DC as here we are using the ads8638 to read the muxout0,1 and 2, where is in the RF and DC we are using an adc on the stm32
//       - All the MUXOUTS are connected to ADS8638 chip 0.
int dac_getTemperature(uint8_t dacNumber)
{
	volatile int TJ;
	volatile float VMUXOUT;

	// 0x1A is the channel that relates to Temperature read
	uint16_t MuxOutChannel=0x1A;

	if(dacNumber >= MAX_LTC_DAC_CHIPS)return 0;

    dac_setMuxOut(dacNumber,MuxOutChannel);
    // read the analogue voltage
    // FUTURE: only 3 muxouts are connected to ADS 0 in this design. If anything added add channel reads in the same way below
		switch(dacNumber)
		{
		case 0:adc_setChannelandRange(0,4,0,0);VMUXOUT = adc_IntegerChannelValuetoVolts(0,4,1);break;
		case 1:adc_setChannelandRange(0,5,0,0);VMUXOUT = adc_IntegerChannelValuetoVolts(0,5,1);break;
		case 2:adc_setChannelandRange(0,6,0,0);VMUXOUT = adc_IntegerChannelValuetoVolts(0,6,1);break;
		default:break;
		}

  // calculate Temperature as per data sheet
  //  TJ = 25�C + (1.4V � VMUXOUT)/(3.7mV/�C)
    TJ = 25 + (1.4-VMUXOUT)/(3.7/1000);

	return TJ;
}


// TOGGLE is an advanced feature, added here for future use if needed and made accessible in dictionary

// Toggle note:
/*
 * T0, T1, T2, T3
 *  each toggle-select bit controls which input register
	(A or B) to receive data from in a write-code operation. When
	the toggle select bit of a given channel is high, write-code
	operations are directed to input register B of the addressed
	channel. When the bit is low, write-code operations are
	directed to input register A.
	Secondly, each toggle select bit enables the corresponding
	channel for a toggle operation.
	e.g. 0xC(16 bits do not care)T0, T1, T2, T3
 */

//Toggle Select, updates DAC n from register A or B based on the dac_toggleRegisters
/*
 * example how to use :
 * For example, to set up channel 3 to
   toggle between codes 4096 and 4200:
 * 1) Write code channel 3 (code = 4096) to register A
		00000011 00010000 00000000
   2) Toggle Select (set bit T3)
		11000000 00000000 00001000
   3) Write code channel 3 (code = 4200) to register B
		00000011 00010000 01101000
   4) update channel3 using serial command 0x1
   		00010011 00010000 01101000

 */

/*
 * important: the chip is not responding to toggle commands, the syntax is debugged and it is correct. However, this functionality is not really necessary anyway
 * Contact linear for support if its ever needed, driver is ready to use for it.
 */




// chose to read from register A or B, A is the default
char* dac_toggleRegisters(uint8_t dacNumber, uint8_t channelNumber,  uint32_t value)
{
	uint16_t combinedCmdAddr;
	uint16_t value_shifted;
	uint8_t  commandWord = 0xC;
	// value is a binary number here 0-->  means read data and span from register A of the DAC
	//                               1-->  means read data and span from register B of the DAC

	// shift addressWord to left by 4 bits so they become MSBs
	//
//	if(value != 0 && value != 1) return "Value Must Be Binary";
	if(dacNumber >= MAX_LTC_DAC_CHIPS)return "BAD DAC NUMBER";

	combinedCmdAddr=(commandWord << 4);

	// shift value to left by channelNumber digits
	// e.g. value of channel 3 is 1 to read from register B. Shift 1 to left by 3 digits so, 0x000...1000
	value_shifted = value << channelNumber;
	// transmit to chip
    SPI_transmit(dacNumber, combinedCmdAddr,value_shifted);

	return "Ok\n";
}



// Global Toggle, to update all DacsLtcValue from their A or B registers on rising and falling edges respectively
// Unlikely to be used
char* dac_globalToggleRegisters(uint8_t dacNumber, uint32_t value)
{
	uint16_t combinedCmdAddr;
	uint8_t  commandWord = 0xD;

	if(dacNumber >= MAX_LTC_DAC_CHIPS)return "BAD DAC NUMBER";

	// value is a binary number here 0-->  means read data and span from register A of the DAC
	//                               1-->  means read data and span from register B of the DAC

	// shift addressWord to left by 4 bits so they become MSBs
	//
	if(value != 0 && value != 1) return "Value Must Be Binary";

	combinedCmdAddr=(commandWord << 4);

	// transmit to chip
    SPI_transmit(dacNumber, combinedCmdAddr,value);

	return "Ok\n";
}



// set reference to any external voltage connected to Pin REF, not possible in current design
//char* dac_setExternalRefVoltage( uint32_t value)
//{
//	uint16_t combinedCmdAddr;
//	uint8_t  commandWord = 0x7;
//	// value is a binary number here 0-->  means external voltage connected to REF PIN disabled (see RD bit in data sheet)
//	//                               1-->  means external voltage connected to REF PIN Enabled  (see RD bit in data sheet)
//
//	// shift addressWord to left by 4 bits so they become MSBs
//	//
//	if(value != 0 || value != 1) return "Value Must Be Binary";
//
//	combinedCmdAddr=(commandWord << 4);
//
//	// transmit to chip
//    SPI_transmit(combinedCmdAddr,value);
//
//	return "Ok\n";
//}

// transmit on SPI to the selected chip
char* SPI_transmit(uint8_t dacNumber, uint8_t combinedCmdAddr, uint16_t value)
{
	 // Four bytes Array the holds the 32 bit command for the DAC
      uint8_t data_array[4]={0};

  	if(dacNumber >= MAX_LTC_DAC_CHIPS)return "BAD DAC NUMBER";


	// pull the corresponding chip select pin nCS to low to start transmission on the selected DAC
    // and then pull high to execute the command
  	// set nCs on all chips
  	for(int ii = 0;ii<MAX_LTC_DAC_CHIPS; ii++)dac_nCs_set(ii,GPIO_PIN_SET);
  	//now reset the desired chip to transmit
  	      dac_nCs_set(dacNumber,GPIO_PIN_RESET);



		  data_array[0] = 0;                           // Only required for 32 byte readback transaction
		  data_array[1] = combinedCmdAddr;             // Build command / address byte
		  data_array[2] = value>>8;                    // MS Byte
		  data_array[3] = value;                       // LS Byte


		// Send the four bytes command to the DAC
		// Future: The DMA has been tested and no issues found. But if found that the nCS goes low before DMA completed transmission it could be a problem.
		//         In such case resort to using a normal polling command. 		// HAL_SPI_Transmit(&hspi4,&data_array,4,10);


		HAL_SPI_Transmit_DMA(&hspi4,&data_array,4);

		// Use below for blocking transmission
		// HAL_SPI_Transmit(&hspi4,&data_array,4,10);


		// pull the corresponding chip select pin nCS to high to execute command on the selected DAC
      	dac_nCs_set(dacNumber,GPIO_PIN_SET);


		// the chip waits 32 clock cycles before it responds back and echo the same command

      	return "Ok\n";
}

// DAC1 STM set VLENS_SET to turn on the HV lens circuit 2.5V corresponds to +-150V, 1.25V corresponds to +-75
char* dac_setSetPoint1(uint16_t value)
{
	// set DAC channel 0 to the new value in steps
	voltageSTMDacRamp(STM_DAC_CHANNEL1,value);

	// comment above and uncomment below for no ramp or set stepsize to 0
//	HAL_DAC_SetValue(&hdac,DAC_CHANNEL_1,DAC_ALIGN_12B_R,value);

	// update the dac value
	stmDac.DacStmValue[0]=value;

	return "Ok\n";

}


// DAC2 STM set Setpoint2
/*char* dac_setSetPoint2(uint16_t value)
{
	// set DAC channel 1 to the new value in steps
	// voltageSTMDacRamp(STM_DAC_CHANNEL2,value);
	// comment above and uncomment below for no ramp or set stepsize to 0
//	HAL_DAC_SetValue(&hdac,DAC_CHANNEL_2,DAC_ALIGN_12B_R,value);

	// udpate the dac value
	stmDac.DacStmValue[1]=value;
	return "Ok\n";

}*/

// read setpoint 1
int dac_getSetPoint1(void)
{
	return stmDac.DacStmValue[0];
}

// read setpoint 2
/*int dac_getSetPoint2(void)
{
	return stmDac.DacStmValue[1];
}*/



// initialize all the LTC and STM DACS as the the LTC DACs remember the last voltage and range setting before powering up!
void dac_init_aout(void)
{

	// Initialize the DAC values/range shadow values for the LTC chips, default is 0-10
    // The stm32 DacsLtcValue are (0-2.5) amplified with a gain of four so they are 0-10 range too
   	for( int jj = 0; jj < MAX_LTC_DAC_CHIPS; jj++ )
   	{
   		// Set up the *LOADDAC line high
   		 nLDAC_set(jj);
   		 // set all channels range to 3 (�10V)
   		dac_setRangeAllChannels(jj,DacRangeList[3].DacRange);

   	 for( int ii = 0; ii < LTC_DAC_CHANNELS; ii++ )
   	 {
   		// set voltage to zero on all outputs and update.
   		// Zero equivalent is "2048, BIPOLAR_RANGES_ZERO_REF" at range 0�10V, 0�5V and 0�2.5V and it is "0, UNIPOLAR_RANGES_ZERO_REF" for all other ranges
   		dac_setAoutAllChannels(jj,BIPOLAR_RANGES_ZERO_REF);

   		// ensure all channels are reading from the default register A (0)
   	    dac_globalToggleRegisters(jj,DEFAULT_TOGGLE_REGISTER);

   	   	// reset the step size to zero on LTC dac channels.(i.e., no stepping)
   	    dacLtc_setStepSize(jj,ii,STEP_SIZE_LTC_DAC);

   	 }

   	}

   	// reset the step size to zero on stm channels
   	for(int ii = 0;ii<STM_DAC_CHANNELS;ii++)dacStm_setStepSize(ii,STEP_SIZE_STM_DAC);

	// start the stm32 DacsLtcValue
	HAL_DAC_Start(&hdac,DAC_CHANNEL_1);
	//HAL_DAC_Start(&hdac,DAC_CHANNEL_2);

   	// set the STM DAC voltage to 0
   	dac_setSetPoint1(UNIPOLAR_RANGES_ZERO_REF);
   //	dac_setSetPoint2(UNIPOLAR_RANGES_ZERO_REF);

}

// Set/Reset chip select on an LTC DAC.

char* dac_nCs_set(uint8_t dacNumber, int value)
{

	GPIO_PinState PinState;

    if(dacNumber > MAX_LTC_DAC_CHIPS)return "BAD DAC Number";
	if(value != 0 && value != 1) return "Value Must Be Binary";

	switch(value)
	{
	case 0:PinState = GPIO_PIN_RESET;break;
	case 1:PinState = GPIO_PIN_SET;break;
	}

	// add more cases for as many DACS on the board
	switch(dacNumber)
	{

	case 0:HAL_GPIO_WritePin(GPIOE, DAC_nCS0_Pin, PinState);break;
	case 1:HAL_GPIO_WritePin(GPIOG, DAC_nCS1_Pin, PinState);break;
	case 2:HAL_GPIO_WritePin(GPIOG, DAC_nCS2_Pin, PinState);break;
	case 3:break;
	case 4:break;
	default:break;
	}

	return "Ok\n";
}



// Set the corresponding LDAC pin for an LTC DAC chip
void nLDAC_set(uint8_t dacNumber)
{
	// add more cases for as many DACS on the board
	switch(dacNumber)
	{
	case 0:HAL_GPIO_WritePin(GPIOE, SPI4_nLDAC_Pin, GPIO_PIN_SET);break;
	case 1:HAL_GPIO_WritePin(GPIOE, SPI4_nLDAC_Pin, GPIO_PIN_SET);break;
	case 2:HAL_GPIO_WritePin(GPIOE, SPI4_nLDAC_Pin, GPIO_PIN_SET);break;
	case 3:break;
	case 4:break;
	default:break;
	}
}


/*
 * Begins: functions for ramping (or stepping voltages)
 */
// set stepsize for all LTC dacs
void dacLtc_setStepSize(uint8_t dacNumber, uint8_t channelNumber, uint16_t value)
{
	ltcDacChips[dacNumber].dacStepSize[channelNumber] = value;
}

//get stepsize for all LTC dacs
int dacLtc_getStepSize(uint8_t dacNumber, uint8_t channelNumber)
{
return ltcDacChips[dacNumber].dacStepSize[channelNumber];
}

// set stepsize for STM dacs
void dacStm_setStepSize(uint8_t channelNumber, uint16_t value)
{
	stmDac.dacStepSize[channelNumber] = value;
}

//get stepsize for STM dacs
int dacStm_getStepSize(uint8_t channelNumber)
{
return stmDac.dacStepSize[channelNumber];
}


// increase or decrease the voltage in steps for the LTC  DAC. Similar function follows for the on chip STM DAC
void voltageLtcDacRamp( uint8_t dacNumber, uint8_t channelNumber,  int value, uint8_t  commandWord)
{
	int numberOfSteps = 0;
	int previousDacValue = dac_getAout(dacNumber,channelNumber);
    uint16_t stepSize = dacLtc_getStepSize(dacNumber,channelNumber);
	// if stepSize is zero execute the command straight without ramping
if(stepSize!=0)
 {
	// if previous value is lower than the new set value then add steps
	if ((value-previousDacValue)> 0)
	{
		numberOfSteps = (value-previousDacValue)/stepSize;
		for (int ii = 0; ii < numberOfSteps; ii++)
		{
			// add steps to the base value
			previousDacValue = previousDacValue + stepSize;
			// just in case previousDacValue > value, though it should not happen
			if(previousDacValue > value)previousDacValue = value;
			// form a command and send it

			//note: previousDacValue is ignored when it comes to update channel ( command code 0x1) this needs a special tweek here
			// ramp up the voltage using a normal set DAC value before you update from the default register
			if(commandWord == 0x1)  commandWord = 0x3;
			// set a step time and wait X secs between increments, i.e., step duration
			vTaskDelay(STEP_TIME);
			dac_setCmdGeneric(dacNumber,channelNumber,commandWord,previousDacValue);
		}
	}
	// if previous value is greater than the new set value then subtract steps
	else
	{
		numberOfSteps = (previousDacValue-value)/stepSize;
		for (int ii = 0; ii < numberOfSteps; ii++)
		{
			// add steps to the base value
			previousDacValue = previousDacValue - stepSize;
			// just in case previousDacValue < value, though it should not happen
			if(previousDacValue < value)previousDacValue = value;
			// form a command and send it

			// ramp up the voltage using a normal set DAC value before you update from the default register
			if(commandWord == 0x1)  commandWord = 0x3;
			// set a step time
			vTaskDelay(STEP_TIME);
			dac_setCmdGeneric(dacNumber,channelNumber,commandWord,previousDacValue);
		}

	}
 }
	// form a command and send the actual value once ramping is complete
	dac_setCmdGeneric(dacNumber,channelNumber,commandWord,value);

}

// increase or decrease the voltage in steps for the STM  DAC. Similar function follows to the LTC DAC with a minor difference in setting the output voltage (i.e., not using an spi command)
void voltageSTMDacRamp(  uint8_t channelNumber,  int value)
{
	int numberOfSteps = 0;
	int previousDacValue = stmDac.DacStmValue[channelNumber];
    uint16_t stepSize=dacStm_getStepSize(channelNumber);

  if(stepSize!=0)
  {
	// if previous value is lower than the new set value then add steps
	if ((value-previousDacValue) > 0)
	{
		numberOfSteps = (value-previousDacValue)/stepSize;
		for (int ii = 0; ii < numberOfSteps; ii++)
		{
			// add steps to the base value
			previousDacValue = previousDacValue + stepSize;
			// just in case previousDacValue > value, though it should not happen
			if(previousDacValue > value)previousDacValue = value;
			// set a step time

			vTaskDelay(STEP_TIME);

			// set the dac value
			switch (channelNumber)
			{
			case 0: HAL_DAC_SetValue(&hdac,DAC_CHANNEL_1,DAC_ALIGN_12B_R,previousDacValue);break;
		//	case 1: HAL_DAC_SetValue(&hdac,DAC_CHANNEL_2,DAC_ALIGN_12B_R,previousDacValue);break;
			}

		}
	}
	// if previous value is greater than the new set value then subtract steps
	else
	{
		numberOfSteps = (previousDacValue-value)/stepSize;
		for (int ii = 0; ii < numberOfSteps; ii++)
		{
			// add steps to the base value
			previousDacValue = previousDacValue - stepSize;
			// just in case previousDacValue < value, though it should not happen
			if(previousDacValue < value)previousDacValue = value;
			// set a step time
			vTaskDelay(STEP_TIME);
			// set the dac value
			switch (channelNumber)
			{
			case 0: HAL_DAC_SetValue(&hdac,DAC_CHANNEL_1,DAC_ALIGN_12B_R,previousDacValue);break;
		//	case 1: HAL_DAC_SetValue(&hdac,DAC_CHANNEL_2,DAC_ALIGN_12B_R,previousDacValue);break;
			}
		}

	}
  }
	// set the actual DAC value once ramping is complete
	switch (channelNumber)
	{
	case 0: HAL_DAC_SetValue(&hdac,DAC_CHANNEL_1,DAC_ALIGN_12B_R,value);break;
//	case 1: HAL_DAC_SetValue(&hdac,DAC_CHANNEL_2,DAC_ALIGN_12B_R,value);break;
	}

}
/*
 *  Ends: functions for ramping (or stepping voltages)
 *
 */

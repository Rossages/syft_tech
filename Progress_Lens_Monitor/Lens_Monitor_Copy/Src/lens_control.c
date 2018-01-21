/*
 * lens_control.c
 *
 *  Created on: 15/06/2017
 *      Author: Esam.Alzqhoul
 */

/*
 * To set voltages on different lenses (i.e, different channels of different DACs) the dac_out.c driver can be used
 * for this purposes with all of its features
 *
 * Below is an illustration example for all basic features required to set a lens voltage
 * Note: if the range on the DAC is set to +-10V (option 0x3, see dac_out.c) then -10V---->-150V and +10V---->+150V given
 * that lens_setVlensVoltageSupply is set to 150V
 *
 *
 * for example: set voltage on Upstream lens 1 U_L1 to 50V in steps of 1 V
 *
 *               U_L1 is on DAC 0, channel 1 so
 *
 * 1- first set the range on the channel to +-10V
 * 	dac_setRange(0,1,0x3)
 *
 * 2-set step size (by default no step size or step size = 0)
 *  steps of 1 V is equivalent to 3,276
 *  so:
 *  dacLtc_setStepSize(0,1,3276)
 *
 * 3- Set the voltage: this can be done in two ways:
 * (i)dac_setAout(dacNumber,uint8_t channelNumber, uint32_t value) this will send  the voltage immediately to the output
 *
 *    dac_setAout(0,1,(50/150)*2047)  // 150V assuming the HV lens is set to 2.5V--->150V
 *    12 bits so 4095/2 is 32767
 *    2047 is basically 4095/2 -----> 0-2047 negative range  2047-65535 is positive range in this example
 *
 *    (50/150)*32767 will result in 3.3V on the DAC output
 *
 * (ii)  dac_setAoutNoUpdate(0,1,(50/150)*65535) this will send  the voltage to the register and no output
 * 		 dac_UpdateChannel(0,1) now send value from register to output
 *
 * to read a lens voltage use the get function on that channel
 *  dac_getAout(0,1)
 */

/*
 * Important: Lenses (Set Points) MAP for the three DAC chips on the Lens controller and their respective channel numbers
 *
schematics (page 0) Type	Description 	                      DAC Chip Number 	DAC Channel Number							  Range
   Lenses Index
6			        Voltage	 Upstream PF	                            0	         0
1			        Voltage	 Upstream L1	                            0	         1                                           �10V
2			        Voltage	 Upstream L2	                            0	         2                                           �10V
3			        Voltage	 Upstream L3	                            0	         3                                           �10V
4			        Voltage	 Upstream L4	                            0	         4                                           �10V
5			        Voltage	 Upstream L5	                            0	         5                                           �10V
N/A			        Voltage	 DAC0_V6	                                0	         6                                           �10V
N/A			        Voltage	 DAC0_V7	                                0	         7                                           �10V
37			        Voltage	 DAC0 MUXOUT	                            0	         All channels on DAC0 can be muxed out       �10V
10			        Voltage	 Flow Tube (FT)	                            1	         0                                           �10V
11			        Voltage	 Downstream L1	                            1	         1                                           �10V
7			        Voltage	 Upstream L6	                            1	         2                                           �10V
17			        Voltage	 PicoAmeter Voltage 	                    1	         3                                           �2.5V
16			        Voltage	 NanoAmeter Voltage 	                    1	         4                                           �10V
42			        Voltage  Set Point on the Guague Terminal (N/A )	1	         5                                           �10V
N/A			        Voltage	 DAC1_V6	                                1	         6                                           �10V
41			        Voltage  Discrimniator Voltage                  	1	         7                                           �10V
38			        Voltage	 DAC1 MUXOUT	                            1	         All channels on DAC1 can be muxed out       �10V
9			        Voltage	 Downstream PF	                            2	         0                                           �10V
8			        Voltage	 Downstream IGB Ion Guide Bias	            2	         1                                           �10V
12			        Voltage	 Downstream L2								2	         2                                           �10V
13			        Voltage	 Downstream L3								2	         3                                           �10V
14			        Voltage	 Downstream L4								2	         4                                           �10V
15			        Voltage	 Downstream L5								2	         5                                           �10V
N/A			        Voltage	 DAC2_V6	                                2	         6                                           �10V
N/A			        Voltage	 DAC2_V7	                                2	         7                                           �10V
39			        Voltage	 DAC2 MUXOUT								2	         All channels on DAC2 can be muxed out       �10V



 */

/* Important: Lenses voltage and current measurement MAP of the five ADCs on the Lens Controller and and their respective Channel Numbers





schematics (page 0) Type	  Description 	                                  ADC Chip Number 	ADC Channel Number      Gain			                        	 Default Range
   Lenses Index

1			       Voltage 	   Upstream L1                                          1	              0                 (10+0.68)/0.68  = 15.705                             �10V
2			       Voltage 	   Upstream L2                                          1	              1                 (10+0.68)/0.68	                        	 �10V
3			       Voltage 	   Upstream L3                                          1	              2                 (10+0.68)/0.68	                        	 �10V
4			       Voltage     Upstream L4	                                        1	              3                 (10+0.68)/0.68                               �10V
5			       Voltage     Upstream L5	                                        1	              4                 (10+0.68)/0.68	                        	 �10V
6			       Voltage 	   Upstream PF   						                1	              5                 (10+0.68)/0.68                               �10V
7			       Voltage 	   Upstream L6                                          1	              6                 (10+0.68)/0.68                               �10V
8			       Voltage 	   Downstream IGB Ion Guide Bias                        1	              7                 (10+0.68)/0.68                               �10V
9			       Voltage 	   Downstream PF                                        2	              0                 (10+0.68)/0.68	                        	 �10V
10			       Voltage 	   Flow Tube (FT)                                       2	              1                 (10+0.68)/0.68                               �10V
11			       Voltage 	   Downstream L1	                                    2	              2                 (10+0.68)/0.68                               �10V
12			       Voltage 	   Downstream L2					                    2	              3                 (10+0.68)/0.68                               �10V
13			       Voltage 	   Downstream L3						                2	              4                 (10+0.68)/0.68                               �10V
14                 Voltage     Downstream L4                                        2                 5                 (10+0.68)/0.68                               �10V
15                 Voltage     Downstream L5                                        2                 6                 (10+0.68)/0.68                               �10V
16			       Voltage 	   NanoAmeter Voltage 	                                2	              7                 (10+0.68)/0.68                               �10V
17                 Voltage     PicoAmeter Voltage                                   3                 0                 (10+0.68)/0.68	                             �2.5V
18                 Current(uA) Upstream L1                                          3                 1                (1000)                                           �2.5V
19                 Current(uA) Upstream L2                                          3                 2                (1000)                                           �2.5V
20                 Current(uA) Upstream L3                                          3                 3                (1000)                                           �2.5V
21                 Current(uA) Upstream L4                                          3                 4                (1000)                                           �2.5V
22                 Current(uA) Upstream L5                                          3                 5                (1000)                                           �2.5V
23                 Current(uA) Upstream PF                                          3                 6                (1000)                                           �2.5V
24                 Current(uA) Upstream L6                                          3                 7                (1000)                                           �2.5V
25                 Current(uA) Downstream IGB                                       4                 0                (1000)                                           �2.5V
26                 Current(uA) Downstream PF                                        4                 1                (1000)                                           �2.5V
27                 Current(uA) Flow Tube (FT)                                       4                 2                (1000)                                           �2.5V
28                 Current(uA) Downstream L1                                        4                 3                (1000)                                           �2.5V
29                 Current(uA) Downstream L2                                        4                 4                (1000)                                           �2.5V
30                 Current(uA) Downstream L3                                        4                 5                (1000)                                           �2.5V
31                 Current(uA) Downstream L4                                        4                 6                (1000)                                           �2.5V
32                 Current(uA) Downstream L5                                        4                 7                (1000)                                           �2.5V
33			       Current(nA) NanoAmeter Current (nA) 				                0	              0                (100)									     �2.5V
34			       Current(pA) PicoAmeter Current (pA)  					        0	              1                (200)                                         �2.5V
35			       Voltage 	   Positive  Voltage supply of Lenses                	0	              2                (470*3+10)/10  = 142
                             �10V





36			       Voltage 	   Negative Voltage  supply of Lenses                	0	              3                (470*3+10)/10                                 �10V
37			       Voltage 	   DAC0 MUXOUT  							            0	              4				   1 Depends on the Muxed out Channel			 �10V
38			       Voltage 	   DAC1 MUXOUT  							            0	              5                1 Depends on the Muxed out Channel            �10V
39			       Voltage 	   DAC2 MUXOUT  							            0	              6                1 Depends on the Muxed out Channel			 �10V
40			       Voltage 	   Gauge Pressure Value       			                0	              7                1                                             �10V                                            �10V

                    Multiplier formula for the 12bit-ADCs (ADS8638):
					range must be set to �10V

					Actual Voltage = ((ADC_Value/4095)*Full_Range + Bottom_Range)*Gain
					e.g. range min =-10 max =+10, gain is (10+0.68)/0.68 and ADC_Value = 4000;
                    		   Actual Voltage = ((4000/4095)*(10-(-10)))+ (-10))*((10+0.68)/0.68) = 149V

 */
#include "global.h"
#include "types.h"



#define PICOAMETER_VOLTAGE_RANGE 0x4
#define DAC1_PICOAMETER_CHANNEL 3
#define LTC_DAC_CHIP1 1
#define PWR_SUPPLY_15V 3

// Lens setting defines
#define MIN_LENS_VOLTAGE            (-150)
#define MAX_LENS_VOLTAGE            (150)
#define MAX_DAC_SETPOINT            (4095)


// turn on the HV lens driver.
/* Example set points:
 * 2.5V ----> +-150
 * 1.25V----> +-75
 * 0.16V----> +-9V
 *Future: Important note on the STM DAC: 0 (currently  buffer is enabled we can only get to 160mV. disable buffer in dac.c to get to 0V)
 */
char* lens_setVlensVoltageSupply(uint16_t value)
{
	dac_setSetPoint1(value);

	return "Ok\n";
}

// get HV lens driver voltage
int lens_getVlensVoltageSupply(void)
{
	return dac_getSetPoint1();
}

// initialize all DAC chips and set ranges
StatusCode_t lens_dacsInit(void)
{
	// turn on the 15V power supply for the DACs and ADCs
	dout_setDout(PWR_SUPPLY_15V,ENABLE);

	// init all DACs, set values on the dacs to zero and and range to +-10
	dac_init_aout();

	//  set range for the Picoameter to +-2.5V on DAC1
	dac_setRange(LTC_DAC_CHIP1,DAC1_PICOAMETER_CHANNEL,PICOAMETER_VOLTAGE_RANGE);

	return STATUS_OK;
}

// initialize all ADS8638 chips and set ranges
StatusCode_t lens_adcInit(void)
{
	// turn on the 15V power supply for the DACs and ADCs
	dout_setDout(PWR_SUPPLY_15V,ENABLE);

	// init all ADS8638, power up and set ranges as per the MAP above
	adc8638_init();

	return STATUS_OK;

}

// DEV: set all lenses to some value. Note that the actual voltage on the output varies depending on the the set range. See notes about ranges at the top of this document
void lensTest(int setValue)
{

   	for( int jj = 0; jj < 3; jj++ )
   	{
   	 for( int ii = 0; ii < 8; ii++ )
   	 {
   	    dac_setAout(jj,ii,setValue);
//   	    setValue = setValue+ 500;
   	 }
//   	setValue=0;

   	}
}

StatusCode_t LensControl_SetLensVoltage(LensId_t lens, int32_t setPointVoltage)
{
    if (setPointVoltage < MIN_LENS_VOLTAGE || setPointVoltage > MAX_LENS_VOLTAGE)
    {
        puts("WARNING - Lens voltage out of range");
        return STATUS_FAIL;
    }

    // TODO: improve this calculation. Currently has an off by one error at 0 volt set point due to truncation. Also
    // assumes that the output range of the lens is symmetrical which may not be true. Should get real lens supply
    // voltage readings and scale based on those.
    uint32_t dacSetpoint = ((((float)(setPointVoltage + MAX_LENS_VOLTAGE))/ (2 * MAX_LENS_VOLTAGE)) * MAX_DAC_SETPOINT);

    if (dacSetpoint < 0 || dacSetpoint > MAX_DAC_SETPOINT)
    {
        puts("WARNING - DAC setpoint out of range");
        return STATUS_FAIL;
    }

    switch (lens)
    {
        // TODO remove magic numbers
        // To mirror the US lenses to the DS lenses
        case US_LENS1_ENUM:
            dac_setAout(0, 1, dacSetpoint);   // US L1

            break;
        case US_LENS2_ENUM:
            dac_setAout(0, 2, dacSetpoint);   // US L2
            break;
        case US_LENS3_ENUM:
            dac_setAout(0, 3, dacSetpoint);   // US L3
            break;
        case US_LENS4_ENUM:
            dac_setAout(0, 4, dacSetpoint);   // US L4
            break;
        case US_LENS5_ENUM:
            dac_setAout(0, 5, dacSetpoint);   // US L5
            break;
        case US_LENS6_ENUM:
            dac_setAout(1, 2, dacSetpoint);   // US L6
            break;
        case US_PREFILT_ENUM:
            dac_setAout(0, 0, dacSetpoint);   // US Prefilter
            break;
        default:
            puts("WARNING - Lens not supported");
            break;
    }

    // TODO return something more useful
    return STATUS_OK;
}

//bool LensControl_isValidStaticSetpoint(int32_t setPointVoltage)
//{
//    if (setPointVoltage < MIN_LENS_VOLTAGE || setPointVoltage > MAX_LENS_VOLTAGE)
//    {
//        return false;
//    }
//
//    return true;
//}

bool LensControl_isValidSetpoint(int32_t setPointVoltage)
{
    // 0 is considered an undesired lens voltage setting -> it is used to signal that the static setpoint should be used instead
    if (setPointVoltage < MIN_LENS_VOLTAGE || setPointVoltage > MAX_LENS_VOLTAGE || setPointVoltage == 0)
//        if (setPointVoltage < MIN_LENS_VOLTAGE || setPointVoltage > MAX_LENS_VOLTAGE)

    {
        return false;
    }

    return true;
}

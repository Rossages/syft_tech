/*
 * lens_control.h
 *
 *  Created on: 15/06/2017
 *      Author: Esam.Alzqhoul
 */

#ifndef LENS_CONTROL_H_
#define LENS_CONTROL_H_
#include "types.h"

typedef enum
{
    US_LENS1_ENUM,
    US_LENS2_ENUM,
    US_LENS3_ENUM,
    US_LENS4_ENUM,
    US_LENS5_ENUM,
    US_LENS6_ENUM,
    US_PREFILT_ENUM
}LensId_t;

char* lens_setVlensVoltageSupply(uint16_t value);
StatusCode_t LensControl_SetLensVoltage(LensId_t lens, int32_t setPointVoltage);
int lens_getVlensVoltageSupply(void);
StatusCode_t lens_dacsInit(void);
void lensTest(int setValue);
StatusCode_t lens_adcInit(void);

bool LensControl_isValidSetpoint(int32_t setPointVoltage);

#endif /* LENS_CONTROL_H_ */

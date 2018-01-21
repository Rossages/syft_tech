/*
 * rs485_ain.h
 *
 *  Created on: 16/12/2016
 *      Author: Esam.Alzqhoul
 */

#ifndef AIN_H_
#define AIN_H_

#include "types.h"


void readAins(void);
int  ain_get( int index );
StatusCode_t update_ain();

#endif /* AIN_H_ */

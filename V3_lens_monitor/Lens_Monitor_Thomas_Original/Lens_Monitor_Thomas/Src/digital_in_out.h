/*
 * digital_in_out.h
 *
 *  Created on: 4/05/2017
 *      Author: Esam.Alzqhoul
 */

#ifndef DIGITAL_IN_OUT_H_
#define DIGITAL_IN_OUT_H_

#include "types.h"

StatusCode_t dout_init( void );
char* dout_setDout( int index, int value );
int dout_get ( int index );
StatusCode_t dout_update( void );

void din_update( void );
int din_get ( int index );
#endif /* DIGITAL_IN_OUT_H_ */

/*
 * types.h
 *
 *  Created on: 29/06/2017
 *      Author: Jason.Orchard
 *
 *  Purpose: To provide common types to syft application, device driver and driver files
 *
 *  Description:
 */

#ifndef INC_TYPES_H_
#define INC_TYPES_H_

#include <stdbool.h>

/*
 * A type used primarily for reporting success or failure of a functions core purpose
 */
typedef enum {
    STATUS_OK,
    STATUS_FAIL
} StatusCode_t;

/*
 * A type used for reporting boolen results
 */
//typedef enum {
//    FALSE = 0,
//    TRUE = 1
//}Bool_t;

#endif /* INC_TYPES_H_ */

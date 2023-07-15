/*
 * hooks.h
 *
 *  Created on: 31 янв. 2021 г.
 *      Author: tochk
 */

#ifndef INC_HOOKS_H_
#define INC_HOOKS_H_


#include <FreeRTOS.h>
#include <task.h>
#include "USART.h"
#include "stm32f4xx.h"



#define __debugError( x ) if ((x) == 0) Error_Handler(__FILE__ ,  __LINE__ )


void Error_Handler(const char *file, uint32_t line);
void vAssertCalled( unsigned long ulLine, const char * const pcFileName );




#endif /* INC_HOOKS_H_ */

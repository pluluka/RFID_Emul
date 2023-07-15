/*
 * CLIThread.h
 *
 *  Created on: 20 июн. 2022 г.
 *      Author: tochk
 */

#ifndef SRC_THREADS_CLITHREAD_H_
#define SRC_THREADS_CLITHREAD_H_


#include <EM4100Thread.h>
#include "stm32f4xx.h"
#include "stm32f4xx_ll_spi.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_exti.h"
#include "stm32f4xx_ll_tim.h"
#include <stdbool.h>
#include "cmsis_os.h"
#include "hooks.h"
#include "SPIFlashTabSystem.h"
//#include <returnStatus.h>
#include "EM4100_RWE.h"
#include "CLI.h"


// Типы сообщений
typedef enum{
	CLI_THREAD_MSG__CMD_RECV = 0,
	CLI_THREAD_MSG__CLI_EN,
	CLI_THREAD_MSG__PRINT
} CLIMsgType_t;
// Формат сообщения
typedef struct {
	CLIMsgType_t	msgType;
	void*			p;
} CLIMsgData_t;


void CLIThread(void const * argument);
void CLIThreadInit(uint32_t threadPriority);



#endif /* SRC_THREADS_CLITHREAD_H_ */

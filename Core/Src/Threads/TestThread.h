/*
 * TestThread.h
 *
 *  Created on: 9 мая 2021 г.
 *      Author: tochk
 */

#ifndef SRC_THREADS_TESTTHREAD_H_
#define SRC_THREADS_TESTTHREAD_H_


#include <OneWireMaster.h>
#include "stm32f4xx.h"
#include "stm32f4xx_ll_spi.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_exti.h"
#include "stm32f4xx_ll_usart.h"
#include "stm32f4xx_ll_tim.h"
#include "cmsis_os.h"
#include "hooks.h"
#include <stdbool.h>
#include <returnStatus.h>
#include "EM4100_RWE.h"


#define EXTBTN_PA1		1
#define EXTBTN_PA2		2
#define EXTBTN_PA3		3


osMessageQId			msgQueueTest;
osThreadId				threadTest;
osPoolId				memPoolTest;
osSemaphoreId			semaphoreTest;


void TestThreadInit(uint32_t threadPriority);




#endif /* SRC_THREADS_TESTTHREAD_H_ */

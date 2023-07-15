/*
 * OneWireThread.h
 *
 *  Created on: 15 июл. 2021 г.
 *      Author: tochk
 */

#ifndef SRC_THREADS_ONEWIRETHREAD_H_
#define SRC_THREADS_ONEWIRETHREAD_H_


#include "stm32f4xx.h"
#include "stm32f4xx_ll_spi.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_exti.h"
#include "stm32f4xx_ll_usart.h"
#include "stm32f4xx_ll_tim.h"
#include "cmsis_os.h"
#include "USART.h"
#include "hooks.h"
#include "cmsis_os.h"
#include <stdbool.h>
#include "OneWireMaster.h"
#include "OneWireSlave.h"
#include "OneWireCRC.h"
#include "SPIFlashTabSystem.h"
#include "CLIThread.h"

/*
typedef enum{
	KEY_NONAME = 0,
	KEY_RW1990
} keyType_t;
*/

// типы сообщений основного потока
typedef enum{
	OW_THREAD_MSG__CMD_READ_ROM = 0,			// инициализация процесса детектирования (попытки чтения ROM)
	OW_THREAD_MSG__CMD_READ_ROM_BEGIN,			// процесс детектирования
	OW_THREAD_MSG__CMD_CLI_EN,					// разрешает ввод команд в CLI
	OW_THREAD_MSG__CMD_WRITE_ROM_RW1990,		// запись на болванку RW1990
	OW_THREAD_MSG__CMD_EMUL_CURRENT,		//
	OW_THREAD_MSG__CMD_EMUL_ALL			//
} OneWireMsgType_t;

// Формат сообщения
typedef struct {
	OneWireMsgType_t	msgType;	// тип сообщения
	void*				p;			// указатель на возможные массивы данных
} OneWireMsgData_t;



void OneWireThreadInit(uint32_t priority);
void OneWireThread(void const * argument);
OneWireDataRec_t* OneWireThreadGetCurrentData(void);


#endif /* SRC_THREADS_ONEWIRETHREAD_H_ */

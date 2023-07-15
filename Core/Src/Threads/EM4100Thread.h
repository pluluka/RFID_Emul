/*
 * EM4100ReaderThread.h
 *
 *  Created on: 27 янв. 2021 г.
 *      Author: tochk
 *
 *      Поток обработки данных с RWE-модуля EM4100
 *
 */

#ifndef SRC_THREADS_EM4100THREAD_H_
#define SRC_THREADS_EM4100THREAD_H_



#include "stm32f4xx.h"
#include "stm32f4xx_ll_spi.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_exti.h"
#include "stm32f4xx_ll_usart.h"
#include "stm32f4xx_ll_tim.h"
#include "cmsis_os.h"
#include <returnStatus.h>
#include "USART.h"
#include "hooks.h"
#include <stdbool.h>
#include "EM4100_RWE.h"
#include "CLIThread.h"
#include "SPIFlashTabSystem.h"


// Типы сообщений
typedef enum{
	EM4100_THREAD_MSG__RWE_DATA_RECEIVED = 0,	// Метка считана (пришли данные от декодера)
	EM4100_THREAD_MSG__RWE_EMUL_DONE,			// Эмуляция метки окончена (сообщение энкодера)
	EM4100_THREAD_MSG__CMD_READ_BEGIN,			// Команда на начало процесса считывания метки
	EM4100_THREAD_MSG__CMD_READ_END,			// Команда на окончание процесса считывания метки
	EM4100_THREAD_MSG__CMD_WRITE_CURRENT,		// Команда на запись текущей метки на болванку
	EM4100_THREAD_MSG__CMD_EMUL_BEGIN,			// Команда на начало процесса эмуляции метки
	EM4100_THREAD_MSG__CMD_EMUL_END,			// Команда на окончание процесса эмуляции метки
	EM4100_THREAD_MSG__CMD_SET_CURRENT,
	EM4100_THREAD_MSG__DEFAULT
} EM4100MsgType_t;

// Режимы эмуляции
typedef enum{
	EM4100_EMUL_MODE_ALL = 0,	// цикличная эмуляция всех меток из БД
	EM4100_EMUL_MODE_CURRENT,	// текущая метка
	EM4100_EMUL_MODE_IDLE 		// режим бездействия (можно использовать для остановки)
} EM4100EmulMode_t;

// Формат сообщения
typedef struct {
	EM4100MsgType_t	msgType;	// тип сообщения
	void*			p;			// указатель на возможные массивы данных
	uint32_t		v1;			// числовой параметр 1
	uint32_t		v2;			// числовой параметр 2
} EM4100MsgData_t;


void EM4100ThreadInit(uint32_t threadPriority);
void EM4100Thread(void const * argument);

EM4100DataRec_t* EM4100ThreadGetCurrentData(void);



#endif /* SRC_THREADS_EM4100THREAD_H_ */

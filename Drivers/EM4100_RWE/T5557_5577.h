/*
 * T5557_5577.h
 *
 *  Created on: 24 мая 2021 г.
 *      Author: tochk
 *
 *      Запись данных на метки T5557, T5577 производства Atmel
 */

#ifndef EM4100_RWE_T5557_5577_H_
#define EM4100_RWE_T5557_5577_H_


#include "stm32f4xx.h"
#include "stm32f4xx_ll_spi.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_tim.h"
#include <stdbool.h>
//#include "TestThread.h"
#include <strings.h>
#include "delay.h"

/* Bit-Rate configuration bit */
#define T5557_BIT_RATE_RF8					(uint32_t)(0<<18)
#define T5557_BIT_RATE_RF16					(uint32_t)(1<<18)
#define T5557_BIT_RATE_RF32					(uint32_t)(2<<18)
#define T5557_BIT_RATE_RF40					(uint32_t)(3<<18)
#define T5557_BIT_RATE_RF50					(uint32_t)(4<<18)
#define T5557_BIT_RATE_RF64					(uint32_t)(5<<18)
#define T5557_BIT_RATE_RF100				(uint32_t)(6<<18)
#define T5557_BIT_RATE_RF128				(uint32_t)(7<<18)
/* Modulation method configuration bit */
#define T5557_MODULATION_DIRECT				(uint32_t)(0<<12)
#define T5557_MODULATION_PSK1				(uint32_t)(1<<12)
#define T5557_MODULATION_PSK2				(uint32_t)(2<<12)
#define T5557_MODULATION_PSK3				(uint32_t)(3<<12)
#define T5557_MODULATION_FSK1				(uint32_t)(4<<12)
#define T5557_MODULATION_FSK2				(uint32_t)(5<<12)
#define T5557_MODULATION_FSK1A				(uint32_t)(6<<12)
#define T5557_MODULATION_FSK2A				(uint32_t)(7<<12)
#define T5557_MODULATION_MANCHESTER			(uint32_t)(8<<12)
#define T5557_MODULATION_BIPHASE50			(uint32_t)(16<<12)
/* Configuration data (block 0) */
#define T5557_SAFER_KEY						(uint32_t)(6<<28)
#define T5557_MAX_BLOCK_1					(uint32_t)(1<<5)
#define T5557_MAX_BLOCK_2					(uint32_t)(2<<5)
#define T5557_MAX_BLOCK_3					(uint32_t)(3<<5)
#define T5557_MAX_BLOCK_4					(uint32_t)(4<<5)
#define T5557_MAX_BLOCK_6					(uint32_t)(6<<5)
#define T5557_PSK_CF_RF2					(uint32_t)(0<<10)
#define T5557_PSK_CF_RF4					(uint32_t)(1<<10)
#define T5557_PSK_CF_RF8					(uint32_t)(2<<10)
#define T5557_PWD_BIT						(uint32_t)(1<<4)



void T5577_Init(void);
void T5577_WriteTagEM4100Simple(uint32_t modulation, uint32_t bitrate, uint8_t * tagData);
void T5577_WriteTagEM4100Debug(void);


#endif /* EM4100_RWE_T5557_5577_H_ */

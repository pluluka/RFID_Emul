/*
 * delay.h
 *
 *  Created on: Sep 9, 2020
 *      Author: tochk
 */

#ifndef DELAY_DELAY_H_
#define DELAY_DELAY_H_

#include "stm32f4xx.h"

__STATIC_INLINE void delay_Init(void)
{
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;	// разрешаем использовать счётчик
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;			// запускаем счётчик
}


__STATIC_INLINE void delay_us(uint32_t us)
{
	uint32_t us_count_tic =  us * (SystemCoreClock / 1000000U);
	DWT->CYCCNT = 0U;
	while(DWT->CYCCNT < us_count_tic);
}


__STATIC_INLINE void delay_ms(uint32_t us)
{
	uint32_t us_count_tic =  us * (SystemCoreClock / 1000U);
	DWT->CYCCNT = 0U;
	while(DWT->CYCCNT < us_count_tic);
}


#endif /* DELAY_DELAY_H_ */

/*
 * Metakom.h
 *
 *  Created on: 6 сент. 2021 г.
 *      Author: tochk
 */

#ifndef ONEWIRE_METAKOM_H_
#define ONEWIRE_METAKOM_H_


#include "stm32f4xx.h"
#include "stm32f4xx_ll_spi.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_tim.h"
#include "stm32f4xx_ll_usart.h"
#include <stdbool.h>
#include <strings.h>
#include "delay.h"


typedef struct{
	uint8_t key[4];
} MetakomKey_t;



void MetakomInit(void);
void MetakomWriteKey(MetakomKey_t *key);


#endif /* ONEWIRE_METAKOM_H_ */

/*
 * OneWire.h
 *
 *  Created on: 23 июн. 2021 г.
 *      Author: tochk
 *
 *      Драйвер протокола 1-Wire
 */

#ifndef ONEWIRE_ONEWIREMASTER_H_
#define ONEWIRE_ONEWIREMASTER_H_


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
#include "OneWireCRC.h"
//#include "CRC8_LT.h"


/* Статусы */
typedef enum {
	OWM_RS_OK = 0,
	OWM_RS_ERROR
}	OWM_ReturnStatus_t;

typedef struct{
	uint8_t romData[8];
} OneWireData_t;


void OneWireMasterInit(void);
OWM_ReturnStatus_t OneWireMasterReadROM(OneWireData_t *data);
OWM_ReturnStatus_t OneWireMasterRW1990_WriteROM(OneWireData_t *data);

void OneWire_TEST(void);


#endif /* ONEWIRE_ONEWIREMASTER_H_ */

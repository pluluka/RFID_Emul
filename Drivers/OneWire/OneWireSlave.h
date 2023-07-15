/*
 * OneWireSlave.h
 *
 *  Created on: 6 июл. 2021 г.
 *      Author: tochk
 */

#ifndef ONEWIRE_ONEWIRESLAVE_H_
#define ONEWIRE_ONEWIRESLAVE_H_


#include "stm32f4xx.h"
#include "stm32f4xx_ll_spi.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_tim.h"
#include <stdbool.h>
#include <strings.h>

#include "cmsis_os.h"
//#include "TestThread.h"


/* Статусы */
typedef enum {
	OWS_RS_OK = 0,
	OWS_RS_ERROR,
}	OWS_ReturnStatus_t;



void OneWireSlaveInit(void);
OWS_ReturnStatus_t OneWireSlaveDS1990_OnWire(uint8_t romData[8]);
void OneWireSlaveDS1990_OffWire(void);
uint8_t OneWireSlaveIsIDLE(void);

void OneWireSlave_TEST(void);


#endif /* ONEWIRE_ONEWIRESLAVE_H_ */

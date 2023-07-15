/*
 * W25QXXXFLASH.h
 *
 *  Created on: 4 нояб. 2021 г.
 *      Author: tochk
 */

#ifndef W25QXXXFLASH_W25QXXXFLASH_H_
#define W25QXXXFLASH_W25QXXXFLASH_H_


#include "stm32f4xx.h"
#include "stm32f4xx_ll_spi.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_gpio.h"
#include <stdbool.h>
#include "delay.h"


#define	W25QXXX_SPI			SPI1
#define W25QXXX_GPIO		GPIOA
#define W25QXXX_CS_PIN		LL_GPIO_PIN_4


void W25QXXX_Init(void);
uint32_t W25QXXX_ReadDeviceID(void);
void W25QXXX_SectorErase(uint32_t address);
void W25QXXX_MemoryWrite(uint32_t address, uint32_t dataLength, uint8_t *data);
void W25QXXX_MemoryRead(uint32_t address, uint32_t dataLength, uint8_t *data);
uint32_t W25QXXX_GetPageSize(void);
uint32_t W25QXXX_GetPageCount(void);

uint32_t W25QXXX_TEST(void);




#endif /* W25QXXXFLASH_W25QXXXFLASH_H_ */

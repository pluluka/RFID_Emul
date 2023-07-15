/*
 * AT45DB161.h
 *
 *  Created on: 21 сент. 2020 г.
 *      Author: tochk
 *
 *      SPI FLASH 2..16 Мбит
 */

#ifndef AT45DB161_AT45DB161_H_
#define AT45DB161_AT45DB161_H_


#include "stm32f4xx.h"
#include "stm32f4xx_ll_spi.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_gpio.h"
#include <stdbool.h>
#include "delay.h"


#define	AT45DB161_SPI		SPI1
#define AT45DB161_GPIO		GPIOA
#define AT45DB161_CS_PIN	LL_GPIO_PIN_4



void AT45DB161_Init(void);
uint8_t AT45DB161_ReadStatus(void);
void AT45DB161_ReadID(uint8_t *id);
void AT45DB161_MemoryRead(uint32_t pageAddress, uint32_t byteAddress, uint32_t dataLength, uint8_t *data);
void AT45DB161_MemoryWrite(uint32_t pageAddress, uint32_t byteAddress, uint32_t dataLength, uint8_t *data);
void AT45DB161_PageFill(uint32_t pageAddress, uint8_t data);
uint32_t AT45DB161_GetPagesCount(void);
uint32_t AT45DB161_GetPageSize(void);

void AT45DB161_SetPageSize_512(void);


void AT45DB161_TEST(uint32_t pageAddress, uint32_t byteAddress, uint32_t dataLength, uint8_t *data);




#endif /* AT45DB161_AT45DB161_H_ */

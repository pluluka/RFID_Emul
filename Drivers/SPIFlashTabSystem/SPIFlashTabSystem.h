/*
 * SPIFlashTabSystem.h
 *
 *  Created on: Oct 21, 2020
 *      Author: tochk
 *
 *      Табличная файловая система для SPI Flash
 */

#ifndef SPIFLASHTABSYSTEM_H_
#define SPIFLASHTABSYSTEM_H_


#include "stm32f4xx.h"
#include "stm32f4xx_ll_spi.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_gpio.h"
#include <stdbool.h>
#include "delay.h"
#include "W25QXXXFLASH.h"

// Запись данных метки EM4100
#pragma pack(push, 1)
typedef struct{
	uint32_t	id;
	uint8_t		data[5];
	uint8_t		modulationMethod;
	uint16_t	bitrate;
	uint8_t		name[7];
} EM4100DataRec_t;
#pragma pack(pop)

// Запись данных OneWire
#pragma pack(push, 1)
typedef struct{
	uint32_t	id;
	uint8_t		romData[8];
	uint8_t		name[7];
} OneWireDataRec_t;
#pragma pack(pop)



void SPIFTabSystemInit(void);
void SPIFTabSystemFormat(void);

uint32_t SPIFTabSystem_EM4100GetCounter(void);
uint32_t SPIFTabSystem_EM4100GetLastID(void);
void SPIFTabSystem_EM4100Add(EM4100DataRec_t *data);
void SPIFTabSystem_EM4100GetRec(uint32_t id, EM4100DataRec_t* data);
void SPIFTabSystem_EM4100FillBufferWithData(uint32_t startID,
			uint8_t *buffer, uint32_t bufferLength, uint32_t *dataCnt);
uint32_t SPIFTabSystem_EM4100GetDataSize(void);

uint32_t SPIFTabSystem_OneWireGetCounter(void);
uint32_t SPIFTabSystem_OneWireGetLastID(void);
void SPIFTabSystem_OneWireAdd(OneWireDataRec_t *data);
void SPIFTabSystem_OneWireGetRec(uint32_t recNum, OneWireDataRec_t* data);
void SPIFTabSystem_OneWireFillBufferWithData(uint32_t startID,
			uint8_t *buffer, uint32_t bufferLength, uint32_t *dataCnt);
uint32_t SPIFTabSystem_OneWireGetDataSize(void);
uint32_t SPIFTabSystem_OneWireGetFirstNum(void);



uint32_t SPIFTabSystem_TEST(void);


#endif /* AT45DB161_SPIFLASHTABSYSTEM_H_ */

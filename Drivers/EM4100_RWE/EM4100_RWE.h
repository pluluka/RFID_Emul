/*
 * EM4100_RWE.h
 *
 *  Created on: 2 мая 2021 г.
 *      Author: tochk
 *
 *      Модуль чтения/записи/эмуляции меток на базе протокола EM4100
 *      (Электрическая принципиальная схема модуля прилагается)
 */

#ifndef EM4100_RWE_EM4100_RWE_H_
#define EM4100_RWE_EM4100_RWE_H_


#include <EM4100Thread.h>
#include "stm32f4xx.h"
#include "stm32f4xx_ll_spi.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_tim.h"
#include <stdbool.h>
#include <strings.h>
#include "T5557_5577.h"


/* Modulation Methods */
#define EM4100_MODULATION_MANCHESTER		1

/* Data BitRates */
#define EM4100_DATA_BITRATE_RF8				8
#define EM4100_DATA_BITRATE_RF16			16
#define EM4100_DATA_BITRATE_RF32			32
#define EM4100_DATA_BITRATE_RF40			40
#define EM4100_DATA_BITRATE_RF50			50
#define EM4100_DATA_BITRATE_RF64			64
#define EM4100_DATA_BITRATE_RF100			100
#define EM4100_DATA_BITRATE_RF128			128


// Данные метки EM4100
typedef struct {
	uint8_t		modulationMethod;
	uint16_t	bitrate;
	uint8_t		data[5];
} em4100TagData_t;

// Типы RW-меток
typedef enum{
	T5557_T5577 = 0,
	UNDEFINE,
	TAG_DEBUG
} em4100TagType_t;



void EM4100_RWE_Reader_Init(void);
void EM4100_RWE_Reader_ON(void);
void EM4100_RWE_Reader_OFF(void);
void EM4100_RWE_Emulator_Init(void);
void EM4100_RWE_EmulatorRunTag(em4100TagData_t* tagData);
void EM4100_RWE_EmulatorRunTagСontinuously(em4100TagData_t* tagData);
void EM4100_RWE_WritterInit(em4100TagType_t type);
void EM4100_RWE_WritterWriteTag(em4100TagType_t type, em4100TagData_t* tagData);

void EM4100_RWE_TEST(void);
void EM4100_RWE_TEST_2(void);



#endif /* EM4100_RWE_EM4100_RWE_H_ */

/*
 * crypto1.h
 *
 *  Created on: 30 июл. 2020 г.
 *      Author: tochk
 */

#ifndef RC522_INC_CRYPTO1_H_
#define RC522_INC_CRYPTO1_H_


#include "stm32f4xx.h"


typedef struct
{
	uint8_t b00_07;
	uint8_t b08_15;
	uint8_t b16_23;
	uint8_t b24_31;
	uint8_t b32_39;
	uint8_t b40_47;
} LFSR_Typedef;


void Crypto1_ChipherInit(uint8_t *key, uint8_t *uid, uint8_t *tagNonce);
void Crypto1_CalculateKeyStream(uint8_t streamSize);
void Crypto1_GetReaderNonce(uint8_t *readerNonce);
void Crypto1_EncryptWithKeyStream(uint8_t *src, uint8_t *dst, uint8_t size);
void Crypto1_CalculateSuc(uint8_t src[4], uint8_t sucCount);


#endif /* RC522_INC_CRYPTO1_H_ */

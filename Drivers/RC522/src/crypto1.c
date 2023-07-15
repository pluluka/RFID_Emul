/*
 * crypto1.c
 *
 *  Created on: 30 июл. 2020 г.
 *      Author: tochk
 */

#include "crypto1.h"


#define CHALLENGE_NONCE_SIZE     4


#define Fa(a, b, c, d)	( ((a | b) ^ (a & b)) ^ (c & (a ^ b) | d) )

#define Fb(a, b, c, d)	( ((a & b) | c) ^ ((a ^ b) & (c | d)) )

#define Fc(a, b, c, d, e)	( (a | ((b | e) & (d ^ e))) ^ ((a ^ (b & d)) & ((c ^ d) | (b & e))) )


uint8_t LFSR[6];
uint8_t UID[6];
uint8_t NT[4];
uint8_t KS[6];


__STATIC_INLINE uint8_t FeedBack_48(uint8_t *lfsr);
__STATIC_INLINE uint8_t KeyStream_48(uint8_t *lfsr);
__STATIC_INLINE void Shift_LFSR_48(uint8_t *lfsr);
__STATIC_INLINE uint8_t FeedBack_16(uint8_t *reg);


void Crypto1_ChipherInit(uint8_t *key, uint8_t *uid, uint8_t *tagNonce)
{
	LFSR[0] = key[0];
	LFSR[1] = key[1];
	LFSR[2] = key[2];
	LFSR[3] = key[3];
	LFSR[4] = key[4];
	LFSR[5] = key[5];

	UID[0] = uid[0];
	UID[1] = uid[1];
	UID[2] = uid[2];
	UID[3] = uid[3];
	UID[4] = uid[4];
	UID[5] = uid[5];

	NT[0] = tagNonce[0];
	NT[1] = tagNonce[1];
	NT[2] = tagNonce[2];
	NT[3] = tagNonce[3];
}

/*
 * Вычисление ключевого потока.
 * [in] streamSize: размер потока в байтах
 *     (каждый байт соответствует операцииям 8-ми сдвигов регистра LSFR)
 * Поток сохраняется в регистре KS
 */
void Crypto1_CalculateKeyStream(uint8_t streamSize)
{
	uint8_t calc_t;

	memset((uint8_t *)KS, 0x00, 6);

	for (uint8_t i = 0; i < streamSize; i ++)
	{
		//вычисляем bit 0 октет i
		Shift_LFSR_48(LFSR);
		calc_t = FeedBack_48(LFSR);
		calc_t ^= NT[i] >> 7;
		calc_t ^= UID[i] >> 7;
		LFSR[5] |= calc_t & 0x01;
		calc_t = KeyStream_48(LFSR) & 0x01;
		calc_t <<= 7;
		KS[i] |= calc_t;
		//вычисляем bit 1 октет i
		Shift_LFSR_48(LFSR);
		calc_t = FeedBack_48(LFSR);
		calc_t ^= NT[i] >> 6;
		calc_t ^= UID[i] >> 6;
		LFSR[5] |= calc_t & 0x01;
		calc_t = KeyStream_48(LFSR) & 0x01;
		calc_t <<= 6;
		KS[i] |= calc_t;
		//вычисляем bit 2 октет i
		Shift_LFSR_48(LFSR);
		calc_t = FeedBack_48(LFSR);
		calc_t ^= NT[i] >> 5;
		calc_t ^= UID[i] >> 5;
		LFSR[5] |= calc_t & 0x01;
		calc_t = KeyStream_48(LFSR) & 0x01;
		calc_t <<= 5;
		KS[i] |= calc_t;
		//вычисляем bit 3 октет i
		Shift_LFSR_48(LFSR);
		calc_t = FeedBack_48(LFSR);
		calc_t ^= NT[i] >> 4;
		calc_t ^= UID[i] >> 4;
		LFSR[5] |= calc_t & 0x01;
		calc_t = KeyStream_48(LFSR) & 0x01;
		calc_t <<= 4;
		KS[i] |= calc_t;
		//вычисляем bit 4 октет i
		Shift_LFSR_48(LFSR);
		calc_t = FeedBack_48(LFSR);
		calc_t ^= NT[i] >> 3;
		calc_t ^= UID[i] >> 3;
		LFSR[5] |= calc_t & 0x01;
		calc_t = KeyStream_48(LFSR) & 0x01;
		calc_t <<= 3;
		KS[i] |= calc_t;
		//вычисляем bit 5 октет i
		Shift_LFSR_48(LFSR);
		calc_t = FeedBack_48(LFSR);
		calc_t ^= NT[i] >> 2;
		calc_t ^= UID[i] >> 2;
		LFSR[5] |= calc_t & 0x01;
		calc_t = KeyStream_48(LFSR) & 0x01;
		calc_t <<= 2;
		KS[i] |= calc_t;
		//вычисляем bit 6 октет i
		Shift_LFSR_48(LFSR);
		calc_t = FeedBack_48(LFSR);
		calc_t ^= NT[i] >> 1;
		calc_t ^= UID[i] >> 1;
		LFSR[5] |= calc_t & 0x01;
		calc_t = KeyStream_48(LFSR) & 0x01;
		calc_t <<= 1;
		KS[i] |= calc_t;
		//вычисляем bit 7 октет i
		Shift_LFSR_48(LFSR);
		calc_t = FeedBack_48(LFSR);
		calc_t ^= NT[i] >> 0;
		calc_t ^= UID[i] >> 0;
		LFSR[5] |= calc_t & 0x01;
		calc_t = KeyStream_48(LFSR) & 0x01;
		calc_t <<= 0;
		KS[i] |= calc_t;
	}
}


/*
 * Функция обратной связи для 48-битн регистра LFSR
 * (вычисленное значение содержится в нулевом бите)
 */
__STATIC_INLINE uint8_t FeedBack_48(uint8_t *lfsr)
{
	uint8_t tmp;

	tmp =  (lfsr[0] >> 7); // bit 0
	tmp ^= (lfsr[0] >> 2); // bit 5
	tmp ^= (lfsr[1] >> 6); // bit 9
	tmp ^= (lfsr[1] >> 5); // bit 10
	tmp ^= (lfsr[1] >> 3); // bit 12
	tmp ^= (lfsr[1] >> 1); // bit 14
	tmp ^= (lfsr[1] >> 0); // bit 15
	tmp ^= (lfsr[2] >> 6); // bit 17
	tmp ^= (lfsr[2] >> 4); // bit 19
	tmp ^= (lfsr[3] >> 7); // bit 24
	tmp ^= (lfsr[3] >> 6); // bit 25
	tmp ^= (lfsr[3] >> 4); // bit 27
	tmp ^= (lfsr[4] >> 4); // bit 35
	tmp ^= (lfsr[4] >> 0); // bit 39
	tmp ^= (lfsr[5] >> 6); // bit 41
	tmp ^= (lfsr[5] >> 5); // bit 42
	tmp ^= (lfsr[5] >> 4); // bit 43

	//tmp &= 0x01;

	return tmp;
}

__STATIC_INLINE uint8_t FeedBack_16(uint8_t *reg)
{
	uint8_t tmp;

	tmp =  (reg[0] >> 7); // bit 0
	tmp ^= (reg[0] >> 5); // bit 2
	tmp ^= (reg[0] >> 4); // bit 3
	tmp ^= (reg[0] >> 2); // bit 5

	return tmp;
}

/*
 * Функция ключевого потока для 48-битн регистра LFSR
 * (вычисленное значение содержится в нулевом бите return value)
 */
__STATIC_INLINE uint8_t KeyStream_48(uint8_t *lfsr)
{
	uint8_t a, b, c, d, e;

	a = Fa((lfsr[1] >> 6), (lfsr[1] >> 4), (lfsr[1] >> 2), (lfsr[1] >> 0));
	b = Fb((lfsr[2] >> 6), (lfsr[2] >> 4), (lfsr[2] >> 2), (lfsr[2] >> 0));
	c = Fb((lfsr[3] >> 6), (lfsr[3] >> 4), (lfsr[3] >> 2), (lfsr[3] >> 0));
	d = Fa((lfsr[4] >> 6), (lfsr[4] >> 4), (lfsr[4] >> 2), (lfsr[4] >> 0));
	e = Fb((lfsr[5] >> 6), (lfsr[5] >> 4), (lfsr[5] >> 2), (lfsr[5] >> 0));

	return Fc(a, b, c, d, e);
}

/*
 * Сдвиг 48-битн регистра LFSR.
 * 47-й бит после выполнения данной функции необходимо заполнить
 * значением 0-го бита вычесленным FeedBack функцией
 */
__STATIC_INLINE void Shift_LFSR_48(uint8_t *lfsr)
{
	lfsr[0] <<= 1;	lfsr[0] |= (lfsr[1] >> 7);
	lfsr[1] <<= 1;	lfsr[1] |= (lfsr[2] >> 7);
	lfsr[2] <<= 1;	lfsr[2] |= (lfsr[3] >> 7);
	lfsr[3] <<= 1;	lfsr[3] |= (lfsr[4] >> 7);
	lfsr[4] <<= 1;	lfsr[4] |= (lfsr[5] >> 7);
	lfsr[5] <<= 1;
}

/*
 * Формирует объявление ридера
 * (По идее тут необходим псевдорандом генерератор)
 * [out] nonce: Nr
 */
void Crypto1_GetReaderNonce(uint8_t *readerNonce)
{
	readerNonce[0] = 0x5F;
	readerNonce[1] = 0xD1;
	readerNonce[2] = 0x07;
	readerNonce[3] = 0xE2;
}

/*
 * Шифрование данных ключевым потоком KS
 * [in] src: шифруемые данные
 * [in] dst: зашифрованные данные
 * [in] size: размер данных в байтах
 */
void Crypto1_EncryptWithKeyStream(uint8_t *src, uint8_t *dst, uint8_t size)
{
	for (uint8_t i = 0; i < size; i ++)
	{
		dst[i] = src[i] ^ KS[i];
	}
}

/*
 * Succesor функция.
 * [in] src: буфер с данными
 * [in] sucCount: количество итераций (64 - для Ar, 96 - для At)
 */
void Crypto1_CalculateSuc(uint8_t src[4], uint8_t sucCount)
{
	uint8_t fl;

	for (uint8_t i = 0; i < sucCount; i ++)
	{
		fl = FeedBack_16(&src[2]);
		src[0] <<= 1;	src[0] |= (src[1] >> 7);
		src[1] <<= 1;	src[1] |= (src[2] >> 7);
		src[2] <<= 1;	src[2] |= (src[3] >> 7);
		src[3] <<= 1;	src[1] |= fl;
	}

}


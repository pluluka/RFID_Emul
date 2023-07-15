/*
 * Metakom.c
 *
 *  Created on: 6 сент. 2021 г.
 *      Author: tochk
 */


#include <Metakom.h>

#define METAKOM_GPIO			GPIOB
#define METAKOM_PIN				LL_GPIO_PIN_10


#define SET_WIRE_HIGH()		LL_GPIO_SetOutputPin(METAKOM_GPIO, METAKOM_PIN)
#define SET_WIRE_LOW()		LL_GPIO_ResetOutputPin(METAKOM_GPIO, METAKOM_PIN)


#define TIME_BIT		150
#define TIME_0_LOW		(uint32_t)(TIME_BIT * 0.4)
#define TIME_0_HIGH		(uint32_t)(TIME_BIT * 0.6)
#define TIME_1_LOW		(uint32_t)(TIME_BIT * 0.6)
#define TIME_1_HIGH		(uint32_t)(TIME_BIT * 0.4)



__STATIC_INLINE void WriteSynchBit(void);
__STATIC_INLINE void WriteZeroBit(void);
__STATIC_INLINE void WriteOneBit(void);
void WriteStartWord(void);
void WriteWord(uint8_t word);


void MetakomInit(void)
{
	LL_GPIO_InitTypeDef	GPIO_InitStruct =			{0};

	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
	GPIO_InitStruct.Pin = METAKOM_PIN;
	//GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_MEDIUM;
	//GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	//GPIO_InitStruct.Alternate = LL_GPIO_AF_1;
	LL_GPIO_Init(METAKOM_GPIO, &GPIO_InitStruct);

	LL_GPIO_SetPinOutputType(METAKOM_GPIO, METAKOM_PIN, LL_GPIO_OUTPUT_OPENDRAIN);
	LL_GPIO_SetPinMode(METAKOM_GPIO, METAKOM_PIN, LL_GPIO_MODE_OUTPUT);
	LL_GPIO_SetAFPin_0_7(METAKOM_GPIO, METAKOM_PIN, LL_GPIO_AF_0);

	delay_Init();
}


void MetakomWriteKey(MetakomKey_t *key)
{
	WriteSynchBit();
	WriteStartWord();
	WriteWord(key->key[0]);
	WriteWord(key->key[1]);
	WriteWord(key->key[2]);
	WriteWord(key->key[3]);
	WriteSynchBit();
}




__STATIC_INLINE void WriteSynchBit(void)
{
	SET_WIRE_HIGH();
	delay_us(TIME_BIT);
}

__STATIC_INLINE void WriteZeroBit(void)
{
	SET_WIRE_LOW();
	delay_us(TIME_0_LOW);
	SET_WIRE_HIGH();
	delay_us(TIME_0_HIGH);
}

__STATIC_INLINE void WriteOneBit(void)
{
	SET_WIRE_LOW();
	delay_us(TIME_1_LOW);
	SET_WIRE_HIGH();
	delay_us(TIME_1_HIGH);
}

void WriteStartWord(void)
{
	WriteZeroBit();
	WriteOneBit();
	WriteZeroBit();
}

void WriteWord(uint8_t word)
{
	((word >> 0) & 0x01) ? WriteOneBit() : WriteZeroBit();
	((word >> 1) & 0x01) ? WriteOneBit() : WriteZeroBit();
	((word >> 2) & 0x01) ? WriteOneBit() : WriteZeroBit();
	((word >> 3) & 0x01) ? WriteOneBit() : WriteZeroBit();
	((word >> 4) & 0x01) ? WriteOneBit() : WriteZeroBit();
	((word >> 5) & 0x01) ? WriteOneBit() : WriteZeroBit();
	((word >> 6) & 0x01) ? WriteOneBit() : WriteZeroBit();

	(((word >> 0) ^ (word >> 1) ^ (word >> 2) ^ (word >> 3) ^ (word >> 4) ^
		(word >> 5) ^ (word >> 6)) & 0x01) ? WriteOneBit() : WriteZeroBit();
}




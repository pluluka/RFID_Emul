/*
 * T5557_5577.c
 *
 *  Created on: 24 мая 2021 г.
 *      Author: tochk
 */

#include "T5557_5577.h"


/*	Периферия генератора 125КГц   [Carrier_125K]*/
#define	CARRIER_TIMER				TIM3
#define	CARRIER_TIMER_CHANNEL		LL_TIM_CHANNEL_CH3
#define	CARRIER_GPIO				GPIOB
#define	CARRIER_GPIO_PIN			LL_GPIO_PIN_0

#define CARRIER_FREQ				125000U								// Частота несущей
#define CARRIER_PERIOD				(uint32_t)(1000000 / CARRIER_FREQ)	// Период (мкс)



/* Данные на карту передаются посредством включения/отключения несущей
    на определенные временные промежутки, которые представлены ниже
    в виде полных циклов периода несущей. (FC)		*/
#define S_GAP_FC			25	// инициация начала передачи данных, F-выкл (по ДШ 10 - 50)
#define W_GAP_FC			20	// промежуток между битами данных, F-выкл (по ДШ 8 - 30)
#define DATA_0_FC			24	// бит данных '0', F-вкл	(по ДШ 16 - 31)
#define DATA_1_FC			56	// бит данных '1', F-вкл	(по ДШ 48 - 63)

#define TAG_PROG_TIME		5600 // время на внутренние операции записи метки, мкс

/* Opcodes*/
#define OPC_RESET				0x00
#define OPC_TESTMODE			0x01
#define OPC_WRITE_PAGE_0		0x02
#define OPC_WRITE_PAGE_1		0x03


uint32_t MakeConfigurationBlock_EM4100(void);
__STATIC_INLINE void Carrier_ON(void);
__STATIC_INLINE void Carrier_OFF(void);
__STATIC_INLINE void SGap(void);
__STATIC_INLINE void WGap(void);
__STATIC_INLINE void Write1(void);
__STATIC_INLINE void Write0(void);
__STATIC_INLINE uint8_t CalcParity4(const uint8_t data);



void T5577_Init(void)
{
	LL_TIM_InitTypeDef 			TIM_InitStruct = 			{0};
	LL_TIM_OC_InitTypeDef 		TIM_OC_InitStruct = 		{0};
	LL_GPIO_InitTypeDef			GPIO_InitStruct =			{0};

	delay_Init();

	/*  	Генератор несущей 125КГц (таймер в режиме PWM)	*/
	uint32_t pwmPeriod = (uint32_t)(SystemCoreClock/CARRIER_FREQ);

	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM3);
	TIM_InitStruct.Prescaler = 1 - 1;
	TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
	TIM_InitStruct.Autoreload = pwmPeriod - 1 ;
	TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
	LL_TIM_Init(CARRIER_TIMER, &TIM_InitStruct);
	LL_TIM_DisableARRPreload(CARRIER_TIMER);
	LL_TIM_OC_EnablePreload(CARRIER_TIMER, CARRIER_TIMER_CHANNEL);
	TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_PWM1;
	TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_DISABLE;
	TIM_OC_InitStruct.OCNState = LL_TIM_OCSTATE_DISABLE;
	TIM_OC_InitStruct.CompareValue = (uint32_t)(pwmPeriod / 2);
	TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_HIGH;
	LL_TIM_OC_Init(CARRIER_TIMER, CARRIER_TIMER_CHANNEL, &TIM_OC_InitStruct);
	LL_TIM_OC_DisableFast(CARRIER_TIMER, CARRIER_TIMER_CHANNEL);
	LL_TIM_SetTriggerOutput(CARRIER_TIMER, LL_TIM_TRGO_RESET);
	LL_TIM_DisableMasterSlaveMode(CARRIER_TIMER);

	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
	GPIO_InitStruct.Pin = CARRIER_GPIO_PIN;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_MEDIUM;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
	GPIO_InitStruct.Alternate = LL_GPIO_AF_2;
	LL_GPIO_Init(CARRIER_GPIO, &GPIO_InitStruct);

	Carrier_ON();
}

/*
 * Несущая выкл.
 */
__STATIC_INLINE void Carrier_ON(void)
{
	LL_TIM_CC_EnableChannel(CARRIER_TIMER, CARRIER_TIMER_CHANNEL);
	LL_TIM_EnableCounter(CARRIER_TIMER);
}

/*
 * Несущая вкл.
 */
__STATIC_INLINE void Carrier_OFF(void)
{
	LL_TIM_DisableCounter(CARRIER_TIMER);
	LL_TIM_CC_DisableChannel(CARRIER_TIMER, CARRIER_TIMER_CHANNEL);
	//LL_GPIO_ResetOutputPin(CARRIER_GPIO, CARRIER_GPIO_PIN);
}

/*
 * Стандартная запись блока
 * Временные промежутки формируются циклами программных задержек,
 * поэтому в случае использования RTOS необходимо это учитывать
 * (например использовать кооперативную многозадачность или критические секции)
 */
void StandartWrite(uint8_t opCode, uint8_t lockBit, uint32_t blockData, uint8_t blockAddr)
{
	//printf("%08X\n", blockData);

	// Opcode
	SGap();
	((opCode >> 1) & 1U) ? Write1() : Write0();

	WGap();
	((opCode >> 0) & 1U) ? Write1() : Write0();

	// LockBit
	WGap();
	((lockBit) & 0x01) ? Write1() : Write0();

	// Block Data
	for (uint8_t i = 0; i < 32; i ++)
	{
		WGap();
		((blockData >> (31 - i)) & 1U) ? Write1() : Write0();
	}

	// Block Address
	for (uint8_t i = 0; i < 3; i ++)
	{
		WGap();
		((blockAddr >> (2 - i)) & 1U) ? Write1() : Write0();
	}
	WGap();
	// Далее необходимо дать метке время на внутренние операций записи
	Carrier_ON();
	delay_us(TAG_PROG_TIME);
}

/*
 *	Start Gap - указание метке на начало процедуры записи
 */
__STATIC_INLINE void SGap(void)
{
	Carrier_OFF();
	delay_us(S_GAP_FC * CARRIER_PERIOD);
}

/*
 * Запись лог '1'
 */
__STATIC_INLINE void Write1(void)
{
	Carrier_ON();
	delay_us(DATA_1_FC * CARRIER_PERIOD);
}

/*
 * Запись лог '0'
 */
__STATIC_INLINE void Write0(void)
{
	Carrier_ON();
	delay_us(DATA_0_FC * CARRIER_PERIOD);
}

/*
 * Разделитель между битами данных
 */
__STATIC_INLINE void WGap(void)
{
	Carrier_OFF();
	delay_us(W_GAP_FC * CARRIER_PERIOD);
}

/*
 * Вычисление бита четности младших 4-х бит
 * [in] data: байт с данными
 */
__STATIC_INLINE uint8_t CalcParity4(const uint8_t data)
{
	return ((data >> 3) ^ (data >> 2) ^ (data >> 1) ^ (data >> 0)) & 0x01;
}


/*
 * Запись обычной метки EM4100
 * [in] tagData:	данные метки (5 байт, например - {0x31, 0x00, 0x23, 0x34, 0xD3})
 */
void T5577_WriteTagEM4100Simple(uint32_t modulation, uint32_t bitrate, uint8_t * tagData)
{
	uint32_t blockData;

	/* Конфигурационный блок  (Block 0)*/
	// Safer key
	blockData = 0;

	blockData |= T5557_SAFER_KEY;
	// Data bit-rate
	blockData |= bitrate;
	// Modulation
	blockData |= modulation;
	// PWD (password off)
	blockData &=~T5557_PWD_BIT;
	// Max block (1)
	blockData |=T5557_MAX_BLOCK_2;


	StandartWrite(OPC_WRITE_PAGE_0, 0, blockData, 0);

	/* Первая часть фрейма  (Block 1)*/
	blockData = 0;
	// заголовок
	blockData |= 0b111111111 << 23;
	// D0 - D3
	blockData |= ((tagData[0] >> 4) & 0xFU) << 19;
	// P0
	blockData |= CalcParity4(tagData[0] >> 4) << 18;
	// D4 - D7
	blockData |= ((tagData[0] >> 0) & 0xFU) << 14;
	// P1
	blockData |= CalcParity4(tagData[0] >> 0) << 13;
	// D8 - D11
	blockData |= ((tagData[1] >> 4) & 0xFU) << 9;
	// P2
	blockData |= CalcParity4(tagData[1] >> 4) << 8;
	// D12 - D15
	blockData |= ((tagData[1] >> 0) & 0xFU) << 4;
	// P3
	blockData |= CalcParity4(tagData[1] >> 0) << 3;
	// D16 - D18
	blockData |= ((tagData[2] >> 5) & 0x7U) << 0;

	StandartWrite(OPC_WRITE_PAGE_0, 0, blockData, 1);

	/* Вторая часть фрейма (Block 2)*/
	blockData = 0;
	// D19
	blockData |= ((tagData[2] >> 4) & 0x1U) << 31;
	// P4
	blockData |= CalcParity4(tagData[2] >> 4) << 30;
	// D20 - D23
	blockData |= ((tagData[2] >> 0) & 0xFU) << 26;
	// P5
	blockData |= CalcParity4(tagData[2] >> 0) << 25;
	// D24 - D27
	blockData |= ((tagData[3] >> 4) & 0xFU) << 21;
	// P6
	blockData |= CalcParity4(tagData[3] >> 4) << 20;
	// D28 - D31
	blockData |= ((tagData[3] >> 0) & 0xFU) << 16;
	// P7
	blockData |= CalcParity4(tagData[3] >> 0) << 15;
	// D32 - D35
	blockData |= ((tagData[4] >> 4) & 0xFU) << 11;
	// P8
	blockData |= CalcParity4(tagData[4] >> 4) << 10;
	// D36 - D39
	blockData |= ((tagData[4] >> 0) & 0xFU) << 6;
	// P9
	blockData |= CalcParity4(tagData[4] >> 0) << 5;
	// PC0 - PC4
	uint8_t pc = 0;
	pc = (tagData[0] >> 4) ^ (tagData[0] >> 0) ^ (tagData[1] >> 4) ^ (tagData[1] >> 0) ^
			(tagData[2] >> 4) ^ (tagData[2] >> 0) ^ (tagData[3] >> 4) ^ (tagData[3] >> 0) ^
			(tagData[4] >> 4) ^ (tagData[4] >> 0);
	blockData |=(pc & 0xFU) << 1;
	// Stop Bit
	blockData |= 0 << 0;

	StandartWrite(OPC_WRITE_PAGE_0, 0, blockData, 2);
}


void T5577_WriteTagEM4100Debug(void)
{
	uint32_t blockData;

	uint8_t tagData[5];

	uint8_t pc = 0;



	/* Конфигурационный блок  (Block 0)*/
	// Safer key
	blockData = 0;

	blockData |= T5557_SAFER_KEY;
	// Data bit-rate
	blockData |= T5557_BIT_RATE_RF64;
	// Modulation
	blockData |= T5557_MODULATION_MANCHESTER;
	// PWD (password off)
	blockData &=~ T5557_PWD_BIT;
	// Max block (1)
	blockData |= T5557_MAX_BLOCK_4;

	StandartWrite(OPC_WRITE_PAGE_0, 0, blockData, 0);


	//******************************* DATA ********************************//
	tagData[0] = 0x37;
	tagData[1] = 0x00;
	tagData[2] = 0x9F;
	tagData[3] = 0x91;
	tagData[4] = 0x20;


	/* Первая часть фрейма  (Block 1)*/
	blockData = 0;
	// заголовок
	blockData |= 0b111111111 << 23;
	// D0 - D3
	blockData |= ((tagData[0] >> 4) & 0xFU) << 19;
	// P0
	blockData |= CalcParity4(tagData[0] >> 4) << 18;
	// D4 - D7
	blockData |= ((tagData[0] >> 0) & 0xFU) << 14;
	// P1
	blockData |= CalcParity4(tagData[0] >> 0) << 13;
	// D8 - D11
	blockData |= ((tagData[1] >> 4) & 0xFU) << 9;
	// P2
	blockData |= CalcParity4(tagData[1] >> 4) << 8;
	// D12 - D15
	blockData |= ((tagData[1] >> 0) & 0xFU) << 4;
	// P3
	blockData |= CalcParity4(tagData[1] >> 0) << 3;
	// D16 - D18
	blockData |= ((tagData[2] >> 5) & 0x7U) << 0;

	StandartWrite(OPC_WRITE_PAGE_0, 0, blockData, 1);

	/* Вторая часть фрейма (Block 2)*/
	blockData = 0;
	// D19
	blockData |= ((tagData[2] >> 4) & 0x1U) << 31;
	// P4
	blockData |= CalcParity4(tagData[2] >> 4) << 30;
	// D20 - D23
	blockData |= ((tagData[2] >> 0) & 0xFU) << 26;
	// P5
	blockData |= CalcParity4(tagData[2] >> 0) << 25;
	// D24 - D27
	blockData |= ((tagData[3] >> 4) & 0xFU) << 21;
	// P6
	blockData |= CalcParity4(tagData[3] >> 4) << 20;
	// D28 - D31
	blockData |= ((tagData[3] >> 0) & 0xFU) << 16;
	// P7
	blockData |= CalcParity4(tagData[3] >> 0) << 15;
	// D32 - D35
	blockData |= ((tagData[4] >> 4) & 0xFU) << 11;
	// P8
	blockData |= CalcParity4(tagData[4] >> 4) << 10;
	// D36 - D39
	blockData |= ((tagData[4] >> 0) & 0xFU) << 6;
	// P9
	blockData |= CalcParity4(tagData[4] >> 0) << 5;
	// PC0 - PC4
	pc = (tagData[0] >> 4) ^ (tagData[0] >> 0) ^ (tagData[1] >> 4) ^ (tagData[1] >> 0) ^
			(tagData[2] >> 4) ^ (tagData[2] >> 0) ^ (tagData[3] >> 4) ^ (tagData[3] >> 0) ^
			(tagData[4] >> 4) ^ (tagData[4] >> 0);
	blockData |=(pc & 0xFU) << 1;
	// Stop Bit
	blockData |= 0 << 0;

	StandartWrite(OPC_WRITE_PAGE_0, 0, blockData, 2);
	//**************************************************************************//


	//******************************* DATA 2 ********************************//
	tagData[0] = 0x0A;
	tagData[1] = 0x00;
	tagData[2] = 0x56;
	tagData[3] = 0x5D;
	tagData[4] = 0x3B;


	/* Первая часть фрейма  (Block 1)*/
	blockData = 0;
	// заголовок
	blockData |= 0b111111111 << 23;
	// D0 - D3
	blockData |= ((tagData[0] >> 4) & 0xFU) << 19;
	// P0
	blockData |= CalcParity4(tagData[0] >> 4) << 18;
	// D4 - D7
	blockData |= ((tagData[0] >> 0) & 0xFU) << 14;
	// P1
	blockData |= CalcParity4(tagData[0] >> 0) << 13;
	// D8 - D11
	blockData |= ((tagData[1] >> 4) & 0xFU) << 9;
	// P2
	blockData |= CalcParity4(tagData[1] >> 4) << 8;
	// D12 - D15
	blockData |= ((tagData[1] >> 0) & 0xFU) << 4;
	// P3
	blockData |= CalcParity4(tagData[1] >> 0) << 3;
	// D16 - D18
	blockData |= ((tagData[2] >> 5) & 0x7U) << 0;

	StandartWrite(OPC_WRITE_PAGE_0, 0, blockData, 3);

	/* Вторая часть фрейма (Block 2)*/
	blockData = 0;
	// D19
	blockData |= ((tagData[2] >> 4) & 0x1U) << 31;
	// P4
	blockData |= CalcParity4(tagData[2] >> 4) << 30;
	// D20 - D23
	blockData |= ((tagData[2] >> 0) & 0xFU) << 26;
	// P5
	blockData |= CalcParity4(tagData[2] >> 0) << 25;
	// D24 - D27
	blockData |= ((tagData[3] >> 4) & 0xFU) << 21;
	// P6
	blockData |= CalcParity4(tagData[3] >> 4) << 20;
	// D28 - D31
	blockData |= ((tagData[3] >> 0) & 0xFU) << 16;
	// P7
	blockData |= CalcParity4(tagData[3] >> 0) << 15;
	// D32 - D35
	blockData |= ((tagData[4] >> 4) & 0xFU) << 11;
	// P8
	blockData |= CalcParity4(tagData[4] >> 4) << 10;
	// D36 - D39
	blockData |= ((tagData[4] >> 0) & 0xFU) << 6;
	// P9
	blockData |= CalcParity4(tagData[4] >> 0) << 5;
	// PC0 - PC4
	pc = (tagData[0] >> 4) ^ (tagData[0] >> 0) ^ (tagData[1] >> 4) ^ (tagData[1] >> 0) ^
			(tagData[2] >> 4) ^ (tagData[2] >> 0) ^ (tagData[3] >> 4) ^ (tagData[3] >> 0) ^
			(tagData[4] >> 4) ^ (tagData[4] >> 0);
	blockData |=(pc & 0xFU) << 1;
	// Stop Bit
	blockData |= 0 << 0;

	StandartWrite(OPC_WRITE_PAGE_0, 0, blockData, 4);
	//**************************************************************************//
}




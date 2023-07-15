/*
 * EM4100_RWE.c
 *
 *  Created on: 2 мая 2021 г.
 *      Author: tochk
 *
 */


#include	"EM4100_RWE.h"

/* Приоритеты обработчиков прерываний */
#define DECODER_ISR_PRIORITY			6
#define ENCODER_ISR_PRIORITY			6

/*	Периферия генератора 125КГц   [Carrier_125K]*/
#define	CARRIER_TIMER				TIM3
#define	CARRIER_TIMER_CHANNEL		LL_TIM_CHANNEL_CH3
#define	CARRIER_GPIO				GPIOB
#define	CARRIER_GPIO_PIN			LL_GPIO_PIN_0

/* Периферия декодера (манчестер)  [Decoder_IN]*/
#define	DECODER_TIMER				TIM1
#define	DECODER_TIMER_CHANNEL		LL_TIM_CHANNEL_CH4
#define	DECODER_GPIO				GPIOA
#define	DECODER_GPIO_PIN			LL_GPIO_PIN_11
#define	DECODER_TIMER_CC_IRQHandler	TIM1_CC_IRQHandler
#define	DECODER_TIMER_UP_IRQHandler	TIM1_UP_TIM10_IRQHandler

/* Периферия энкодера (манчестер)  [Encoder_OUT]*/
#define ENCODER_TIMER				TIM4
#define ENCODER_GPIO				GPIOB
#define ENCODER_GPIO_PIN			LL_GPIO_PIN_1
#define ENCODER_TIMER_UP_IRQHandler	TIM4_IRQHandler

/*  [Emulation_ON]  */
#define EMULATION_ON_GPIO			GPIOB
#define EMULATION_ON_GPIO_PIN		LL_GPIO_PIN_2


#define CARRIER_FREQUENCY			125000U	// Частота несущей


/* Временные интервалы несущей и модуляции сигнала, мкс */
#define CARRIER_PERIOD					8	// длительность периода несущей (для 125КГц Tp = 8мкс)
#define EMULATOR_BITRATE				64	// длительность одного бита эмулятора (в периодах несущей)
#define MODULATOR_FREQ_DEVIATION		19	// девиация битрейта (в процентах)

#define MODULATOR_PERIOD_LOW_EDGE(MCC)	((CARRIER_PERIOD * MCC) - (((MODULATOR_FREQ_DEVIATION * MCC)/100) * CARRIER_PERIOD))
#define MODULATOR_PERIOD_HIGH_EDGE(MCC)	((CARRIER_PERIOD * MCC) + (((MODULATOR_FREQ_DEVIATION * MCC)/100) * CARRIER_PERIOD))

#define SET_ENCODER_PIN_HIGH		LL_GPIO_SetOutputPin(ENCODER_GPIO, ENCODER_GPIO_PIN)
#define SET_ENCODER_PIN_LOW			LL_GPIO_ResetOutputPin(ENCODER_GPIO, ENCODER_GPIO_PIN)

#define ENCODER_REPTITION_CYCLES	40 // кол-во повторений эмуляции метки

#define HEAD_LENGTH					9	// длина заголовка
#define DATA_LENGTH					55	// длина тела кадра (вместе с битами четности)

/* Проверка полученного кадра по битам четности (для выключения - закомментировать) */
#define DECODER_PARITY_CHECK


/* Тело кадра с данными
D00	D01	D02	D03	 P0		8 bit version  number
D04	D05	D06	D07	 P1		or customer ID.
D08	D09	D10	D11	 P2		Each group of 4 bits 32 Data Bits
D12	D13	D14	D15	 P3		is followed by an Even
D16	D17	D18	D19	 P4		parity bit
D20	D21	D22	D23	 P5
D24	D25	D26	D27	 P6
D28	D29	D30	D31	 P7
D32	D33	D34	D35	 P8
D36	D37	D38	D39	 P9
PC0	PC1	PC2	PC3	 S0		4 column Parity bits, 1 stop bit (0) */
typedef struct {
	uint8_t frame[11];
}	dataFrame_t;
__IO dataFrame_t dataFrame = {0};

/* Статусы */
#define RS_OK		0x00
#define RS_ERROR	0xFF

em4100TagData_t 	*em4100Data;
EM4100MsgData_t		*em4100MsgData;
extern osMessageQId	msgQueueEM4100;
extern osPoolId		memPoolEM4100;
extern osPoolId		memPoolEM4100_RWE_Data;


/*=================== Машина состояний декодера (считывание метки) ====================*/
/* Вызов машины на каждом детектированном перепаде
 * На входе - поток данных манчестер инвертированный
 * начало данных определяется синхропоследовательностью [0,1].
 * Синхропоследовательность образована стоп-битом [0] и первым битом заголовка [1],
 *  далее следует оставшаяся часть заголовкока [1,1,1,1,1,1,1,1] */

/* Возможные состояния декодера */
typedef enum {
	DECODER_WAIT = 0,
	DECODER_SYNCH_DETECT,
	DECODER_HEAD_DETECT,
	DECODER_READ_DATA,
	DECODER_DONE
}	decoderState_t;

__IO decoderState_t 	decoderState;		// текущие состояния декодера
__IO uint32_t			decoderSimpleTime;	// время между активными перепадами уровней на входе декодера, мкс
__IO uint32_t			decoderBitCnt;		// счетчик бит кадра
__IO uint32_t			modulatorBitRate;	// длительность одного бита (в циклах несущей)


void fsDecoderWait(void);
void fsDecoderSynchDetect(void);
void fsDecoderHeadDetect(void);
void fsDecoderReadData(void);

// Массив указателей на функции-обработчики состояний (реализация вызова машины состояний декодера)
void (*fsDecoderState[])() = {	fsDecoderWait,
								fsDecoderSynchDetect,
								fsDecoderHeadDetect,
								fsDecoderReadData	};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////// Машина состояний энкодера (эмуляция) ///////////////////////
/*	Вызов машины - каждую половину периода длительности бита
 * Cигнал формируется манипуляцией вывода ENCODER_GPIO */

/* Возможные состояния энкодера */
typedef enum {
	ENCODER_HEAD_SEND,
	ENCODER_DATA_SEND
}	encoderState_t;

/* Режимы работы */
typedef enum {
	ENCODER_RUN_MODE_ONCE,			// одинарный проход( с заданным количеством повторений)
	ENCODER_RUN_MODE_CONTINUOUSLY	// бесперерывная эмуляция
}	encoderRunMode_t;;

__IO encoderState_t 	encoderState;			// текущие состояния энкодера
__IO uint32_t 			encoderHeadCnt;			// счетчик бит заголовка
__IO uint32_t 			encoderDataCnt;			// счетчик бит тела кадра
__IO uint32_t 			encoderReptitionCnt;	// счетчик повторений
__IO encoderRunMode_t	encoderRunMode;			// режим работы

void fsEncoderHeadSend(void);
void fsEncoderDataSend(void);

// Массив указателей на функции-обработчики состояний (реализация вызова машины состояний энкодера)
void (*fsEncoderState[])() = {fsEncoderHeadSend, fsEncoderDataSend};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



__STATIC_INLINE void Carrier_ON(void);
__STATIC_INLINE void Carrier_OFF(void);
__STATIC_INLINE void Decoder_ON(void);
__STATIC_INLINE void Decoder_OFF(void);
__STATIC_INLINE void Encoder_ON(void);
__STATIC_INLINE void Encoder_OFF(void);
uint8_t FrameParityCheck(__IO dataFrame_t * frame);
void MakeDataFromFrame(__IO dataFrame_t * frame, uint8_t * data);
void MakeFrameFromData(uint8_t *data, __IO dataFrame_t * frame);
__STATIC_INLINE uint8_t CalcParity4(const uint8_t data);


/*
 * Прерывание таймера декодера - обновление
 */
void DECODER_TIMER_UP_IRQHandler(void)
{
	// Если счетчик отсчитал до конца (за период счета активных перепадов не детектировано),
	// сбрасываем состояние декодера
	if (LL_TIM_IsActiveFlag_UPDATE(DECODER_TIMER))
	{
		LL_TIM_ClearFlag_UPDATE (DECODER_TIMER);
		decoderState = DECODER_WAIT;

		//osThreadYield();
	}
}
/*
 * Прерывание таймера декодера - захват
 */
void DECODER_TIMER_CC_IRQHandler(void)
{
	/* обнаружен захват перепада уровня */
	if (LL_TIM_IsActiveFlag_CC4 (DECODER_TIMER))
	{
		decoderSimpleTime = DECODER_TIMER->CNT;
		LL_TIM_ClearFlag_CC4 (DECODER_TIMER);
		// Вызов машины состояний декодера
		fsDecoderState[decoderState]();
		//TerminalThreadSendMsg(decoderState);//TTTTTTTTTTT
		//osThreadYield();
	}
}

/*
 * Прерывание таймера энкодера - обновление
 */
void ENCODER_TIMER_UP_IRQHandler(void)
{

	if (LL_TIM_IsActiveFlag_UPDATE(ENCODER_TIMER))
	{
		LL_TIM_ClearFlag_UPDATE (ENCODER_TIMER);
		// Вызов машины состояний энкодера
		fsEncoderState[encoderState]();
	}
}



/*
 *	Функция машины состояний декодера.
 *	Ожидание синхропоследовательности
 *  */
void fsDecoderWait(void)
{

	if (LL_GPIO_IsInputPinSet (DECODER_GPIO, DECODER_GPIO_PIN))
	{	// перепад [0 --> 1] детектирован
		DECODER_TIMER->CNT = 0;
		decoderState = DECODER_SYNCH_DETECT;
	}
	else
	{
		DECODER_TIMER->CNT = 0;
	}
}

/*
 * Функция машины состояний декодера.
 * Детектирование синхропоследовательности
 */
void fsDecoderSynchDetect(void)
{
	// Определяем BitRate
	if ((decoderSimpleTime > MODULATOR_PERIOD_LOW_EDGE(EM4100_DATA_BITRATE_RF8)) &&
		(decoderSimpleTime < MODULATOR_PERIOD_HIGH_EDGE(EM4100_DATA_BITRATE_RF8)))
		modulatorBitRate = EM4100_DATA_BITRATE_RF8;
	else if ((decoderSimpleTime > MODULATOR_PERIOD_LOW_EDGE(EM4100_DATA_BITRATE_RF16)) &&
		(decoderSimpleTime < MODULATOR_PERIOD_HIGH_EDGE(EM4100_DATA_BITRATE_RF16)))
		modulatorBitRate = EM4100_DATA_BITRATE_RF16;
	else if ((decoderSimpleTime > MODULATOR_PERIOD_LOW_EDGE(EM4100_DATA_BITRATE_RF32)) &&
		(decoderSimpleTime < MODULATOR_PERIOD_HIGH_EDGE(EM4100_DATA_BITRATE_RF32)))
		modulatorBitRate = EM4100_DATA_BITRATE_RF32;
	else if ((decoderSimpleTime > MODULATOR_PERIOD_LOW_EDGE(EM4100_DATA_BITRATE_RF40)) &&
		(decoderSimpleTime < MODULATOR_PERIOD_HIGH_EDGE(EM4100_DATA_BITRATE_RF40)))
		modulatorBitRate = EM4100_DATA_BITRATE_RF40;
	else if ((decoderSimpleTime > MODULATOR_PERIOD_LOW_EDGE(EM4100_DATA_BITRATE_RF50)) &&
		(decoderSimpleTime < MODULATOR_PERIOD_HIGH_EDGE(EM4100_DATA_BITRATE_RF50)))
		modulatorBitRate = EM4100_DATA_BITRATE_RF50;
	else if ((decoderSimpleTime > MODULATOR_PERIOD_LOW_EDGE(EM4100_DATA_BITRATE_RF64)) &&
		(decoderSimpleTime < MODULATOR_PERIOD_HIGH_EDGE(EM4100_DATA_BITRATE_RF64)))
		modulatorBitRate = EM4100_DATA_BITRATE_RF64;
	else if ((decoderSimpleTime > MODULATOR_PERIOD_LOW_EDGE(EM4100_DATA_BITRATE_RF100)) &&
		(decoderSimpleTime < MODULATOR_PERIOD_HIGH_EDGE(EM4100_DATA_BITRATE_RF100)))
		modulatorBitRate = EM4100_DATA_BITRATE_RF100;
	else if ((decoderSimpleTime > MODULATOR_PERIOD_LOW_EDGE(EM4100_DATA_BITRATE_RF128)) &&
		(decoderSimpleTime < MODULATOR_PERIOD_HIGH_EDGE(EM4100_DATA_BITRATE_RF128)))
		modulatorBitRate = EM4100_DATA_BITRATE_RF128;
	else
	{	//сброс состояния декодера
		DECODER_TIMER->CNT = 0;
		decoderState = DECODER_WAIT;
		decoderBitCnt = 0;
		return;
	}


	if (!LL_GPIO_IsInputPinSet (DECODER_GPIO, DECODER_GPIO_PIN))
	{
		DECODER_TIMER->CNT = 0;
		decoderBitCnt = 1;
		decoderState = DECODER_HEAD_DETECT;
		return;
	}
	else
	{	//сброс состояния декодера
		DECODER_TIMER->CNT = 0;
		decoderState = DECODER_WAIT;
		decoderBitCnt = 0;
	}

	/*
	modulatorBitRate = DATA_BITRATE_RF64;

	if ((decoderSimpleTime > MODULATOR_PERIOD_LOW_EDGE(modulatorBitRate)) &&
		(decoderSimpleTime < MODULATOR_PERIOD_HIGH_EDGE(modulatorBitRate)))
	{
		if (!LL_GPIO_IsInputPinSet (DECODER_GPIO, DECODER_GPIO_PIN))
		{
			DECODER_TIMER->CNT = 0;
			decoderBitCnt = 1;
			decoderState = DECODER_HEAD_DETECT;
			return;
		}
	}

	DECODER_TIMER->CNT = 0;
	decoderState = DECODER_WAIT;
	decoderBitCnt = 0;
	*/
}

/*
 * Функция машины состояний декодера.
 * Детектирование заголовка
 */
void fsDecoderHeadDetect(void)
{
	// если попался перепад на 1/2 периода, пропускаем его
	if ((decoderSimpleTime > MODULATOR_PERIOD_LOW_EDGE(modulatorBitRate)/2) &&
		(decoderSimpleTime < MODULATOR_PERIOD_HIGH_EDGE(modulatorBitRate)/2))
	{
		return;
	}
	// перепад на полном периоде, то что надо
	else if ((decoderSimpleTime > MODULATOR_PERIOD_LOW_EDGE(modulatorBitRate)) &&
			 (decoderSimpleTime < MODULATOR_PERIOD_HIGH_EDGE(modulatorBitRate)))
	{
		DECODER_TIMER->CNT = 0;
		// перепад [1 --> 0] (соответствует лог '1'), запоминаем
		if (!LL_GPIO_IsInputPinSet (DECODER_GPIO, DECODER_GPIO_PIN))
		{
			decoderBitCnt ++;
			// заголовок детектирован полностью, переходим к след. состоянию
			if (decoderBitCnt == HEAD_LENGTH)
			{
				decoderBitCnt = 0;
				decoderState = DECODER_READ_DATA;
			}
			return;
		}
		// перепад [0 --> 1] (соответствует лог '0'). Детектируем синхросигнал поновой.
		else
		{
			decoderBitCnt = 0;
			decoderState = DECODER_SYNCH_DETECT;
			return;
		}
	}
	// интервал перепада не удовлетворяет временным интервалам, сброс состояния декодера
	else
	{
		DECODER_TIMER->CNT = 0;
		decoderBitCnt = 0;
		decoderState = DECODER_WAIT;
	}
}

/*
 * Функция машины состояний декодера.
 * Чтение данных фрейма
 */
void fsDecoderReadData(void)
{
	osStatus   status;

	// если попался перепад на 1/2 периода, пропускаем его
	if ((decoderSimpleTime > MODULATOR_PERIOD_LOW_EDGE(modulatorBitRate)/2) &&
		(decoderSimpleTime < MODULATOR_PERIOD_HIGH_EDGE(modulatorBitRate)/2))
	{
		return;
	}
	// перепад на полном периоде
	else if ((decoderSimpleTime > MODULATOR_PERIOD_LOW_EDGE(modulatorBitRate)) &&
			 (decoderSimpleTime < MODULATOR_PERIOD_HIGH_EDGE(modulatorBitRate)))
	{
		DECODER_TIMER->CNT = 0;
		// записываем полученый бит в буфер
		// перепад [1 --> 0] (соответствует лог '1')
		if (!LL_GPIO_IsInputPinSet (DECODER_GPIO, DECODER_GPIO_PIN))
		{
			dataFrame.frame[(uint32_t)(decoderBitCnt / 5)]  |=  1U <<  (4 -  (uint32_t)(decoderBitCnt % 5));
		}
		// перепад [0 --> 1] (соответствует лог '0')
		else
		{
			dataFrame.frame[(uint32_t)(decoderBitCnt / 5)]  &=  ~(1U << (4 -   (uint32_t)(decoderBitCnt % 5)));
		}
		decoderBitCnt ++;

		// Данные приняты в полном объеме
		if (decoderBitCnt == DATA_LENGTH)
		{
			// тормозим декодер
			Decoder_OFF();
			Carrier_OFF();

			// ================================================================
			// Тут размещается блок инструкций для дальнейшей обработки данных.
			// (проверка на валидность, извлечение данных метки, семафор...)
			// После обработки, для дальнейшей работы, декодер необходимо
			// запускать повторно вызовом Decoder_ON()
			// ================================================================
			#ifdef DECODER_PARITY_CHECK
			if (FrameParityCheck(&dataFrame) == RS_ERROR)
			{
				Decoder_ON();
				Carrier_ON();
				return;
			}
			#endif //DECODER_PARITY_CHECK
			// Отправляем сообщение потоку EM4100 с принятыми данными
			em4100MsgData = (EM4100MsgData_t *)osPoolAlloc(memPoolEM4100);
			if (em4100MsgData != NULL)
			{
				em4100Data = (em4100TagData_t *)osPoolAlloc(memPoolEM4100_RWE_Data);
				if (em4100Data != NULL)
				{
					MakeDataFromFrame(&dataFrame, em4100Data->data);
					em4100Data->bitrate = modulatorBitRate;
					em4100Data->modulationMethod = EM4100_MODULATION_MANCHESTER;
					em4100MsgData->msgType = EM4100_THREAD_MSG__RWE_DATA_RECEIVED;
					em4100MsgData->p = (void*)em4100Data;
					// Добавляем указатель в очередь сообщений
					if (osMessagePut(msgQueueEM4100, (uint32_t)(em4100MsgData), 0) != osOK)
					{
						__debugError(0);
						// если возникли проблемы с очередью, освобождаем выделенную память
						osPoolFree(memPoolEM4100, em4100MsgData);
						osPoolFree(memPoolEM4100_RWE_Data, em4100Data);
						Decoder_ON();
						Carrier_ON();
						return;
					}
				}
				else
				{
					// если возникли проблемы с очередью, освобождаем выделенную память
					osPoolFree(memPoolEM4100, em4100MsgData);
					Decoder_ON();
					Carrier_ON();
					__debugError(0);
					return;
				}
			}
			else
			{
				__debugError(0);
				return;
			}
		}
	}
	// интервал перепада не удовлетворяет временным интервалам, сброс состояния декодера
	else
	{
		DECODER_TIMER->CNT = 0;
		decoderBitCnt = 0;
		memset(dataFrame.frame, 0x00, sizeof(dataFrame));
		decoderState = DECODER_WAIT;
	}
}

/*
 * Функция машины состояний энкодера.
 * Заголовок
 */
void fsEncoderHeadSend(void)
{
	if ((encoderHeadCnt % 2) == 0)
		SET_ENCODER_PIN_HIGH;
	else
		SET_ENCODER_PIN_LOW;

	encoderHeadCnt ++;

	// был сформирован последний бит заголовка
	if (encoderHeadCnt == HEAD_LENGTH * 2)
	{
		encoderState = ENCODER_DATA_SEND;
	}
}

/*
 * Функция машины состояний энкодера.
 * Тело кадра (вместе с битами ччетности)
 */
void fsEncoderDataSend(void)
{
	uint32_t bitCnt =	(uint32_t)(encoderDataCnt / 2);
	uint32_t byteCnt =(uint32_t)(bitCnt / 5);
	uint32_t bitOffset = 4 - ((uint32_t)(bitCnt % 5));

	uint8_t currentBit = (dataFrame.frame[byteCnt] >> bitOffset) & 1U;

	// для всех четных полупериодов выствляем 1 для '1', 0 для '0'
	if ((encoderDataCnt % 2) == 0)
		(currentBit & 1U) ? SET_ENCODER_PIN_HIGH : SET_ENCODER_PIN_LOW;
	//  и наоборот
	else
		(currentBit & 1U) ? SET_ENCODER_PIN_LOW : SET_ENCODER_PIN_HIGH;

	encoderDataCnt ++;
	// кадр передан полностью, снова передаем заголовок
	if (encoderDataCnt == DATA_LENGTH * 2)
	{
		encoderHeadCnt = 0;
		encoderDataCnt = 0;
		encoderState = ENCODER_HEAD_SEND;

		encoderReptitionCnt ++;
		if (encoderReptitionCnt >= ENCODER_REPTITION_CYCLES)
		{
			// если задан беспрерывный режим эмуляции, сбрасываем счетчик
			if (encoderRunMode == ENCODER_RUN_MODE_CONTINUOUSLY)
			{
				encoderReptitionCnt = 0;
				return;
			}
			//encoderReptitionCnt = 0;
			// Метка была отправлена нужное кол-во раз, тормозим энкодер
			Encoder_OFF();

			// Тут размещается код обработки события окончания эмуляции метки
			// (флаг, семафор, очередь сообщений и т.п..)
			// Отправляем сообщение потоку об окончании эмуляции
			// Для последующего запуска  энкодера - вызов Encoder_ON();
			em4100MsgData = (EM4100MsgData_t *)osPoolAlloc(memPoolEM4100);
			if (em4100MsgData != NULL)
			{
				em4100MsgData->msgType = EM4100_THREAD_MSG__RWE_EMUL_DONE;
				// Добавляем указатель в очередь сообщений
				if (osMessagePut(msgQueueEM4100, (uint32_t)(em4100MsgData), 0) != osOK)
				{
					__debugError(0);
					// если возникли проблемы с очередью, освобождаем выделенную память
					osPoolFree(memPoolEM4100, em4100MsgData);
					return;
				}
			}
			else
			{
				// если возникли проблемы с очередью, освобождаем выделенную память
				osPoolFree(memPoolEM4100, em4100MsgData);
				__debugError(0);
				return;
			}

		}
	}
}


/*
 * Инициализация режима ридера
 * Конфигурация портов:
 * Carrier_125K		- выход таймера (ШИМ 50/50 генератор 125КГц)
 * Decoder_IN		- вход таймера (захват)
 * Emulation_ON	- выход '0'
 * Encoder_OUT		- выход '0'
 */
void EM4100_RWE_Reader_Init(void)
{
	LL_TIM_InitTypeDef 			TIM_InitStruct = 			{0};
	LL_TIM_OC_InitTypeDef 		TIM_OC_InitStruct = 		{0};
	LL_TIM_IC_InitTypeDef		TIM_IC_InitStruct = 		{0};
	LL_GPIO_InitTypeDef			GPIO_InitStruct =			{0};

	/*  	Генератор несущей 125КГц (таймер в режиме PWM)	*/
	uint32_t pwmPeriod = (uint32_t)(SystemCoreClock/CARRIER_FREQUENCY);

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
	// Прерывание по переполнению откл
	LL_TIM_DisableIT_UPDATE(CARRIER_TIMER);
	NVIC_DisableIRQ(TIM3_IRQn);

	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
	GPIO_InitStruct.Pin = CARRIER_GPIO_PIN;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_MEDIUM;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	GPIO_InitStruct.Alternate = LL_GPIO_AF_2;
	LL_GPIO_Init(CARRIER_GPIO, &GPIO_InitStruct);

	//Carrier_ON();

	/* 	Порт - вход анализатора данных
	 	 	 (выход с демодулятора,  данные - манчестерский код)	*/

	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM1);
	TIM_InitStruct.Prescaler = (uint32_t)(SystemCoreClock/1000000U) - 1;
	TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
	TIM_InitStruct.Autoreload = 0xFFFF - 1 ;
	TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
	LL_TIM_Init(DECODER_TIMER, &TIM_InitStruct);

	TIM_IC_InitStruct.ICActiveInput = LL_TIM_ACTIVEINPUT_DIRECTTI;
	TIM_IC_InitStruct.ICFilter = LL_TIM_IC_FILTER_FDIV1;
	TIM_IC_InitStruct.ICPolarity = LL_TIM_IC_POLARITY_BOTHEDGE;
	TIM_IC_InitStruct.ICPrescaler = LL_TIM_ICPSC_DIV1;
	LL_TIM_IC_Init(DECODER_TIMER, DECODER_TIMER_CHANNEL, &TIM_IC_InitStruct);
	// Прерывание по захвату
	LL_TIM_EnableIT_CC4 (DECODER_TIMER);
	NVIC_SetPriority(TIM1_CC_IRQn, DECODER_ISR_PRIORITY);
	NVIC_EnableIRQ(TIM1_CC_IRQn);
	if (LL_TIM_IsActiveFlag_CC4 (DECODER_TIMER))
	{
		LL_TIM_ClearFlag_CC4 (DECODER_TIMER);
	}
	// Прерывание по переполнению
	LL_TIM_EnableIT_UPDATE(DECODER_TIMER);
	NVIC_SetPriority(TIM1_UP_TIM10_IRQn, DECODER_ISR_PRIORITY);
	NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);
	if (LL_TIM_IsActiveFlag_UPDATE (DECODER_TIMER))
	{
		LL_TIM_ClearFlag_UPDATE (DECODER_TIMER);
	}

	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
	GPIO_InitStruct.Pin = DECODER_GPIO_PIN;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_MEDIUM;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	GPIO_InitStruct.Alternate = LL_GPIO_AF_1;
	LL_GPIO_Init(DECODER_GPIO, &GPIO_InitStruct);

	decoderState = DECODER_WAIT;
	decoderSimpleTime = 0;
	decoderBitCnt = 0;

	//Decoder_ON();

	/*    Emulation_ON   */
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
	GPIO_InitStruct.Pin = EMULATION_ON_GPIO_PIN;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
	GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
	LL_GPIO_Init(EMULATION_ON_GPIO, &GPIO_InitStruct);
	LL_GPIO_ResetOutputPin(EMULATION_ON_GPIO, EMULATION_ON_GPIO_PIN);
	/*    Encoder_OUT   */
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
	GPIO_InitStruct.Pin = ENCODER_GPIO_PIN;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
	GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
	LL_GPIO_Init(ENCODER_GPIO, &GPIO_InitStruct);
	LL_GPIO_ResetOutputPin(ENCODER_GPIO, ENCODER_GPIO_PIN);
}

void EM4100_RWE_Reader_ON(void)
{
	Decoder_ON();
	Carrier_ON();
}

void EM4100_RWE_Reader_OFF(void)
{
	Carrier_OFF();
	Decoder_OFF();
}

/*
 * Инициализация режима эмулятора
 * Конфигурация портов:
 * Carrier_125K		- выход '0'
 * Decoder_IN		- вход плавающий
 * Emulation_ON	- выход 0/1
 * Encoder_OUT		- выход 0/1
 */
void EM4100_RWE_Emulator_Init(void)
{
	LL_TIM_InitTypeDef 			TIM_InitStruct = 			{0};
	LL_GPIO_InitTypeDef			GPIO_InitStruct =			{0};

	/*  	Счетчик будет отсчитывать длительность полубита	*/
	uint32_t halfBitPeriod = (uint32_t)((SystemCoreClock/CARRIER_FREQUENCY) * (EMULATOR_BITRATE/2));

	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM4);
	TIM_InitStruct.Prescaler = 1 - 1;
	TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
	TIM_InitStruct.Autoreload = halfBitPeriod - 1 ;
	TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
	LL_TIM_Init(ENCODER_TIMER, &TIM_InitStruct);
	// Прерывание по переполнению
	NVIC_SetPriority(TIM4_IRQn, ENCODER_ISR_PRIORITY);
	NVIC_EnableIRQ(TIM4_IRQn);
	if (LL_TIM_IsActiveFlag_UPDATE (ENCODER_TIMER))
	{
		LL_TIM_ClearFlag_UPDATE (ENCODER_TIMER);
	}

	/** Инициализация GPIO **/
	/* Encoder_OUT */
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
	GPIO_InitStruct.Pin = ENCODER_GPIO_PIN;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_MEDIUM;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
	GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
	LL_GPIO_Init(ENCODER_GPIO, &GPIO_InitStruct);
	LL_GPIO_SetOutputPin(ENCODER_GPIO, ENCODER_GPIO_PIN);
	/* Emulation_ON */
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
	GPIO_InitStruct.Pin = EMULATION_ON_GPIO_PIN;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_MEDIUM;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
	GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
	LL_GPIO_Init(EMULATION_ON_GPIO, &GPIO_InitStruct);
	/* Decoder_IN */
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
	GPIO_InitStruct.Pin = DECODER_GPIO_PIN;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	LL_GPIO_Init(DECODER_GPIO, &GPIO_InitStruct);
	/* Carrier_125K */
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
	GPIO_InitStruct.Pin = CARRIER_GPIO_PIN;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
	GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
	LL_GPIO_Init(CARRIER_GPIO, &GPIO_InitStruct);

}

/*
 * Однократная эмуляция одной метки (с заданным кол-вом повторений),
 * кол-во повторений задано в ENCODER_REPTITION_CYCLES
 * [in] tagData:	данные метки
 */
void EM4100_RWE_EmulatorRunTag(em4100TagData_t* tagData)
{
	encoderRunMode = ENCODER_RUN_MODE_ONCE;

	MakeFrameFromData(tagData->data, &dataFrame);

	Encoder_ON();
}


void EM4100_RWE_EmulatorRunTagСontinuously(em4100TagData_t* tagData)
{
	encoderRunMode = ENCODER_RUN_MODE_CONTINUOUSLY;

	MakeFrameFromData(tagData->data, &dataFrame);

	Encoder_ON();
}

/*
 * Инициализация режима записи меток EM4100
 * [in] type: тип метки
 */
void EM4100_RWE_WritterInit(em4100TagType_t type)
{
	switch (type){
	case (T5557_T5577):
	{
		T5577_Init();
	}
	case (UNDEFINE):
	{
		return;
	}
	default:
		return;
	}
}

/*
 * Запись метки EM4100
 * [in] type:			тип метки
 * [in] tagData:	данные метки
 */
void EM4100_RWE_WritterWriteTag(em4100TagType_t type, em4100TagData_t* tagData)
{
	uint32_t modulation;
	uint32_t bitrate;

	switch (type){
	case (T5557_T5577):
	{
		switch(tagData->modulationMethod){
		case (EM4100_MODULATION_MANCHESTER):
			modulation = T5557_MODULATION_MANCHESTER;
			break;
		default:
			return;
		}
		switch(tagData->bitrate){
		case (EM4100_DATA_BITRATE_RF8):
			bitrate = T5557_BIT_RATE_RF8;
			break;
		case (EM4100_DATA_BITRATE_RF16):
			bitrate = T5557_BIT_RATE_RF16;
			break;
		case (EM4100_DATA_BITRATE_RF32):
			bitrate = T5557_BIT_RATE_RF32;
			break;
		case (EM4100_DATA_BITRATE_RF40):
			bitrate = T5557_BIT_RATE_RF40;
			break;
		case (EM4100_DATA_BITRATE_RF50):
			bitrate = T5557_BIT_RATE_RF50;
			break;
		case (EM4100_DATA_BITRATE_RF64):
			bitrate = T5557_BIT_RATE_RF64;
			break;
		case (EM4100_DATA_BITRATE_RF100):
			bitrate = T5557_BIT_RATE_RF100;
			break;
		case (EM4100_DATA_BITRATE_RF128):
			bitrate = T5557_BIT_RATE_RF128;
			break;
		default:
			return;
		}
		//printf("Write tag %08X %08X\n", modulation, bitrate);
		T5577_WriteTagEM4100Simple(modulation, bitrate, tagData->data);
		break;
	}
	case (UNDEFINE):
	{
		return;
	}
	case (TAG_DEBUG):
	{
		T5577_WriteTagEM4100Debug();
		break;
	}
	default:
		return;
	}
}

/*
 * Энкодер манчестер вкл.
 */
__STATIC_INLINE void Encoder_OFF(void)
{
	// Выключаем развязку оптореле
	LL_GPIO_ResetOutputPin(EMULATION_ON_GPIO, EMULATION_ON_GPIO_PIN);

	// Тормозим таймер
	LL_TIM_DisableIT_UPDATE(ENCODER_TIMER);
	LL_TIM_DisableCounter(ENCODER_TIMER);

	LL_GPIO_SetOutputPin(ENCODER_GPIO, ENCODER_GPIO_PIN);
}

/*
 * Энкодер манчестер вкл.
 */
__STATIC_INLINE void Encoder_ON(void)
{
	// Включаем развязку оптореле
	LL_GPIO_SetOutputPin(EMULATION_ON_GPIO, EMULATION_ON_GPIO_PIN);

	// Запускаем таймер
	encoderHeadCnt = 0;
	encoderDataCnt = 0;
	encoderReptitionCnt = 0;
	encoderState = ENCODER_HEAD_SEND;
	LL_TIM_EnableIT_UPDATE(ENCODER_TIMER);
	LL_TIM_EnableCounter(ENCODER_TIMER);
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
}

/*
 * Декодер выкл.
 */
__STATIC_INLINE void Decoder_ON(void)
{
	decoderState = DECODER_WAIT;

	LL_TIM_EnableIT_CC4 (DECODER_TIMER);
	LL_TIM_CC_EnableChannel(DECODER_TIMER, DECODER_TIMER_CHANNEL);
	LL_TIM_EnableCounter(DECODER_TIMER);
}

/*
 * Декодер выкл.
 */
__STATIC_INLINE void Decoder_OFF(void)
{
	LL_TIM_DisableIT_CC4 (DECODER_TIMER);
	LL_TIM_CC_DisableChannel(DECODER_TIMER, DECODER_TIMER_CHANNEL);
	LL_TIM_DisableCounter(DECODER_TIMER);
}

/*
 * Проверка принятого фрейма на валидность по битам четности
 * (P - строки, PC - столбцы)
 * (Возможно, стоит внедрить проверку в процесс декодера)
 */
uint8_t FrameParityCheck(__IO dataFrame_t * frame)
{
	uint8_t colParity = 0;
	uint8_t strParity = 0;

	// биты четности P
	for (uint8_t i = 0; i < 10; i ++)
	{
		strParity = (frame->frame[i] >> 0) ^(frame->frame[i] >> 1) ^(frame->frame[i] >> 2) ^(frame->frame[i] >> 3) ^(frame->frame[i] >> 4);
		if (strParity & 1U)
			return RS_ERROR;
		colParity ^= frame->frame[i];
	}
	// биты четности PC
	if ((colParity >> 1) ^ (frame->frame[10] >> 1))
		return RS_ERROR;
	// стоп-бит
	if (frame->frame[10] & 1U)
		return RS_ERROR;

	return RS_OK;
}

/*
 * Сборка фрейма данных ключа из принятого кадра
 * [in] frame:	принятый декодером валидный кадр
 * [out] data:		буфер под данные метки (5 байт)
 */
void MakeDataFromFrame(__IO dataFrame_t * frame, uint8_t * data)
{
	data[0] =  ((frame->frame[0] >> 1) << 4) | (frame->frame[1] >> 1);
	data[1] =  ((frame->frame[2] >> 1) << 4) | (frame->frame[3] >> 1);
	data[2] =  ((frame->frame[4] >> 1) << 4) | (frame->frame[5] >> 1);
	data[3] =  ((frame->frame[6] >> 1) << 4) | (frame->frame[7] >> 1);
	data[4] =  ((frame->frame[8] >> 1) << 4) | (frame->frame[9] >> 1) ;
}

/*
 * Сборка кадра
 * [in]   data: данные для отправки (5 байт)
 * [out] frame: буфер под данные тела кадра
 */
void MakeFrameFromData(uint8_t *data, __IO dataFrame_t * frame)
{
	// Данные блоками по 4 бит плюс 5-й бит-четности
	// блок 0
	frame->frame[0] = (data[0] >> 3) & 0b00011110;
	frame->frame[0] |= CalcParity4(frame->frame[0] >> 1);
	// блок 1
	frame->frame[1] = (data[0] << 1) & 0b00011110;
	frame->frame[1] |= CalcParity4(frame->frame[1] >> 1);
	// блок 2
	frame->frame[2] = (data[1] >> 3) & 0b00011110;
	frame->frame[2] |= CalcParity4(frame->frame[2] >> 1);
	// блок 3
	frame->frame[3] = (data[1] << 1) & 0b00011110;
	frame->frame[3] |= CalcParity4(frame->frame[3] >> 1);
	// блок 4
	frame->frame[4] = (data[2] >> 3) & 0b00011110;
	frame->frame[4] |= CalcParity4(frame->frame[4] >> 1);
	// блок 5
	frame->frame[5] = (data[2] << 1) & 0b00011110;
	frame->frame[5] |= CalcParity4(frame->frame[5] >> 1);
	// блок 6
	frame->frame[6] = (data[3] >> 3) & 0b00011110;
	frame->frame[6] |= CalcParity4(frame->frame[6] >> 1);
	// блок 7
	frame->frame[7] = (data[3] << 1) & 0b00011110;
	frame->frame[7] |= CalcParity4(frame->frame[7] >> 1);
	// блок 8
	frame->frame[8] = (data[4] >> 3) & 0b00011110;
	frame->frame[8] |= CalcParity4(frame->frame[8] >> 1);
	// блок 9
	frame->frame[9] = (data[4] << 1) & 0b00011110;
	frame->frame[9] |= CalcParity4(frame->frame[9] >> 1);

	// блок 10 биты четности столбцов, образованных блоками данных
	frame->frame[10] = 0;
	for (uint8_t i = 0; i < 10; i++)
	{
		frame->frame[10] ^= frame->frame[i];
	}

	// стоп бит (0)
	frame->frame[10] &= 0b11111110;
}

/*
 * Вычисление бита четности младших 4-х бит
 * [in] data: байт с данными
 */
__STATIC_INLINE uint8_t CalcParity4(const uint8_t data)
{
	return ((data >> 3) ^ (data >> 2) ^ (data >> 1) ^ (data >> 0)) & 0x01;
}



void EM4100_RWE_TEST(void)
{
	uint8_t data[5];

	//Decoder_OFF();

	if (FrameParityCheck(&dataFrame) == RS_ERROR)
	{
		printf("Frame not valid! " );
		Decoder_ON();
		return;
	}

	MakeDataFromFrame(&dataFrame, data);
	for (uint8_t i = 0; i < 5; i ++)
	{
		printf("%02X ", data[i]);
	}
	printf("\n");

	Decoder_ON();
}

void EM4100_RWE_TEST_2(void)
{
	uint8_t data[5] = {0x0A, 0x00, 0x56, 0x5D, 0x3B};

	MakeFrameFromData(data, &dataFrame);
	Encoder_ON();
}




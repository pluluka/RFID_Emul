
#include "OneWireSlave.h"


/* Приоритеты обработчиков прерываний */
#define OWS_ISR_PRIORITY			(configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1)


#define MASTER_RESETPULSE_WAIT_LIMIT	// Ограничение ожидания команды MASTERа.
 	 	 	 	 	 	 	 	 	 	// Время ожидания ограничено значением TIME_DEFAULT_WAIT,
										// по истечении данного временного промежутка МС перейдет в IDLE.
										// Для отключения - закоментировать(время ожидания будет неограниченным),
										// однако в этом случае ввод МС в IDLE возможен только вручную -
										// вызовом OneWireSlaveDS1990_OffWire() (режим по умолчанию).



#define	OWS_TIMER					TIM2
#define	OWS_TIMER_CHANNEL			LL_TIM_CHANNEL_CH4
#define	OWS_GPIO					GPIOA
#define	OWS_GPIO_PIN				LL_GPIO_PIN_3
#define	OWS_TIMER_IRQHandler		TIM2_IRQHandler


#define SET_WIRE_HIGH()				LL_GPIO_SetOutputPin(OWS_GPIO, OWS_GPIO_PIN)
#define SET_WIRE_LOW()				LL_GPIO_ResetOutputPin(OWS_GPIO, OWS_GPIO_PIN)

// Times (us)
#define TIME_DEFAULT_WAIT			60000	// Время ожидания команды MASTERа
#define TIME_WRITE_ONWIRE_PULSE		100		// Время импульса "Хост One-Wire на линии"
#define TIME_END					1000	// Время до повторного вызова МС после входа в IDLE
// Initialization Procedure: Reset and Presence Pulses times
#define TIME_RSTL					480	// Reset Low Time ( > 480 )
#define TIME_PDH					20	// Presence-Detect High Time ( 15 .. 60 )
#define TIME_PDL					115	// Presence-Detect Low Time  ( 60 .. 240 )
#define TIME_REC					1	// Recovery Time			 ( > 1 )
// Write/Read time-slots
#define TIME_SLOT_MIN				61	// Time Slot Duration
#define TIME_W1L_MIN				1	// Write-One Low Time
#define TIME_W1L_MAX				15	// Write-One Low Time
#define TIME_W0L_MIN				60	// Write-Zero Low Time
#define TIME_W0L_MAX				240	// Write-Zero Low Time
#define TIME_RL						5	// Read Low Time ( 1 .. (15-g) )

// ROM Function Commands
// Поддерживаемые на данный момент (те что МС в состоянии обработать)
#define CMD_READ_ROM				0x33 // Read 8-byte ROM
#define CMD_READ_ROM_0				0x0F // Read 8-byte ROM


/*=================== Машина состояний (МС) One-Wire Slave ====================*/

typedef enum {
	INIT = 0,
	READ_COMMAND,
	WRITE_RESPONCE
}	OneWireSlaveProcessState_t;

/* Возможные состояния */
typedef enum {
	IDLE = 0,						// Бездействие, МС неактивна (соответствует физическому отсутствию на шине)
	BEGIN,							// Начало работы МС, (соответствует моменту подсоединения к шине)
	WRITE_ONWIRE_PULSE,
	READ_RESET_PULSE_BEGIN,
	READ_RESET_PULSE_END,
	WRITE_PRESENCE_PULSE_BEGIN,
	WRITE_PRESENCE_PULSE_END,
	READ_DATA_BEGIN,
	READ_DATA_END,
	DETECT_WRITE_DATA,
	WRITE_DATA,
	END,
	STOP
}	OneWireSlaveState_t;

__IO OneWireSlaveState_t			owsState;		// текущее состояние МС
__IO OneWireSlaveProcessState_t		owsProcState;	// текущий процесс

//__IO uint16_t		owsLLPT;			// длительность последнего импульса

__IO uint8_t		owsROMData[8];		// буфер ROM

__IO uint8_t		owsReadDataBitCnt;	// счетчик полученых бит данных
__IO uint8_t		owsReadData;		// полученые данные (например коды команд)
__IO uint8_t		owsWriteData[8];	// буфер с данными на отправку
__IO uint8_t		owsWriteDataLength;	// кол-во байт которые нужно отправить
__IO uint8_t		owsWriteDataCnt;	// счетчик отправленных байт
__IO uint8_t		owsWriteDataBitCnt;	// счетчик отправленных бит


uint8_t owsIsStateWithUpdateTimer(OneWireSlaveState_t state);
uint8_t owsIsStateCCTimer(OneWireSlaveState_t state);
__STATIC_INLINE void owsCommandProcessing(uint8_t cmd);

void fsIdle(void);
void fsBegin(void);
void fsWriteOnWirePulse(void);
void fsReadResetPulseBegin(void);
void fsReadResetPulseEnd(void);
void fsWritePresencePulseBegin(void);
void fsWritePresencePulseEnd(void);
void fsReadDataBegin(void);
void fsReadDataEnd(void);
void fsDetectWriteData(void);
void fsWriteData(void);
void fsEnd(void);
void fsStop(void);


// Массив указателей на функции-обработчики состояний (реализация вызова МС)
void (*fsmOWS[])() = 	{	fsIdle,
							fsBegin,
							fsWriteOnWirePulse,
							fsReadResetPulseBegin,
							fsReadResetPulseEnd,
							fsWritePresencePulseBegin,
							fsWritePresencePulseEnd,
							fsReadDataBegin,
							fsReadDataEnd,
							fsDetectWriteData,
							fsWriteData,
							fsEnd,
							fsStop	};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




__STATIC_INLINE void SetPinStateToWrite(void);
__STATIC_INLINE void SetPinStateToTimerIC(void);




/*
 * Прерывание таймера
 * Вызов МС
 */
void OWS_TIMER_IRQHandler(void)
{
	/* Счетчик отсчитал до конца */
	if (LL_TIM_IsActiveFlag_UPDATE(OWS_TIMER))
	{
		LL_TIM_ClearFlag_UPDATE (OWS_TIMER);

#ifdef MASTER_RESETPULSE_WAIT_LIMIT
		// вышло время ожидания команды MASTERа
		if ((owsState == READ_RESET_PULSE_BEGIN) || (owsState == READ_DATA_BEGIN)
			|| (owsState == DETECT_WRITE_DATA))
		{
			owsState = END;
			fsmOWS[owsState]();
			return;
		}
#endif
		// отработка событий формирования длительностей
		if (owsIsStateWithUpdateTimer(owsState))
		{
			fsmOWS[owsState]();
			return;
		}
	}

	/* обнаружен захват перепада уровня */
	if (LL_TIM_IsActiveFlag_CC4(OWS_TIMER))
	{
		LL_TIM_ClearFlag_CC4(OWS_TIMER);
		if (owsIsStateCCTimer(owsState))
		{
			fsmOWS[owsState]();
			return;
		}
	}
}


/*
 * Инициализация One-Wire Slave модуля
 */
void OneWireSlaveInit(void)
{
	LL_TIM_InitTypeDef 			TIM_InitStruct = 			{0};
	LL_TIM_IC_InitTypeDef		TIM_IC_InitStruct = 		{0};
	LL_GPIO_InitTypeDef			GPIO_InitStruct =			{0};

	// Период таймера, 1 тик - 1 мкс
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2);
	TIM_InitStruct.Prescaler = (uint32_t)(SystemCoreClock/1000000U) - 1;
	TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
	TIM_InitStruct.Autoreload = 0xFFFF - 1 ;
	TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
	LL_TIM_Init(OWS_TIMER, &TIM_InitStruct);
	// захват по обоим фронтам
	TIM_IC_InitStruct.ICActiveInput = LL_TIM_ACTIVEINPUT_DIRECTTI;
	TIM_IC_InitStruct.ICFilter = LL_TIM_IC_FILTER_FDIV1;
	TIM_IC_InitStruct.ICPolarity = LL_TIM_IC_POLARITY_BOTHEDGE;
	TIM_IC_InitStruct.ICPrescaler = LL_TIM_ICPSC_DIV1;
	LL_TIM_IC_Init(OWS_TIMER, OWS_TIMER_CHANNEL, &TIM_IC_InitStruct);

	// Прерывание по захвату
	LL_TIM_DisableIT_CC4(OWS_TIMER);
	if (LL_TIM_IsActiveFlag_CC4(OWS_TIMER))
	{
		LL_TIM_ClearFlag_CC4(OWS_TIMER);
	}
	// Прерывание по переполнению
	LL_TIM_DisableIT_UPDATE(OWS_TIMER);
	if (LL_TIM_IsActiveFlag_UPDATE (OWS_TIMER))
	{
		LL_TIM_ClearFlag_UPDATE (OWS_TIMER);
	}


	NVIC_SetPriority(TIM2_IRQn, OWS_ISR_PRIORITY);
	NVIC_EnableIRQ(TIM2_IRQn);

	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
	GPIO_InitStruct.Pin = OWS_GPIO_PIN;
	//GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
	//GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	//GPIO_InitStruct.Alternate = LL_GPIO_AF_1;
	LL_GPIO_Init(OWS_GPIO, &GPIO_InitStruct);

	// Ставим МС в режим ожидания
	owsState = IDLE;
	//fsmOWS[owsState]();
}

/*
 * Эмуляция подключения таблетки DS1990 к шине OW
 * [in] romData: rom-код таблетки
 * Для повторного вызова данной функции, необходимо сначало вызвать
 * ф-ю отсоединения таблетки от шины OW ( OneWire_DS1990_OffWire() )
 */
OWS_ReturnStatus_t OneWireSlaveDS1990_OnWire(uint8_t romData[8])
{
	if (owsState != IDLE)
		return OWS_RS_ERROR;
	memcpy(owsROMData, romData, 8);
	// Запускаем МС
	owsState = BEGIN;
	fsmOWS[owsState]();

	return OWS_RS_OK;
}

/*
 * Эмуляция отсоединения таблетки от шины OW
 * (освобождение МС - ввод в состояние IDLE)
 */
void OneWireSlaveDS1990_OffWire(void)
{
	owsState = END;
	fsmOWS[owsState]();
}

/*
 * Если МС в режиме ожидания (МС отработала полный цикл) возвращает '1'
 */
uint8_t OneWireSlaveIsIDLE(void)
{
	if (owsState == IDLE)
		return 1;
	return 0;
}

/*
 * Установка режима пина на запись в шину OW.
 * Вывод в режиме "открытый коллектор" - последовательно резистор 100 - 150 Ом
 */
__STATIC_INLINE void SetPinStateToWrite(void)
{
	LL_TIM_CC_DisableChannel(OWS_TIMER, OWS_TIMER_CHANNEL);
	LL_TIM_DisableIT_CC4(OWS_TIMER);

	//LL_GPIO_SetAFPin_0_7(OWS_GPIO, OWS_GPIO_PIN, LL_GPIO_AF_0);
	LL_GPIO_SetPinOutputType(OWS_GPIO, OWS_GPIO_PIN, LL_GPIO_OUTPUT_OPENDRAIN);
	LL_GPIO_SetPinMode(OWS_GPIO, OWS_GPIO_PIN, LL_GPIO_MODE_OUTPUT);
}

/*
 * Режим пина - вход канала захват/сравнение таймера
 * (для режимов чтения данных с шины OW)
 */
__STATIC_INLINE void SetPinStateToTimerIC(void)
{
	LL_GPIO_SetAFPin_0_7(OWS_GPIO, OWS_GPIO_PIN, LL_GPIO_AF_1);
	LL_GPIO_SetPinMode(OWS_GPIO, OWS_GPIO_PIN, LL_GPIO_MODE_ALTERNATE);
	LL_GPIO_SetPinOutputType(OWS_GPIO, OWS_GPIO_PIN, LL_GPIO_OUTPUT_PUSHPULL);

	LL_TIM_CC_EnableChannel(OWS_TIMER, OWS_TIMER_CHANNEL);
	LL_TIM_EnableIT_CC4(OWS_TIMER);
}



/*
 * Возвращает 1 если текущее состояние - одно из состояний
 * с реакцией на событие переполнения счетчика
 */
uint8_t owsIsStateWithUpdateTimer(OneWireSlaveState_t state)
{
	switch (state){
	case WRITE_ONWIRE_PULSE:
		return 1;
	case WRITE_PRESENCE_PULSE_BEGIN:
		return 1;
	case WRITE_PRESENCE_PULSE_END:
		return 1;
	case WRITE_DATA:
		return 1;
	case STOP:
		return 1;
	default:
		return 0;
	}
	return 0;
}

/*
 * Возвращает 1 если текущее состояние - одно из состояний
 * с реакцией на событие захвата таймера
 */
uint8_t owsIsStateCCTimer(OneWireSlaveState_t state)
{
	switch (state){
	case READ_RESET_PULSE_BEGIN:
		return 1;
	case READ_RESET_PULSE_END:
		return 1;
	case READ_DATA_BEGIN:
		return 1;
	case READ_DATA_END:
		return 1;
	case DETECT_WRITE_DATA:
		return 1;
	default:
		return 0;
	}
	return 0;
}

/*
 * Распознование команд и перевод МС в соответствующее состояние
 */
__STATIC_INLINE void owsCommandProcessing(uint8_t cmd)
{
	switch(cmd){
	case CMD_READ_ROM:
	{
		memcpy(owsWriteData, owsROMData, 8);
		owsWriteDataLength = 8;
		OWS_TIMER->CNT = 0;
		owsWriteDataCnt = 0;
		owsWriteDataBitCnt = 0;
		owsState = DETECT_WRITE_DATA;
		owsProcState = WRITE_RESPONCE;
	}
	case CMD_READ_ROM_0:
	{
		memcpy(owsWriteData, owsROMData, 8);
		owsWriteDataLength = 8;
		OWS_TIMER->CNT = 0;
		owsWriteDataCnt = 0;
		owsWriteDataBitCnt = 0;
		owsState = DETECT_WRITE_DATA;
		owsProcState = WRITE_RESPONCE;
	}
	default:
		break;
	}
}


/*
 * Режим ожидания (бездействие МС)
 * Здесь размещается дальнейшая обработка события
 * окончания эмуляции (флаг, семафор, сообщение и т.п..)
 */
void fsIdle(void)
{

}

/*
 * Подготовка к завершению работы МС
 */
void fsEnd(void)
{
	//TerminalThreadSendMsg(0xE0000000);// MSG
	LL_TIM_DisableCounter(OWS_TIMER);
	OWS_TIMER->CNT = 0;
	LL_TIM_SetAutoReload(OWS_TIMER, TIME_END);
	LL_TIM_CC_DisableChannel(OWS_TIMER, OWS_TIMER_CHANNEL);
	owsReadData = 0x00;
	owsReadDataBitCnt = 0;
	owsProcState = INIT;
	owsState = STOP;
	LL_TIM_EnableIT_UPDATE(OWS_TIMER);
	LL_TIM_EnableCounter(OWS_TIMER);
}

/*
 * Ввод МС в состояние IDLE
 */
void fsStop(void)
{
	//TerminalThreadSendMsg(0xEE000000);// MSG
	LL_TIM_DisableCounter(OWS_TIMER);
	owsState = IDLE;
	fsmOWS[owsState]();
}

/*
 * Стартовая функция МС.
 * Выдача в линию импульса присутствия One-Wire устройства - начало.
 */
void fsBegin(void)
{
	SetPinStateToWrite();
	SET_WIRE_LOW();
	OWS_TIMER->CNT = 0;
	LL_TIM_SetAutoReload(OWS_TIMER, TIME_WRITE_ONWIRE_PULSE);
	LL_TIM_EnableIT_UPDATE(OWS_TIMER);
	LL_TIM_EnableCounter(OWS_TIMER);
	owsState = WRITE_ONWIRE_PULSE;

	//TerminalThreadSendMsg(0xBE000000);// MSG
}

/*
 * Выдача в линию импульса присутствия One-Wire устройства - окончание.
 */
void fsWriteOnWirePulse(void)
{
	// формируем фронт импульса
	SET_WIRE_HIGH();
	// переходим к состоянию ожидания MASTER Tx "RESET PULSE"
	OWS_TIMER->CNT = 0;
	LL_TIM_SetAutoReload(OWS_TIMER, TIME_DEFAULT_WAIT);
	LL_TIM_IC_SetPolarity(OWS_TIMER, OWS_TIMER_CHANNEL, LL_TIM_IC_POLARITY_FALLING);
	SetPinStateToTimerIC();
	owsState = READ_RESET_PULSE_BEGIN;

	//OneWireThreadSendMsg__DRV_SLAVE_ONWIRE();
}

/*
 * Начало детектирования импульса - MASTER Tx "RESET PULSE"
 * Обнаружен спад на линии OW
 */
void fsReadResetPulseBegin(void)
{
	OWS_TIMER->CNT = 0;
	LL_TIM_IC_SetPolarity(OWS_TIMER, OWS_TIMER_CHANNEL, LL_TIM_IC_POLARITY_RISING);
	owsState = READ_RESET_PULSE_END;
}

/*
 * Окончание детектирования импульса - MASTER Tx "RESET PULSE"
 */
void fsReadResetPulseEnd(void)
{
	uint16_t lwTime = OWS_TIMER->CNT;
	OWS_TIMER->CNT = 0;
	// Время импульса вне допустимого
	if (lwTime < TIME_RSTL)
	{
		// сбрасываем, ждем следующего MASTER Tx "RESET PULSE"
		OWS_TIMER->CNT = 0;
		LL_TIM_SetAutoReload(OWS_TIMER, TIME_DEFAULT_WAIT);
		LL_TIM_IC_SetPolarity(OWS_TIMER, OWS_TIMER_CHANNEL, LL_TIM_IC_POLARITY_FALLING);
		SetPinStateToTimerIC();
		owsState = READ_RESET_PULSE_BEGIN;

		//TerminalThreadSendMsg(0xDD100000);// MSG
		return;
	}
	// Если всё ок, выжидаем время до формирования MASTER Rx "PRESENCE PULSE"
	LL_TIM_SetAutoReload(OWS_TIMER, TIME_PDH);
	SetPinStateToWrite();
	owsState = WRITE_PRESENCE_PULSE_BEGIN;

	//OneWireThreadSendMsg__DRV_SLAVE_RESET_DETECTED(); // MESSAGE
	//TerminalThreadSendMsg(0xDE100000);// MSG
}

/*
 * Начало формирования импульса MASTER Rx "PRESENCE PULSE"
 */
void fsWritePresencePulseBegin(void)
{
	SET_WIRE_LOW();
	OWS_TIMER->CNT = 0;
	LL_TIM_SetAutoReload(OWS_TIMER, TIME_PDL);

	owsState = WRITE_PRESENCE_PULSE_END;
}

/*
 * Окончание формирования импульса MASTER Rx "PRESENCE PULSE"
 */
void fsWritePresencePulseEnd(void)
{
	SET_WIRE_HIGH();
	OWS_TIMER->CNT = 0;
	LL_TIM_SetAutoReload(OWS_TIMER, TIME_DEFAULT_WAIT);
	LL_TIM_IC_SetPolarity(OWS_TIMER, OWS_TIMER_CHANNEL, LL_TIM_IC_POLARITY_FALLING);
	SetPinStateToTimerIC();
	// Переход к режиму чтения команды Master-a
	owsState = READ_DATA_BEGIN;
	owsProcState = READ_COMMAND;

	owsReadData = 0x00;
	owsReadDataBitCnt = 0;

	//TerminalThreadSendMsg(0xDE200000);// MSG
}

/*
 * Начало детектирования слота данных
 */
void fsReadDataBegin(void)
{
	uint16_t lwTime = OWS_TIMER->CNT;

	OWS_TIMER->CNT = 0;
	// Недостаточное время между следованием тайм-слотов - сброс
	if (lwTime < TIME_REC)
	{
		// сбрасываем, ждем следующего MASTER Tx "RESET PULSE"
		OWS_TIMER->CNT = 0;
		LL_TIM_SetAutoReload(OWS_TIMER, TIME_DEFAULT_WAIT);
		LL_TIM_IC_SetPolarity(OWS_TIMER, OWS_TIMER_CHANNEL, LL_TIM_IC_POLARITY_FALLING);
		SetPinStateToTimerIC();
		owsState = READ_RESET_PULSE_BEGIN;

		//TerminalThreadSendMsg(0xDD200000);// MSG
		return;
	}
	LL_TIM_IC_SetPolarity(OWS_TIMER, OWS_TIMER_CHANNEL, LL_TIM_IC_POLARITY_RISING);
	owsState = READ_DATA_END;
}

/*
 * Окончание детектирования слота данных
 */
void fsReadDataEnd(void)
{
	uint16_t lwTime = OWS_TIMER->CNT;
	OWS_TIMER->CNT = 0;
	// Тип таймслота - 1
	if ((lwTime > TIME_W1L_MIN) && (lwTime < TIME_W1L_MAX))
	{
		owsReadData |= (1 << owsReadDataBitCnt);
	}
	// Тип таймслота - 0
	else if ((lwTime > (TIME_W0L_MIN - 2)) && (lwTime < TIME_W0L_MAX))
	{// Тип таймслота - 0
		owsReadData &= ~(1 << owsReadDataBitCnt);
	}
	// Тайм-слот не опознан , сброс МС
	else
	{
		// сбрасываем, ждем следующего MASTER Tx "RESET PULSE"
		OWS_TIMER->CNT = 0;
		LL_TIM_SetAutoReload(OWS_TIMER, TIME_DEFAULT_WAIT);
		LL_TIM_IC_SetPolarity(OWS_TIMER, OWS_TIMER_CHANNEL, LL_TIM_IC_POLARITY_FALLING);
		SetPinStateToTimerIC();
		owsState = READ_RESET_PULSE_BEGIN;
		return;
	}

	owsReadDataBitCnt ++;

	// Если приняты все 8 бит данных (0 .. 7) и это команда
	if (owsReadDataBitCnt > 7)
	{
		//TerminalThreadSendMsg(owsReadData);// MSG
		if (owsProcState == READ_COMMAND)
		{
			//TerminalThreadSendMsg(owsReadData);// MSG
			owsCommandProcessing(owsReadData);
			return;
		}
		else
		{
			// сбрасываем, ждем следующего MASTER Tx "RESET PULSE"
			OWS_TIMER->CNT = 0;
			LL_TIM_SetAutoReload(OWS_TIMER, TIME_DEFAULT_WAIT);
			LL_TIM_IC_SetPolarity(OWS_TIMER, OWS_TIMER_CHANNEL, LL_TIM_IC_POLARITY_FALLING);
			SetPinStateToTimerIC();
			owsState = READ_RESET_PULSE_BEGIN;
			return;
		}
	}

	LL_TIM_IC_SetPolarity(OWS_TIMER, OWS_TIMER_CHANNEL, LL_TIM_IC_POLARITY_FALLING);
	owsState = READ_DATA_BEGIN;
}

/*
 * Детектирование слота записи. Детектирован спад.
 * Начало формирования слота записи.
 */
void fsDetectWriteData(void)
{
	uint16_t lwTime = OWS_TIMER->CNT;
	OWS_TIMER->CNT = 0;
	// Недостаточное время между следованием тайм-слотов - сброс
	if (lwTime < TIME_REC)
	{
		// сбрасываем, ждем следующего MASTER Tx "RESET PULSE"
		OWS_TIMER->CNT = 0;
		LL_TIM_SetAutoReload(OWS_TIMER, TIME_DEFAULT_WAIT);
		LL_TIM_IC_SetPolarity(OWS_TIMER, OWS_TIMER_CHANNEL, LL_TIM_IC_POLARITY_FALLING);
		SetPinStateToTimerIC();
		owsState = READ_RESET_PULSE_BEGIN;

		//TerminalThreadSendMsg(0xDD400000);// MSG
		return;
	}
	// Начинаем формировать слот записи
	SET_WIRE_LOW();
	SetPinStateToWrite();

	// Определяем время удержания линии в LOW состоянии для формирования
	// тайм-слота 0 или 1, в зависимости от текущего на отправку бита
	uint16_t lowTime = ((owsWriteData[owsWriteDataCnt] >> owsWriteDataBitCnt) & 0x01) ?
			TIME_RL : (TIME_SLOT_MIN - TIME_RL);

	LL_TIM_SetAutoReload(OWS_TIMER, lowTime);

	owsState = WRITE_DATA;
}

/*
 * Окончание формирования слота записи, таймер отсчитал длительность слота.
 * Установка линии в HIGH
 */
void fsWriteData(void)
{
	SET_WIRE_HIGH();
	OWS_TIMER->CNT = 0;

	owsWriteDataBitCnt ++;
	// отправлены все 8 бит
	if (owsWriteDataBitCnt >= 8)
	{
		owsWriteDataBitCnt = 0;
		owsWriteDataCnt ++;
	}
	// отправлены все байты, переход в режим ожидания след команды
	if (owsWriteDataCnt >= owsWriteDataLength)
	{
		//owsState = END;
		//fsmOWS[owsState]();
		// сбрасываем, ждем следующего MASTER Tx "RESET PULSE"
		OWS_TIMER->CNT = 0;
		LL_TIM_SetAutoReload(OWS_TIMER, TIME_DEFAULT_WAIT);
		LL_TIM_IC_SetPolarity(OWS_TIMER, OWS_TIMER_CHANNEL, LL_TIM_IC_POLARITY_FALLING);
		SetPinStateToTimerIC();
		owsState = READ_RESET_PULSE_BEGIN;

		//TerminalThreadSendMsg(0xDD500000);// MSG
		return;
	}
	// еще не все отправлено, продолжаем крутить запись
	LL_TIM_SetAutoReload(OWS_TIMER, TIME_DEFAULT_WAIT);
	LL_TIM_IC_SetPolarity(OWS_TIMER, OWS_TIMER_CHANNEL, LL_TIM_IC_POLARITY_FALLING);
	SetPinStateToTimerIC();
	owsState = DETECT_WRITE_DATA;

}


void OneWireSlave_TEST(void)
{
	owsState = BEGIN;
	fsmOWS[owsState]();
}

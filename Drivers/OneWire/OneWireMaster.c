/*
 * OneWire.c
 *
 *  Created on: 23 июн. 2021 г.
 *      Author: tochk
 */

#include <OneWireMaster.h>

#define ONE_WIRE_GPIO			GPIOB
#define ONE_WIRE_GPIO_PIN		LL_GPIO_PIN_10

// Initialization Procedure: Reset and Presence Pulses times
#define TIME_RSTL		480	// Reset Low Time				( > 480 )
#define TIME_RSTH		480	// Reset High Time				( > 480 )
#define	TIME_MSP		70	// Presence-Detect Sample Time	( 60 .. 75 )
#define TIME_REC		2	// Recovery Time				( > 1 )
// Write/Read time-slots
// g - время роста/спада напряжения на подтягивающем резисторе до порогов лог. 1/0
#define TIME_SLOT		65	// Time Slot Duration			( > 61 )
#define TIME_W1L		5	// Write-One Low Time			( 1  .. 15 )
#define TIME_W0L		60	// Write-Zero Low Time			( 60 .. 120 )
#define TIME_RL			5	// Read Low Time				( 1  .. (15-g) )
#define TIME_MSR		8	// Read Sample Time				( (TIME_RL+g) .. 15 )
// RW1990 times
#define TIME_RW1990_TSLOT_DELAY 10000	// Время удержания HIGH после каждого тайм-слота
										// (необходим для корректной записи RW1990)

#define WIRE_HIGH	1U
#define WIRE_LOW	0U

#define SET_WIRE_HIGH()		LL_GPIO_SetOutputPin(ONE_WIRE_GPIO, ONE_WIRE_GPIO_PIN)
#define SET_WIRE_LOW()		LL_GPIO_ResetOutputPin(ONE_WIRE_GPIO, ONE_WIRE_GPIO_PIN)
#define GET_WIRE_STATE()	(LL_GPIO_IsInputPinSet(ONE_WIRE_GPIO, ONE_WIRE_GPIO_PIN) ? WIRE_HIGH : WIRE_LOW)

// ROM Function Commands
#define CMD_READ_ROM_0				0x0F // See below
#define CMD_READ_ROM				0x33 // Read 8-byte ROM
#define CMD_SEARCH_ROM				0xF0 // Search ROM
#define CMD_MATH_ROM				0x55 // Match ROM
#define CMD_SKIP_ROM				0xCC // Skip ROM
// RW1990 Commands
#define CMD_RW1990_WRITE_MODE		0xD1 // Write ROM mode enable/disable
#define CMD_RW1990_WRITE_ROM		0xD5 // Write 8-byte ROM



__STATIC_INLINE void SetPinStateToWrite(void);
__STATIC_INLINE void SetPinStateToRead(void);
OWM_ReturnStatus_t ResetAndPresence(void);
void WriteOneTimeSlot(void);
void WriteZeroTimeSlot(void);
uint8_t ReadDataTimeSlot(void);
void SendCommand(uint8_t cmd);
uint8_t ReadData(void);
void RW1990WriteOneTimeSlot(void);
void RW1990WriteZeroTimeSlot(void);
void RW1990SendCommand(uint8_t cmd);


void OneWireMasterInit(void)
{
	// Base GPIO Init
	LL_GPIO_InitTypeDef		GPIO_InitStruct = {0};

	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
	GPIO_InitStruct.Pin = ONE_WIRE_GPIO_PIN;
	//GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
	//GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	//GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
	LL_GPIO_Init(ONE_WIRE_GPIO, &GPIO_InitStruct);

	delay_Init();
}

/*
 * РЕАЛИЗОВАНО НА ЗАДЕРЖКАХ
 * Чтение данных ROM устройств DS1990 (iButton)
 * [out] data: Данные ROM, data[0]	- 8-bit family code
 * 						   data[1..6]- unique 48-bit serial number
 * 						  			(data[1] - low byte, data[6]- high byte)
 * 						   data[5]	- 8-bit CRC
 * [return]:  0 - succes
 */
OWM_ReturnStatus_t OneWireMasterReadROM(OneWireData_t *data)
{
	if (ResetAndPresence() != OWM_RS_OK)
		return OWM_RS_ERROR;

	SendCommand(CMD_READ_ROM);

	data->romData[0] = ReadData();
	data->romData[1] = ReadData();
	data->romData[2] = ReadData();
	data->romData[3] = ReadData();
	data->romData[4] = ReadData();
	data->romData[5] = ReadData();
	data->romData[6] = ReadData();
	data->romData[7] = ReadData();
/*
	// сверяем CRC
	uint8_t crc = OneWireCRC8(data->romData, 7);
	if (crc ^ data->romData[7])
		return OW_RS_CRC_FAIL;
*/
	return OWM_RS_OK;
}

/*
 * РЕАЛИЗОВАНО НА ЗАДЕРЖКАХ
 * Запись 8-байтного кода на болванки RW1990
 * [in] data: Данные ROM, data[0]	- 8-bit family code
 * 						  data[1..6]- unique 48-bit serial number
 * 						  			(data[1] - low byte, data[6]- high byte)
 * 						  data[5]	- 8-bit CRC
 */
OWM_ReturnStatus_t OneWireMasterRW1990_WriteROM(OneWireData_t *data)
{
	// Инициализация передачи команды
	if (ResetAndPresence() != OWM_RS_OK)
		return OWM_RS_ERROR;
	// Разрешаем запись запись
	RW1990SendCommand(CMD_RW1990_WRITE_MODE);
	RW1990WriteZeroTimeSlot();
	// Инициализация передачи команды
	if (ResetAndPresence() != OWM_RS_OK)
		return OWM_RS_ERROR;
	// Команда начала записи данных
	RW1990SendCommand(CMD_RW1990_WRITE_ROM);
	// Пишем данные
	for(uint8_t i = 0; i < 8; i ++)
	{
		((data->romData[i] >> 0) & 0x01) ? RW1990WriteZeroTimeSlot() : RW1990WriteOneTimeSlot();
		((data->romData[i] >> 1) & 0x01) ? RW1990WriteZeroTimeSlot() : RW1990WriteOneTimeSlot();
		((data->romData[i] >> 2) & 0x01) ? RW1990WriteZeroTimeSlot() : RW1990WriteOneTimeSlot();
		((data->romData[i] >> 3) & 0x01) ? RW1990WriteZeroTimeSlot() : RW1990WriteOneTimeSlot();
		((data->romData[i] >> 4) & 0x01) ? RW1990WriteZeroTimeSlot() : RW1990WriteOneTimeSlot();
		((data->romData[i] >> 5) & 0x01) ? RW1990WriteZeroTimeSlot() : RW1990WriteOneTimeSlot();
		((data->romData[i] >> 6) & 0x01) ? RW1990WriteZeroTimeSlot() : RW1990WriteOneTimeSlot();
		((data->romData[i] >> 7) & 0x01) ? RW1990WriteZeroTimeSlot() : RW1990WriteOneTimeSlot();
	}
	// Инициализация передачи команды
	if (ResetAndPresence() != OWM_RS_OK)
		return OWM_RS_ERROR;
	// Запрещаем запись
	RW1990SendCommand(CMD_RW1990_WRITE_MODE);
	RW1990WriteOneTimeSlot();

	return OWM_RS_OK;
}


/*
 * Установка режима пина на запись.
 * (Вывод в режиме "открытый коллектор", а потому последовательно
 *  ставить резистор 100 - 150 Ом, дабы не спалить вывод ненароком)
 */
__STATIC_INLINE void SetPinStateToWrite(void)
{
	LL_GPIO_SetPinOutputType(ONE_WIRE_GPIO, ONE_WIRE_GPIO_PIN, LL_GPIO_OUTPUT_OPENDRAIN);
	LL_GPIO_SetPinMode(ONE_WIRE_GPIO, ONE_WIRE_GPIO_PIN, LL_GPIO_MODE_OUTPUT);
}

/*
 * Режим пина - чтение
 */
__STATIC_INLINE void SetPinStateToRead(void)
{
	LL_GPIO_SetPinMode(ONE_WIRE_GPIO, ONE_WIRE_GPIO_PIN, LL_GPIO_MODE_INPUT);
}

/*
__STATIC_INLINE void SetPinState_Emulation(void)
{
	LL_GPIO_SetPinOutputType(ONE_WIRE_GPIO, ONE_WIRE_GPIO_PIN, LL_GPIO_OUTPUT_PUSHPULL);
	LL_GPIO_SetPinMode(ONE_WIRE_GPIO, ONE_WIRE_GPIO_PIN, LL_GPIO_MODE_OUTPUT);
	LL_GPIO_SetPinPull(ONE_WIRE_GPIO, ONE_WIRE_GPIO_PIN, LL_GPIO_PULL_NO);
}
*/

/*
 * Процедура инициализации обмена данными
 */
OWM_ReturnStatus_t ResetAndPresence(void)
{
	// Ожидаем пока линия уйдет в High
	SetPinStateToRead();
	while (GET_WIRE_STATE() != WIRE_HIGH)
	{
		delay_us(TIME_REC);
	}

	// Формируем "MASTER TX RESET PULSE"
	SetPinStateToWrite();
	SET_WIRE_LOW();
	delay_us(TIME_RSTL);
	SET_WIRE_HIGH();

	// Ждем "MASTER RX PRESENCE PULSE"
	SetPinStateToRead();
	delay_us(TIME_MSP);
	uint8_t retState = GET_WIRE_STATE();

	// Выжидаем оставшееся время
	delay_us(TIME_RSTH - TIME_MSP);

	return retState ? OWM_RS_ERROR : OWM_RS_OK;
}

/*
 * Формирование слота соответствующего записи "1"
 */
void WriteOneTimeSlot(void)
{
	/*
	// Ожидаем пока линия уйдет в High
	SetPinStateToRead();
	while (GET_WIRE_STATE() != WIRE_HIGH)
	{
		delay_us(TIME_REC);
	}
	*/
	SetPinStateToWrite();
	SET_WIRE_LOW();
	delay_us(TIME_W1L);
	SET_WIRE_HIGH();
	// Выжидаем оставшееся время слота
	delay_us(TIME_SLOT - TIME_W1L);
}

/*
 * Формирование слота соответствующего записи - "0"
 */
void WriteZeroTimeSlot(void)
{
	SetPinStateToWrite();
	SET_WIRE_LOW();
	delay_us(TIME_W0L);
	SET_WIRE_HIGH();
	// Выжидаем оставшееся время слота
	delay_us(TIME_SLOT - TIME_W0L);
}

/*
 * Формирование слота соответствующего чтению бита данных
 */
uint8_t ReadDataTimeSlot(void)
{

	SetPinStateToWrite();
	SET_WIRE_LOW();
	delay_us(TIME_RL);
	SET_WIRE_HIGH();

	// Читаем значение бита
	SetPinStateToRead();
	delay_us(TIME_MSR - TIME_RL);
	uint8_t retState = GET_WIRE_STATE();
	// Выжидаем оставшееся время слота
	delay_us(TIME_SLOT - TIME_MSR);
	return retState;
}

void SendCommand(uint8_t cmd)
{
	((cmd >> 0) & 0x01) ? WriteOneTimeSlot() : WriteZeroTimeSlot();
	((cmd >> 1) & 0x01) ? WriteOneTimeSlot() : WriteZeroTimeSlot();
	((cmd >> 2) & 0x01) ? WriteOneTimeSlot() : WriteZeroTimeSlot();
	((cmd >> 3) & 0x01) ? WriteOneTimeSlot() : WriteZeroTimeSlot();
	((cmd >> 4) & 0x01) ? WriteOneTimeSlot() : WriteZeroTimeSlot();
	((cmd >> 5) & 0x01) ? WriteOneTimeSlot() : WriteZeroTimeSlot();
	((cmd >> 6) & 0x01) ? WriteOneTimeSlot() : WriteZeroTimeSlot();
	((cmd >> 7) & 0x01) ? WriteOneTimeSlot() : WriteZeroTimeSlot();
}

uint8_t ReadData(void)
{
	uint8_t resData = 0;

	resData |= ReadDataTimeSlot() << 0;
	resData |= ReadDataTimeSlot() << 1;
	resData |= ReadDataTimeSlot() << 2;
	resData |= ReadDataTimeSlot() << 3;
	resData |= ReadDataTimeSlot() << 4;
	resData |= ReadDataTimeSlot() << 5;
	resData |= ReadDataTimeSlot() << 6;
	resData |= ReadDataTimeSlot() << 7;
}


/*
 * Формирование слота соответствующего записи - "0"
 * для болванок RW1990
 */
void RW1990WriteZeroTimeSlot(void)
{
	SetPinStateToWrite();
	SET_WIRE_LOW();
	delay_us(TIME_W0L);
	SET_WIRE_HIGH();
	// Выжидаем оставшееся время слота
	delay_us(TIME_RW1990_TSLOT_DELAY);
}

/*
 * Формирование слота соответствующего записи "1"
 * для болванок RW1990
 */
void RW1990WriteOneTimeSlot(void)
{
	SetPinStateToWrite();
	SET_WIRE_LOW();
	delay_us(TIME_W1L);
	SET_WIRE_HIGH();
	// Выжидаем оставшееся время слота
	delay_us(TIME_RW1990_TSLOT_DELAY);
}

void RW1990SendCommand(uint8_t cmd)
{
	((cmd >> 0) & 0x01) ? RW1990WriteOneTimeSlot() : RW1990WriteZeroTimeSlot();
	((cmd >> 1) & 0x01) ? RW1990WriteOneTimeSlot() : RW1990WriteZeroTimeSlot();
	((cmd >> 2) & 0x01) ? RW1990WriteOneTimeSlot() : RW1990WriteZeroTimeSlot();
	((cmd >> 3) & 0x01) ? RW1990WriteOneTimeSlot() : RW1990WriteZeroTimeSlot();
	((cmd >> 4) & 0x01) ? RW1990WriteOneTimeSlot() : RW1990WriteZeroTimeSlot();
	((cmd >> 5) & 0x01) ? RW1990WriteOneTimeSlot() : RW1990WriteZeroTimeSlot();
	((cmd >> 6) & 0x01) ? RW1990WriteOneTimeSlot() : RW1990WriteZeroTimeSlot();
	((cmd >> 7) & 0x01) ? RW1990WriteOneTimeSlot() : RW1990WriteZeroTimeSlot();
}




void OneWire_TEST(void)
{
	if (ResetAndPresence() == OWM_RS_OK)
		printf("ResetAndPresence DONE! %06d\n", 1);
	else
		printf("ResetAndPresence ERROR! %06d\n", 0);
}

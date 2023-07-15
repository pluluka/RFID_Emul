/*
 * W25QXXXFLASH.c
 *
 *  Created on: 4 нояб. 2021 г.
 *      Author: tochk
 */
#include "W25QXXXFLASH.h"

/* Тип микросхемы (нужное раскомментировать) */
//#define W25Q32FV
#define W25Q64FV
//#define W25Q128FV
//#define W25Q256FV

#define PAGE_SIZE						256
#define SECTOR_SIZE						4096

#ifdef W25Q32FV
#define PAGE_COUNT						16384
#endif //W25Q32FV
#ifdef W25Q64FV
#define PAGE_COUNT						32768
#endif //W25Q64FV

#define DIMMY_BYTE						0xAA

// Commands
#define CMD_WRITE_ENABLE				0x06
#define CMD_WRITE_DISABLE				0x04
#define CMD_READ_STATUS_REG1			0x05
#define CMD_READ_STATUS_REG2			0x35
#define CMD_WRITE_STATUS_REG			0x01
#define CMD_PAGE_PROGRAM				0x02
#define CMD_SECTOR_ERASE				0x20
#define CMD_BLOCK_ERASE_32				0x52
#define CMD_BLOCK_ERASE_64				0xD8
#define CMD_CHIP_ERASE					0xC7
#define CMD_POWER_DOWN					0xB9
#define CMD_READ_DATA					0x03
#define CMD_READ_DEVICE_ID				0x90
#define CMD_ENABLE_RESET				0x66
#define CMD_RESET						0x99

// Status register-1 flags
#define SR1_BUSY						0x01 // Erase/Write in progress
#define SR1_WEL							0x02 // Write enable lath
#define SR1_BP0							0x04 // Block protect bit0
#define SR1_BP1							0x08 // Block protect bit1
#define SR1_BP2							0x10 // Block protect bit2
#define SR1_TB							0x20 // Top/Bottom protect
#define SR1_SEC							0x40 // Sector protect
#define SR1_SPR0						0x80 // Status register protect 0
// Status register-2 flags
#define SR2_SRP1						0x01 // Status register protect 1
#define SR2_QE							0x02 // Quad enable
#define SR2_R							0x04 // Reserved
#define SR2_LB1							0x08 // Security register lock bit 1
#define SR2_LB2							0x10 // Security register lock bit 2
#define SR2_LB3							0x20 // Security register lock bit 3
#define SR2_CMP							0x40 // Comlement protect
#define SR2_SUS							0x80 // Suspend status




#define CS_HIGH()		LL_GPIO_SetOutputPin(W25QXXX_GPIO, W25QXXX_CS_PIN)
#define CS_LOW()		LL_GPIO_ResetOutputPin(W25QXXX_GPIO, W25QXXX_CS_PIN)


void spi_Init(SPI_TypeDef* SPIx, GPIO_TypeDef *ChipSelectGPIO, uint32_t ChipSelectPIN);
__STATIC_INLINE uint8_t spi_SendData(uint8_t data);
__STATIC_INLINE void spi_WriteData(uint8_t data);
__STATIC_INLINE uint8_t spi_Clear_RXNE(void);
__STATIC_INLINE void spi_WaitForTransmissionEnd(void);
__STATIC_INLINE void Delay(volatile uint32_t nCount);

uint8_t ReadStatusRegister1(void);
uint8_t ReadStatusRegister2(void);



/*
 * Инициализация
 */
void W25QXXX_Init(void)
{
	spi_Init(W25QXXX_SPI, W25QXXX_GPIO, W25QXXX_CS_PIN);
}

/*
 * Возвращет Manufaturer ID + Device ID
 */
uint32_t W25QXXX_ReadDeviceID(void)
{
	uint32_t id = 0;

	CS_LOW();
	spi_SendData(CMD_READ_DEVICE_ID);
	spi_SendData(0x00);
	spi_SendData(0x00);
	spi_SendData(0x00);
	id |= spi_SendData(DIMMY_BYTE) << 8; // Manufaturer ID
	id |= spi_SendData(DIMMY_BYTE) << 0; // Device ID
	CS_HIGH();

	return id;
}

/*
 * Стирает сектор (заполняет '1' для последующей записи)
 * [in] address: адрес 24 бит, сектор содержащий данный адрес будет стёрт.
 */
void W25QXXX_SectorErase(uint32_t address)
{
	// Ожидаем окончания внутренних процедур записи/чтения микросхемы
	while ((ReadStatusRegister1() & SR1_BUSY) == SR1_BUSY)
		;

	// разрешаем запись
	CS_LOW();
	spi_SendData(CMD_WRITE_ENABLE);
	spi_WaitForTransmissionEnd();
	CS_HIGH();

	CS_LOW();
	spi_SendData(CMD_SECTOR_ERASE);
	spi_SendData((address >> 16) & 0xFF);
	spi_SendData((address >> 8) & 0xFF);
	spi_SendData((address >> 0) & 0xFF);
	spi_WaitForTransmissionEnd();
	CS_HIGH();

	// запрещаем запись
	CS_LOW();
	spi_SendData(CMD_WRITE_DISABLE);
	spi_WaitForTransmissionEnd();
	CS_HIGH();
}

/*
 * ВНИМАНИЕ!!! Память типа NOR-Flash. Перед записью необходимо выполнить ERASE SECTOR,
 * в данном случае все ячейки заполняются "1" (необходимое условие для записи NOR-Flash).
 * Запись данных. Данные пишутся последовательно начиная с адреса address.
 * [in] address: адрес 24 бит, [8..23] - адрес страницы, [0..7] - адрес байта в странице
 * [in] length:  длина данных
 * [in] data:    данные
 */
void W25QXXX_MemoryWrite(uint32_t address, uint32_t dataLength, uint8_t *data)
{
	// Ожидаем окончания внутренних процедур записи/чтения микросхемы
	while ((ReadStatusRegister1() & SR1_BUSY) == SR1_BUSY)
		;

	// разрешаем запись
	CS_LOW();
	spi_SendData(CMD_WRITE_ENABLE);
	spi_WaitForTransmissionEnd();
	CS_HIGH();

	// Команда на запись
	CS_LOW();
	spi_SendData(CMD_PAGE_PROGRAM);
	spi_SendData((address >> 16) & 0xFF);
	spi_SendData((address >> 8) & 0xFF);
	spi_SendData((address >> 0) & 0xFF);

	// Пишем
	while (dataLength --)
	{
		//TerminalThreadSendMsg(*data);
		spi_SendData(*data);
		data ++;
	}
	spi_WaitForTransmissionEnd();
	CS_HIGH();

	// запрещаем запись
	CS_LOW();
	spi_SendData(CMD_WRITE_DISABLE);
	spi_WaitForTransmissionEnd();
	CS_HIGH();
}

/*
 * Последовательное считывание данных
 * [in]	 address: адрес 24 бит, [8..23] - адрес страницы, [0..7] - адрес байта в странице
 * [in]	 length:  длина данных
 * [out] data:    данные
 */
void W25QXXX_MemoryRead(uint32_t address, uint32_t dataLength, uint8_t *data)
{
	// Ожидаем окончания внутренних процедур записи/чтения микросхемы
	while ((ReadStatusRegister1() & SR1_BUSY) == SR1_BUSY)
		;

	// Команда на чтение
	CS_LOW();
	spi_SendData(CMD_READ_DATA);
	spi_SendData((address >> 16) & 0xFF);
	spi_SendData((address >> 8)  & 0xFF);
	spi_SendData((address >> 0)  & 0xFF);

	// Читаем
	while (dataLength --)
	{
		//TerminalThreadSendMsg(*data);
		*data = spi_SendData(DIMMY_BYTE);
		data ++;
	}

	spi_WaitForTransmissionEnd();
	CS_HIGH();
}

uint32_t W25QXXX_GetPageSize(void)
{
	return PAGE_SIZE;
}

uint32_t W25QXXX_GetPageCount(void)
{
	return PAGE_COUNT;
}


uint8_t ReadStatusRegister1(void)
{
	uint8_t sr = 0;

	CS_LOW();
	spi_SendData(CMD_READ_STATUS_REG1);
	sr = spi_SendData(DIMMY_BYTE);
	spi_WaitForTransmissionEnd();
	CS_HIGH();

	return sr;
}

uint8_t ReadStatusRegister2(void)
{
	uint8_t sr = 0;

	CS_LOW();
	spi_SendData(CMD_READ_STATUS_REG2);
	sr = spi_SendData(DIMMY_BYTE);
	spi_WaitForTransmissionEnd();
	CS_HIGH();

	return sr;
}






uint32_t W25QXXX_TEST(void)
{
	return ReadStatusRegister1();
}


/*
 * инициализация в режиме мастера с дефолтным набором настроек
 * вывод NSS программно подтянут к VCC
 */
void spi_Init(SPI_TypeDef* SPIx, GPIO_TypeDef *ChipSelectGPIO, uint32_t ChipSelectPIN)
{
	  LL_SPI_InitTypeDef SPI_InitStruct = {0};
	  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

	  if (SPIx == SPI1)
	  {
		  /* Peripheral clock enable */
		  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SPI1);

		  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
		  /**SPI1 GPIO Configuration
		  PA5   ------> SPI1_SCK
		  PA6   ------> SPI1_MISO
		  PA7   ------> SPI1_MOSI
		  */
		  GPIO_InitStruct.Pin = LL_GPIO_PIN_5|LL_GPIO_PIN_6|LL_GPIO_PIN_7;
		  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
		  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
		  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
		  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
		  GPIO_InitStruct.Alternate = LL_GPIO_AF_5;
		  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	  }

	  else if (SPIx == SPI2)
	  {
		  /* Peripheral clock enable */
		  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_SPI2);

		  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
		  /**SPI2 GPIO Configuration
		  PB13   ------> SPI2_SCK
		  PB14   ------> SPI2_MISO
		  PB15   ------> SPI2_MOSI
		  */
		  GPIO_InitStruct.Pin = LL_GPIO_PIN_13|LL_GPIO_PIN_14|LL_GPIO_PIN_15;
		  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
		  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
		  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
		  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
		  GPIO_InitStruct.Alternate = LL_GPIO_AF_5;
		  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	  }

	  else if (SPIx == SPI3)
	  {
		  /* Peripheral clock enable */
		  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_SPI3);

		  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
		  /**SPI3 GPIO Configuration
		  PB3   ------> SPI3_SCK
		  PB4   ------> SPI3_MISO
		  PB5   ------> SPI3_MOSI
		  */
		  GPIO_InitStruct.Pin = LL_GPIO_PIN_3|LL_GPIO_PIN_4|LL_GPIO_PIN_5;
		  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
		  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
		  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
		  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
		  GPIO_InitStruct.Alternate = LL_GPIO_AF_6;
		  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	  }

	  /* SPI parameter configuration*/
	  SPI_InitStruct.TransferDirection = LL_SPI_FULL_DUPLEX;
	  SPI_InitStruct.Mode = LL_SPI_MODE_MASTER;
	  SPI_InitStruct.DataWidth = LL_SPI_DATAWIDTH_8BIT;
	  SPI_InitStruct.ClockPolarity = LL_SPI_POLARITY_LOW;
	  SPI_InitStruct.ClockPhase = LL_SPI_PHASE_1EDGE;
	  SPI_InitStruct.NSS = LL_SPI_NSS_SOFT;
	  SPI_InitStruct.BaudRate = LL_SPI_BAUDRATEPRESCALER_DIV8;
	  SPI_InitStruct.BitOrder = LL_SPI_MSB_FIRST;
	  SPI_InitStruct.CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE;
	  SPI_InitStruct.CRCPoly = 10;

	  if (LL_SPI_IsEnabled(SPIx) == 0x00000001U)
		  LL_SPI_Disable(SPIx);
	  LL_SPI_Init(SPIx, &SPI_InitStruct);
	  LL_SPI_SetStandard(SPIx, LL_SPI_PROTOCOL_MOTOROLA);
	  LL_SPI_Enable(SPIx);


	  if (ChipSelectGPIO == GPIOA)
		  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
	  if (ChipSelectGPIO == GPIOB)
		  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);

	  GPIO_InitStruct.Pin = ChipSelectPIN;
	  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
	  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	  GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
	  GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
	  LL_GPIO_Init(ChipSelectGPIO, &GPIO_InitStruct);

	  CS_HIGH();
}


/*
 * Отправка байта в очередь на передачу и получение ответа
 */
__STATIC_INLINE uint8_t spi_SendData(uint8_t data)
{
	 // ждём пока регистр DR скинет данные в сдвиговый регистр
	while(!(W25QXXX_SPI->SR & SPI_SR_TXE));
	// отправляем данные
	W25QXXX_SPI->DR = (uint16_t)data;
	//ждём пока придёт ответ
	while(!(W25QXXX_SPI->SR & SPI_SR_RXNE));
	//считываем полученные данные
	return W25QXXX_SPI->DR;
}


/*
 *  Отправка байта в очередь на передачу
 */
__STATIC_INLINE void spi_WriteData(uint8_t data)
{
	 // ждём пока регистр DR скинет данные в сдвиговый регистр
	while(!(W25QXXX_SPI->SR & SPI_SR_TXE));
	//отправляем данные
	W25QXXX_SPI->DR = (uint16_t)data;
}

/*
 * Сбрасывает флаг RX Enable
 */
__STATIC_INLINE uint8_t spi_Clear_RXNE(void)
{
	return W25QXXX_SPI->DR;
}

/*
 * Ждем, пока передатчик выплюнет данные в линию
 */
__STATIC_INLINE void spi_WaitForTransmissionEnd(void)
{
	while(W25QXXX_SPI->SR & SPI_SR_BSY);
}

__STATIC_INLINE void Delay(volatile uint32_t nCount)
{
	while(nCount--)
	{
		__asm("NOP");
	}
}



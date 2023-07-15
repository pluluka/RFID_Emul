/*
 * AT45DB161.c
 *
 *  Created on: 21 сент. 2020 г.
 *      Author: tochk
 */


#include "AT45DB161.h"


#define PAGES_COUNT		4094 // количество страниц


#define	PAGE_SIZE_512	// размер страницы 512 (закоментировать для 528)


// Status register flags
#define STATUS_RDY			0x80
#define STATUS_COMP			0x40
#define STATUS_PROTECT		0x02

// Commands
// Read Commands
#define	MM_PAGE_READ			0xD2	// Main Memory Page Read
#define CA_READ_LEG				0xE8	// Continuous Array Read (Legacy Command)
#define	CA_READ_LOW				0x03	// Continuous Array Read (Low Frequency)
#define	CA_READ_HIGH			0x0B	// Continuous Array Read (High Frequency)
#define BUFFER1_READ_LOW		0xD1	// Buffer 1 Read (Low Frequency)
#define BUFFER2_READ_LOW		0xD3	// Buffer 2 Read (Low Frequency)
#define BUFFER1_READ			0xD4	// Buffer 1 Read
#define BUFFER2_READ			0xD6	// Buffer 2 Read
#define MMP_TO_BUFF1_TRANSFER	0x53	// Main Memory Page to Buffer 1 Transfer
#define MMP_TO_BUFF2_TRANSFER	0x55	// Main Memory Page to Buffer 2 Transfer
// Program and Erase Commands
#define BUFFER1_WRITE			0x84	// Buffer 1 Write
#define BUFFER2_WRITE			0x87	// Buffer 2 Write
#define BUFF1_TO_MMPP_ERS		0x83	// Buffer 1 to Main Memory Page Program with Built-in Erase
#define BUFF2_TO_MMPP_ERS		0x86	// Buffer 2 to Main Memory Page Program with Built-in Erase
#define BUFF1_TO_MMPP			0x88	// Buffer 1 to Main Memory Page Program without Built-in Erase
#define BUFF2_TO_MMPP			0x89	// Buffer 2 to Main Memory Page Program without Built-in Erase
#define PAGE_ERASE				0x81	// Page Erase
#define BLOCK_ERASE				0x50	// Block Erase
#define SECTOR_ERASE			0x7C	// Sector Erase
#define CHIP_ERASE				0xC7	// Chip Erase  0xC7, 0x94, 0x80, 0x9A
#define MMPP_THR_BUFF1			0x82	// Main Memory Page Program Through Buffer 1
#define MMPP_THR_BUFF2			0x85	// Main Memory Page Program Through Buffer 2
// Protection and Security Commands
#define EN_SECTOR_PROT			0x3D2A7FA9 // Enable Sector Protection
#define DIS_SECTOR_PROT			0x3D2A7F9A // Enable Sector Protection
#define ERASE_SECTOR_PROT_REG	0x3D2A7FCF // Erase Sector Protection Register
#define PROG_SECTOR_PROT_REG	0x3D2A7FFC // Program Sector Protection Register
#define READ_SECTOR_PROT_REG	0x32					  // Read Sector Protection Register
// Additional Commands
#define STATUS_REG_READ			0xD7		// Status Register Read
#define DEVICE_ID_READ			0x9F		// Manufacturer and Device ID Read



#define CS_HIGH()		LL_GPIO_SetOutputPin(AT45DB161_GPIO, AT45DB161_CS_PIN)
#define CS_LOW()		LL_GPIO_ResetOutputPin(AT45DB161_GPIO, AT45DB161_CS_PIN)


__STATIC_INLINE void Delay(__IO uint32_t nCount);
static void GPIO__Init(void);
void SPI__Init(SPI_TypeDef* SPIx);
__STATIC_INLINE uint8_t spi_SendData(uint8_t data);
__STATIC_INLINE void spi_WriteData(uint8_t data);
__STATIC_INLINE uint8_t spi_Clear_RXNE(void);
__STATIC_INLINE void spi_WaitForTransmissionEnd(void);
__STATIC_INLINE uint8_t ReadStatus(void);


void AT45DB161_Init(void)
{
	GPIO__Init();
	SPI__Init(AT45DB161_SPI);


}

uint32_t AT45DB161_GetPagesCount(void)
{
	return PAGES_COUNT;
}

uint32_t AT45DB161_GetPageSize(void)
{
#ifdef PAGE_SIZE_512
	return 512;
#else
	return 528;
#endif
}

/*
 * Получает значение регистра статуса
 */
uint8_t AT45DB161_ReadStatus(void)
{
	return ReadStatus();
}

/*
 * Получает значение идентификационных данных
 */
void AT45DB161_ReadID(uint8_t *id)
{
	spi_WaitForTransmissionEnd();
	CS_LOW();
	spi_SendData(DEVICE_ID_READ);
	id[0] = spi_SendData(0xFF);
	id[1] = spi_SendData(0xFF);
	id[2] = spi_SendData(0xFF);
	id[3] = spi_SendData(0xFF);
	CS_HIGH();
}

/*
 * Установка размера страницы 512 байт
 * Выполняется один раз за цикл жизни чипа
 */
void AT45DB161_SetPageSize_512(void)
{
	spi_WaitForTransmissionEnd();
	CS_LOW();
	spi_SendData(0x3D);
	spi_SendData(0x2A);
	spi_SendData(0x80);
	spi_SendData(0xA6);
	spi_WaitForTransmissionEnd();
	CS_HIGH();
}

/*
 * Последовательное считывание данных напрямую из флешпамяти (минуя буфер)
 * [in] pageAddress: адрес страницы (12 бит, макс значение - 4094)
 * [in] byteAddress: адрес байта в странице (в зависимости от размера страницы, 9 или 10 бит)
 * [in] dataLength:  длина данных (неограничено, в случае достижения посл. элемента, чтение начнется с нулевого)
 * [out] dataBuff:   буфер для считываемых данных
 */
void AT45DB161_MemoryRead(uint32_t pageAddress, uint32_t byteAddress, uint32_t dataLength, uint8_t *data)
{
	uint32_t address;

	// формируем три байта адреса
#ifdef PAGE_SIZE_512
	// биты[0..8] - адрес байта, биты[9..20] - адрес страницы
	address = (byteAddress & 0x1FF) | ((pageAddress & 0xFFF) << 9);
#else
	// биты[0..9] - адрес байта, биты[10..21] - адрес страницы
	address = (byteAddress & 0x3FF) | ((pageAddress & 0xFFF) << 10);
#endif

	spi_WaitForTransmissionEnd();
	CS_LOW();

	spi_SendData(CA_READ_HIGH);		// отправляем opcode
	spi_SendData(address >> 16);	// отправляем байт 1
	spi_SendData(address >> 8);		// отправляем байт 2
	spi_SendData(address & 0xFF);	// отправляем байт 3
	spi_SendData(0xFF);				// отправляем don't_care хвост

	while (dataLength --)
	{
		*data ++ = spi_SendData(0xFF);
	}

	CS_HIGH();
}


void AT45DB161_Buffer1Read(uint32_t byteAddress, uint32_t dataLength, uint8_t *data)
{
	uint32_t address;

	// формируем три байта адреса
#ifdef PAGE_SIZE_512
	// биты[0..7] - don't_care, биты[8..16] - адрес с которого начнется считывание, биты[17..24] - don't_care
	address = byteAddress & 0x1FF;
#else
	// биты[0..14] - don't_care, биты[15..24] - адрес с которого начнется считывание, биты[25..32] - don't_care
	address = byteAddress & 0x3FF;
#endif

	spi_WaitForTransmissionEnd();
	CS_LOW();

	spi_SendData(BUFFER1_READ);		// отправляем opcode
	spi_SendData(address >> 16);	// отправляем байт 1
	spi_SendData(address >> 8);		// отправляем байт 2
	spi_SendData(address & 0xFF);	// отправляем байт 3
	spi_SendData(0xFF);				// отправляем don't_care хвост

	while (dataLength --)
	{
		*data ++ = spi_SendData(0xFF);
	}

	CS_HIGH();
}

/*
 * Запись данных в произвольную область флеш-памяти.
 * Запись производится в три этапа:
 * 1. Считывание страницы из флеш-памяти в буфер
 * 2. Запись в буфер необходимых данных
 * 3. Запись содержимого буфера в флеш-память
 * [in] pageAddress: адрес страницы (12 бит, макс значение - 4094)
 * [in] byteAddress: адрес байта в странице (в зависимости от размера страницы, 9 или 10 бит)
 * [in] dataLength:  длина данных
 * [in] dataBuff:    данные
 */
void AT45DB161_MemoryWrite(uint32_t pageAddress, uint32_t byteAddress, uint32_t dataLength, uint8_t *data)
{
	uint32_t address;

	// формируем три байта адреса в странице (под команду MMP_TO_BUFF1_TRANSFER)
	address = 0U;
#ifdef PAGE_SIZE_512
	// биты[0..8] - don't_care биты, биты[9..20] - адрес страницы, биты[21..23] - don't_care биты
	address = (pageAddress & 0xFFF) << 9;
#else
	// биты[0..9] - don't_care биты, биты[10..21] - адрес страницы, биты[22..23] - don't_care биты
	address = (pageAddress & 0xFFF) << 10;
#endif

	// Считываем страницу из флешпамяти в буфер 1
	spi_WaitForTransmissionEnd();
	CS_LOW();

	spi_SendData(MMP_TO_BUFF1_TRANSFER);	// отправляем opcode
	spi_SendData(address >> 16);			// отправляем байт 1
	spi_SendData(address >> 8);				// отправляем байт 2
	spi_SendData(address & 0xFF);			// отправляем байт 3

	spi_WaitForTransmissionEnd();
	CS_HIGH();

	// Ожидаем окончания внутренних процедур записи микросхемы
	while ((ReadStatus() & STATUS_RDY) == 0);

	// формируем три байта адреса в странице (под команду BUFFER1_WRITE)
#ifdef PAGE_SIZE_512
	// биты[0..8] - адрес байта, биты[9..23] - don't_care биты
	address = byteAddress & 0x1FF;
#else
	// биты[0..9] - адрес байта, биты[10..23] - don't_care биты
	address = byteAddress & 0x3FF;
#endif

	// Пишем данные в буфер_1
	spi_WaitForTransmissionEnd();
	CS_LOW();

	spi_SendData(BUFFER1_WRITE);	// отправляем opcode
	spi_SendData(address >> 16);	// отправляем байт 1
	spi_SendData(address >> 8);		// отправляем байт 2
	spi_SendData(address & 0xFF);	// отправляем байт 3

	while (dataLength --)
	{
		spi_SendData(*data ++);
	}

	spi_WaitForTransmissionEnd();
	CS_HIGH();

	// формируем три байта адреса в странице (под команду BUFF1_TO_MMPP)
	address = 0U;
	// биты[0..8] - don't_care, биты[9..20] - адрес страницы, биты[21..23] - don't_care
	address = (pageAddress & 0xFFF) << 9;

	// Выгружаем содержимое буфера_1 в флешпамять
	spi_WaitForTransmissionEnd();
	CS_LOW();

	spi_SendData(BUFF1_TO_MMPP_ERS);	// отправляем opcode
	spi_SendData(address >> 16);		// отправляем байт 1
	spi_SendData(address >> 8);			// отправляем байт 2
	spi_SendData(address & 0xFF);		// отправляем байт 3

	spi_WaitForTransmissionEnd();
	CS_HIGH();

	// Ожидаем окончания внутренних процедур записи микросхемы
	while (!(ReadStatus() & STATUS_RDY));

}


/*
 * Заполняет всю страницу значением data
 * [in] pageAddress: адрес страницы (12 бит, макс значение - 4094)
 * [in] data: данные
 */
void AT45DB161_PageFill(uint32_t pageAddress, uint8_t data)
{
	uint32_t address;
	uint32_t dataLength;

	address = 0U;
	// формируем три байта адреса в странице (под команду BUFFER1_WRITE)
#ifdef PAGE_SIZE_512
	// биты[0..8] - адрес байта, биты[9..23] - don't_care биты
	dataLength = 512;
#else
	// биты[0..9] - адрес байта, биты[10..23] - don't_care биты
	dataLength = 528;
#endif

	// Пишем данные в буфер_1
	spi_WaitForTransmissionEnd();
	CS_LOW();

	spi_SendData(BUFFER1_WRITE);	// отправляем opcode
	spi_SendData(address >> 16);	// отправляем байт 1
	spi_SendData(address >> 8);		// отправляем байт 2
	spi_SendData(address & 0xFF);	// отправляем байт 3

	while (dataLength --)
	{
		spi_SendData(data);
	}

	spi_WaitForTransmissionEnd();
	CS_HIGH();

	// формируем три байта адреса в странице (под команду BUFF1_TO_MMPP)
	address = 0U;
	// биты[0..8] - don't_care, биты[9..20] - адрес страницы, биты[21..23] - don't_care
	address = (pageAddress & 0xFFF) << 9;

	// Выгружаем содержимое буфера_1 в флешпамять
	spi_WaitForTransmissionEnd();
	CS_LOW();

	spi_SendData(BUFF1_TO_MMPP_ERS);	// отправляем opcode
	spi_SendData(address >> 16);		// отправляем байт 1
	spi_SendData(address >> 8);			// отправляем байт 2
	spi_SendData(address & 0xFF);		// отправляем байт 3

	spi_WaitForTransmissionEnd();
	CS_HIGH();

	// Ожидаем окончания внутренних процедур записи микросхемы
	while (!(ReadStatus() & STATUS_RDY));
}


/*
 * Получает значение регистра статуса
 */
__STATIC_INLINE uint8_t ReadStatus(void)
{
	uint8_t res;

	spi_WaitForTransmissionEnd();
	CS_LOW();
	spi_SendData(STATUS_REG_READ);
	res = spi_SendData(0xFF);
	CS_HIGH();
	return res;
}


static void GPIO__Init(void)
{
	LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
	if (AT45DB161_GPIO == GPIOA)
		LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
	if (AT45DB161_GPIO == GPIOB)
		LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);

	GPIO_InitStruct.Pin = AT45DB161_CS_PIN;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
	GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
	LL_GPIO_Init(AT45DB161_GPIO, &GPIO_InitStruct);

	CS_HIGH();
}

/*
 * инициализация в режиме мастера с дефолтным набором настроек
 * вывод NSS программно подтянут к VCC
 */
void SPI__Init(SPI_TypeDef* SPIx)
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
	  SPI_InitStruct.BaudRate = LL_SPI_BAUDRATEPRESCALER_DIV2;
	  SPI_InitStruct.BitOrder = LL_SPI_MSB_FIRST;
	  SPI_InitStruct.CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE;
	  SPI_InitStruct.CRCPoly = 10;

	  if (LL_SPI_IsEnabled(SPIx) == 0x00000001U)
		  LL_SPI_Disable(SPIx);
	  LL_SPI_Init(SPIx, &SPI_InitStruct);
	  LL_SPI_SetStandard(SPIx, LL_SPI_PROTOCOL_MOTOROLA);

	  LL_SPI_Enable(SPIx);
}

/*
 * Отправка байта в очередь на передачу и получение ответа
 */
__STATIC_INLINE uint8_t spi_SendData(uint8_t data)
{
	 // ждём пока регистр DR скинет данные в сдвиговый регистр
	while(!(AT45DB161_SPI->SR & SPI_SR_TXE));
	// отправляем данные
	AT45DB161_SPI->DR = (uint16_t)data;
	//ждём пока придёт ответ
	while(!(AT45DB161_SPI->SR & SPI_SR_RXNE));
	//считываем полученные данные
	return AT45DB161_SPI->DR;
}


/*
 *  Отправка байта в очередь на передачу
 */
__STATIC_INLINE void spi_WriteData(uint8_t data)
{
	 // ждём пока регистр DR скинет данные в сдвиговый регистр
	while(!(AT45DB161_SPI->SR & SPI_SR_TXE));
	//отправляем данные
	AT45DB161_SPI->DR = (uint16_t)data;
}

/*
 * Сбрасывает флаг RX Enable
 */
__STATIC_INLINE uint8_t spi_Clear_RXNE(void)
{
	return AT45DB161_SPI->DR;
}


/*
 * Ждем, пока передатчик выплюнет данные в линию
 */
__STATIC_INLINE void spi_WaitForTransmissionEnd(void)
{
	while(AT45DB161_SPI->SR & SPI_SR_BSY);
}

__STATIC_INLINE void Delay(volatile uint32_t nCount)
{
	while(nCount--)
	{
		__asm("NOP");
	}
}


void AT45DB161_TEST(uint32_t pageAddress, uint32_t byteAddress, uint32_t dataLength, uint8_t *data)
{


}

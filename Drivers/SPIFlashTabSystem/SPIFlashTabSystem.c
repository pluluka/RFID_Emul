/*
 * SPIFlashTabSystem.c
 *
 *  Created on: Oct 21, 2020
 *      Author: tochk
 */

#include "SPIFlashTabSystem.h"

/*
 * Адреса стартовых страниц групп
 * различных типов записей
 */
#define EM4100_START_PAGE		100	// стартовая страница таблицы EM4100
#define ONEWIRE_START_PAGE		800	// стартовая страница таблицы OneWire

#define FORMAT_VALUE			0xFF // этим будет заполнятся флешка при форматировании


#define ONE_WIRE_FIRST_REC_NUM	0
#define EM4100_FIRST_REC_NUM	0

typedef struct{
	uint32_t pagesCount;
	uint32_t pageSize;
} SPIFlash_Typedef;
SPIFlash_Typedef spiFlash;

// Информационное поле EM4100 (данные о таблице),
// 1-я запись в таблице(нулевая по счету) - всегда информационное поле
// (её размер в данном случае совпадает с размером записи таблицы)
#pragma pack(push, 1)
typedef struct{
	uint32_t	counter;
	uint8_t		rezerv[sizeof(EM4100DataRec_t) - sizeof(uint32_t)];
} EM4100TabInfo_t;
#pragma pack(pop)
EM4100TabInfo_t em4100TabInfo;


// Информационное поле OneWire (данные о таблице),
// 1-я запись в таблице - всегда информационное поле
// (её размер в данном случае совпадает с размером записи таблицы)
#pragma pack(push, 1)
typedef struct{
	uint32_t	counter;
	uint8_t		rezerv[sizeof(OneWireDataRec_t) - sizeof(uint32_t)];
} OneWireTabInfo_t;
#pragma pack(pop)
OneWireTabInfo_t oneWireTabInfo;

/*
 * Интерфейс драйвера SPI Flash.
 * Псевдонимы функций используемой SPI Flash.
 */
#define SPIFlash_Init() \
		AT45DB161_Init()
#define MemoryRead(pageAddress, byteAddress, dataLength, pData) \
		AT45DB161_MemoryRead(pageAddress, byteAddress, dataLength, pData)
#define MemoryWrite(pageAddress, byteAddress, dataLength, pData) \
		AT45DB161_MemoryWrite(pageAddress, byteAddress, dataLength, pData)
#define PageFill(pageAddress, pData) \
		AT45DB161_PageFill(pageAddress, pData)
#define GetPageCount() \
		AT45DB161_GetPagesCount()
#define GetPageSize() \
		AT45DB161_GetPageSize()



static void EM4100CreateTable(void);
__STATIC_INLINE uint32_t EM4100GetCounter(void);
__STATIC_INLINE void EM4100SetCounter(uint32_t cnt);
static uint32_t EM4100GetLastID(void);
__STATIC_INLINE void EM4100GetAddress(uint32_t id, uint32_t *pageAddress, uint32_t *byteAddress);
static void OneWireCreateTable(void);
__STATIC_INLINE void OneWireSetCounter(uint32_t cnt);
__STATIC_INLINE uint32_t OneWireGetCounter(void);
static void OneWireCreateTable(void);
static uint32_t  OneWireGetLastID(void);
__STATIC_INLINE void OneWireGetAddress(uint32_t recNum, uint32_t *pageAddress, uint32_t *byteAddress);



void SPIFTabSystemInit(void)
{
	SPIFlash_Init();
	spiFlash.pageSize = GetPageSize();
	spiFlash.pagesCount = GetPageCount();
}

/*
 * Форматирует флеху
 */
void SPIFTabSystemFormat(void)
{


	for (uint32_t i = 0; i < spiFlash.pagesCount; i ++)
	{
		PageFill(i, FORMAT_VALUE);
	}

	EM4100CreateTable();
	OneWireCreateTable();
}


//=============================== EM4100 ==============================

/*
 * Возвращает число записей EM4100
 */
uint32_t SPIFTabSystem_EM4100GetCounter(void)
{
	return EM4100GetCounter();
}

/*
 * Возвращает id последней записи
 */
uint32_t SPIFTabSystem_EM4100GetLastID(void)
{
	return EM4100GetLastID();
}

/*
 * Добавляет новую запись в таблицу
 * [in] data:	указатель на структуру с данными
 * * На данный момент запись всегда добавляется в конец таблицы с новым id.
 */
void SPIFTabSystem_EM4100Add(EM4100DataRec_t *data)
{
	uint32_t pageAddress;
	uint32_t byteAddress;
	uint32_t cnt;
	uint32_t id;
	uint32_t dataLength;

	cnt = EM4100GetCounter();

	// Получаем id последней записи
	if (cnt == 0)
	{	// это первая запись
		id = 0;
	}
	else
	{	// вычисляем адрес последней записи
		EM4100GetAddress(cnt - 1, &pageAddress, &byteAddress);
		dataLength = sizeof((EM4100DataRec_t){}.id);
		MemoryRead(pageAddress, byteAddress, dataLength, &id);
		id = id + 1;
	}

	data->id = id;

	// вычисляем адрес для новой записи
	EM4100GetAddress(cnt, &pageAddress, &byteAddress);
	MemoryWrite(pageAddress, byteAddress, sizeof(EM4100DataRec_t), data);

	EM4100SetCounter(cnt + 1);
}

/*
 * Возвращает одну запись
 * [in] id: 	id записи
 * [out] data:	указатель на структуру с данными
 */
void SPIFTabSystem_EM4100GetRec(uint32_t id, EM4100DataRec_t* data)
{
	uint32_t pageAddress;
	uint32_t byteAddress;
	uint32_t dataLength;

	EM4100GetAddress(id, &pageAddress, &byteAddress);
	dataLength = sizeof(EM4100DataRec_t);

	MemoryRead(pageAddress, byteAddress, dataLength, data);
}

/*
 * Последовательно заполняет буфер данными(полями data)
 * [in] startID:		id с которого начнется заполнение
 * [in] buffer:			сюда будут сливаться данные
 * [in] bufferLength:	размер буфера
 * [out] dataCnt:		число записанных данных (полей data)
 */
void SPIFTabSystem_EM4100FillBufferWithData(uint32_t startID,
			uint8_t *buffer, uint32_t bufferLength, uint32_t *dataCnt)
{
	uint32_t pageAddress;
	uint32_t byteAddress;
	uint32_t dataLength;
	uint32_t lastID;
	uint32_t id;
	uint32_t cnt;

	id = startID;
	lastID = EM4100GetLastID();
	cnt = 0;
	dataLength = sizeof((EM4100DataRec_t){}.data);

	while (id <= lastID)
	{
		EM4100GetAddress(id, &pageAddress, &byteAddress);
		byteAddress += (uint32_t)(&((EM4100DataRec_t*)0)->data);
		MemoryRead(pageAddress, byteAddress, dataLength, buffer);

		id ++;
		cnt ++;
		buffer += dataLength;

		// буфер закончился
		if ((bufferLength - (cnt * dataLength)) < dataLength)
		{
			break;
		}
	}

	*dataCnt = cnt;
}

/*
 * Возвращает размер поля 'data'
 */
uint32_t SPIFTabSystem_EM4100GetDataSize(void)
{
	return sizeof((EM4100DataRec_t){}.data);
}



static uint32_t  EM4100GetLastID(void)
{
	return EM4100GetCounter() - 1;
}

__STATIC_INLINE void EM4100GetAddress(uint32_t id, uint32_t *pageAddress, uint32_t *byteAddress)
{
	*pageAddress = ((id + 1) * sizeof(EM4100DataRec_t)) / spiFlash.pageSize;
	*pageAddress += EM4100_START_PAGE;
	*byteAddress = ((id + 1) * sizeof(EM4100DataRec_t)) % spiFlash.pageSize;
}

static void EM4100CreateTable(void)
{
	EM4100SetCounter(0);
}

__STATIC_INLINE void EM4100SetCounter(uint32_t cnt)
{
	em4100TabInfo.counter = cnt;

	// узнаем смещение поля в структуре в битах
	uint32_t offset = (uint32_t)(&((EM4100TabInfo_t*)0)->counter);
	uint32_t size = sizeof((EM4100TabInfo_t){}.counter);

	//TerminalThreadSendMsg(0xFF000001);
	//TerminalThreadSendMsg(em4100TabInfo.counter);
	MemoryWrite(EM4100_START_PAGE, offset, size, &em4100TabInfo.counter);
}

__STATIC_INLINE uint32_t EM4100GetCounter(void)
{
	uint32_t cnt = 0;

	MemoryRead(EM4100_START_PAGE, 0, sizeof(uint32_t), &cnt);

	//TerminalThreadSendMsg(0xFF000002);
	//TerminalThreadSendMsg(cnt);

	return cnt;
}



//============================ OneWire ==============================

/*
 * Возвращает число записей OneWire
 */
uint32_t SPIFTabSystem_OneWireGetCounter(void)
{
	return OneWireGetCounter();
}

/*
 * Возвращает id последней записи OneWire
 */
uint32_t SPIFTabSystem_OneWireGetLastID(void)
{
	return OneWireGetLastID();
}

/*
 * Добавляет новую запись в таблицу OneWire
 * [in] data:	указатель на структуру с данными
 */
void SPIFTabSystem_OneWireAdd(OneWireDataRec_t *data)
{
	uint32_t pageAddress;
	uint32_t byteAddress;
	uint32_t recCnt;

	recCnt = OneWireGetCounter();

	data->id = OneWireGetLastID() + 1;

	// вычисляем адрес для новой записи
	OneWireGetAddress(recCnt, &pageAddress, &byteAddress);
	MemoryWrite(pageAddress, byteAddress, sizeof(OneWireDataRec_t), data);

	OneWireSetCounter(recCnt + 1);
}

/*
 * Возвращает одну запись OneWire
 * [in] recNum: 	номер записи
 * [out]  data:	указатель на структуру с данными
 */
void SPIFTabSystem_OneWireGetRec(uint32_t recNum, OneWireDataRec_t* data)
{
	uint32_t pageAddress;
	uint32_t byteAddress;
	uint32_t dataLength;

	OneWireGetAddress(recNum, &pageAddress, &byteAddress);
	dataLength = sizeof(OneWireDataRec_t);

	MemoryRead(pageAddress, byteAddress, dataLength, data);
}

uint32_t SPIFTabSystem_OneWireGetFirstNum(void)
{
	return ONE_WIRE_FIRST_REC_NUM;
}

/*
 * Последовательно заполняет буфер данными(полями romData)
 * [in] startID:		номер записи с которой начнется заполнение
 * [in] buffer:			сюда будут сливаться данные
 * [in] bufferLength:	размер буфера
 * [out] dataCnt:		число записанных данных (полей data)
 */
void SPIFTabSystem_OneWireFillBufferWithData(uint32_t startRecNum,
			uint8_t *buffer, uint32_t bufferLength, uint32_t *dataCnt)
{
	uint32_t pageAddress;
	uint32_t byteAddress;
	uint32_t dataLength;
	uint32_t lastRecNum;
	uint32_t recNum;
	uint32_t cnt;

	recNum = startRecNum;
	lastRecNum = OneWireGetCounter() - 1;
	cnt = 0;
	dataLength = sizeof((OneWireDataRec_t){}.romData);

	while (recNum <= lastRecNum)
	{
		OneWireGetAddress(recNum, &pageAddress, &byteAddress);
		byteAddress += (uint32_t)(&((OneWireDataRec_t*)0)->romData);
		MemoryRead(pageAddress, byteAddress, dataLength, buffer);

		recNum ++;
		cnt ++;
		buffer += dataLength;

		// буфер закончился
		if ((bufferLength - (cnt * dataLength)) < dataLength)
		{
			break;
		}
	}

	*dataCnt = cnt;
}

/*
 * Возвращает размер поля 'data' OneWire
 */
uint32_t SPIFTabSystem_OneWireGetDataSize(void)
{
	return sizeof((OneWireDataRec_t){}.romData);
}




static uint32_t  OneWireGetLastID(void)
{
	uint32_t pageAddress;
	uint32_t byteAddress;
	uint32_t recCnt;
	uint32_t id;
	uint32_t dataLength;

	recCnt = OneWireGetCounter();

	// Получаем id последней записи
	if (recCnt == 0)
	{	// это первая запись
		id = 0;
	}
	else
	{	// вычисляем адрес последней записи
		OneWireGetAddress(recCnt - 1, &pageAddress, &byteAddress);
		dataLength = sizeof((OneWireDataRec_t){}.id);
		// Получаем id
		MemoryRead(pageAddress, byteAddress, dataLength, &id);
	}

	return id;
}

__STATIC_INLINE void OneWireGetAddress(uint32_t recNum, uint32_t *pageAddress, uint32_t *byteAddress)
{
	*pageAddress = ONEWIRE_START_PAGE;
	*pageAddress += ((recNum + 1) * sizeof(OneWireDataRec_t)) / spiFlash.pageSize;
	*byteAddress = ((recNum + 1) * sizeof(OneWireDataRec_t)) % spiFlash.pageSize;
}

static void OneWireCreateTable(void)
{
	OneWireSetCounter(0);
}

__STATIC_INLINE void OneWireSetCounter(uint32_t cnt)
{
	oneWireTabInfo.counter = cnt;

	uint32_t offset = (uint32_t)(&((OneWireTabInfo_t*)0)->counter);
	uint32_t size = sizeof((OneWireTabInfo_t){}.counter);

	MemoryWrite(ONEWIRE_START_PAGE, offset, size, &oneWireTabInfo.counter);
}

__STATIC_INLINE uint32_t OneWireGetCounter(void)
{
	uint32_t cnt = 0;

	MemoryRead(ONEWIRE_START_PAGE, 0, sizeof((OneWireTabInfo_t){}.counter), &cnt);

	return cnt;
}








uint32_t SPIFTabSystem_TEST(void)
{
	OneWireCreateTable();
}


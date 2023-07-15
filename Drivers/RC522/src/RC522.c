/*
 * RC522.c
 *
 *  Created on: 11 апр. 2020 г.
 *      Author: tochk
 */

#include "RC522.h"
#include "USART.h"
#include "MifareClassic.h"
#include "crypto1.h"

extern char str_buff[200];



// GPIO macros
#define RESET_SET()      LL_GPIO_SetOutputPin(RC522_GPIO_RESET, RC522_GPIO_RESET_PIN)
#define RESET_RESET()    LL_GPIO_ResetOutputPin(RC522_GPIO_RESET, RC522_GPIO_RESET_PIN)
#define NSEL_SET()       LL_GPIO_SetOutputPin(RC522_GPIO_NSEL, RC522_GPIO_NSEL_PIN)
#define NSEL_RESET()     LL_GPIO_ResetOutputPin(RC522_GPIO_NSEL, RC522_GPIO_NSEL_PIN)

#define SPI_AFN_OFF		0x00
#define SPI_AFN_ON		0x01

static void GPIO_Init(void);
static void SPI_Init(SPI_TypeDef *SPIx);

__STATIC_INLINE uint16_t SPI_TransmitReceive(SPI_TypeDef *SPIx, uint16_t data);
__STATIC_INLINE void Delay(__IO uint32_t nCount);
static void CalulateCRCA(uint8_t *data, uint8_t len, uint8_t *crc);


static void PCD_WriteReg(uint8_t address, uint8_t value);
static void PCD_ClearBitMask(uint8_t address,uint8_t mask);
static void PCD_SetBitMask(uint8_t address,uint8_t mask);
static uint8_t PCD_ReadReg(uint8_t address);
static rc522_status PCD_CalulateCRCA(uint8_t *inData, uint8_t len, uint8_t *outData);
static rc522_status PCD_Communicate(uint8_t  command, uint8_t  *inData, uint8_t   inLen,
                                    uint8_t  *outData, uint32_t *outLen);
static rc522_status PCD_Authenticate(uint8_t command, uint8_t blockAddr, uint8_t *key, uint8_t *uid, uint8_t uidSize);

static rc522_status PICC_RequestA(uint8_t request, uint8_t *atqa);
static rc522_status PICC_AntiCollisionA(uint8_t cascadeCode, uint8_t *uid);
static rc522_status PICC_SelectA(uint8_t cascadeCode, uint8_t *uid, uint8_t *sak);
static rc522_status PICC_CardSelectA(PICC_ProtBaseInfTypeDef *PICC_ProtBase);
static rc522_status PICC_Read(uint8_t blockAddr, uint8_t *buffer, uint8_t bufferSize);





/*
 * Поиск карт
 * PICC_ProtBase: сюда будут сливаться полученные данные atqa
 * return: в случае успешной отработки ф-я возвращает TRUE
 */
bool RC522_CardDetect(PICC_ProtBaseInfTypeDef *PICC_ProtBase)
{
	rc522_status st;

	// Reset baud rates
	PCD_WriteReg(RC522_TxModeReg, 0x00);
	PCD_WriteReg(RC522_RxModeReg, 0x00);

	st = PICC_RequestA(PICC_REQIDL, PICC_ProtBase->ATQA);

	return ((st == STATUS_OK) || (st == STATUS_COLLISION)) ? true : false;
}


/*
 * Процедура полного цикла каскадных уровней биткадровой антиколлизии
 * Результат успешной отработки - полный UID карты.
 */
static rc522_status PICC_CardSelectA(PICC_ProtBaseInfTypeDef *PICC_ProtBase)
{

	uint8_t currentCascadeLevel;
	uint8_t cascadeCode;
	uint8_t uidIndex;
	rc522_status status;
	uint8_t i;
	uint8_t crc_a[2];

	PICC_ProtBase->UID_SIZE = 0;
	memset(PICC_ProtBase->UID, 0x00, 10);
	memset(PICC_ProtBase->SAK, 0x00, 3);

	i = 3;
	currentCascadeLevel = 1; //PICC_ANTICOLL_CL1;

	while (i)
	{
		i --;

		switch (currentCascadeLevel)
		{
			case 1:
				cascadeCode = PICC_ANTICOLL_CL1;
				uidIndex = 0;
				break;

			case 2:
				cascadeCode = PICC_ANTICOLL_CL2;
				uidIndex = 3;
				break;

			case 3:
				cascadeCode = PICC_ANTICOLL_CL3;
				uidIndex = 6;
				break;

			default:
				return STATUS_IERROR;
				break;
		}

		status = PICC_AntiCollisionA(cascadeCode, ((PICC_ProtBase->UID) + uidIndex));

		if (status != STATUS_OK)
		{
			return STATUS_ERROR;
		}

		status = PICC_SelectA(cascadeCode, (PICC_ProtBase->UID + uidIndex), PICC_ProtBase->SAK);

		if (status != STATUS_OK)
		{
			return STATUS_ERROR;
		}

		// Чекаем CRC SAK
		CalulateCRCA((uint8_t *)&(PICC_ProtBase->SAK[0]), 1, crc_a);
		if ((crc_a[0] != PICC_ProtBase->SAK[1]) || (crc_a[1] != PICC_ProtBase->SAK[2]))
		{
			return STATUS_CRC_ERROR;
		}

		if (PICC_ProtBase->SAK[0] & 0x04)
		{// UID not comlete
			currentCascadeLevel ++;
		}
		else if (PICC_ProtBase->SAK[0] & 0x20)
		{// UID complete,  PICC compliant with ISO/IEC 14443-4
			PICC_ProtBase->UID_SIZE = uidIndex + 4;
			return STATUS_OK;
		}
		else
		{// UID complete,  PICC not compliant with ISO/IEC 14443-4
			PICC_ProtBase->UID_SIZE = uidIndex + 4;
			return STATUS_OK;
		}
	}

	return STATUS_ERROR;
}


bool RC522_CardSelect(PICC_ProtBaseInfTypeDef *PICC_ProtBase)
{
	return (PICC_CardSelectA(PICC_ProtBase) == STATUS_OK) ?  true : false;
}


void RC522_CardHalt(void)
{
	uint8_t atqa[2];

	PICC_RequestA(PICC_HALT, atqa);
}


void RC522_Init(void)
{
	GPIO_Init();
	SPI_Init(RC522_SPI);
	RC522_Reset();
	RC522_AntennaOff();
	RC522_Config();
}


void RC522_Reset(void)
{
    RESET_SET();
    Delay(10);
    RESET_RESET();
    Delay(60000);
    RESET_SET();
    Delay(500);
    PCD_WriteReg(RC522_CommandReg, PCD_RESETPHASE);
    Delay(2000);


    PCD_WriteReg(RC522_ModeReg, 0x3D);
    PCD_WriteReg(RC522_TReloadRegL, 30);
    PCD_WriteReg(RC522_TReloadRegH, 0);
    PCD_WriteReg(RC522_TModeReg, 0x8D);
    PCD_WriteReg(RC522_TPrescalerReg, 0x3E);
    PCD_WriteReg(RC522_TxAutoReg, 0x40);

    PCD_ClearBitMask(RC522_TestPinEnReg, 0x80);
    PCD_WriteReg(RC522_TxAutoReg, 0x40);
}

/*
 * Запись данных в регистр
 * address: адрес регистра
 * value:   данные
 */
static void PCD_WriteReg(uint8_t address, uint8_t value)
{
    uint8_t ucAddr;
    NSEL_RESET();
    ucAddr = ((address<<1) & 0x7E);
    SPI_TransmitReceive(RC522_SPI, ucAddr);
    SPI_TransmitReceive(RC522_SPI, value);
    NSEL_SET();
}

/*
 * Сброс данных регистра по битовой маске
 * address: адрес регистра
 */
static void PCD_ClearBitMask(uint8_t address, uint8_t mask)
{
	uint8_t tmp = 0x00;
    tmp = PCD_ReadReg(address);
    PCD_WriteReg(address, tmp & ~mask);  // clear bit mask
}

/*
 * Чтение данных регистра
 * address: адрес регистра
 */
static uint8_t PCD_ReadReg(uint8_t address)
{
     uint8_t ucAddr;
     uint8_t ucResult=0;

     NSEL_RESET();
     ucAddr = ((address<<1)&0x7E) | 0x80;

     SPI_TransmitReceive(RC522_SPI, ucAddr);
     ucResult = SPI_TransmitReceive(RC522_SPI, 0xff);
     NSEL_SET();
     return ucResult;
}


void RC522_AntennaOn(void)
{
    uint8_t i;
    i = PCD_ReadReg(RC522_TxControlReg);
    if (!(i & 0x03))
    {
    	PCD_SetBitMask(RC522_TxControlReg, 0x03);
    }
}


void RC522_AntennaOff(void)
{
	PCD_ClearBitMask(RC522_TxControlReg, 0x03);
}


static void PCD_SetBitMask(uint8_t address, uint8_t mask)
{
    uint8_t tmp = 0x0;
    tmp = PCD_ReadReg(address);
    PCD_WriteReg(address, tmp | mask);  // set bit mask
}


void RC522_Config(void)
{
	PCD_ClearBitMask(RC522_Status2Reg, 0x08);
	PCD_WriteReg(RC522_ModeReg, 0x3D);         //3F
	PCD_WriteReg(RC522_RxSelReg, 0x86);        //84
	PCD_WriteReg(RC522_RFCfgReg, 0x7F);        //4F
	PCD_WriteReg(RC522_TReloadRegL, 30);
	PCD_WriteReg(RC522_TReloadRegH, 0x00);
	PCD_WriteReg(RC522_TModeReg, 0x8D);
	PCD_WriteReg(RC522_TPrescalerReg, 0x3E);

	Delay(5000);
	RC522_AntennaOn();
}



static rc522_status PCD_Authenticate(uint8_t command, uint8_t blockAddr, uint8_t *key, uint8_t *uid, uint8_t uidSize)
{
	// Build command buffer
	uint8_t sendData[12];
	uint8_t dataSize = sizeof(sendData);

	rc522_status status;

	sendData[0] = command;
	sendData[1] = blockAddr;

	for (uint8_t i = 0; i < MF_CLASSIC_KEY_SIZE; i ++)
	{
		sendData[2 + i] = *(key + i);
	}
	// Use the last uid bytes as specified in http://cache.nxp.com/documents/application_note/AN10927.pdf
	// section 3.2.5 "MIFARE Classic Authentication".
	// The only missed case is the MF1Sxxxx shortcut activation,
	// but it requires cascade tag (CT) byte, that is not part of uid.
	for (uint8_t i = 0; i < 4; i++)
	{
		sendData[8 + i] = *(uid + i + uidSize - 4);
	}

    status = PCD_Communicate(PCD_AUTHENT, &sendData[0], dataSize, &sendData[0], &dataSize);


    return status;
}


/**
 * Reads 16 bytes (+ 2 bytes CRC_A) from the active PICC.
 *
 * For MIFARE Classic the sector containing the block must be authenticated before calling this function.
 *
 * For MIFARE Ultralight only addresses 00h to 0Fh are decoded.
 * The MF0ICU1 returns a NAK for higher addresses.
 * The MF0ICU1 responds to the READ command by sending 16 bytes starting from the page address defined by the command argument.
 * For example; if blockAddr is 03h then pages 03h, 04h, 05h, 06h are returned.
 * A roll-back is implemented: If blockAddr is 0Eh, then the contents of pages 0Eh, 0Fh, 00h and 01h are returned.
 *
 * The buffer must be at least 18 bytes because a CRC_A is also returned.
 * Checks the CRC_A before returning STATUS_OK.
 *
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
static rc522_status PICC_Read(uint8_t  blockAddr, 	  ///< MIFARE Classic: The block (0-0xFF) number. MIFARE Ultralight: The first page to return data from.
					          uint8_t *buffer,	      ///< The buffer to store the data in
					          uint8_t bufferSize     ///< Buffer size, at least 18 bytes. Also number of bytes returned if STATUS_OK.
                             )
{
	rc522_status status;
	uint32_t recvBuffSize;


	// Sanity check
	if (buffer == NULL || bufferSize < RC522_MAX_FRAME_LEN)
	{
		return STATUS_NO_ROOM;
	}

	// Build command buffer
	*(buffer + 0) = PICC_READ;
	*(buffer + 1) = blockAddr;
	// Calculate CRC_A
	status = PCD_CalulateCRCA(buffer, 2, (uint8_t *)(buffer + 2));
	if (status != STATUS_OK)
	{
		return status;
	}

    status = PCD_Communicate(PCD_TRANSCEIVE, buffer, 4, buffer, &recvBuffSize);

    return status;
}


/*
 * Отправка запроса (REQA, WUPA, HLTA) и получение ответа atqa
 * request: тип запроса
 * atqa: указатель на массив содержайщий atqa (16 бит)
 */
static rc522_status PICC_RequestA(uint8_t request, uint8_t *atqa)
{
   uint8_t status;
   uint32_t  unLen;
   uint8_t ucComMF522Buf[2];

   // включаем модуль MIFARE® Crypto1
   PCD_ClearBitMask(RC522_Status2Reg, 0x08);
   // включаем передачу данных на выходах TX1,TX2 на частоте несущей 13.56 MHz
   PCD_SetBitMask(RC522_TxControlReg, 0x03);
   ucComMF522Buf[0] = request;

   PCD_WriteReg(RC522_BitFramingReg, 0x07); // посылка 7 бит


   // отправляем код запроса
   status = PCD_Communicate(PCD_TRANSCEIVE, ucComMF522Buf, 1, ucComMF522Buf, &unLen);
   if ((status == STATUS_OK) && (unLen == 2))
   {
       *atqa       = ucComMF522Buf[0];
       *(atqa + 1) = ucComMF522Buf[1];
   }
   else
   {
	   status = STATUS_ERROR;
   }

   return status;
}


/*
 * Обмен данными с PICC
 * command:     команда (RC522_PCD_TRANSCEIVE, RC522_PCD_AUTHENT)
 * *inData:    указатель на буфер с данными для отправки на PICC
 *  inLenByte:   длина данных на отправку
 * *outData:   указатель на буфер для входящих данных
 * *pOutLenBit: указатель на переменную с длиной полученых данных
 */
static rc522_status PCD_Communicate(uint8_t  command, uint8_t  *inData, uint8_t inLen,
                                    uint8_t  *outData, uint32_t *outLen)
{
    uint8_t lastBits;
    uint8_t irq_reg, err_reg, irqEn, irqWait, reg;
    uint32_t i;


    switch (command)
    {
    case PCD_AUTHENT:
    	irqEn   = 0x12;
    	irqWait = 0x10;
        break;
    case PCD_TRANSCEIVE:
        irqEn   = 0x77;
        irqWait = 0x30; // RxIRq, IdleIRq
        break;
    default:
        irqEn   = 0x00;
        irqWait = 0x00;
        break;
    }

    //PCD_WriteReg(RC522_MfRxReg, (1<<4));
    //PCD_WriteReg(RC522_ComIEnReg, irqEn|0x80);     // Включаем передачу запросов на прерывания на пин IRQ
    PCD_WriteReg(RC522_CommandReg, PCD_IDLE);        // Тормозим выполнение всех текущих команд
    PCD_WriteReg(RC522_ComIrqReg, 0x7F);             // Сбрасываем все 7 бит регистра прерываний
    PCD_SetBitMask(RC522_FIFOLevelReg, 0x80);        // Очищаем буфер FIFO

    // заполняем буфер FIFO данными
    for (i = 0; i < inLen; i++)
    {
    	PCD_WriteReg(RC522_FIFODataReg, inData[i]);
    }

    // отправляем команду на выполнение
    PCD_WriteReg(RC522_CommandReg, command);

    // для команды начала обмена данными с PICC
    if (command == PCD_TRANSCEIVE)
    {
    	// устанавливаем флаг начала процесса обмена данными с PICC
    	PCD_SetBitMask(RC522_BitFramingReg, 0x80);
    }

    // ждем окончания приёма данных от PICC
    for (i = 3000; i > 0; i--)
    {
    	irq_reg = PCD_ReadReg(RC522_ComIrqReg);  //читаем значение регистра прерываний

    	if (irq_reg & 0x01)
    	{//вышло время внутр. таймера rc522 (данные не получены)
    		return STATUS_PCD_TIMEOUT;
    	}

    	if (irq_reg & irqWait)
    	{// данные получены
    		break;
    	}

    }

    if (i == 0)
    {//вышло время ожидания (наш МК)
    	return STATUS_TIMEOUT;
    }

    PCD_ClearBitMask(RC522_BitFramingReg, 0x80); //сбрасываем флаг процесса обмена данными с PICC

    err_reg = PCD_ReadReg(RC522_ErrorReg);   //читаем значение регистра ошибок

    if (err_reg & 0x13)
    {//Ошибки: BufferOvfl, ParityErr, ProtocolErr
    	return STATUS_ERROR;
    }

    if (command == PCD_TRANSCEIVE)
    {
   	    // вычисление длины полученных данных
      	reg = PCD_ReadReg(RC522_FIFOLevelReg);

      	*outLen = reg;

        if (reg > RC522_MAX_FRAME_LEN)
        {
        	return STATUS_OVERFRAME;
        }

        // чтение данных с буфера FIFO
        for (i = 0; i < reg; i++)
        {
        	outData[i] = PCD_ReadReg(RC522_FIFODataReg);
        }
    }

    if (err_reg & 0x08)
    {//Обнаружена коллизия
    	return STATUS_COLLISION;
    }

    //PCD_SetBitMask(RC522_ControlReg, 0x80);
    //PCD_WriteReg(RC522_CommandReg, RC522_PCD_IDLE);

    return STATUS_OK;
}



/*
 * Процедура бит-кадровой антиколлизии в пределах одного каскадного уровня.
 *
 * cascadeLevel: код текущего каскадного уровня процедуры антиколлизии
 * uid:          4 полных байта UID (в случае успешной отработки)
 */
static rc522_status PICC_AntiCollisionA(uint8_t cascadeCode, uint8_t *uid)
{

	uint8_t txLastBits, rxAlign;
	uint8_t count, index, reqLen, checkBit;
	uint32_t respLen;
	uint8_t validBitsUID = 0;
	uint8_t buff[9] = {0};
	uint8_t *respBuff;
	rc522_status status = STATUS_ERROR;
	uint8_t collReg, collPos;
	uint8_t i = 0x00;



    //PCD_ClearBitMask(RC522_Status2Reg, 0x08);
    PCD_ClearBitMask(RC522_CollReg, 0x80);

    buff[0] = cascadeCode;


	while (i < MAX_ANTICOLL_ITERATION)
	{
		i++;

		txLastBits = validBitsUID % 8;
		rxAlign = txLastBits;
		count = validBitsUID / 8;	// Number of whole bytes in the UID part.
		index = 2 + count;			// Number of whole bytes: SEL + NVB + UIDs

		buff[1] = (index << 4) + txLastBits;	// NVB - Number of Valid Bits

		reqLen = index + (txLastBits ? 1 : 0);
		respBuff = &buff[index];

		// RxAlign -> BitFramingReg[6..4]. TxLastBits -> BitFramingReg[2..0]
		PCD_WriteReg(RC522_BitFramingReg, (rxAlign << 4) | txLastBits);

		// Transmit the buffer and receive the response.
		status = PCD_Communicate(PCD_TRANSCEIVE, buff, reqLen, respBuff, &respLen);

		if (status == STATUS_COLLISION)
		{
			// CollReg[7..0] bits are: ValuesAfterColl reserved CollPosNotValid CollPos[4:0]
			collReg = PCD_ReadReg(RC522_CollReg);
			if (collReg & 0x20) // CollPosNotValid
			{
				return STATUS_COLLISION; // Without a valid collision position we cannot continue
			}
			collPos = collReg & 0x1F; // Values 0-31, 0 means bit 32.
			if (collPos == 0)
			{
				collPos = 32;
			}
			if (collPos <= validBitsUID) // No progress - should not happen
			{
				return STATUS_IERROR;
			}
			// Choose the PICC with the bit set.
			validBitsUID = collPos;
			count = validBitsUID % 8; // The bit to modify
			checkBit = (validBitsUID - 1) % 8;
			index = 1 + (validBitsUID / 8) + (count ? 1 : 0); // First byte is index 0.
			buff[index] |= (1 << checkBit);
		}
		else if (status == STATUS_OK)
		{
			*(uid + 0) = buff[2];
			*(uid + 1) = buff[3];
			*(uid + 2) = buff[4];
			*(uid + 3) = buff[5];

			return status;
		}
		else
		{
			return STATUS_COLLISION;
		}
	}

	return STATUS_COLLISION;

}


/*
 * Процедура (SELECT) в пределах одного каскадного уровня.
 * Если процедура биткадровой антиколлизии пройдена успешно,
 * то PICC должна передать SAK с каскадным битом.
 * Если каскадный бит установлен, необходимо перейти не след. каскадный уровень.
 *
 * cascadeLevel: код текущего каскадного уровня процедуры антиколлизии
 * uid:          4 полных байта UID (результат отработки PICC_AntiCollisionA)
 * sak:          ответ PICC, sak[0] - SAK, sak[1..2] - CRC_A
 */
static rc522_status PICC_SelectA(uint8_t cascadeCode, uint8_t *uid, uint8_t *sak)
{

	uint8_t reqBuff[10]  = {0};
	uint8_t respBuff[10]  = {0};
	rc522_status status = STATUS_ERROR;
	uint32_t respLen = 0;


	reqBuff[0] = cascadeCode;
	reqBuff[1] = 0x70;
	reqBuff[2] = *(uid + 0);
	reqBuff[3] = *(uid + 1);
	reqBuff[4] = *(uid + 2);
	reqBuff[5] = *(uid + 3);
	reqBuff[6] = reqBuff[2] ^ reqBuff[3] ^ reqBuff[4] ^ reqBuff[5];

	status = PCD_CalulateCRCA(reqBuff, 7, &reqBuff[7]);

	if (status != STATUS_OK)
	{
		return status;
	}

	PCD_WriteReg(RC522_BitFramingReg, 0x00);

	status = PCD_Communicate(PCD_TRANSCEIVE, reqBuff, 9, respBuff, &respLen);

	if (respLen != 3)
	{
		return STATUS_ERROR;
	}
	else
	{
		*(sak + 0) = *(respBuff + 0);
		*(sak + 1) = *(respBuff + 1);
		*(sak + 2) = *(respBuff + 2);
	}

	return status;

}



static rc522_status PCD_CalulateCRCA(uint8_t *inData, uint8_t len, uint8_t *outData)
{
	uint16_t i, n;

    PCD_WriteReg(RC522_CommandReg, PCD_IDLE); // Stop any active command.
    PCD_ClearBitMask(RC522_DivIrqReg, 0x04);        // Clear the CRCIRq interrupt request bit
    PCD_SetBitMask(RC522_FIFOLevelReg, 0x80);       // FlushBuffer = 1, FIFO initialization


    // Write data to the FIFO
    for (i = 0; i < len; i ++)
    {
    	PCD_WriteReg(RC522_FIFODataReg, *(inData + i));
    }

    PCD_WriteReg(RC522_CommandReg, PCD_CALCCRC); // Start the calculation

    i = 0xFFFF;
    while (i)
    {
        n = PCD_ReadReg(RC522_DivIrqReg);

		if (n & 0x04) // CRCIRq bit set - calculation done
		{
			PCD_WriteReg(RC522_CommandReg, PCD_IDLE); // Stop calculating CRC for new content in the FIFO.
			// Transfer the result from the registers to the result buffer
		    *(outData + 0) = PCD_ReadReg(RC522_CRCResultRegL);
		    *(outData + 1) = PCD_ReadReg(RC522_CRCResultRegM);
			return STATUS_OK;
		}

        i --;
    }

    return STATUS_ERROR;



}



static void CalulateCRCA(uint8_t *data, uint8_t len, uint8_t *crc)
{

	uint8_t chBlock;
	uint16_t wCRC = 0x6363; // ITU-V.41

	while (len--)
	{
		chBlock = *data++;

		{
			chBlock = chBlock ^ (uint8_t)(wCRC & 0x00FF);
			chBlock = chBlock ^ (chBlock << 4);
			wCRC = (wCRC >> 8) ^ ((uint16_t)chBlock << 8) ^ ((uint16_t)chBlock << 3) ^ ((uint16_t)chBlock >> 4);
		}


	}

	*(crc + 0) = (uint8_t)(wCRC & 0x00FF);
	*(crc + 1) = (uint8_t)((wCRC >> 8) & 0x00FF);

}



__STATIC_INLINE uint16_t SPI_TransmitReceive(SPI_TypeDef *SPIx, uint16_t data)
{
	//ждём пока регистр DR скинет данные в сдвиговый регистр
	while(LL_SPI_IsActiveFlag_TXE(SPIx) == 0UL) ;
	//отправляем данные
	LL_SPI_TransmitData16(SPIx, data);
	//ждём пока придёт ответ
	while(LL_SPI_IsActiveFlag_RXNE(SPIx) == 0UL) ;
	//считываем полученные данные
	return LL_SPI_ReceiveData16(SPIx);
}



__STATIC_INLINE void Delay(__IO uint32_t nCount)
{
	//nCount *= 20;
    while(nCount--)
    {
	    __asm("NOP");
    }
}


static void GPIO_Init(void)
{
	  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

	  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);

	  GPIO_InitStruct.Pin = RC522_GPIO_RESET_PIN;
	  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
	  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	  LL_GPIO_Init(RC522_GPIO_RESET, &GPIO_InitStruct);
	  RESET_SET();

	  GPIO_InitStruct.Pin = RC522_GPIO_NSEL_PIN;
	  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
	  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	  LL_GPIO_Init(RC522_GPIO_NSEL, &GPIO_InitStruct);
	  RESET_SET();
}


/*
 * инициализация в режиме мастера с дефолтным набором настроек
 * вывод NSS программно подтянут к VCC
 */
static void SPI__Init(SPI_TypeDef* SPIx, uint8_t afn)
{
	  LL_SPI_InitTypeDef SPI_InitStruct = {0};
	  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

	  if (SPIx == SPI1)
	  {

		  /* Peripheral clock enable */
		  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SPI1);
		  if (afn == SPI_AFN_OFF)
		  {
			  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
			  GPIO_InitStruct.Pin = LL_GPIO_PIN_5|LL_GPIO_PIN_6|LL_GPIO_PIN_7;
			  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
			  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
			  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
			  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
			  GPIO_InitStruct.Alternate = LL_GPIO_AF_5;
			  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);
		  }
		  if (afn == SPI_AFN_ON)
		  {
			  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
			  GPIO_InitStruct.Pin = LL_GPIO_PIN_3|LL_GPIO_PIN_4|LL_GPIO_PIN_5;
			  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
			  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
			  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
			  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
			  GPIO_InitStruct.Alternate = LL_GPIO_AF_5;
			  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);
		  }
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


bool RC522_ReadBlock(PICC_ProtBaseInfTypeDef *PICC_ProtBaseInf, uint8_t blockAddr, uint8_t *key, uint8_t *buff, uint8_t buffSize)
{
	rc522_status status;

	status = PCD_Authenticate(PICC_AUTHENT1A, blockAddr, key, &(PICC_ProtBaseInf->UID[0]), PICC_ProtBaseInf->UID_SIZE);
	if (status != STATUS_OK)
	{
		// error in the auth. procedure
		return false;
	}
	status = PICC_Read(blockAddr, buff, buffSize);
	if (status != STATUS_OK)
	{
		// error in the auth. procedure
		return false;
	}

	return true;
}


rc522_status RC522_test(PICC_ProtBaseInfTypeDef *PICC_ProtBaseInf)
{
	rc522_status status = 0x00;
	uint8_t reqBuff[10]  = {0};
	uint8_t respBuff[10]  = {0};
	uint8_t respLen = 0;



	// -------- AUTHENTICATE PASS 1
	// --  send an authentication request to the tag
	// --  and give challenge notagNonce Nt (TOKEN RB)
	reqBuff[0] = PICC_AUTHENT1A;
	reqBuff[1] = 0;
	CalulateCRCA(reqBuff, 2, &reqBuff[2]);

	PCD_WriteReg(RC522_BitFramingReg, 0x00);
	status = PCD_Communicate(PCD_TRANSCEIVE, reqBuff, 4, respBuff, &respLen);

	if (respLen != 4)
	{
		/*
        USART_SendStr(USART1, "ERROR RESP LEN 1: ");
	    memset((char *)str_buff, 0x00, 10);
	    sprintf(str_buff, "%02X ", respLen);
	    USART_SendStr(USART1, str_buff);
	    USART_SendEndLine(USART1);
		//return STATUS_ERROR;
		*/
	}

	// -------- AUTHENTICATE PASS 2

	uint8_t key[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	uint8_t uid[4];
	uid[0] = PICC_ProtBaseInf->UID[0];
	uid[1] = PICC_ProtBaseInf->UID[1];
	uid[2] = PICC_ProtBaseInf->UID[2];
	uid[3] = PICC_ProtBaseInf->UID[3];

	uint8_t tagNonce[8] = {0};
	tagNonce[0] = respBuff[0];
	tagNonce[1] = respBuff[1];
	tagNonce[2] = respBuff[2];
	tagNonce[3] = respBuff[3];


	/*{
		USART_SendStr(USART1, "Nt: ");
		memset((char *)str_buff, 0x00, 10);
		for(uint8_t i = 0; i < 4; i ++)
		{
			sprintf(str_buff, "%02X ", tagNonce[i]);
			USART_SendStr(USART1, str_buff);
		}
		USART_SendEndLine(USART1);
	}*/

	uint8_t NR[4];
	uint8_t encryptNR[4];
	uint8_t AR[4];
	uint8_t encryptAR[4];
	{
		/*
		 * Математические основы данного алгоритма шифрования подробно рассмотрены в труде:
		 * "5211 - SECURITY IN RFID DEVICES"
		 * Барселонский университет автоматики.
		 * 2013г
		 */

		//Инициализация шифратора
		Crypto1_ChipherInit(key, uid, tagNonce);
		//Формируем объявление ридера
		Crypto1_GetReaderNonce(NR);
		//Формируем ключевой поток KS1
		Crypto1_CalculateKeyStream(MF_CLASSIC_NONCE_SIZE);
		//Шифруем потоком KS1 объявление ридера
		Crypto1_EncryptWithKeyStream(NR, encryptNR, MF_CLASSIC_NONCE_SIZE);
		//Формируем ключевой поток KS2
		Crypto1_CalculateKeyStream(MF_CLASSIC_NONCE_SIZE);
		//Вычисляем ответ Ar на объявление метки Nt
		AR[0] = tagNonce[0]; AR[1] = tagNonce[1]; AR[2] = tagNonce[2]; AR[3] = tagNonce[3];
		Crypto1_CalculateSuc(tagNonce, 64);
		//Шифруем потоком KS2 ответ Ar
		Crypto1_EncryptWithKeyStream(AR, encryptAR, MF_CLASSIC_NONCE_SIZE);
	}



	reqBuff[0] = encryptNR[0];
	reqBuff[1] = encryptNR[1];
	reqBuff[2] = encryptNR[2];
	reqBuff[3] = encryptNR[3];
	reqBuff[4] = encryptAR[0];
	reqBuff[5] = encryptAR[1];
	reqBuff[6] = encryptAR[2];
	reqBuff[7] = encryptAR[3];

	/*{
		USART_SendStr(USART1, "REQ: ");
		memset((char *)str_buff, 0x00, 10);
		for(uint8_t i = 0; i < 8; i ++)
		{
			sprintf(str_buff, "%02X ", reqBuff[i]);
			USART_SendStr(USART1, str_buff);
		}
		USART_SendEndLine(USART1);
	}*/

	//CalulateCRCA(reqBuff, 2, &reqBuff[2]);

	PCD_WriteReg(RC522_BitFramingReg, 0x00);
	status = PCD_Communicate(PCD_TRANSCEIVE, reqBuff, 8, respBuff, &respLen);

	if (respLen != 4)
	{
        USART_SendStr(USART1, "ERROR RESP(10) LEN : ");
	    memset((char *)str_buff, 0x00, 10);
	    sprintf(str_buff, "%02X ", respLen);
	    USART_SendStr(USART1, str_buff);
	    USART_SendEndLine(USART1);

		return STATUS_ERROR;
	}

	USART_SendStr(USART1, "ACCESS PERMITED!");
	USART_SendEndLine(USART1);
	return status;


}


void  RC522_test_1(void)
{
	/*
	uint8_t reqBuff[10]  = {0xEF, 0xEA, 0x1C, 0xDA, 0x8D, 0x65, 0x00, 0x00, 0x00, 0x00};

	CalulateCRCA(reqBuff, 6, &reqBuff[6]);

	USART_SendStr(USART1, "tagNonce: ");
	memset((char *)str_buff, 0x00, 10);
	//for(uint8_t i = 0; i < 10; i ++)
	for(uint8_t i = 0; i < 8; i ++)
	{
		sprintf(str_buff, "%02X ", reqBuff[i]);
		USART_SendStr(USART1, str_buff);
	}
	USART_SendEndLine(USART1);
	*/
}








/*
 *
    	{
    	    USART_SendStr(USART1, "INTERNAL TIMEOUT");
    	    USART_SendEndLine(USART1);
    	}


        {
            USART_SendStr(USART1, "OUT DATA LEN: ");
    	    memset((char *)str_buff, 0x00, 60);
    	    sprintf(str_buff, "%02X ", reg);
    	    USART_SendStr(USART1, str_buff);
    	    USART_SendEndLine(USART1);
        }


    {
    	USART_SendStr(USART1, "COMMAND: ");
    	memset((char *)str_buff, 0x00, 60);
    	sprintf(str_buff, "%02X", command);
    	USART_SendStr(USART1, str_buff);
    	USART_SendEndLine(USART1);

        USART_SendStr(USART1, "IN DATA: ");
	    memset((char *)str_buff, 0x00, 60);
	    memset((char *)ts, 0x00, 20);
	    for (int i = 0; i < inLen; i ++)
		    strcat(ts, "%02X ");
	    sprintf(str_buff, ts, inData[0], inData[1], inData[2], inData[3], inData[4], inData[5], inData[6], inData[7], inData[8], inData[9]);
	    //sprintf(str_buff, "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X", inData[0], inData[1], inData[2], inData[3], inData[4], inData[5], inData[6], inData[7], inData[8], inData[9]);
	    USART_SendStr(USART1, str_buff);
	    USART_SendEndLine(USART1);
    }

 */


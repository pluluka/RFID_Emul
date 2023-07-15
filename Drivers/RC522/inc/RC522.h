/*
 * RC522.h
 *
 *  Created on: 11 апр. 2020 г.
 *      Author: tochk
 */

#ifndef RC522_INC_RC522_H_
#define RC522_INC_RC522_H_




#include "stm32f4xx.h"
#include "stm32f4xx_ll_spi.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_gpio.h"
#include <stdbool.h>


#define NULL (uint32_t)0x00000000


// SPI module
#define RC522_SPI SPI2
// RESET pin (RST)
#define RC522_GPIO_RESET       GPIOB
#define RC522_GPIO_RESET_PIN   LL_GPIO_PIN_7
// NSEL pin (NSS)
#define RC522_GPIO_NSEL        GPIOB
#define RC522_GPIO_NSEL_PIN    LL_GPIO_PIN_6



#define STATUS_OK                  0x00
#define STATUS_NOTAGERR            0x01
#define STATUS_ERROR               0x02
#define STATUS_COLLISION           0x03
#define STATUS_TIMEOUT             0x04
#define STATUS_OVERFRAME           0x05
#define STATUS_IERROR              0x06
#define STATUS_CRC_ERROR           0x07
#define STATUS_NO_ROOM             0x08
#define STATUS_PCD_TIMEOUT         0x09


/*
 * Commands sent to the PICC.
 */
#define PICC_REQIDL           0x26  // REQA command, Type A. Invites PICCs in state IDLE to go to READY and prepare for anticollision or selection. 7 bit frame.
#define PICC_REQALL           0x52  // WUPA command, Type A. Invites PICCs in state IDLE and HALT to go to READY(*) and prepare for anticollision or selection. 7 bit frame.
#define PICC_ANTICOLL_CL1     0x93  // Anti collision/Select, Cascade Level 1
#define PICC_ANTICOLL_CL2     0x95  // Anti collision/Select, Cascade Level 2
#define PICC_ANTICOLL_CL3	  0x97  // Anti collision/Select, Cascade Level 3
#define PICC_AUTHENT1A        0x60  // Perform authentication with Key A
#define PICC_AUTHENT1B        0x61  // Perform authentication with Key B
#define PICC_READ             0x30  // Reads one 16 byte block from the authenticated sector of the PICC. Also used for MIFARE Ultralight.
#define PICC_WRITE            0xA0  // Writes one 16 byte block to the authenticated sector of the PICC. Called "COMPATIBILITY WRITE" for MIFARE Ultralight.
#define PICC_DECREMENT        0xC0  // Decrements the contents of a block and stores the result in the internal data register.
#define PICC_INCREMENT        0xC1  // Increments the contents of a block and stores the result in the internal data register.
#define PICC_RESTORE          0xC2  // Reads the contents of a block into the internal data register.
#define PICC_TRANSFER         0xB0  // Writes the contents of the internal data register to a block.
#define PICC_HALT             0x50  // HaLT command, Type A. Instructs an ACTIVE PICC to go to state HALT.



/*
 * MFRC522 Commands Overview
 */
#define PCD_IDLE              0x00   // No action; cancels current command execution
#define PCD_MEM               0x01   // Stores 25 byte into the internal buffer
#define PCD_GENRANDOMID       0x02   // Generates a 10 byte random ID number
#define PCD_CALCCRC           0x03   // Activates the CRC co-processor or performs a selftest
#define PCD_TRANSMIT          0x04   // Transmits data from the FIFO buffer
#define PCD_NOCMDCHANGE       0x07   // No command change
#define PCD_RECEIVE           0x08   // Activates the receiver circuitry
#define PCD_TRANSCEIVE        0x0C   // Transmits data from FIFO buffer to the antenna and activates
#define PCD_AUTHENT           0x0E   // Performs the MIFARE® standard authentication as a reader
#define PCD_RESETPHASE        0x0F   // Resets the MFRC522


/*
 * MFRC522 Registers Overview
 */
// Page 0
#define     RC522_RFU00                 0x00
#define     RC522_CommandReg            0x01
#define     RC522_ComIEnReg             0x02
#define     RC522_DivlEnReg             0x03
#define     RC522_ComIrqReg             0x04
#define     RC522_DivIrqReg             0x05
#define     RC522_ErrorReg              0x06
#define     RC522_Status1Reg            0x07
#define     RC522_Status2Reg            0x08
#define     RC522_FIFODataReg           0x09
#define     RC522_FIFOLevelReg          0x0A
#define     RC522_WaterLevelReg         0x0B
#define     RC522_ControlReg            0x0C
#define     RC522_BitFramingReg         0x0D
#define     RC522_CollReg               0x0E
#define     RC522_RFU0F                 0x0F
// Page 1
#define     RC522_RFU10                 0x10
#define     RC522_ModeReg               0x11
#define     RC522_TxModeReg             0x12
#define     RC522_RxModeReg             0x13
#define     RC522_TxControlReg          0x14
#define     RC522_TxAutoReg             0x15
#define     RC522_TxSelReg              0x16
#define     RC522_RxSelReg              0x17
#define     RC522_RxThresholdReg        0x18
#define     RC522_DemodReg              0x19
#define     RC522_RFU1A                 0x1A
#define     RC522_RFU1B                 0x1B
#define     RC522_MfTxReg               0x1C
#define     RC522_MfRxReg               0x1D
#define     RC522_RFU1E                 0x1E
#define     RC522_SerialSpeedReg        0x1F
// Page 2
#define     RC522_RFU20                 0x20
#define     RC522_CRCResultRegM         0x21
#define     RC522_CRCResultRegL         0x22
#define     RC522_RFU23                 0x23
#define     RC522_ModWidthReg           0x24
#define     RC522_RFU25                 0x25
#define     RC522_RFCfgReg              0x26
#define     RC522_GsNReg                0x27
#define     RC522_CWGsCfgReg            0x28
#define     RC522_ModGsCfgReg           0x29
#define     RC522_TModeReg              0x2A
#define     RC522_TPrescalerReg         0x2B
#define     RC522_TReloadRegH           0x2C
#define     RC522_TReloadRegL           0x2D
#define     RC522_TCounterValueRegH     0x2E
#define     RC522_TCounterValueRegL     0x2F
// Page 3
#define     RC522_RFU30                 0x30
#define     RC522_TestSel1Reg           0x31
#define     RC522_TestSel2Reg           0x32
#define     RC522_TestPinEnReg          0x33
#define     RC522_TestPinValueReg       0x34
#define     RC522_TestBusReg            0x35
#define     RC522_AutoTestReg           0x36
#define     RC522_VersionReg            0x37
#define     RC522_AnalogTestReg         0x38
#define     RC522_TestDAC1Reg           0x39
#define     RC522_TestDAC2Reg           0x3A
#define     RC522_TestADCReg            0x3B
#define     RC522_RFU3C                 0x3C
#define     RC522_RFU3D                 0x3D
#define     RC522_RFU3E                 0x3E
#define     RC522_RFU3F		            0x3F

#define MAX_ANTICOLL_ITERATION          40
#define RC522_MAX_FRAME_LEN             18

typedef uint8_t rc522_status;


/*
 * Типовые значения ATQA и SAK приведены в MifareClassic.h
*/
typedef struct
{
	uint8_t ATQA[2];
	uint8_t UID[10];
	uint8_t UID_SIZE;
	uint8_t SAK[3];
} PICC_ProtBaseInfTypeDef;



void RC522_Init(void);
void RC522_Reset(void);
void RC522_AntennaOff(void);
void RC522_AntennaOn(void);
void RC522_Config(void);

bool RC522_CardDetect(PICC_ProtBaseInfTypeDef *PICC_ProtBase);
bool RC522_CardSelect(PICC_ProtBaseInfTypeDef *PICC_ProtBase);
void RC522_CardHalt(void);
bool RC522_ReadBlock(PICC_ProtBaseInfTypeDef *PICC_ProtBaseInf, uint8_t blockAddr,
		uint8_t *key, uint8_t *buff, uint8_t buffSize);



rc522_status RC522_test(PICC_ProtBaseInfTypeDef *PICC_ProtBaseInf);
void  RC522_test_1(void);

//defines



/*
 * PCD  - терминальное оборудование близкого действия
 * PICC - карта или объект близкого действия
 * ATQA - Ответ на Запрос, тип A (Answer То reQuest, Туре А);
 * NVB - число допустимых бит, тип A (Number of Valid Bits);
 * SEL - код выбора
 *
 */

#endif /* RC522_INC_RC522_H_ */

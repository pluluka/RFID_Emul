/*
 * OneWireThread.c
 *
 *  Created on: 15 июл. 2021 г.
 *      Author: tochk
 *
 *      Вся обработка данных в потоке.
 *      Функции чтения/записи/эмуляции реализованы в виде соответствующих интерфейсных функций.
 *      Результат отработки интерфейсной функции - сообщение потоку,
 *      даже в случае неудачного исхода - фейла (например, неудачная попытка идентификации устройства)
 *      Далее поток обрабатывает сообщение и отдает управление.
 */

#include "OneWireThread.h"



#define THREAD_STACK_SIZE		250
#define MSG_QUEUE_DEPTH			10

/*
// типы сообщений дравера
typedef enum{
	DRV_SLAVE_ONWIRE = 0,		// ключ сформировал на шине импульс присутствия
	DRV_SLAVE_RESET_DETECTED,	// детектирование импульса - MASTER Tx "RESET PULSE"
	DRV_SLAVE_DATA_READ,		// поступили данные (8 бит данных, как правило это код команды)
	DRV_SLAVE_IDLE				// обмен данными закончен, драйвер в режиме ожидания
} OneWireMsgDriver_t;
*/

//	базовая информация БД
typedef struct{
	uint32_t recCount;
	int currentRecNum;
} OneWireDataBaseInfo_t;


static __IO uint32_t 	threadIDLEPriority;		// приоритет потока в режиме ожидания

osThreadId		threadOneWire;
osMessageQId	msgQueueOneWire;
osPoolId		memPoolOneWire;

extern osThreadId		threadCLI;
extern osMessageQId		msgQueueCLI;
extern osPoolId			memPoolCLI;


OneWireDataRec_t		owCurrentData;	// текущие данные
OneWireDataBaseInfo_t	owDBInf;

static char 	strBuff[100];	//строковый буфер


__STATIC_INLINE CLI_SendMsg_CLI_EN(void);



void OneWireThreadInit(uint32_t priority)
{
	threadIDLEPriority = priority;
	// Очередь сообщений основного потока
	osMessageQDef(msgQueueOneWire, MSG_QUEUE_DEPTH, uint32_t);
	msgQueueOneWire = osMessageCreate(osMessageQ(msgQueueOneWire), NULL);
	// Создаем пулы под данные очереди
	osPoolDef(memPoolOneWire, MSG_QUEUE_DEPTH, OneWireMsgData_t);
	memPoolOneWire = osPoolCreate(osPool(memPoolOneWire));
	// Очередь сообщений драйвера
	//osMessageQDef(msgQueueOneWireDrv, MSG_QUEUE_DEPTH_DRV, uint32_t);
	//msgQueueOneWireDrv = osMessageCreate(osMessageQ(msgQueueOneWireDrv), NULL);
	// Создаем задачу
	osThreadDef(OneWireThread, OneWireThread, threadIDLEPriority, 0, THREAD_STACK_SIZE);
	threadOneWire = osThreadCreate(osThread(OneWireThread), NULL);
	if (threadOneWire == NULL)
	{
		__debugError(0);
	}

	memset(&owCurrentData, 0x00, sizeof(OneWireDataRec_t));
}


/*
 * Задача.
 */
void OneWireThread(void const * argument)
{
	osEvent				evt;
	osStatus   			status;
	OneWireMsgData_t	*msgData, *selfMsgData;
	OneWireData_t		oneWireData;


	//Пока-что тормозим поток
	//osThreadSuspend(NULL);

	/**** Поток ****/
	for(;;)
	{
		evt = osMessageGet(msgQueueOneWire, osWaitForever);
		if (evt.status == osEventMessage)
		{
			msgData = (OneWireMsgData_t*)evt.value.p;

			switch (msgData->msgType){

				case OW_THREAD_MSG__CMD_READ_ROM:
				{
					OneWireMasterInit();
					CLI_PrintString("Waiting for a MC..\r\n\r\n");

					selfMsgData = (OneWireMsgData_t *)osPoolAlloc(memPoolOneWire);
					if (selfMsgData != NULL)
					{
						selfMsgData->msgType = OW_THREAD_MSG__CMD_READ_ROM_BEGIN;
						selfMsgData->p = NULL;
						if (osMessagePut(msgQueueOneWire, (uint32_t)(selfMsgData), 0) != osOK)
						{
							__debugError(0);
							osPoolFree(memPoolOneWire, selfMsgData);
							break;
						}
					}

					break;
				}
				case OW_THREAD_MSG__CMD_READ_ROM_BEGIN:
				{
					if (OneWireMasterReadROM(&oneWireData) == OWM_RS_OK)
					{
						memset(strBuff, 0x00, sizeof(strBuff));
						CLI_PrintString("Detected DS1990 compatable device\r\n");
						sprintf(strBuff, "ROM data: %02X %02X %02X %02X %02X %02X %02X %02X \r\n", oneWireData.romData[0], oneWireData.romData[1],
								oneWireData.romData[2], oneWireData.romData[3], oneWireData.romData[4], oneWireData.romData[5],
								oneWireData.romData[6], oneWireData.romData[7]);
						memcpy(owCurrentData.romData, oneWireData.romData, 8);
						CLI_PrintString(strBuff);
						CLI_PrintString("\r\n");

						osDelay(1000);
						//CLI_SendMsg_CLI_EN();
					}

					// Отправляем сообщение сами себе (повтор попытки считывания ROM)
					selfMsgData = (OneWireMsgData_t *)osPoolAlloc(memPoolOneWire);
					if (selfMsgData != NULL)
					{
						selfMsgData->msgType = OW_THREAD_MSG__CMD_READ_ROM_BEGIN;
						selfMsgData->p = NULL;
						if (osMessagePut(msgQueueOneWire, (uint32_t)(selfMsgData), 0) != osOK)
						{
							__debugError(0);
							osPoolFree(memPoolOneWire, selfMsgData);
							break;
						}
					}
					osDelay(100);

					break;
				}
				case OW_THREAD_MSG__CMD_CLI_EN:
					{
						CLI_SendMsg_CLI_EN();
						break;
					}
				case OW_THREAD_MSG__CMD_WRITE_ROM_RW1990:
				{
					OneWireMasterInit();
					CLI_PrintString("Writting RW1990..\r\n");
					if (OneWireMasterRW1990_WriteROM((OneWireData_t*)owCurrentData.romData) == OWM_RS_OK)
					{
						CLI_PrintString("DONE!\r\n");
					}
					else
					{
						CLI_PrintString("ERROR!\r\n");
					}

					CLI_SendMsg_CLI_EN();
					break;
				}

				case OW_THREAD_MSG__CMD_EMUL_CURRENT:
				{
					OneWireSlaveInit();
					CLI_PrintString("Starting emulation MC at current ROM..\r\n");
					OneWireSlaveDS1990_OnWire(owCurrentData.romData);
					break;
				}

				case OW_THREAD_MSG__CMD_EMUL_ALL:
				{

					break;
				}

				default:
				{
					break;
				}
			}
		}

		osPoolFree(memPoolOneWire, msgData);
		/* После отработки цикла потока (например - обработки сообщения), проверяем приоритет потока
		   и если он был изменен (например - основным потоком, для гарантированной обработки сообщения)
		   восстанавливаем приоритет до приоритета режима ожидания. */
		if (osThreadGetPriority(osThreadGetId()) != threadIDLEPriority)
		{
		    status = osThreadSetPriority (osThreadGetId(), threadIDLEPriority);
		    if (status != osOK)
		    {
		    	__debugError(0);

		    }
		}

		// отдаем управление другим задачам
		osThreadYield();
	}
}



OneWireDataRec_t* OneWireThreadGetCurrentData(void)
{
	return &owCurrentData;
}



__STATIC_INLINE CLI_SendMsg_CLI_EN(void)
{
	CLIMsgData_t*	msgData = (CLIMsgData_t *)osPoolAlloc(memPoolCLI);
	if (msgData != NULL)
	{
		msgData->msgType = CLI_THREAD_MSG__CLI_EN;
		msgData->p = NULL;
		// Добавляем указатель в очередь сообщений
		if (osMessagePut(msgQueueCLI, (uint32_t)(msgData), 0) == osOK)
		{
			osThreadYield();
		}
		else
		{// если возникли проблемы с очередью, освобождаем выделенную память
			osPoolFree(memPoolCLI, msgData);
			__debugError(0);
		}
	}
}



/*
 * EM4100ReaderThread.c
 *
 *  Created on: 27 янв. 2021 г.
 *      Author: tochk
 */


#include <EM4100Thread.h>



#define THREAD_STACK_SIZE		250
#define MSG_QUEUE_DEPTH			2	//????

#define REC_SIZE				5	//размер одной записи
#define REC_BUFFER_COUNT		10	//макс. кол-во записей в буфере
#define REC_BUFFER_SIZE			(REC_SIZE * REC_BUFFER_COUNT)

 //таймаут между запусками EM4100-ридера в периодическом режиме (мс)
#define  EM4100_READER_TIMEOUT		1000


typedef enum{
	RUN_ALL_ONCE,
	RUN_ALL_FOREWER
} em4100emulRunMode_t;

//	базовая информация БД
typedef struct{
	uint32_t recCount;
} EM4100DataBaseInfo_t;

typedef struct{
	EM4100EmulMode_t	emulMode;
	uint32_t 			currentRecID;
	uint32_t			lastRecID;
} EM4100EmulData_t;


osThreadId		threadEM4100;
osMessageQId	msgQueueEM4100;
osPoolId		memPoolEM4100;
osPoolId		memPoolEM4100_RWE_Data;

extern osThreadId		threadCLI;
extern osMessageQId		msgQueueCLI;
extern osPoolId			memPoolCLI;


static __IO uint32_t 	threadIDLEPriority;				// приоритет потока в режиме ожидания

EM4100DataRec_t			em4100currentData;				// данные меток под запись в БД

em4100TagData_t			recBuffer[REC_BUFFER_COUNT];	//буфер под записи из БД
em4100emulRunMode_t		em4100emulRunMode;

//строковый буфер
static char 	strBuff[50];


static void RunEmulation(em4100TagData_t *data, uint32_t dataCount);
__STATIC_INLINE void ThreadResume(void);
__STATIC_INLINE CLI_SendMsg_CLI_EN(void);




void EM4100ThreadInit(uint32_t priority)
{
	threadIDLEPriority = priority;

	// Создаем очередь
	osMessageQDef(msgQueueEM4100, MSG_QUEUE_DEPTH, uint32_t);
	msgQueueEM4100 = osMessageCreate(osMessageQ(msgQueueEM4100), NULL);
	// Создаем пулы под данные очереди
	osPoolDef(memPoolEM4100, MSG_QUEUE_DEPTH, EM4100MsgData_t);
	memPoolEM4100 = osPoolCreate(osPool(memPoolEM4100));
	osPoolDef(memPoolEM4100_RWE_Data, MSG_QUEUE_DEPTH, em4100TagData_t);
	memPoolEM4100_RWE_Data = osPoolCreate(osPool(memPoolEM4100_RWE_Data));

	// Создаем задачу
	osThreadDef(EM4100Thread, EM4100Thread, threadIDLEPriority, 0, THREAD_STACK_SIZE);
	threadEM4100 = osThreadCreate(osThread(EM4100Thread), NULL);
	if (threadEM4100 == NULL)
	{
		__debugError(0);
	}

}

/*
 * Задача ридера.
 */
void EM4100Thread(void const * argument)
{
	osEvent				evt;
	em4100TagData_t		*em4100RWEData, em4100Data;
	EM4100DataRec_t 	recData;
	osStatus   			status;
	EM4100MsgData_t*	msgData;
	EM4100EmulData_t	emulData;

	//Пока-что тормозим поток
	//osThreadSuspend(NULL);

	/**** Поток ****/
	for(;;)
	{
		evt = osMessageGet(msgQueueEM4100, osWaitForever);
		if (evt.status == osEventMessage)
		{
			msgData = (EM4100MsgData_t*)evt.value.p;

			switch (msgData->msgType){
			case EM4100_THREAD_MSG__RWE_DATA_RECEIVED:
				{
					em4100RWEData = (em4100TagData_t*)msgData->p;
					memcpy(em4100currentData.data, em4100RWEData->data, 5);
					em4100currentData.modulationMethod = em4100RWEData->modulationMethod;
					em4100currentData.bitrate = em4100RWEData->bitrate;
					// освобождаем выделенную память в пуле
					osPoolFree(memPoolEM4100_RWE_Data, em4100RWEData);

					CLI_PrintString("Detected tag!\r\n");

					CLI_PrintString("modulation: ");
					switch (em4100currentData.modulationMethod)
					{
					case (EM4100_MODULATION_MANCHESTER):
						{
							CLI_PrintString("Manchester\r\n");
							break;
						}
					default:
						break;
					}

					memset(strBuff, 0x00, sizeof(strBuff));
					sprintf(strBuff, "bit-rate: RF_%03d\r\n", em4100currentData.bitrate);
					CLI_PrintString(strBuff);
					memset(strBuff, 0x00, sizeof(strBuff));
					sprintf(strBuff, "code: %02X %02X %02X %02X %02X\r\n", em4100currentData.data[0], em4100currentData.data[1],
							em4100currentData.data[2], em4100currentData.data[3], em4100currentData.data[4]);
					CLI_PrintString(strBuff);
					CLI_PrintString("\r\n");

					osDelay(EM4100_READER_TIMEOUT);
					EM4100_RWE_Reader_ON();

					break;
				}
			case EM4100_THREAD_MSG__RWE_EMUL_DONE:
				{
					if (emulData.emulMode == EM4100_EMUL_MODE_IDLE)
					{
						break;
					}
					else if (emulData.emulMode == EM4100_EMUL_MODE_CURRENT)
					{
						//memcpy(em4100Data.data, em4100currentData.data, 5);
						//em4100Data.bitrate = em4100currentData.bitrate;
						//em4100Data.modulationMethod = em4100currentData.modulationMethod;
						EM4100_RWE_EmulatorRunTag(&em4100Data);
					}
					else if (emulData.emulMode == EM4100_EMUL_MODE_ALL)
					{
						if (emulData.currentRecID > emulData.lastRecID)
							emulData.currentRecID = 0;
						SPIFTabSystem_EM4100GetRec(emulData.currentRecID, &recData);
						em4100Data.bitrate = recData.bitrate;
						memcpy(em4100Data.data, recData.data, 5);

						EM4100_RWE_EmulatorRunTag(&em4100Data);
					}

					break;//EM4100_EMUL_MODE_IDLE
				}
			case EM4100_THREAD_MSG__CMD_READ_BEGIN:
				{
					CLI_PrintString("Waiting a tag..\r\n");
					EM4100_RWE_Reader_Init();
					EM4100_RWE_Reader_ON();
					break;
				}
			case EM4100_THREAD_MSG__CMD_READ_END:
				{
					CLI_PrintString("Stopping the 125K RF decoder..\r\n\r\n");
					EM4100_RWE_Reader_OFF();
					CLI_SendMsg_CLI_EN();
					break;
				}
			case EM4100_THREAD_MSG__CMD_WRITE_CURRENT:
				{
					CLI_PrintString("Writting the T55XX tag..\r\n\r\n");

					em4100TagData_t tagData;
					memcpy(tagData.data, em4100currentData.data, 5);
					tagData.bitrate = em4100currentData.bitrate;
					tagData.modulationMethod = em4100currentData.modulationMethod;

					EM4100_RWE_WritterInit(T5557_T5577);
					EM4100_RWE_WritterWriteTag(T5557_T5577, &tagData);

					CLI_PrintString("Writting done!\r\n\r\n");

					CLI_SendMsg_CLI_EN();
					break;
				}
			case EM4100_THREAD_MSG__CMD_EMUL_BEGIN:
			{

				emulData.emulMode = msgData->v1;
				if (emulData.emulMode == EM4100_EMUL_MODE_CURRENT)
				{
					EM4100_RWE_Emulator_Init();

					CLI_PrintString("Emulation is in progress. (press the 'enter' to stop)\r\n\r\n");

					memcpy(em4100Data.data, em4100currentData.data, 5);
					em4100Data.bitrate = em4100currentData.bitrate;
					em4100Data.modulationMethod = em4100currentData.modulationMethod;
					EM4100_RWE_EmulatorRunTag(&em4100Data);
				}
				else if (emulData.emulMode == EM4100_EMUL_MODE_ALL)
				{
					EM4100_RWE_Emulator_Init();

					if (SPIFTabSystem_EM4100GetCounter() == 0)
					{
						CLI_PrintString("Error: not found EM4100 records in database!\r\n\r\n");
						CLI_SendMsg_CLI_EN();
						break;
					}

					emulData.currentRecID = 0;
					emulData.lastRecID = SPIFTabSystem_EM4100GetLastID();
					SPIFTabSystem_EM4100GetRec(emulData.currentRecID, &recData);
					em4100Data.bitrate = recData.bitrate;
					memcpy(em4100Data.data, recData.data, 5);
					//em4100Data.modulationMethod = recData.modulationMethod;

					memset(strBuff, 0x00, sizeof(strBuff));
					sprintf(strBuff, "Total records count in database: %04d\r\n\r\n", emulData.lastRecID + 1);
					CLI_PrintString(strBuff);
					CLI_PrintString("Emulation is in progress. (press the 'enter' to stop)\r\n\r\n");

					EM4100_RWE_EmulatorRunTag(&em4100Data);
					emulData.currentRecID ++;
				}
				else
				{
					CLI_SendMsg_CLI_EN();
				}

				break;
			}
			case EM4100_THREAD_MSG__CMD_EMUL_END:
			{
				CLI_PrintString("Stopping the 125K RF encoder..\r\n\r\n");
				emulData.emulMode = EM4100_EMUL_MODE_IDLE;
				CLI_SendMsg_CLI_EN();
				break;
			}
			case EM4100_THREAD_MSG__CMD_SET_CURRENT:
			{
				em4100RWEData = (em4100TagData_t*)msgData->p;
				memcpy(em4100currentData.data, em4100RWEData->data, 5);
				em4100currentData.modulationMethod = em4100RWEData->modulationMethod;
				em4100currentData.bitrate = em4100RWEData->bitrate;
				CLI_PrintString("Current tag data modified.\r\n\r\n");
				CLI_SendMsg_CLI_EN();
				break;
			}
			default:
				{
					break;
				}
			}

		}

		osPoolFree(memPoolEM4100, msgData);
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


EM4100DataRec_t* EM4100ThreadGetCurrentData(void)
{
	return &em4100currentData;
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


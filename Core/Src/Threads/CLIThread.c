/*
 * CLIThread.c
 *
 *  Created on: 20 июн. 2022 г.
 *      Author: tochk
 */

#include "CLIThread.h"
#include "CLICommandList.h"


#define THREAD_STACK_SIZE		500
#define MSG_QUEUE_DEPTH			4



osThreadId			threadCLI;
osMessageQId		msgQueueCLI;
osPoolId			memPoolCLI;

static __IO uint32_t 	threadIDLEPriority; // приоритет потока в режиме ожидания




void CLIThreadInit(uint32_t priority)
{
	threadIDLEPriority = priority;
	// Создаем очередь
	osMessageQDef(msgQueueCLI, MSG_QUEUE_DEPTH, uint32_t);
	msgQueueCLI = osMessageCreate(osMessageQ(msgQueueCLI), NULL);
	// Создаем пул под данные очереди
	osPoolDef(memPoolCLI, MSG_QUEUE_DEPTH, CLIMsgData_t);
	memPoolCLI = osPoolCreate(osPool(memPoolCLI));
	// Создаем задачу
	osThreadDef(CLIThread, CLIThread, threadIDLEPriority, 0, THREAD_STACK_SIZE);
	threadCLI = osThreadCreate(osThread(CLIThread), NULL);
	if (threadCLI == NULL)
	{
		__debugError(0);
	}

	CLICommandList_Init();
	CLI_Init();
}


/*
 * Обработка нажатий тачпада и кнопок
 * Информация прилетает в очередь от ISR тачпада и кнопок
 */
void CLIThread(void const * argument)
{
	osEvent				evt;
	osStatus   			status;
	CLIMsgData_t*		msgData;


	/**** Поток ****/
	for(;;)
	{
		evt = osMessageGet(msgQueueCLI, osWaitForever);
		if (evt.status == osEventMessage)
		{
			msgData = (CLIMsgData_t*)evt.value.p;

			switch (msgData->msgType){
			case CLI_THREAD_MSG__CMD_RECV: // Пришли данные (команда с терминала)
				{
					uint8_t cliProcRes = CLI_ProcessCurrentLine();
					if (cliProcRes == CLI_STATUS_COMMAND_CLI_EN)
					{
						CLICmdList_t* currentCmd = CLI_GetCurrentCmd();
						// если существует текущая активная команда
						if (currentCmd != NULL)
						{
							// если у нее есть функция остановки текущих ее процессов
							if (currentCmd->KillFunc != NULL)
							{
								// передаем этой ф-ии управление и запрещаем работу с CLI
								// (по завершении своих дел, ф-я должна сама разрешить работу CLI,
								//  посредством создания сообщения CLI_THREAD_MSG__CLI_EN этому потоку)
								CLI_SetCurrentCmd(currentCmd->KillFunc());
								break;
							}
						}
						CLI_AllowCommands();
					}
					else if (cliProcRes == CLI_STATUS_COMMAND_FUNCTION_UNDEF)
					{
						CLI_AllowCommands();
					}
					else if (cliProcRes == CLI_STATUS_COMMAND_NOT_FOUND)
					{
						CLI_AllowCommands();
					}
					break;
				}
			case CLI_THREAD_MSG__CLI_EN:
				{
					CLI_AllowCommands();
					break;
				}
			case CLI_THREAD_MSG__PRINT:
				{
					break;
				}
			default: // (CLI_THREAD_MSG__CLI_EN)
				{
					break;
				}
			}


			// освобождаем память в memory-pool после прочтения данных
			osPoolFree(memPoolCLI, msgData);

		}

		/* Отдаем управление другим задачам - повышая их приоритет.
		 * Далее задача сама должна понизить себе приоритет, отработав цикл.*/
		/*
	    status = osThreadSetPriority(EM4100Thread_GetThreadId(), osThreadGetPriority(osThreadGetId()) + 1);
	    if (status != osOK)
	    {
	    	__debugError(0);
	    }
	    */

		osThreadYield();
	}
}




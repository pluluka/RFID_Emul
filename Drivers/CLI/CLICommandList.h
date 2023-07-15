/*
 * CLICommandList.h
 *
 *  Created on: 28 июн. 2022 г.
 *      Author: plu
 *
 * Здесь создается структура командного интерфейса.
 * Команды задаются в соответствующих массивах (см. пример команд help, exit)
 * Функция-обработчик команды должна возвращать указатель на командный лист,
 * данный лист будет установлен в качестве текущего. Кроме того, функция-обработчик
 * должна разрешить дальнейший ввод команд в CLI (используйте ф-ю CLI_SendMsg_CLI_EN(void)),
 * поскольку при передаче ей управления ввод команд запрещается.
 */

#ifndef CLI_CLICOMMANDLIST_H_
#define CLI_CLICOMMANDLIST_H_


#include "EM4100Thread.h"
#include "OneWireThread.h"




const char cmd_def__help[] =			"help";
const char help_def__help[] =			"[help]\r\n"
	    								" * help:  show help\r\n"
	    								"\r\n";
const char cmd_def__exit[] =			"exit";
const char help_def__exit[] =			"[exit]\r\n"
	    								" * exit:  back prevois menu\r\n"
	    								"\r\n";
//- Command list MAIN ----------------------------------------------------------/
const char cmd_main__em4100[] =			"em4100";
const char help_main__em4100[] =		"[em4100]\r\n"
	    								" * em4100:  enter to em4100 protocol tools\r\n"
	    								"\r\n";
const char cmd_main__onewire[] =		"onewire";
const char help_main__onewire[] =		"[onewire]\r\n"
	    								" * onewire:  enter to onewire protocol tools\r\n"
	    								"\r\n";

//- Command list EM4100 --------------------------------------------------------/
const char cmd_em4100__current[] =		"current";
const char help_em4100__current[] =		"[current]\r\n"
	    								" * current:  show current tag data\r\n"
	    								"\r\n";
const char cmd_em4100__detect[] =			"detect";
const char help_em4100__detect[] =		"[detect]\r\n"
	    								" * detect:  detect the em4100 compatable tag\r\n"
	    								"\r\n";
const char cmd_em4100__clone[] =		"clone";
const char help_em4100__clone[] =		"[clone]\r\n"
	    								" * clone [BLANK_CHIP_TYPE] :  write tag data a the blank tag\r\n"
										"   BLANK_CHIP_TYPE: blank tag mc type, now avaible types:\r\n"
										"     T55XX   - atmel T5557, T5577\r\n";
const char cmd_em4100__emulation[] =	"emulation";
const char help_em4100__emulation[] =	"[emulation]\r\n"
			    						" * emulation [MODE] :  run emulation tag\r\n"
										"   MODE: 'emulation mode:\r\n"
										"     all  - all records into database\r\n"
										"     current  - current record\r\n"
	    								"\r\n";
const char cmd_em4100__database[] =		"database";
const char help_em4100__database[] =	"[database]\r\n"
	    								" * database [ACTION] [P1] [P2] [P3]: database operations\r\n"
										"   ACTION: action types:\r\n"
										"     show  	- show all records\r\n"
										"     add   	- add rec into database:\r\n"
										"         P1 - tag code (5 byte hex value XX-XX-XX-XX-XX)\r\n"
										"         P2 - bitrate (8,16,32,40,50,64,100,128), default - 64\r\n"
										"         P3 - name (max 7 letters)\r\n"
										"         * if P1 P2 P3 not define, will added current tag\r\n"
										"     current	- set record into DB as a current tag data:\r\n"
										"         P1 - record ID\r\n"
	    								"\r\n";
//- Command list OneWire --------------------------------------------------------/
const char cmd_onewire__detect[] =		"detect";
const char help_onewire__detect[] =		"[detect]\r\n"
	    								" * detect:  detect the 1-Wire compatable MC\r\n"
	    								"\r\n";
const char cmd_onewire__current[] =		"current";
const char help_onewire__current[] =	"[current] [ACTION] [P1] [P2]: current ROM operations\r\n"
										"   ACTION: action types:\r\n"
										"     set  	- set current ROM data:\r\n"
										"         P1 - ROM code (8 byte hex value XX-XX-XX-XX-XX-XX-XX-XX)\r\n"
										"         P2 - name (max 7 letters)\r\n"
										"   * if ACTION not define, will shows current ROM\r\n"
	    								"\r\n";
const char cmd_onewire__clone[] =		"clone";
const char help_onewire__clone[] =		"[clone]\r\n"
	    								" * clone [BLANK_CHIP_TYPE] :  write tag data a the blank tag\r\n"
										"   BLANK_CHIP_TYPE: blank tag mc type, now avaible types:\r\n"
										"     RW1990   - \r\n";
const char cmd_onewire__emulation[] =	"emulation";
const char help_onewire__emulation[] =	"[emulation]\r\n"
			    						" * emulation [MODE] :  run emulation ROM\r\n"
										"   MODE: 'emulation mode:\r\n"
										"     all  - all records into database\r\n"
										"     current  - current ROM\r\n";



//- Функции-обработчики ----------------------------------------------------------/
static void* def__exit(uint8_t argc, char *argv[], void* cmdList, uint32_t cmdNum);
static void* def__help(uint8_t argc, char *argv[], void* cmdList, uint32_t cmdNum);

static void* main__em4100(uint8_t argc, char *argv[], void* cmdList, uint32_t cmdNum);
static void* main__onewire(uint8_t argc, char *argv[], void* cmdList, uint32_t cmdNum);

static void* em4100__current(uint8_t argc, char *argv[], void* cmdList, uint32_t cmdNum);
static void* em4100__detect(uint8_t argc, char *argv[], void* cmdList, uint32_t cmdNum);
static void* em4100__clone(uint8_t argc, char *argv[], void* cmdList, uint32_t cmdNum);
static void* em4100__emulation(uint8_t argc, char *argv[], void* cmdList, uint32_t cmdNum);
static void* em4100__database(uint8_t argc, char *argv[], void* cmdList, uint32_t cmdNum);
static void* em4100__exit(uint8_t argc, char *argv[], void* cmdList, uint32_t cmdNum);

static void* onewire__detect(uint8_t argc, char *argv[], void* cmdList, uint32_t cmdNum);
static void* onewire__current(uint8_t argc, char *argv[], void* cmdList, uint32_t cmdNum);
static void* onewire__clone(uint8_t argc, char *argv[], void* cmdList, uint32_t cmdNum);
static void* onewire__emulation(uint8_t argc, char *argv[], void* cmdList, uint32_t cmdNum);

//- Функции-терминаторы ----------------------------------------------------------/
static void* em4100__detect_killFunc(void);
static void* em4100__emulation_killFunc(void);
static void* onewire__detect_killFunc(void);
static void* onewire__emulation_killFunc(void);


//- Командные листы  ----------------------------------------------------------/
//	Последняя запись листа представляет собой пустую запись,
//	в случае если лист является листом вложенного уровня
//	в этой записи в поле SubList указывается адрес листа уровня выше,
//	а в поле help указывается его номер в листе уровня выше (начиная с 0)

CLICmdList_t cli_onewireList[] = {
		{cmd_onewire__current, onewire__current, NULL, help_onewire__current, NULL},
		{cmd_onewire__detect, onewire__detect, onewire__detect_killFunc, help_onewire__detect, NULL},
		{cmd_onewire__clone, onewire__clone, NULL, help_onewire__clone, NULL},
		{cmd_onewire__emulation, onewire__emulation, onewire__emulation_killFunc, help_onewire__emulation, NULL},
		{cmd_def__exit, def__exit, NULL, help_def__exit, NULL},			// Exit function
		{cmd_def__help, def__help, NULL, help_def__help, NULL},			// Help function
		{NULL, NULL, NULL, NULL}
};

CLICmdList_t cli_em4100List[] = {
		{cmd_em4100__current, em4100__current, NULL, help_em4100__current, NULL},
		{cmd_em4100__detect, em4100__detect, em4100__detect_killFunc, help_em4100__detect, NULL},
		{cmd_em4100__clone, em4100__clone, NULL, help_em4100__clone, NULL},
		{cmd_em4100__emulation, em4100__emulation, em4100__emulation_killFunc, help_em4100__emulation, NULL},
		{cmd_em4100__database, em4100__database, NULL, help_em4100__database, NULL},
		{cmd_def__exit, def__exit, NULL, help_def__exit, NULL},			// Exit function
		{cmd_def__help, def__help, NULL, help_def__help, NULL},			// Help function
		{NULL, NULL, NULL, NULL}
};

CLICmdList_t cli_mainList[] = {
		{cmd_main__em4100, main__em4100, NULL, help_main__em4100, cli_em4100List},
		{cmd_main__onewire, main__onewire, NULL, help_main__onewire, cli_onewireList},
		{cmd_def__exit, def__exit, NULL, help_def__exit, NULL},			// Exit function
		{cmd_def__help, def__help, NULL, help_def__help, NULL},			// Help function
		{NULL, NULL, NULL, NULL}
};


extern osMessageQId		msgQueueEM4100;
extern osPoolId			memPoolEM4100;
extern osPoolId			memPoolEM4100_RWE_Data;
extern osThreadId		threadCLI;
extern osMessageQId		msgQueueCLI;
extern osPoolId			memPoolCLI;
extern osThreadId		threadOneWire;
extern osMessageQId		msgQueueOneWire;
extern osPoolId			memPoolOneWire;

char CLIStringBuffer[100];

__STATIC_INLINE CLI_SendMsg_CLI_EN(void);

/*
 * Инициализация.
 */
void CLICommandList_Init(void)
{
	// Указание владельцев вложеных командных листов
	CLI_SetListOwnerCmd(cli_em4100List, cli_mainList, 0);
	CLI_SetListOwnerCmd(cli_onewireList, cli_mainList, 1);
	// Установка текущего листа
	CLI_SetCurrentCmdList(cli_mainList);
}


static void* def__exit(uint8_t argc, char *argv[], void* cmdList, uint32_t cmdNum)
{
	// перемещаемся к последней записи листа
	CLICmdList_t* list = (CLICmdList_t*)cmdList;//CLI_GetCmdListLength((CLICmdList_t*)cmdList)
	list += CLI_GetCmdListLength((CLICmdList_t*)cmdList);
	// смотрим, если указан владелец, возвращаем его указатель
	if (list->SubList != NULL)
	{
		CLI_SendMsg_CLI_EN();
		return list->SubList;
	}
	CLI_SendMsg_CLI_EN();
	return cmdList;
}

static void* def__help(uint8_t argc, char *argv[], void* cmdList, uint32_t cmdNum)
{
	CLICmdList_t* list = (CLICmdList_t*)cmdList;

	CLI_PrintString("\r\n");
	//	Если параметр не указан, выводим справку по всем
	//	командам текущего листа
	if (argc == 0)
	{
		list -= cmdNum;
		for(uint32_t i = 0; i <= cmdNum; i ++)
		{
			if (list->Help != NULL)
				CLI_PrintString(list->Help);
			list ++;
		}
		CLI_SendMsg_CLI_EN();
		return cmdList;
	}
	//while ()
	CLI_SendMsg_CLI_EN();
	return cmdList;
}




static void* main__em4100(uint8_t argc, char *argv[], void* cmdList, uint32_t cmdNum)
{
	CLICmdList_t* list = (CLICmdList_t*)cmdList;
	//list += cmdNum;
	//	Если параметр не указан
	if (argc == 0)
	{
		CLI_SendMsg_CLI_EN();
		return	(void*)list->SubList;
	}
	CLI_SendMsg_CLI_EN();
	return (void*)list;
}

static void* main__onewire(uint8_t argc, char *argv[], void* cmdList, uint32_t cmdNum)
{
	CLICmdList_t* list = (CLICmdList_t*)cmdList;
	//list += cmdNum;
	//	Если параметр не указан
	if (argc == 0)
	{
		CLI_SendMsg_CLI_EN();
		return	(void*)list->SubList;
	}
	CLI_SendMsg_CLI_EN();
	return (void*)list;
}


static void* em4100__current(uint8_t argc, char *argv[], void* cmdList, uint32_t cmdNum)
{
	CLI_PrintString("\r\n");
	EM4100DataRec_t* tagData = EM4100ThreadGetCurrentData();
	//CLI_PrintString("\r\nCurrent tag data: ");
	memset(CLIStringBuffer, 0x00, sizeof(CLIStringBuffer));
	sprintf(CLIStringBuffer, "Current tag data: %02X %02X %02X %02X %02X\r\n",
			tagData->data[0], tagData->data[1], tagData->data[2], tagData->data[3],
			tagData->data[4]);
	CLI_PrintString(CLIStringBuffer);
	CLI_SendMsg_CLI_EN();
	return cmdList;
}

static void* em4100__detect(uint8_t argc, char *argv[], void* cmdList, uint32_t cmdNum)
{
	EM4100MsgData_t		*em4100MsgData;
	em4100MsgData = (EM4100MsgData_t *)osPoolAlloc(memPoolEM4100);
	if (em4100MsgData != NULL)
	{
		em4100MsgData->msgType = EM4100_THREAD_MSG__CMD_READ_BEGIN;
		em4100MsgData->p = NULL;
		if (osMessagePut(msgQueueEM4100, (uint32_t)(em4100MsgData), 0) != osOK)
		{
			__debugError(0);
			osPoolFree(memPoolEM4100, em4100MsgData);
			return;
		}
	}

	return cmdList;
}

static void* em4100__clone(uint8_t argc, char *argv[], void* cmdList, uint32_t cmdNum)
{
	// Проверяем наличие первого параметра
	if ((argc) && (argv[0] != NULL))
	{
		if (strcmp(argv[0], "T55XX") == 0)
		{	// отправляем соответствующее сообщение потоку ридера EM4100
			EM4100MsgData_t		*em4100MsgData;
			em4100MsgData = (EM4100MsgData_t *)osPoolAlloc(memPoolEM4100);
			if (em4100MsgData != NULL)
			{
				em4100MsgData->msgType = EM4100_THREAD_MSG__CMD_WRITE_CURRENT;
				em4100MsgData->p = NULL;
				if (osMessagePut(msgQueueEM4100, (uint32_t)(em4100MsgData), 0) != osOK)
				{
					__debugError(0);
					osPoolFree(memPoolEM4100, em4100MsgData);
					//CLI_SendMsg_CLI_EN();
					return;
				}
			}
		}
	}
	else
	{
		CLI_SendMsg_CLI_EN();
	}

	//CLI_SendMsg_CLI_EN();
	return cmdList;
}

static void* em4100__emulation(uint8_t argc, char *argv[], void* cmdList, uint32_t cmdNum)
{
	// Проверяем наличие первого параметра
	if ((argc) && (argv[0] != NULL))
	{
		// ########## all - эмулировать все метки из бд ###########
		if (strcmp(argv[0], "all") == 0)
		{	// отправляем соответствующее сообщение потоку ридера EM4100
			EM4100MsgData_t		*em4100MsgData;
			em4100MsgData = (EM4100MsgData_t *)osPoolAlloc(memPoolEM4100);
			if (em4100MsgData != NULL)
			{
				em4100MsgData->msgType = EM4100_THREAD_MSG__CMD_EMUL_BEGIN;
				em4100MsgData->v1 = EM4100_EMUL_MODE_ALL;
				if (osMessagePut(msgQueueEM4100, (uint32_t)(em4100MsgData), 0) != osOK)
				{
					osPoolFree(memPoolEM4100, em4100MsgData);
					CLI_SendMsg_CLI_EN();
					__debugError(0);
					return;
				}
			}
		}

		// ########## current - эмулировать текущую метку ###########
		if (strcmp(argv[0], "current") == 0)
		{	// отправляем соответствующее сообщение потоку ридера EM4100
			EM4100MsgData_t		*em4100MsgData;
			em4100MsgData = (EM4100MsgData_t *)osPoolAlloc(memPoolEM4100);
			if (em4100MsgData != NULL)
			{
				em4100MsgData->msgType = EM4100_THREAD_MSG__CMD_EMUL_BEGIN;
				em4100MsgData->v1 = EM4100_EMUL_MODE_CURRENT;
				if (osMessagePut(msgQueueEM4100, (uint32_t)(em4100MsgData), 0) != osOK)
				{
					osPoolFree(memPoolEM4100, em4100MsgData);
					CLI_SendMsg_CLI_EN();
					__debugError(0);
					return;
				}
			}
		}

		//########## параметр не распознан ##########
		else
		{
			CLI_SendMsg_CLI_EN();
		}
	}
	else
	{
		CLI_SendMsg_CLI_EN();
	}

	return cmdList;
}

static void* em4100__database(uint8_t argc, char *argv[], void* cmdList, uint32_t cmdNum)
{
	EM4100DataRec_t data = {0}, *pdata;

	// Проверяем наличие первого параметра
	if ((argc) && (argv[0] != NULL))
	{
		CLI_PrintString("\r\n");

		//########## show - вывести все записи БД ###########
		if (strcmp(argv[0], "show") == 0)
		{
			CLI_PrintString(" ID      DATA        BitRate Name\r\n");
			uint32_t lastID = SPIFTabSystem_EM4100GetLastID();
			for (uint32_t id = 0; id <= lastID; id ++)
			{
				SPIFTabSystem_EM4100GetRec(id, &data);
				memset(CLIStringBuffer, 0x00, sizeof(CLIStringBuffer));
				sprintf(CLIStringBuffer, "%04d  %02X-%02X-%02X-%02X-%02X   %03d    %s\r\n",
						data.id, data.data[0], data.data[1], data.data[2], data.data[3],
						data.data[4], data.bitrate, data.name);
				CLI_PrintString(CLIStringBuffer);
			}
			CLI_PrintString("\r\n");
			CLI_SendMsg_CLI_EN();
		}

		//########## add - внести запись в БД ##########
		else if (strcmp(argv[0], "add") == 0)
		{
			// если указаны дополнительные параметры
			if ((argc > 1) && (argv[1] != NULL))
			{
				char *p1 = argv[1];
				// меняем тире на пробелы
				//p1[2] = ' '; p1[5] = ' '; p1[8] = ' '; p1[11] = ' ';
				// проверяем параметр P1 (5-байтный код метки XX-XX-XX-XX-XX)
				for(uint8_t i = 0; i < 5; i ++)
				{
					int pb = strtol(p1, NULL, 16);
					if ((pb < 0) || (pb > 0xFF))
					{
						CLI_PrintString("Error in parametr P1!\r\n\r\n");
						CLI_SendMsg_CLI_EN();
						return cmdList;
					}
					data.data[i] = (uint16_t)pb;
					p1 += 3;
				}

				// проверяем наличие параметра P2
				if (argv[2] != NULL)
				{
					char *p2 = argv[2];
					int pb = atoi(p2);
					if ((pb < 0) || (pb > 0xFF))
					{
						CLI_PrintString("Error in parametr P2!\r\n\r\n");
						CLI_SendMsg_CLI_EN();
						return cmdList;
					}
					data.bitrate = (uint16_t)pb;
				}
				else
				{  // если параметр не указан - заносим дефолтное значение
					data.bitrate = 64;
				}

				// проверяем наличие параметра P3
				if (argv[3] != NULL)
				{
					uint32_t len = 0;
					uint32_t recNameSize = sizeof(((EM4100DataRec_t*)(0))->name);
					char *p3 = argv[3];
					while (*p3)
					{
						if (len > recNameSize - 2)
							break;
						data.name[len] = *p3;
						len ++;
						p3 ++;
					}
				}

				SPIFTabSystem_EM4100Add(&data);
				memset(CLIStringBuffer, 0x00, sizeof(CLIStringBuffer));
				sprintf(CLIStringBuffer, "Tag %02X %02X %02X %02X %02X added to database.\r\n",
						data.data[0], data.data[1], data.data[2], data.data[3], data.data[4]);
				CLI_PrintString(CLIStringBuffer);
				CLI_PrintString("\r\n");
			}

			// если дополнительных параметров нет, заносим данные текущей метки в БД
			else
			{
				pdata = EM4100ThreadGetCurrentData();
				memcpy(&data, pdata, sizeof(EM4100DataRec_t));
				SPIFTabSystem_EM4100Add(&data);
				memset(CLIStringBuffer, 0x00, sizeof(CLIStringBuffer));
				sprintf(CLIStringBuffer, "Tag %02X %02X %02X %02X %02X added to database.\r\n",
						data.data[0], data.data[1], data.data[2], data.data[3], data.data[4]);
				CLI_PrintString(CLIStringBuffer);
				CLI_PrintString("\r\n");
			}

			CLI_SendMsg_CLI_EN();
		}

		//########## current установить запись из БД как текущую ##########
		else if (strcmp(argv[0], "current") == 0)
		{
			// если указаны дополнительные параметры (данные метки вносятся вручную)
			if ((argc > 1) && (argv[1] != NULL))
			{
				char *p1 = argv[1];
				int pb = strtol(p1, NULL, 10);
				if ((pb < 0) || (pb > SPIFTabSystem_EM4100GetLastID()))
				{
					CLI_PrintString("Error in parametr P1!\r\n\r\n");
					CLI_SendMsg_CLI_EN();
					return cmdList;
				}

				SPIFTabSystem_EM4100GetRec(pb, &data);

				em4100TagData_t		*em4100TadData;
				EM4100MsgData_t		*em4100MsgData;

				em4100TadData = (em4100TagData_t *)osPoolAlloc(memPoolEM4100_RWE_Data);
				if (em4100TadData != NULL)
				{
					em4100TadData->bitrate = data.bitrate;
					em4100TadData->modulationMethod = data.modulationMethod;
					memcpy(em4100TadData->data, data.data, 5);

					em4100MsgData = (EM4100MsgData_t *)osPoolAlloc(memPoolEM4100);
					if (em4100MsgData != NULL)
					{
						em4100MsgData->msgType = EM4100_THREAD_MSG__CMD_SET_CURRENT;
						em4100MsgData->p = em4100TadData;
						if (osMessagePut(msgQueueEM4100, (uint32_t)(em4100MsgData), 0) != osOK)
						{
							osPoolFree(memPoolEM4100_RWE_Data, em4100TadData);
							osPoolFree(memPoolEM4100, em4100MsgData);
							__debugError(0);
							CLI_SendMsg_CLI_EN();
							return;
						}
					}
					else
					{
						CLI_SendMsg_CLI_EN();
					}
				}
				else
				{
					CLI_SendMsg_CLI_EN();
				}

			}
		}

		//########## параметр не распознан ##########
		else
		{
			CLI_SendMsg_CLI_EN();
		}

	}

	// параметр не указан
	else
	{
		CLI_SendMsg_CLI_EN();
	}

	return cmdList;
}

static void* onewire__detect(uint8_t argc, char *argv[], void* cmdList, uint32_t cmdNum)
{

	OneWireMsgData_t		*oneWireMsgData;
	oneWireMsgData = (OneWireMsgData_t *)osPoolAlloc(memPoolOneWire);
	if (oneWireMsgData != NULL)
	{
		oneWireMsgData->msgType = OW_THREAD_MSG__CMD_READ_ROM;
		oneWireMsgData->p = NULL;
		if (osMessagePut(msgQueueOneWire, (uint32_t)(oneWireMsgData), 0) != osOK)
		{
			__debugError(0);
			osPoolFree(memPoolOneWire, oneWireMsgData);
			return;
		}
	}

	return cmdList;
}

static void* onewire__current(uint8_t argc, char *argv[], void* cmdList, uint32_t cmdNum)
{
	OneWireDataRec_t data = {0}, *pdata;
	uint32_t recNameSize;

	// Проверяем наличие первого параметра
	if ((argc) && (argv[0] != NULL))
	{
		//########## set - ввести данные текущей ROM вручную ###########
		if (strcmp(argv[0], "set") == 0)
		{
			// если указаны дополнительные параметры
			if ((argc > 1) && (argv[1] != NULL))
			{
				pdata = OneWireThreadGetCurrentData();

				char *p1 = argv[1];
				// проверяем параметр P1 (8-байтный ROM XX-XX-XX-XX-XX-XX-XX-XX)
				for(uint8_t i = 0; i < 8; i ++)
				{
					int pb = strtol(p1, NULL, 16);
					if ((pb < 0) || (pb > 0xFF))
					{
						CLI_PrintString("Error in parametr P1!\r\n\r\n");
						CLI_SendMsg_CLI_EN();
						return cmdList;
					}
					data.romData[i] = (uint16_t)pb;
					p1 += 3;
				}
				memcpy(pdata->romData, data.romData, 8);

				// проверяем наличие параметра P2
				if (argv[2] != NULL)
				{
					uint32_t len = 0;
					recNameSize = sizeof(((OneWireDataRec_t*)(0))->name);
					char *p3 = argv[3];
					while (*p3)
					{
						if (len > recNameSize - 2)
							break;
						data.name[len] = *p3;
						len ++;
						p3 ++;
					}
					memcpy(pdata->name, data.name, recNameSize);
				}

			}
		}

		CLI_SendMsg_CLI_EN();
	}
	else
	{
		CLI_PrintString("\r\n");
		OneWireDataRec_t *tagData = OneWireThreadGetCurrentData();
		memset(CLIStringBuffer, 0x00, sizeof(CLIStringBuffer));
		sprintf(CLIStringBuffer, "Current ROM data: %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
				tagData->romData[0], tagData->romData[1], tagData->romData[2], tagData->romData[3],
				tagData->romData[4], tagData->romData[5], tagData->romData[6], tagData->romData[7]);
		CLI_PrintString(CLIStringBuffer);
		CLI_SendMsg_CLI_EN();
	}

	return cmdList;
}

static void* onewire__clone(uint8_t argc, char *argv[], void* cmdList, uint32_t cmdNum)
{
	// Проверяем наличие первого параметра
	if ((argc) && (argv[0] != NULL))
	{
		if (strcmp(argv[0], "RW1990") == 0)
		{	// отправляем соответствующее сообщение потоку
			OneWireMsgData_t		*oneWireMsgData;
			oneWireMsgData = (OneWireMsgData_t *)osPoolAlloc(memPoolOneWire);
			if (oneWireMsgData != NULL)
			{
				oneWireMsgData->msgType = OW_THREAD_MSG__CMD_WRITE_ROM_RW1990;
				oneWireMsgData->p = NULL;
				if (osMessagePut(msgQueueOneWire, (uint32_t)(oneWireMsgData), 0) != osOK)
				{
					__debugError(0);
					osPoolFree(memPoolOneWire, oneWireMsgData);
					return;
				}
			}
		}
	}
	else
	{
		CLI_SendMsg_CLI_EN();
	}

	//CLI_SendMsg_CLI_EN();
	return cmdList;
}

static void* onewire__emulation(uint8_t argc, char *argv[], void* cmdList, uint32_t cmdNum)
{
	// Проверяем наличие первого параметра
	if ((argc) && (argv[0] != NULL))
	{
		// ########## all - эмулировать все ROM из бд ###########
		if (strcmp(argv[0], "current") == 0)
		{
			OneWireMsgData_t		*oneWireMsgData;
			oneWireMsgData = (OneWireMsgData_t *)osPoolAlloc(memPoolOneWire);
			if (oneWireMsgData != NULL)
			{
				oneWireMsgData->msgType = OW_THREAD_MSG__CMD_EMUL_CURRENT;
				oneWireMsgData->p = NULL;
				if (osMessagePut(msgQueueOneWire, (uint32_t)(oneWireMsgData), 0) != osOK)
				{
					__debugError(0);
					osPoolFree(memPoolOneWire, oneWireMsgData);
					return;
				}
			}
		}

		// ########## current - эмулировать текущую ROM ###########
		else if (strcmp(argv[0], "all") == 0)
		{

		}

		//########## параметр не распознан ##########
		else
		{
			CLI_SendMsg_CLI_EN();
		}
	}
	else
	{
		CLI_SendMsg_CLI_EN();
	}

	return cmdList;
}



static void* em4100__detect_killFunc(void)
{
	EM4100MsgData_t		*em4100MsgData;
	em4100MsgData = (EM4100MsgData_t *)osPoolAlloc(memPoolEM4100);
	if (em4100MsgData != NULL)
	{
		em4100MsgData->msgType = EM4100_THREAD_MSG__CMD_READ_END;
		em4100MsgData->p = NULL;
		if (osMessagePut(msgQueueEM4100, (uint32_t)(em4100MsgData), 0) != osOK)
		{
			__debugError(0);
			osPoolFree(memPoolEM4100, em4100MsgData);
			return NULL;
		}
	}

	return NULL;
}

static void* em4100__emulation_killFunc(void)
{
	EM4100MsgData_t		*em4100MsgData;
	em4100MsgData = (EM4100MsgData_t *)osPoolAlloc(memPoolEM4100);
	if (em4100MsgData != NULL)
	{
		em4100MsgData->msgType = EM4100_THREAD_MSG__CMD_EMUL_END;
		em4100MsgData->p = NULL;
		if (osMessagePut(msgQueueEM4100, (uint32_t)(em4100MsgData), 0) != osOK)
		{
			__debugError(0);
			osPoolFree(memPoolEM4100, em4100MsgData);
			return NULL;
		}
	}

	return NULL;
}

static void* onewire__detect_killFunc(void)
{
	osEvent evt;

	// очищаем очередь сообщений
	do{
		evt = osMessageGet(msgQueueOneWire, 0);
		if (evt.status == osEventMessage)
		{
			osPoolFree(memPoolOneWire, (OneWireMsgData_t*)evt.value.p);
		}
	} while(evt.status != osOK);

	OneWireMsgData_t		*oneWireMsgData;
	oneWireMsgData = (OneWireMsgData_t *)osPoolAlloc(memPoolOneWire);
	if (oneWireMsgData != NULL)
	{
		oneWireMsgData->msgType = OW_THREAD_MSG__CMD_CLI_EN;
		oneWireMsgData->p = NULL;
		if (osMessagePut(msgQueueOneWire, (uint32_t)(oneWireMsgData), 0) != osOK)
		{
			__debugError(0);
			osPoolFree(memPoolOneWire, oneWireMsgData);
			return;
		}
	}

	return NULL;
}

static void* onewire__emulation_killFunc(void)
{
	osEvent evt;

	// очищаем очередь сообщений
	do{
		evt = osMessageGet(msgQueueOneWire, 0);
		if (evt.status == osEventMessage)
		{
			osPoolFree(memPoolOneWire, (OneWireMsgData_t*)evt.value.p);
		}
	} while(evt.status != osOK);

	OneWireMsgData_t		*oneWireMsgData;
	oneWireMsgData = (OneWireMsgData_t *)osPoolAlloc(memPoolOneWire);
	if (oneWireMsgData != NULL)
	{
		oneWireMsgData->msgType = OW_THREAD_MSG__CMD_CLI_EN;
		oneWireMsgData->p = NULL;
		if (osMessagePut(msgQueueOneWire, (uint32_t)(oneWireMsgData), 0) != osOK)
		{
			__debugError(0);
			osPoolFree(memPoolOneWire, oneWireMsgData);
			return;
		}
	}

	return NULL;
}



/*
 * Создает сообщение потоку CLI разрешить ввод команд
 */
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



#endif /* CLI_CLICOMMANDLIST_H_ */

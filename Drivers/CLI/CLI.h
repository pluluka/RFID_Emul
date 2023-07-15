/*
 * CLI.h
 *
 *  Created on: 14 июн. 2022 г.
 *      Author: tochk
 *
 *      Драйвер командной строки для МК STM32
 *
 */

#ifndef CLI_CLI_H_
#define CLI_CLI_H_


#include "stddef.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "USART.h"


#define CLI_USART					USART1
#define CLI_USART_IRQHandler 		USART1_IRQHandler	//псевдоним обработчика прерывания
#define USART_ISR_PRIORITY			6					//приоритет обработчика прерывания
#define USART_BAUDRATE				9600U


/* Коды ошибок/статусов */
#define CLI_STATUS_OK                 		0x00
#define CLI_STATUS_ERROR              		0x01
#define CLI_STATUS_TOKEN_OVERAGE      		0x02
#define CLI_STATUS_EMPTY_LINE         		0x03
#define CLI_STATUS_BUFF_OUT_OF_RANGE  		0x04
#define CLI_STATUS_LINE_NOT_COMPARE	  		0x05
#define CLI_STATUS_COMMAND_NOT_FOUND		0x06
#define CLI_STATUS_COMMAND_FUNCTION_UNDEF	0xA0
#define CLI_STATUS_COMMAND_GO_TO_SUBLIST	0xA1
#define CLI_STATUS_COMMAND_CLI_EN			0xA2
#define CLI_STATUS_COMMAND_PROCESSED		0xA3


/* Структура содержащая указатели на данные команды.
 * Массив данных структур образует лист команд		*/
#pragma pack(push, 1)
typedef struct{
	char* Command;						// команда
										// указатель на функцию-обработчик команды
	void* (*Func)(	uint8_t argc,			// количество передаваемых параметров команды
					char *argv[],			// массив с указателями на параметры
					void* cmdList,			// указатель на лист содержащий данную команду
					uint32_t cmdNum);		// номер команды в листе
	void* (*KillFunc)(void);			// указатель на функцию-терминатор процессов команды
										// 	   (необходима для корректного завершения процессов команды)
	char* Help;							// текст справки по команде
	void* SubList;						// указатель на лист команд верхнего уровня
}	CLICmdList_t;
#pragma pack(pop)


void CLI_Init(void);
void CLI_Enable(void);
void CLI_Disable(void);
void CLI_SetHostName(char *name);
void CLI_AllowCommands(void);
uint8_t CLI_ProcessCurrentLine(void);
void CLI_PrintString(char* string);
void CLI_PrintChar(char ch);
void CLI_SetCurrentCmdList(CLICmdList_t* cmdList);
int CLI_GetCmdListLength(CLICmdList_t* list);
void CLI_SetListOwnerCmd(CLICmdList_t* list, CLICmdList_t* ownerCmdList, uint32_t ownerCmdNum);
CLICmdList_t* CLI_GetCurrentCmd(void);
void CLI_SetCurrentCmd(CLICmdList_t* cmd);


#endif /* CLI_CLI_H_ */

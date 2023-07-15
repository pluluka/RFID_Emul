/*
 * Console.h
 *
 *  Created on: May 26, 2020
 *      Author: tochk
 */

#ifndef USART_CONSOLE_H_
#define USART_CONSOLE_H_


#include "USART.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>



/* Defines -------------------------------------------------------------------*/
#define RX_BUFF_SIZE              		256                //размер приемного буфера

#define CONSOLE_USART__IRQHandler 		USART1_IRQHandler  //псевдоним обработчика прерывания
#define USART_ISR_PRIORITY				6	//приоритет обработчика прерывания

#define TOKEN_MAX_SIZE                  12   //макс размер лексемы (плюс нулевой символ)
#define TOKEN_MAX_NUM                   6    //макс количество лексем
#define TOKEN_DELIMITERS_MAX_NUM        5    //макс количество разделителей
#define HOSTNAME_MAX_LENGTH             12   //макс длина имени хоста
#define CONFIG_LEVEL_STRING_MAX_LENGTH  20   //макс длина строки

// Коды ошибок
#define STATUS_OK                 0x00
#define STATUS_ERROR              0x01
#define STATUS_TOKEN_OVERAGE      0x02
#define STATUS_EMPTY_LINE         0x03
#define STATUS_BUFF_OUT_OF_RANGE  0x04

#ifndef __TYPEDEF_BOOL
#define __TYPEDEF_BOOL
typedef enum
{
  FALSE = 0U,
  TRUE =  1U
} bool;
#endif

typedef uint8_t returnStatus;

//лексемы распарсенной строки
typedef struct
{
	uint8_t t0[TOKEN_MAX_SIZE];
	uint8_t t1[TOKEN_MAX_SIZE];
	uint8_t t2[TOKEN_MAX_SIZE];
	uint8_t t3[TOKEN_MAX_SIZE];
	uint8_t t4[TOKEN_MAX_SIZE];
	uint8_t t5[TOKEN_MAX_SIZE];
} TokensTypeDef;


returnStatus ConsoleInit(USART_TypeDef *USARTx, uint32_t BaudRate);
void ConsoleNewLine(void);
void ConsoleClearRecvLineDone(void);
bool ConsoleIsRecvLineDone(void);
void ConsolePrint(char* str);
void ConsolePrintN(char* str);
void ConsoleNewComandLine(void);
returnStatus ConsoleSetHostName(char *name);
returnStatus ConsoleSetTokenDelimiters(char *delim);
uint8_t ConsoleParseLine(void);
uint8_t ConsoleGetTokensNum(void);
uint8_t ConsoleCompareTokenString(char *str, uint8_t tokenNumber);
uint8_t ConsoleSetCurrentConfigLevel(uint8_t level, const char *levelStr);
uint8_t ConsoleGetCurrentConfigLevel(void);
char* ConsoleGetTokenString(uint8_t tokenNumber);

void ConsoleProcess(void);
uint8_t ConsoleProcessInit(void);




#endif /* USART_CONSOLE_H_ */

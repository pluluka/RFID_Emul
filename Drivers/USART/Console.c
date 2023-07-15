 /*
 * Console.с
 *
 *  Created on: May 26, 2020
 *      Author: tochk
 */
#include "Console.h"



/* Private variables ---------------------------------------------------------*/
static USART_TypeDef *usartInstance;
static char hostName[HOSTNAME_MAX_LENGTH] = {0};
static char tokenDelimiters[TOKEN_DELIMITERS_MAX_NUM] = {0};
static char currentConfigLevelString[CONFIG_LEVEL_STRING_MAX_LENGTH] = {0};

static __IO uint8_t  rxLineDone = FALSE;                     // флаг приема полной строки на USART
static __IO uint8_t  rxData;                             // переменная для принятого байта по USART
static __IO char	rxBuff[RX_BUFF_SIZE] = {0};            // буфер для приема по USART
static __IO uint8_t  rxDataCnt = 0;                      // счетчик принятых на USART байтов

static uint8_t consoleCurrentCfgLevel = 0x00;
static TokensTypeDef tokens;
static uint8_t tokensNum;

char ss[4];


static returnStatus ParseLine(char *lineBuff, char *delimiter, char *tokens, uint8_t *tokensNum);
static returnStatus SetCurrentConfigLevelString(const char *levelStr);
__STATIC_INLINE void clearRxLineDone(void);



/* Interruption and Exception Handlers ------------------------*/

/*
 * Обработка приема на консольном USART
 */
void CONSOLE_USART_IRQHandler(void)
{
    //если прерывание по приему байта
	if (usartInstance->SR & USART_SR_RXNE)
    {
		//читаем данные из регистра
		rxData = (uint8_t)usartInstance->DR;
		//если строчка еще не обработана, пропускаем принятые данные
		if (rxLineDone)
		{
			return;
		}
		//запихиваем новые данные на отправку(эхо ответ на консоль ПК)
		usartInstance->DR = rxData;

	  	//memset((uint8_t *)ss, 0x00, 4);
		//sprintf(ss, "%02X", rxData);
		//ConsolePrintN(ss);

		//проверяем наличие символа конца команды (символ 'Enter')
		if (rxData == '\r')
		{
			rxBuff[rxDataCnt] = 0x00;
			rxLineDone = 1;
			rxDataCnt = 0;

			return;
		}
		//проверяем наличие символа backspace
		else if ((rxData == '\b') || (rxData == 0x7F))
		{
			if (!rxDataCnt)
			{
				return;
			}
			rxDataCnt --;
			rxBuff[rxDataCnt] = 0x00;

			return;
		}
		else
		{
			//заполняем буфер
			rxBuff[rxDataCnt] = rxData;
			rxDataCnt ++;
		}

		//= чтобы счетчик не вылез за границы буфера
		if (rxDataCnt >= RX_BUFF_SIZE - 2)
		{
			rxDataCnt = 0;
			memset((uint8_t *)rxBuff, 0x00, RX_BUFF_SIZE);
		}
    }
}


/*
 * Парсит строку на лексемы
 * lineBuff:  буфер со строкой (должен заканчиваться значением 0x00)
 * delimiter: разделитель лексем (например '\t'-пробел)
 * tokens:    найденные лексемы
 * tokensNum: число найденных лексем
 */
static returnStatus ParseLine(char *lineBuff, char *delimiter, char *tokens, uint8_t *tokensNum)
{
	char *p;
	uint8_t n = 0;

	//selecting the first token
	p = strtok(lineBuff, delimiter);
	while (p != NULL)
	{
		strcpy(tokens, p);
		//selecting the other tokens
		p = strtok(NULL, delimiter);

		n ++;
		tokens += TOKEN_MAX_SIZE;

		if (n > TOKEN_MAX_NUM)
		{
			return STATUS_TOKEN_OVERAGE;
		}
	}

	*tokensNum += n;

	return STATUS_OK;
}


__STATIC_INLINE void clearRxLineDone(void)
{
	 rxLineDone = FALSE;
}


/*
 *  Инициализация
 *  USARTx:   USART1, USART2..
 *  BaudRate: скорость 9600, 19200..
 */
returnStatus ConsoleInit(USART_TypeDef *USARTx, uint32_t BaudRate)
 {
 	returnStatus status;

 	usartInstance = USARTx;
 	status = USART_DefaultInit(usartInstance, BaudRate);
 	USART_EnableIRQ(usartInstance, USART_IT_RXNE, USART_ISR_PRIORITY);

 	return status;
 }


/*
 * Вывод строки на консоль
 */
void ConsolePrint(char* str)
{
	USART_SendStr(usartInstance, str);
}

/*
 * Вывод строки на консоль
 */
void ConsolePrintN(char* str)
{
	USART_SendByte(usartInstance, '\n');
	USART_SendByte(usartInstance, '\r');
	USART_SendStr(usartInstance, str);
}


 /*
  * Перенос на след строку
  */
 void ConsoleNewLine(void)
 {
 	  USART_SendByte(usartInstance, '\n');
 	  USART_SendByte(usartInstance, '\r');
 }


 /* Состояние флага обработки строки
  * Возвращает '1' если принята строка (пользователь нажал Enter)
  * Флаг rxLineDone сбрасывается в ручную функцией clearRxLineDone(void)
  * по окончании разбора и обработки строки
  */
 bool ConsoleIsRecvLineDone(void)
 {
	 return rxLineDone ? TRUE : FALSE;
 }


 void ConsoleClearRecvLineDone(void)
 {
	 clearRxLineDone();
 }


/*
 * Отправляет строку приглашения ввода команды
 */
 void ConsoleNewComandLine(void)
 {
	  USART_SendByte(usartInstance, '\n');
	  USART_SendByte(usartInstance, '\r');
	  USART_SendStr(usartInstance, hostName);

	  if (*currentConfigLevelString)
	  {
		  USART_SendByte(usartInstance, '(');
		  USART_SendStr(usartInstance, currentConfigLevelString);
		  USART_SendByte(usartInstance, ')');
	  }

	  USART_SendByte(usartInstance, '>');

	  clearRxLineDone();
 }


 /*
  * Установка отображаемого имени хоста
  */
 returnStatus ConsoleSetHostName(char *name)
 {
	 int i = 0;

	 memset(hostName, 0x00, HOSTNAME_MAX_LENGTH);

	 while(*name)
	 {
		 hostName[i] = *name;

		 i ++;
		 if (i >= HOSTNAME_MAX_LENGTH)
		 {
			 return STATUS_BUFF_OUT_OF_RANGE;
		 }

		 name ++;
	 }

	 return STATUS_OK;
 }


 /*
  * Установка разделителей строки
  */
 returnStatus ConsoleSetTokenDelimiters(char *delim)
 {
	 int i = 0;

	 memset(tokenDelimiters, 0x00, TOKEN_DELIMITERS_MAX_NUM);

	 while(*delim)
	 {
		 tokenDelimiters[i] = *delim;

		 i ++;
		 if (i >= TOKEN_DELIMITERS_MAX_NUM)
		 {
			 return STATUS_BUFF_OUT_OF_RANGE;
		 }

		 delim ++;
	 }

	 return STATUS_OK;
 }


 /*
  * Парсит строку на лексемы
  * В случае успешной отработки возвращает '0'
  */
bool ConsoleParseLine(void)
{
	returnStatus status;

	tokensNum = 0;
	status = ParseLine(rxBuff, tokenDelimiters, (char *)&tokens, &tokensNum);

	return status ? FALSE : TRUE;
}


static returnStatus SetCurrentConfigLevelString(const char *levelStr)
{
	 int i = 0;

	 memset(currentConfigLevelString, 0x00, CONFIG_LEVEL_STRING_MAX_LENGTH);

	 while(*levelStr)
	 {
		 currentConfigLevelString[i] = *levelStr;

		 i ++;
		 if (i >= CONFIG_LEVEL_STRING_MAX_LENGTH)
		 {
			 return STATUS_BUFF_OUT_OF_RANGE;
		 }

		 levelStr ++;
	 }

	 return STATUS_OK;
}


/*
 * Возвращает количество токенов в распарсеной строке
 */
uint8_t ConsoleGetTokensNum(void)
{
	return tokensNum;
}


/*
 *
 */
uint8_t ConsoleSetCurrentConfigLevel(uint8_t level, const char *levelStr)
{
	if (SetCurrentConfigLevelString(levelStr))
	{
		return 0;
	}

	consoleCurrentCfgLevel = level;

	return 1;
}


/*
 *
 */
uint8_t ConsoleGetCurrentConfigLevel(void)
{
	return consoleCurrentCfgLevel;
}

/*
 * Сравнивает строку с одним из токенов
 */
bool ConsoleCompareTokenString(char *str, uint8_t tokenNumber)
{
	char *pTok;

	pTok = (char *)&tokens;
	pTok =  pTok + (TOKEN_MAX_SIZE * tokenNumber);

	return (strcmp(str, pTok) == 0) ? TRUE : FALSE;
}

char* ConsoleGetTokenString(uint8_t tokenNumber)
{
	char *pTok;

	pTok = (char *)&tokens;
	pTok =  pTok + (TOKEN_MAX_SIZE * tokenNumber);

	return pTok;
}

 /*
 returnStatus ShowTokens(void)
 {
 	uint8_t *pt;
 	uint8_t tokNum = 0;
 	returnStatus status;
 	TokensTypeDef tokens;

 	volatile char freq_str_buff[30] = {0};

 	status = ParseLine(rxBuff, tokenDelimiters, (uint8_t *)&tokens, &tokNum);

 	if ((tokNum == 0) || (status))
 	{
 		clearRxLineDone();
 		return status;
 	}

 	pt = (uint8_t *)&tokens;

 	while(tokNum)
 	{
 		ConsoleNewLine();
 		ConsolePrintf(pt);
 		pt += TOKEN_MAX_SIZE;
 		tokNum --;
 	}

 	clearRxLineDone();
 	return STATUS_OK;
 }
*/

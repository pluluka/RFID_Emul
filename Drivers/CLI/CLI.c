/*
 * CLI.c
 *
 *  Created on: 14 июн. 2022 г.
 *      Author: tochk
 */

#include "CLI.h"
#include "CLIThread.h"

#define RX_BUFF_SIZE              	256 //размер приемного буфера

#define TOKEN_MAX_SIZE				60   //макс размер лексемы
#define TOKEN_MAX_NUM				8    //макс количество лексем
#define TOKEN_DELIMITERS_MAX_NUM	5    //макс количество разделителей
#define HOSTNAME_MAX_LENGTH			12   //макс длина имени хоста




static __IO uint8_t	_cliEcho = 0;					// флаг разрешения ретрансляции входящих данных
static __IO uint8_t _rxLineDone = 1;				// флаг приема полной строки на USART
static __IO uint8_t _rxData;						// переменная для принятого байта по USART
static __IO uint8_t _rxBuff[RX_BUFF_SIZE] = {0};	// буфер для приема по USART
static __IO uint8_t _rxDataCnt = 0;					// счетчик принятых на USART байтов


static CLICmdList_t* currentCmdList;
static CLICmdList_t* currentCmd;

static char tokensBuff[(TOKEN_MAX_SIZE + 1) * TOKEN_MAX_NUM] = {0};	// буфер лексем
static char* tokenPointers[TOKEN_MAX_NUM] = {NULL};					// массив указателей на лексемы с буфера
static char hostName[HOSTNAME_MAX_LENGTH] = {0};					// отображаемое имя хоста
static char tokenDelimiters[TOKEN_DELIMITERS_MAX_NUM] = {0};		// разделители лексем в строке (по умолчанию пробел)
static char welcomeString[] = {"## STM32 CLI Ver.1.10 ##\r\n\r\n"};

extern osThreadId		threadCLI;
extern osMessageQId		msgQueueCLI;
extern osPoolId			memPoolCLI;



static uint8_t ParseLine(char* lineBuff, char* delimiter, char* tokens, uint8_t* tokensNum);
static uint8_t SetTokenDelimiters(char* delim);
static void SetHostName(char* name);
__STATIC_INLINE void ClearRxLineDone(void);
__STATIC_INLINE void AllowReceivingCommands(void);
__STATIC_INLINE void PrintString(char* str);
__STATIC_INLINE void PrintChar(char ch);
__STATIC_INLINE void SetSubListString(char* str);
__STATIC_INLINE int GetCmdListLength(CLICmdList_t* list);
__STATIC_INLINE void EchoDisable(void);
__STATIC_INLINE void EchoEnable(void);


/*
 * Обработка приема на консольном USART
 */
void CLI_USART_IRQHandler(void)
{
    //если прерывание по приему байта
	if (CLI_USART->SR & USART_SR_RXNE)
    {
		// читаем данные из регистра
		_rxData = (uint8_t)CLI_USART->DR;
		// если поднят флаг запрета чтения входящих данных с порта, курим дальше
		if (_cliEcho)
		{
			USART_SendByte(CLI_USART, _rxData);
		}
		//запихиваем новые данные на отправку(эхо ответ на консоль ПК)

		if (_rxData == '\n')
		{
			return;
		}
		//проверяем наличие символа конца команды (символ 'Enter')
		if (_rxData == '\r')
		{
			if ((_rxLineDone) && (_rxDataCnt != 0))
			{
				return;
			}

			USART_SendByte(CLI_USART, '\n');
			_rxBuff[_rxDataCnt] = *tokenDelimiters;
			_rxDataCnt ++;
			_rxBuff[_rxDataCnt] = 0x00;
			_rxLineDone = 1;
			_rxDataCnt = 0;

			//==== Дальнейшая обработка принятой строки
			//==== (передача сообщения потоку RTOS и т.п..)

				// Создаем сообщение потоку CLI с вновь поступившей строкой
				// Выделяем память в пуле под новое сообщение
				CLIMsgData_t*	msgData = (CLIMsgData_t *)osPoolAlloc(memPoolCLI);
				if (msgData != NULL)
				{
					msgData->msgType = CLI_THREAD_MSG__CMD_RECV;
					msgData->p = (void*)_rxBuff;
					// Добавляем указатель в очередь сообщений
					if (osMessagePut(msgQueueCLI, (uint32_t)(msgData), 0) == osOK)
					{
						osThreadYield();
						return;
					}
					else
					{// если возникли проблемы с очередью, освобождаем выделенную память
						osPoolFree(memPoolCLI, msgData);
						__debugError(0);
					}
				}
				else
				{
					__debugError(0);
				}

			return;
		}

		//проверяем наличие символа backspace
		else if ((_rxData == '\b') || (_rxData == 0x7F))
		{
			if (!_rxDataCnt)
			{
				return;
			}
			_rxDataCnt --;
			_rxBuff[_rxDataCnt] = 0x00;

			return;
		}
		else
		{
			//заполняем буфер
			_rxBuff[_rxDataCnt] = _rxData;
			_rxDataCnt ++;
		}

		//= чтобы счетчик не вылез за границы буфера
		if (_rxDataCnt >= RX_BUFF_SIZE - 2)
		{
			_rxDataCnt = 0;
			memset((uint8_t *)_rxBuff, 0x00, RX_BUFF_SIZE);
		}
    }
}







void CLI_Init(void)
{
	//---- USART Init
	USART_DefaultInit(CLI_USART, USART_BAUDRATE);
	USART_EnableIRQ(CLI_USART, USART_IT_RXNE, USART_ISR_PRIORITY);
	SetTokenDelimiters(" ");
	SetHostName("CLI");

	PrintString(welcomeString);

	AllowReceivingCommands();
}


/*
 * Установка отображаемого имени хоста
 */
void CLI_SetHostName(char *name)
{
	SetHostName(name);
}

/*
 * Вкл. прием комманд
 */
void CLI_AllowCommands(void)
{
	AllowReceivingCommands();
}

/*
void CLI_Enable(void)
{
	ReadingDataEnable();
}

void CLI_Disable(void)
{
	ReadingDataDisable();
}
*/

void CLI_SetCurrentCmdList(CLICmdList_t* cmdList)
{
	currentCmdList = cmdList;
}

/*
 *	Обработка текущей строки (содержание буфера _rxBuff).
 *	Будет осуществлен поиск команды с текущей строки, в случае если команда будет найдена
 *	управление будет передано функции по адресу указанному в структуре команды.
 *	cmdList:		лист команд в котором будет осуществлен поиск команды с текущей строки
 *	listLength:		длина листа (кол-во команд)
 */
uint8_t CLI_ProcessCurrentLine(void)
{
	char*			tokens;
	uint8_t			tokensNum;
	uint8_t			res;
	CLICmdList_t*   list;
	uint32_t		cmdNum = 0;

	list = currentCmdList;
	// если буфер пустой (нажата клавиша enter без ввода команды)
	// (стоит отметить, что последний символ в буфере заполняется кодом символа-разделителя)
	if (_rxBuff[0] == *tokenDelimiters)
	{
		return CLI_STATUS_COMMAND_CLI_EN;
	}
	res = ParseLine(_rxBuff, tokenDelimiters, tokensBuff, &tokensNum);
	if (res != CLI_STATUS_OK)
	{
		return res;
	}

	tokens = tokensBuff;

	while(list->Command)
	{
		if (strcmp(tokens, list->Command) == 0)
		{
			while(*tokens++) ;
			// Если у команды есть функция-обработчик, отдаем ей управление.
			// При этом отключается ввод команд, дальнейшее разрешение ввода
			// возлагается на плечи функции-обработчика команды.
			if (list->Func == NULL)
			{
				return CLI_STATUS_COMMAND_FUNCTION_UNDEF;
			}
			EchoDisable();
			void* tlist = list->Func(tokensNum - 1, tokenPointers + 1, (void*)list, cmdNum);
			// если результат отработки ф-ии - переход к другому командному листу
			if (tlist != (void*)list)
			{
				currentCmdList = (CLICmdList_t*)tlist;
				return CLI_STATUS_COMMAND_GO_TO_SUBLIST;
			}
			currentCmd = (CLICmdList_t*)tlist;
			return CLI_STATUS_COMMAND_PROCESSED;
		}
		list ++;
		cmdNum ++;
	}

	return CLI_STATUS_COMMAND_NOT_FOUND;
}

CLICmdList_t* CLI_GetCurrentCmd(void)
{
	return currentCmd;
}

void CLI_SetCurrentCmd(CLICmdList_t* cmd)
{
	currentCmd = cmd;
	//return;
}

void CLI_PrintString(char* str)
{
	PrintString(str);
    //memset(subListString, 0x00, SUBLIST_STRING_LENGTH);
}

void CLI_PrintChar(char ch)
{
	PrintChar(ch);
    //memset(subListString, 0x00, SUBLIST_STRING_LENGTH);
}

void CLI_SetSubListString(char* str)
{
	SetSubListString(str);
}

int CLI_GetCmdListLength(CLICmdList_t* list)
{
	return GetCmdListLength(list);
}

/*
 * Устанавливает владельцев вложенных командных листов
 *  list:			лист, которому необходимо указать владельца
 *	ownerCmdList:	лист команды владельца
 *	ownerCmdNum:	номер команды в листе
 *
 */
void CLI_SetListOwnerCmd(CLICmdList_t* list, CLICmdList_t* ownerCmdList, uint32_t ownerCmdNum)
{
	list += GetCmdListLength(list);
	list->SubList = ownerCmdList;
	list->Help = (uint32_t)(ownerCmdNum);
}



/*
 * Парсит строку на лексемы
 * lineBuff:  буфер со строкой (должен заканчиваться значением 0x00)
 * delimiter: разделитель лексем (например '\t'-пробел)
 * tokens:    найденные лексемы
 * tokensNum: число найденных лексем
 */
static uint8_t ParseLine(char *lineBuff, char *delimiter, char *tokens, uint8_t *tokensNum)
{
	char 		*p;
	uint8_t 	n = 0;
	uint32_t 	len = 0;

	for (uint32_t i = 0; i < TOKEN_MAX_NUM; i ++)
	{
		tokenPointers[i] = NULL;
	}

	//selecting the first token
	p = strtok(lineBuff, delimiter);
	while (p != NULL)
	{
		len = strlen(p);
		if (len > TOKEN_MAX_SIZE)
		{
			return CLI_STATUS_TOKEN_OVERAGE;
		}
		strcpy(tokens, p);
		tokenPointers[n] = p;
		//selecting the other tokens
		p = strtok(NULL, delimiter);

		tokens += len;
		*tokens = 0x00;
		tokens ++;
		n ++;

		if (n > TOKEN_MAX_NUM)
		{
			return CLI_STATUS_TOKEN_OVERAGE;
		}
	}

	*tokensNum = n;

	return CLI_STATUS_OK;
}

/*
 * Установка разделителей строки
 */
static uint8_t SetTokenDelimiters(char *delim)
{
	 int i = 0;

	 memset(tokenDelimiters, 0x00, TOKEN_DELIMITERS_MAX_NUM);

	 while(*delim)
	 {
		 tokenDelimiters[i] = *delim;

		 i ++;
		 if (i >= TOKEN_DELIMITERS_MAX_NUM)
		 {
			 return CLI_STATUS_BUFF_OUT_OF_RANGE;
		 }

		 delim ++;
	 }

	 return CLI_STATUS_OK;
}

/*
 * Установка отображаемого имени хоста
 */
static void SetHostName(char *name)
{
	 memset(hostName, 0x00, HOSTNAME_MAX_LENGTH);

	 for (uint32_t i = 0; i < HOSTNAME_MAX_LENGTH; i ++)
	 {
		 hostName[i] = name[i];
		 if (!name[i])
			 break;
	 }
}

/*
 * Сбрасывает флаг занятости процессов обработки команд
 */
__STATIC_INLINE void ClearRxLineDone(void)
{
	 _rxLineDone = 0;
}

/*
 * Разрешает ввод комманд в CLI
 */
__STATIC_INLINE void AllowReceivingCommands(void)
{

	char* str;
	memset(_rxBuff, 0x00, sizeof(_rxBuff));
	memset(tokensBuff, 0x00, sizeof(tokensBuff));
	//printf("%s ", hostName);
	str = hostName;
	while (*str)
	{
		USART_SendByte(CLI_USART, *str);
		str ++;
	}

	// Добавляем название владельца листа, если таковой есть
	int currentCmdListLen = GetCmdListLength(currentCmdList);
	CLICmdList_t* ownList = (CLICmdList_t*)(currentCmdList + currentCmdListLen)->SubList;
	if (ownList != NULL)
	{
		ownList += (uint32_t)((currentCmdList + currentCmdListLen)->Help);
		str = ownList->Command;
		if (*str)
		{
			USART_SendByte(CLI_USART, '/');
			while (*str)
			{
				USART_SendByte(CLI_USART, *str);
				str ++;
			}
		}
	}

	USART_SendByte(CLI_USART, ':');
	USART_SendByte(CLI_USART, ' ');

	EchoEnable();
	ClearRxLineDone();
}

/*
 * Вывод в консоль строки
 */
__STATIC_INLINE void PrintString(char* str)
{
    if(str == NULL)
    	return;

	USART_SendStr(CLI_USART, str);
}

__STATIC_INLINE void PrintChar(char ch)
{
    if(ch == 0x00)
    	return;

	USART_SendData(CLI_USART, ch);
}

/*
 * Вычисляет длину командного листа (без последней записи,
 * которая содержит адрес листа уровнем выше)
 */
__STATIC_INLINE int GetCmdListLength(CLICmdList_t* list)
{
	int len = 0;

	while(list->Command)
	{
		len ++;
		list ++;
	}

	return len;
}

__STATIC_INLINE void EchoDisable(void)
{
	_cliEcho = 0;
}

__STATIC_INLINE void EchoEnable(void)
{
	_cliEcho = 1;
}


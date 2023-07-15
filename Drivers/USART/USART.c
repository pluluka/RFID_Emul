/*
 * USART.c
 *
 *  Created on: 10 мая 2020 г.
 *      Author: tochk
 */

#include "USART.h"


__STATIC_INLINE void USART_SendData8(USART_TypeDef* USARTx, uint8_t data);


/*
 * Инициализация модуля USART с дефолтными настройками
 * USARTx:    модуль usart (USART1, USART2..)
 * BaudRate:  скорость  (9600U, 19200U..)
 */
returnStatus USART_DefaultInit(USART_TypeDef *USARTx, uint32_t BaudRate)
{
	LL_USART_InitTypeDef USART_InitStruct = {0};


	/* Peripheral clock enable */
	if (USARTx == USART1)
	{
		  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);
	}
	else if (USARTx == USART2)
	{
		  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);
	}
	else if (USARTx == USART6)
	{
		  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART6);
	}
	else
	{
		return USART_RS_CLOCK_SEL_ERROR;
	}

	/* GPIO Init */
	if (USART_GPIOInit(USARTx))
		return USART_RS_GPIO_INIT_ERROR;

	/* USART initialization and enable */
	USART_InitStruct.BaudRate = 9600;
	USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
	USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
	USART_InitStruct.Parity = LL_USART_PARITY_NONE;
	USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
	USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
	//USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_8;
	LL_USART_Init(USARTx, &USART_InitStruct);
	LL_USART_ConfigAsyncMode(USARTx);
	LL_USART_Enable(USARTx);

	return USART_RS_OK;
}

/*
 * Инициализация GPIO
 */
returnStatus USART_GPIOInit(USART_TypeDef *USARTx)
{
	LL_GPIO_InitTypeDef GPIO_InitStruct = {0};


	if (USARTx == USART1)
	{
		  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
		  /**USART1 GPIO Configuration
		  PA9   ------> USART1_TX
		  PA10   ------> USART1_RX
		  */
		  GPIO_InitStruct.Pin = LL_GPIO_PIN_9|LL_GPIO_PIN_10;
		  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
		  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
		  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
		  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
		  GPIO_InitStruct.Alternate = LL_GPIO_AF_7;
		  if (LL_GPIO_Init(GPIOA, &GPIO_InitStruct) != SUCCESS)
			  return USART_RS_ERROR;
	}
	else if (USARTx == USART2)
	{
		  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
		  /**USART2 GPIO Configuration
		  PA2   ------> USART2_TX
		  PA3   ------> USART2_RX
		  */
		  GPIO_InitStruct.Pin = LL_GPIO_PIN_2|LL_GPIO_PIN_3;
		  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
		  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
		  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
		  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
		  GPIO_InitStruct.Alternate = LL_GPIO_AF_7;
		  if (LL_GPIO_Init(GPIOA, &GPIO_InitStruct) != SUCCESS)
			  return USART_RS_ERROR;
	}
	else if (USARTx == USART6)
	{
		  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
		  /**USART6 GPIO Configuration
		  PA11   ------> USART6_TX
		  PA12   ------> USART6_RX
		  */
		  GPIO_InitStruct.Pin = LL_GPIO_PIN_11|LL_GPIO_PIN_12;
		  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
		  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
		  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
		  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
		  GPIO_InitStruct.Alternate = LL_GPIO_AF_8;
		  if (LL_GPIO_Init(GPIOA, &GPIO_InitStruct) != SUCCESS)
			  return USART_RS_ERROR;
	}
	else
	{
		return USART_RS_ERROR;
	}

	return USART_RS_OK;
}


/*
 * отправка одного байта с ожиданием разрешения на передачу
 */
__STATIC_INLINE void USART_SendData8(USART_TypeDef* USARTx, uint8_t data)
{
	// ждем пока регистр DR скинет данные в сдвиговый регистр
    while(!(USARTx->SR & USART_SR_TXE))
    {
    	;
    }
    USARTx->DR = data;
}


/*
 * отправка строки (последний символ - 0x00)
 */
void USART_SendStr(USART_TypeDef* USARTx, char *str)
{
	while(!(USARTx->SR & USART_SR_TC));

    while(*str)
    {
    	USART_SendData8(USARTx, *str++);
    }
}


void USART_SendEndLine(USART_TypeDef* USARTx)
{
	  USART_SendByte(USARTx, '\n');
	  USART_SendByte(USARTx, '\r');
}


/*
 * Enable an USART interrupt
 */
returnStatus USART_EnableIRQ(USART_TypeDef* USARTx, uint16_t USART_IT, uint32_t priority)
{
	/* Select interrupt type */
	if (USART_IT == USART_IT_TXE)
	{
		LL_USART_EnableIT_TXE(USARTx);
	}
	else if (USART_IT == USART_IT_RXNE)
	{
		LL_USART_EnableIT_RXNE(USARTx);
	}
	else if (USART_IT == USART_IT_TC)
	{
		LL_USART_EnableIT_TC(USARTx);
	}
	else if (USART_IT == USART_IT_IDLE)
	{
		LL_USART_EnableIT_IDLE(USARTx);
	}
	else if (USART_IT == USART_IT_PE)
	{
		LL_USART_EnableIT_PE(USARTx);
	}
	else if (USART_IT == USART_IT_ERR)
	{
		LL_USART_EnableIT_ERROR(USARTx);
	}
	else
	{
		return USART_RS_IT_SEL_ERROR;
	}

	/* Enable NVIC IRQ */
	if (USARTx == USART1)
	{
		NVIC_SetPriority(USART1_IRQn, priority);
		NVIC_EnableIRQ(USART1_IRQn);
	}
	if (USARTx == USART2)
	{
		NVIC_SetPriority(USART2_IRQn, priority);
		NVIC_EnableIRQ(USART2_IRQn);
	}
	if (USARTx == USART6)
	{
		NVIC_SetPriority(USART6_IRQn, priority);
		NVIC_EnableIRQ(USART6_IRQn);
	}

	return USART_RS_OK;
}


/*
 * Disable an USART interrupt
 */
returnStatus USART_DisableIRQ(USART_TypeDef* USARTx, uint16_t USART_IT)
{
	/* Select interrupt type */
	if (USART_IT == USART_IT_TXE)
	{
		LL_USART_DisableIT_TXE(USARTx);
	}
	else if (USART_IT == USART_IT_RXNE)
	{
		LL_USART_DisableIT_RXNE(USARTx);
	}
	else if (USART_IT == USART_IT_TC)
	{
		LL_USART_DisableIT_TC(USARTx);
	}
	else if (USART_IT == USART_IT_IDLE)
	{
		LL_USART_DisableIT_IDLE(USARTx);
	}
	else if (USART_IT == USART_IT_PE)
	{
		LL_USART_DisableIT_PE(USARTx);
	}
	else if (USART_IT == USART_IT_ERR)
	{
		LL_USART_DisableIT_ERROR(USARTx);
	}
	else
	{
		return USART_RS_IT_SEL_ERROR;
	}

	return USART_RS_OK;
}











////////////////////////

/*
void USART1_IRQHandler()
{
    //= если прерывание по приему байта
	if (TRM_USART->SR & USART_SR_RXNE)
    {
        USARTx->SR &= ~USART_SR_RXNE;     // сбрасываем флаг прерывания
		trm_rx_data = (uint8_t) USART1->DR;     //- читаем данные из регистра
		USART1->DR = trm_rx_data;      //- запихиваем новые данные на отправку
    }
}
*/

/*
 *
 		  USART_SendEndLine(USART1);
		  memset((uint8_t *)freq_str_buff, 0x00, 30);
		  USART_SendStr(USART1, "error code: ");
		  sprintf(freq_str_buff, "%010d", status);
		  USART_SendStr(USART1, freq_str_buff);
		  USART_SendEndLine(USART1);

		  	memset((uint8_t *)freq_str_buff, 0x00, 30);
			sprintf(freq_str_buff, "%04d", status);

 */

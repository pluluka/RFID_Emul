/*
 * USART.h
 *
 *  Created on: 10 мая 2020 г.
 *      Author: tochk
 */

#ifndef USART_USART_H_
#define USART_USART_H_


#include "stm32f4xx.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_usart.h"


typedef uint8_t returnStatus;

/* Errors */
#define USART_RS_OK                 (uint8_t)0x00
#define USART_RS_ERROR              (uint8_t)0x01
#define USART_RS_GPIO_INIT_ERROR    (uint8_t)0x11
#define USART_RS_CLOCK_SEL_ERROR    (uint8_t)0x12
#define USART_RS_USART_INIT_ERROR   (uint8_t)0x13
#define USART_RS_IT_SEL_ERROR       (uint8_t)0x14

/* Interrutps */
#define  USART_IT_CTS    ((uint16_t)0x096A) // CTS change interrupt (not available for UART4 and UART5)
#define  USART_IT_LBD    ((uint16_t)0x0846) // LIN Break detection interrupt
#define  USART_IT_TXE    ((uint16_t)0x0727) // Transmit Data Register empty interrupt
#define  USART_IT_TC     ((uint16_t)0x0626) // Transmission complete interrupt
#define  USART_IT_RXNE   ((uint16_t)0x0525) // Receive Data register not empty interrupt
#define  USART_IT_IDLE   ((uint16_t)0x0424) // Idle line detection interrupt
#define  USART_IT_PE     ((uint16_t)0x0028) // Parity Error interrupt
#define  USART_IT_ERR    ((uint16_t)0x0060) // Error interrupt(Frame error, noise error, overrun error)


returnStatus USART_GPIOInit(USART_TypeDef *USARTx);
returnStatus USART_DefaultInit(USART_TypeDef *USARTx, uint32_t BaudRate);
returnStatus USART_EnableIRQ(USART_TypeDef* USARTx, uint16_t USART_IT, uint32_t priority);
returnStatus USART_DisableIRQ(USART_TypeDef* USARTx, uint16_t USART_IT);
void USART_SendStr(USART_TypeDef* USARTx, char *str);
void USART_SendEndLine(USART_TypeDef* USARTx);


/*
 * отправка одного байта с ожиданием окончания передачи
 */
__STATIC_INLINE USART_SendByte(USART_TypeDef* USARTx, uint8_t data)
{
	// ждем пока сдвиговый регистр выплюнет данные
    while(!(USARTx->SR & USART_SR_TC))
    {
    	;
    }
    USARTx->DR = data;
}




#endif /* USART_USART_H_ */

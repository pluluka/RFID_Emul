/*
 * ExtButton.h
 *
 *  Created on: Nov 8, 2020
 *      Author: tochk
 */

#ifndef EXTBUTTON_EXTBUTTON_H_
#define EXTBUTTON_EXTBUTTON_H_


#include "stm32f4xx.h"
#include "stm32f4xx_ll_spi.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_exti.h"
#include <stdbool.h>
#include "delay.h"


#define EXTBTN_ISR_PRIORITY			6


/*
void EXTI2_IRQHandler(void)
{
	// сбрасываем флаг прерывания
	EXTI->PR |= EXTI_PR_PR2;



	// запрещаем прерывания
	//EXTI->IMR &= ~LL_EXTI_LINE_2;
	// разрешаем прерывания
	//EXTI->IMR |= LL_EXTI_LINE_2;
}
*/

void ExtButtonPA2_Init(void)
{
	LL_GPIO_InitTypeDef GPIO_InitStruct;
	LL_EXTI_InitTypeDef exti;

	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);

	GPIO_InitStruct.Pin = LL_GPIO_PIN_2;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
	GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
	LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/***  Настраиваем внешнее прерывание ***/
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);

	// Подключаем вывод к  соответствующей линии EXTI
	// EXTICR[0] - линии exti0..exti3
	// в нашем случае линия exti2 (PA2)
	SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI2;
	SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI2_PA;

	exti.Line_0_31 = LL_EXTI_LINE_2;
	exti.LineCommand = ENABLE;
	exti.Mode = LL_EXTI_MODE_IT;
	exti.Trigger = LL_EXTI_TRIGGER_FALLING;

	LL_EXTI_Init(&exti);

	NVIC_SetPriority(EXTI2_IRQn, EXTBTN_ISR_PRIORITY);
	NVIC_EnableIRQ(EXTI2_IRQn);
}


void ExtButtonPA3_Init(void)
{
	LL_GPIO_InitTypeDef GPIO_InitStruct;
	LL_EXTI_InitTypeDef exti;

	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);

	GPIO_InitStruct.Pin = LL_GPIO_PIN_3;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
	GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
	LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/***  Настраиваем внешнее прерывание ***/
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);

	// Подключаем вывод к  соответствующей линии EXTI
	// EXTICR[0] - линии exti0..exti3
	// в нашем случае линия exti2 (PA2)
	SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI3;
	SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI3_PA;

	exti.Line_0_31 = LL_EXTI_LINE_3;
	exti.LineCommand = ENABLE;
	exti.Mode = LL_EXTI_MODE_IT;
	exti.Trigger = LL_EXTI_TRIGGER_FALLING;

	LL_EXTI_Init(&exti);

	NVIC_SetPriority(EXTI3_IRQn, EXTBTN_ISR_PRIORITY);
	NVIC_EnableIRQ(EXTI3_IRQn);
}

void ExtButtonPA1_Init(void)
{
	LL_GPIO_InitTypeDef GPIO_InitStruct;
	LL_EXTI_InitTypeDef exti;

	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);

	GPIO_InitStruct.Pin = LL_GPIO_PIN_1;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
	GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
	LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/***  Настраиваем внешнее прерывание ***/
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);

	// Подключаем вывод к  соответствующей линии EXTI
	// EXTICR[0] - линии exti0..exti3
	// в нашем случае линия exti2 (PA2)
	SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI1;
	SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI1_PA;

	exti.Line_0_31 = LL_EXTI_LINE_1;
	exti.LineCommand = ENABLE;
	exti.Mode = LL_EXTI_MODE_IT;
	exti.Trigger = LL_EXTI_TRIGGER_FALLING;

	LL_EXTI_Init(&exti);

	NVIC_SetPriority(EXTI1_IRQn, EXTBTN_ISR_PRIORITY);
	NVIC_EnableIRQ(EXTI1_IRQn);
}




#endif /* EXTBUTTON_EXTBUTTON_H_ */

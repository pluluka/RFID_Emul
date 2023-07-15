/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */


/* Includes ------------------------------------------------------------------*/
#include <EM4100Thread.h>
//#include <OneWireMaster.h>
#include "main.h"
#include "ReDefSTDIO.h"
//#include "LCD_Console.h"
#include "FONTS.h"
#include "FONT_SimSun_17_10.h"
#include "delay.h"
#include "USART.h"
#include "AT45DB161.h"
#include "SPIFlashTabSystem.h"
//#include "xpt2046.h"
#include "ExtButton.h"
#include "cmsis_os.h"
#include "EM4100_RWE.h"
#include "Metakom.h"
//#include "TestThread.h"
//#include "ExtButton.h"
//#include "OneWireSlave.h"
//#include "W25QXXXFLASH.h"
//#include "CLI.h"
#include "CLIThread.h"


/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
__IO void Delay(__IO uint32_t nCount);


void DisplayInit(void);
void ExtButtonsInit(void);
void ScreenButtonsInit(void);




int main(void)
{
	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */

	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

	/* Все 4 бита регистра приоритетов прерываний используются под приоритет вытеснения */
	NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);	//????????????????????//

	/* Configure the system clock */
	SystemClock_Config();

	/* Инициализация перефирии */
	//delay_Init();

	/* переопределяем поток вывода printf в USART */
	StdOutFile_Init();


	/* Файловая система на базе SPI FLASH */
	SPIFTabSystemInit();
	//printf("Formating the flash...\n");
	//SPIFTabSystemFormat();
	//printf("Done!\n");


	uint8_t status = AT45DB161_ReadStatus();
	printf("File system Init...\n");
	printf("FLASH type: AT45DB161. Status register: %02X\n", status);
	uint8_t id[4];
	AT45DB161_ReadID(id);
	printf("AT45DB161 device ID: %02X %02X %02X %02X\n", id[0],id[1],id[2],id[3]);
	uint32_t dataCnt = SPIFTabSystem_EM4100GetCounter();
	printf("EM4100 total records count: %04d\n", dataCnt);
	dataCnt = SPIFTabSystem_OneWireGetCounter();
	printf("OneWire total records count: %04d\n", dataCnt);



	printf("std out test..\n");
	//TerminalThreadInit(osPriorityNormal);

	/*EM4100  ************/
	EM4100ThreadInit(osPriorityNormal);
	//printf("EM4100ThreadInit done. Free heap size: %06d\n", xPortGetFreeHeapSize());

	/* Main ******************/
	CLIThreadInit(osPriorityAboveNormal);
	//printf("CLIThreadInit done. Free heap size: %06d\n", xPortGetFreeHeapSize());

	/*1-Wire ************/
	OneWireThreadInit(osPriorityNormal);
	//printf("OneWireThreadInit done. Free heap size: %06d\n", xPortGetFreeHeapSize());


	//TestThreadInit(osPriorityNormal);


	//ExtButtonPA2_Init();
	//ExtButtonPA1_Init();


    /* Start scheduler */
	osKernelStart();


	while (1)
	{
		//ConsoleSendString("Detected tag:\n");
		//MetakomWriteKey(&keyy);
		//EM4100_RWE_EmulatorRunTag(&tag);
		//Delay(200 * 6000);
		//osMessagePut(msgQueueTest, (uint32_t)123, 0) ;
		//ScreenButtonsProcess();
	}
}



void ExtButtonsInit(void)
{
	ExtButtonPA2_Init();
}





/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_2);
  while(LL_FLASH_GetLatency()!= LL_FLASH_LATENCY_2)
  {
  }
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE2);
  LL_RCC_HSE_Enable();

   /* Wait till HSE is ready */
  while(LL_RCC_HSE_IsReady() != 1)
  {

  }
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_25, 168, LL_RCC_PLLP_DIV_2);
  LL_RCC_PLL_Enable();

   /* Wait till PLL is ready */
  while(LL_RCC_PLL_IsReady() != 1)
  {

  }
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_2);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
  {

  }
  LL_Init1msTick(84000000);
  LL_SetSystemCoreClock(84000000);
  LL_RCC_SetTIMPrescaler(LL_RCC_TIM_PRESCALER_TWICE);
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
	LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);

	/**/
	LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_3);

	/**/
	GPIO_InitStruct.Pin = LL_GPIO_PIN_3;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	LL_GPIO_Init(GPIOB, &GPIO_InitStruct);



	/* GPIO Ports Clock Enable */
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
	/**/
	LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_11);
	/**/
	GPIO_InitStruct.Pin = LL_GPIO_PIN_11;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    /**/
    LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_12);
    /**/
    GPIO_InitStruct.Pin = LL_GPIO_PIN_12;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

}


__IO void Delay(__IO uint32_t nCount)
{
	while(nCount--)
	{
		__asm("NOP");
	}
}



#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{

}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

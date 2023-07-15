/*
 * hooks.c
 *
 *  Created on: 31 янв. 2021 г.
 *      Author: tochk
 */


#include "hooks.h"



void vAssertCalled( unsigned long ulLine, const char * const pcFileName )
{
  taskDISABLE_INTERRUPTS(); // game over

  printf("Error: vAssertCalled\n");
  printf(pcFileName);
  printf("\n");
  printf(" Line: %04d\n", ulLine);

  for( ;; );
}



/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(const char *file, uint32_t line)
{
	//taskDISABLE_INTERRUPTS(); // game over

	printf("Error: Error_Handler\n");
	printf("file: ");
	printf(file);
	printf(" \n");
	printf("line: %04d \n", line);

	for( ;; );
}

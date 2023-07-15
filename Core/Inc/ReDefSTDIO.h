/*
 * ReDefSTDIO.h
 *
 *  Created on: 13 янв. 2021 г.
 *      Author: tochk
 *      Переопределяем стандартный поток вывода printf
 *      (например в USART)
 */

#ifndef SRC_REDEFSTDIO_H_
#define SRC_REDEFSTDIO_H_



#include  <errno.h>
#include  <sys/unistd.h> // STDOUT_FILENO, STDERR_FILENO
#include "USART.h"

// сюда будем направлять поток
#define STDOUT_USART		USART1


int _write(int file, char *data, int len)
{
   if ((file != STDOUT_FILENO) && (file != STDERR_FILENO))
   {
      errno = EBADF;
      return -1;
   }
   for (int i = 0; i < len; i ++)
   {
	   if (*data == '\n')
	   {
		   USART_SendByte(STDOUT_USART, '\n');
		   USART_SendByte(STDOUT_USART, '\r');
	   }
	   else
	   {
		   USART_SendByte(STDOUT_USART, *data++);
	   }
   }

   return 0;
}


void StdOutFile_Init(void)
{
	USART_DefaultInit(USART1, 9600);
	printf("\n\n##### DEBUG ON USART1 INIT DONE #####\n");
}



#endif /* SRC_REDEFSTDIO_H_ */

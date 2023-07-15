/*
 * TestThread.c
 *
 *  Created on: 9 мая 2021 г.
 *      Author: tochk
 */


#include "TestThread.h"


#define THREAD_STACK_SIZE		250	//Размер стека задачи
#define MSG_QUEUE_DEPTH			1		//глубина очереди





void TestThread(void const * argument);




void TestThreadInit(uint32_t threadPriority)
{
	// Создаем семафор
	osSemaphoreDef (semaphoreTest);
	semaphoreTest = osSemaphoreCreate(osSemaphore(semaphoreTest), 1);
	// Захватываем семафор сразу после создания, в дальнейшем его будет выдавать только ISR
	osSemaphoreWait(semaphoreTest, 10);

	// Создаем очередь
	osMessageQDef(msgQueueTest, MSG_QUEUE_DEPTH, uint32_t);
	msgQueueTest = osMessageCreate(osMessageQ(msgQueueTest), NULL);

	// Создаем пул под данные очереди
	/*
	osPoolDef(memPoolTest, MSG_QUEUE_DEPTH, CtrlSourceData_t);
	memPoolTest = osPoolCreate(osPool(memPoolTest));
	*/

	// Создаем задачу
	osThreadDef(TestThread, TestThread, threadPriority, 0, THREAD_STACK_SIZE);
	threadTest = osThreadCreate(osThread(TestThread), NULL);
	if (threadTest == NULL)
	{
		__debugError(0);
	}

	//OneWireMasterInit();
	//OneWireSlaveInit();
}

uint8_t ii;

void TestThread(void const * argument)
{
	osEvent		evt;
	int32_t		val;

	OneWireData_t ds1990Data;
	//uint8_t uniData[8] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
	OneWireData_t uniData;
	uniData.romData[0] = 0xFF;
	uniData.romData[1] = 0xFF;
	uniData.romData[2] = 0xFF;
	uniData.romData[3] = 0xFF;
	uniData.romData[4] = 0xFF;
	uniData.romData[5] = 0xFF;
	uniData.romData[6] = 0xFF;
	uniData.romData[7] = 0x14;

	//OneWireSlaveDS1990_OnWire(uniData);

	OneWireMasterInit();

	/**** Поток ****/
	for(;;)
	{
		/*
		// ожидаем семафор
       val = osSemaphoreWait(semaphoreTest, osWaitForever);
		 */
       //USART_SendByte(USART1, 0x35);

		evt = osMessageGet(msgQueueTest, osWaitForever);
		if (evt.status == osEventMessage)
		{
			val = evt.value.v;

			if (val > 9)
				printf("%02X\n ", val);


			switch (val){
			case EXTBTN_PA2:
			{
				//osDelay(2);

				uint8_t res = OneWireMasterReadROM(&ds1990Data);
			 	if (res == OWM_RS_OK)
			 	{
			    	printf("%02X ", ds1990Data.romData[0]);
			    	printf("%02X ", ds1990Data.romData[6]);
			    	printf("%02X ", ds1990Data.romData[5]);
			    	printf("%02X ", ds1990Data.romData[4]);
			    	printf("%02X ", ds1990Data.romData[3]);
			    	printf("%02X ", ds1990Data.romData[2]);
			    	printf("%02X ", ds1990Data.romData[1]);
			    	printf("%02X\n", ds1990Data.romData[7]);
			   	}
			   	else
			   	{
			    	printf("%02X ", ds1990Data.romData[0]);
			    	printf("%02X ", ds1990Data.romData[6]);
			    	printf("%02X ", ds1990Data.romData[5]);
			    	printf("%02X ", ds1990Data.romData[4]);
			    	printf("%02X ", ds1990Data.romData[3]);
			    	printf("%02X ", ds1990Data.romData[2]);
			    	printf("%02X ", ds1990Data.romData[1]);
			    	printf("%02X\n", ds1990Data.romData[7]);
			    	printf("Read error code %02X\n", res);
			   	}

				break;
			}
			case EXTBTN_PA1:
			{

				uint8_t cnt[4] = {0x22,0x21,0x20,0x19};
				W25QXXX_MemoryWrite(101 , 4, cnt);
				uint8_t data[4] = {0};
				W25QXXX_MemoryRead(101, 4, data);
				printf("data %02X%02X%02X%02X\n", data[0], data[1], data[2], data[3]);

				/*
				em4100TagData_t tagData;

				tagData.data[0] = 0x0A;
				tagData.data[1] = 0x00;
				tagData.data[2] = 0x56;
				tagData.data[3] = 0x5D;
				tagData.data[4] = 0x3B;

				EM4100_RWE_WritterInit(T5557_T5577);
				EM4100_RWE_WritterWriteTag(TAG_DEBUG, &tagData);
				*/

				/*
				//uniData[4] ++;
				OneWireSlaveDS1990_OffWire();
				while (OneWireSlaveDS1990_OnWire(uniData))
				{
					osDelay(1);
				}
				osDelay(50);

				uint8_t res = OneWireMasterReadROM(&ds1990Data);
			 	if (res == OWM_RS_OK)
			 	{
			    	printf("%02X ", ds1990Data.romData[0]);
			    	printf("%02X ", ds1990Data.romData[6]);
			    	printf("%02X ", ds1990Data.romData[5]);
			    	printf("%02X ", ds1990Data.romData[4]);
			    	printf("%02X ", ds1990Data.romData[3]);
			    	printf("%02X ", ds1990Data.romData[2]);
			    	printf("%02X ", ds1990Data.romData[1]);
			    	printf("%02X\n", ds1990Data.romData[7]);
			   	}
			   	else
			   	{
			    	printf("%02X ", ds1990Data.romData[0]);
			    	printf("%02X ", ds1990Data.romData[6]);
			    	printf("%02X ", ds1990Data.romData[5]);
			    	printf("%02X ", ds1990Data.romData[4]);
			    	printf("%02X ", ds1990Data.romData[3]);
			    	printf("%02X ", ds1990Data.romData[2]);
			    	printf("%02X ", ds1990Data.romData[1]);
			    	printf("%02X\n", ds1990Data.romData[7]);
			    	printf("Read error code %02X\n", res);
			   	}

				break;
				*/
/*
				taskENTER_CRITICAL();
					uint8_t res = OneWireMasterRW1990_WriteROM(&uniData);
				taskEXIT_CRITICAL();

			 	if (res != OWM_RS_OK)
			 	{
			 		printf("Write error code %02X\n", res);
			 	}
			 	else
			 		printf("Write done! \n", res);

				break;
				*/

				break;

			}
			default:
				break;
			}

		}

		osThreadYield();

	}
}


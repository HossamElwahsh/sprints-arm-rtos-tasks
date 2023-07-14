/*
 * FreeRTOS Kernel V10.2.0
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/* 
	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used.
*/


/*
 * Creates all the demo application tasks, then starts the scheduler.  The WEB
 * documentation provides more details of the demo application tasks.
 * 
 * Main.c also creates a task called "Check".  This only executes every three 
 * seconds but has the highest priority so is guaranteed to get processor time.  
 * Its main function is to check that all the other tasks are still operational.
 * Each task (other than the "flash" tasks) maintains a unique count that is 
 * incremented each time the task successfully completes its function.  Should 
 * any error occur within such a task the count is permanently halted.  The 
 * check task inspects the count of each task to ensure it has changed since
 * the last time the check task executed.  If all the count variables have 
 * changed all the tasks are still executing error free, and the check task
 * toggles the onboard LED.  Should any task contain an error at any time 
 * the LED toggle rate will change from 3 seconds to 500ms.
 *
 */

/* Standard includes. */
#include <stdlib.h>
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "lpc21xx.h"
#include "semphr.h"

/* Peripheral includes. */
#include "serial.h"
#include "GPIO.h"

/*-----------------------------------------------------------*/

/* Constants to setup I/O and processor. */
#define mainBUS_CLK_FULL	( ( unsigned char ) 0x01 )

/* Constants for the ComTest demo application tasks. */
#define mainCOM_TEST_BAUD_RATE	( ( unsigned long ) 115200 )

#define APP_PRI_DEFAULT	0
#define APP_PRI_LOW 		1
#define APP_PRI_MED 		2
#define APP_PRI_HIGH 		3

#define APP_TASK_1_MS_DELAY 100
#define APP_TASK_2_MS_DELAY 500


#define APP_HEAVY_LOAD_CYCLES 10000

// helper macros
#define NULL_PTR ((void *)0)

typedef struct{
	const signed 		char 	* 	cs_str_msg;
				unsigned 	short 		u8_msg_length;
}st_task_msg_data_t;

/*
 * Configure the processor for use with the Keil demo board.  This is very
 * minimal as most of the setup is managed by the settings in the project
 * file.
 */
static void prvSetupHardware( void );
static void task_uart_write_100ms(void * pvParameters);
static void task_uart_write_500ms(void * pvParameters);
/*-----------------------------------------------------------*/

SemaphoreHandle_t gl_semaphore_handle_uart_mutex;

/*
 * Application entry point:
 * Starts all the other tasks, then starts the scheduler. 
 */
int main( void )
{
	st_task_msg_data_t st_task_msg_data_l_task_100ms = {
	(const signed char *)"task 1 hello\n", 
		13
	};
	
	st_task_msg_data_t st_task_msg_data_l_task_500ms = {
	(const signed char *)"task 2 hi\n", 
		10
	};
	
	
	/* Setup the hardware for use with the Keil demo board. */
	prvSetupHardware();

	// create UART Mutex
	gl_semaphore_handle_uart_mutex = xSemaphoreCreateMutex();
	
	/* Create Tasks here */
	// uart 100ms writer
	xTaskCreate(
		task_uart_write_100ms						,	// pvTaskCode		:	Task Function
		"uart-1"												,	// pcName				:	Task Friendly Name
		configMINIMAL_STACK_SIZE				,	// usStackDepth	:	number of words for task stack size
		&st_task_msg_data_l_task_100ms	,	// pvParameters	: A value that is passed as the paramater to the created task.
		APP_PRI_DEFAULT									,	// uxPriority		:	The priority at which the created task will execute.
		NULL											 				// [out] task handle
	);
	
	
	// uart 500ms writer
	xTaskCreate(
		task_uart_write_500ms						,	// pvTaskCode		:	Task Function
		"uart-2"												,	// pcName				:	Task Friendly Name
		configMINIMAL_STACK_SIZE				,	// usStackDepth	:	number of words for task stack size
		&st_task_msg_data_l_task_500ms	,	// pvParameters	: A value that is passed as the paramater to the created task.
		APP_PRI_DEFAULT									,	// uxPriority		:	The priority at which the created task will execute.
		NULL											 				// [out] task handle
	);
	

	/* Now all the tasks have been started - start the scheduler.

	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used here. */
	vTaskStartScheduler();

	/* Should never reach here!  If you do then there was not enough heap
	available for the idle task to be created. */
	for( ;; );
}
/*-----------------------------------------------------------*/


static void task_uart_write_100ms(void * pvParameters)
{
	st_task_msg_data_t l_st_task_msg_data;
	unsigned short u8_l_write_loop_counter;
	
	for(;;)
	{
		// param check
		if(NULL_PTR != pvParameters)
		{
			// proper cast
			l_st_task_msg_data = ( * (st_task_msg_data_t * ) pvParameters);
			
			// send message 10 times on UART if available
			
			// try capture UART Mutex
			; // block until taken
			if(pdTRUE == xSemaphoreTake(gl_semaphore_handle_uart_mutex, portMAX_DELAY))
			{
				// write on UART				
				for(u8_l_write_loop_counter = 0; u8_l_write_loop_counter < 10; u8_l_write_loop_counter++)
				{
					while(pdFALSE == vSerialPutString(l_st_task_msg_data.cs_str_msg, l_st_task_msg_data.u8_msg_length));
				}
				
				// give mutex back
				xSemaphoreGive(gl_semaphore_handle_uart_mutex);
			}
			
		}
		else
		{
			// Bad Param
		}
		
		vTaskDelay(APP_TASK_1_MS_DELAY); // delay 100ms
	}
	
	// undefined behavior fail-safe
	vTaskDelete(NULL);
}

static void task_uart_write_500ms(void * pvParameters)
{
	st_task_msg_data_t l_st_task_msg_data;
	unsigned short u8_l_write_loop_counter;
	int u32_l_dummy_load_counter;
	
	for(;;)
	{
		// param check
		if(NULL_PTR != pvParameters)
		{
			// proper cast
			l_st_task_msg_data = ( * (st_task_msg_data_t * ) pvParameters);
			
			// send message 10 times on UART if available
			
			// try capture UART Mutex
			; // block until taken
			if(pdTRUE == xSemaphoreTake(gl_semaphore_handle_uart_mutex, portMAX_DELAY))
			{
				// write on UART 10 times
				for(u8_l_write_loop_counter = 0; u8_l_write_loop_counter < 10; u8_l_write_loop_counter++)
				{
					while(pdFALSE == vSerialPutString(l_st_task_msg_data.cs_str_msg, l_st_task_msg_data.u8_msg_length));
					
					// heavy load simulator per every write
					for(u32_l_dummy_load_counter = APP_HEAVY_LOAD_CYCLES; u32_l_dummy_load_counter > 0; u32_l_dummy_load_counter--)
					{
						// dummy heavy load
					}
				}
				
				// give mutex back
				xSemaphoreGive(gl_semaphore_handle_uart_mutex);
			}
			
		}
		else
		{
			// Bad Param
		}
		vTaskDelay(APP_TASK_2_MS_DELAY); // delay 500ms
	}
	
	// undefined behavior fail-safe
	vTaskDelete(NULL);
}

/* Function to reset timer 1 */
void timer1Reset(void)
{
	T1TCR |= 0x2;
	T1TCR &= ~0x2;
}

/* Function to initialize and start timer 1 */
static void configTimer1(void)
{
	T1PR = 1000;
	T1TCR |= 0x1;
}

static void prvSetupHardware( void )
{
	/* Perform the hardware setup required.  This is minimal as most of the
	setup is managed by the settings in the project file. */

	/* Configure UART */
	xSerialPortInitMinimal(mainCOM_TEST_BAUD_RATE);

	/* Configure GPIO */
	GPIO_init();
	
	/* Config trace timer 1 and read T1TC to get current tick */
	configTimer1();

	/* Setup the peripheral bus to be the same as the PLL output. */
	VPBDIV = mainBUS_CLK_FULL;
}
/*-----------------------------------------------------------*/



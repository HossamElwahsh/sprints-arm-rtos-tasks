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
#include <string.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "lpc21xx.h"
#include "queue.h"
//#include "semphr.h"
//#include "event_groups.h"

/* Peripheral includes. */
#include "serial.h"
#include "GPIO.h"

/*-----------------------------------------------------------*/

/* Constants to setup I/O and processor. */
#define mainBUS_CLK_FULL	( ( unsigned char ) 0x01 )

/* Constants for the ComTest demo application tasks. */
#define mainCOM_TEST_BAUD_RATE	( ( unsigned long ) 115200 )

/* APP CONFIG */
#define BTN_1_PORT				PORT_0
#define BTN_1_PIN					PIN0
#define BTN_2_PORT				PORT_0
#define BTN_2_PIN					PIN1

#define BTN_MS_DEBOUNCE 50

#define APP_UART_PERIODIC_WRITER_MS_DELAY 100
#define APP_UART_STR_MAX_LEN 							20
#define APP_UART_QUEUE_LENGTH 						20

// helper macros
#define NULL_PTR ((void *)0)

#define APP_PRI_DEFAULT	0
#define APP_PRI_LOW 		1
#define APP_PRI_MED 		2
#define APP_PRI_HIGH 		3

// App Queue Messages Flags (max 0xFF)
#define APP_NOTIF_BTN_1_PRESSED 	0x01
#define APP_NOTIF_BTN_1_RELEASED 	0x02
#define APP_NOTIF_BTN_2_PRESSED 	0x03
#define APP_NOTIF_BTN_2_RELEASED 	0x04
#define APP_NOTIF_PERIODIC_PRINT	0x05

#define APP_MSG_BTN_1_PRESSED 	"Button 1 pressed\n"
#define APP_MSG_BTN_1_RELEASED 	"Button 1 released\n"
#define APP_MSG_BTN_2_PRESSED 	"Button 2 pressed\n"
#define APP_MSG_BTN_2_RELEASED 	"Button 2 released\n"
#define APP_MSG_PERIODIC				"periodic %d\n"

/* Typedefs */

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
static void task_btn_1_handler(void * pvParameters);
static void task_btn_2_handler(void * pvParameters);
static void task_uart_consumer(void *pvParameters);
static void task_uart_periodic_writer(void * pvParameters);

// Notifiers Functions Prototypes
static void notify_uart_consumer(unsigned short u8_a_notification);
/*-----------------------------------------------------------*/

/* Global Variables */
TaskHandle_t gl_TaskHandle_uart_consumer;
//SemaphoreHandle_t gl_semaphore_handle_uart_mutex;
//EventGroupHandle_t gl_evt_grp_hndl_buttons;
QueueHandle_t gl_uart_queue;

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
	//gl_semaphore_handle_uart_mutex = xSemaphoreCreateMutex();
	
	/* Tasks Creation */
	
	// uart periodic writer
	xTaskCreate(
		task_uart_periodic_writer	,	// pvTaskCode		:	Task Function
		"uart-wrt"								,	// pcName				:	Task Friendly Name
		configMINIMAL_STACK_SIZE	,	// usStackDepth	:	number of words for task stack size
		NULL_PTR									,	// pvParameters	: A value that is passed as the paramater to the created task.
		APP_PRI_DEFAULT						,	// uxPriority		:	The priority at which the created task will execute.
		NULL												// [out] task handle
	);
	
	// btn 1 handler
	xTaskCreate(
		task_btn_1_handler				,	// pvTaskCode		:	Task Function
		"btn1hnd"									,	// pcName				:	Task Friendly Name
		configMINIMAL_STACK_SIZE	,	// usStackDepth	:	number of words for task stack size
		NULL											,	// pvParameters	: A value that is passed as the paramater to the created task.
		APP_PRI_DEFAULT 					,	// uxPriority		:	The priority at which the created task will execute.
		NULL												// [out] task handle
	);
	
	// btn 2 handler
	xTaskCreate(
		task_btn_2_handler				,	// pvTaskCode		:	Task Function
		"btn2hnd"									,	// pcName				:	Task Friendly Name
		configMINIMAL_STACK_SIZE	,	// usStackDepth	:	number of words for task stack size
		NULL											,	// pvParameters	: A value that is passed as the paramater to the created task.
		APP_PRI_DEFAULT 					,	// uxPriority		:	The priority at which the created task will execute.
		NULL												// [out] task handle
	);
	
	// uart consumer
	xTaskCreate(
		task_uart_consumer					,	// pvTaskCode		:	Task Function
		"uart-csr"									,	// pcName				:	Task Friendly Name
		configMINIMAL_STACK_SIZE		,	// usStackDepth	:	number of words for task stack size
		NULL												,	// pvParameters	: A value that is passed as the paramater to the created task.
		APP_PRI_DEFAULT 						,	// uxPriority		:	The priority at which the created task will execute.
		&gl_TaskHandle_uart_consumer	// [out] task handle
	);
	
	// Uart Queue Creation
	gl_uart_queue = xQueueCreate( APP_UART_QUEUE_LENGTH			,	// uxQueueLength	: The maximum number of items the queue can hold at any one time.
																sizeof(unsigned short)			// uxItemSize 		:	The size, in bytes, required to hold each item in the queue.
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

static void task_uart_periodic_writer(void * pvParameters)
{
	for(;;)
	{
		// notify uart consumer every APP_UART_PERIODIC_WRITER_MS_DELAY
		notify_uart_consumer(APP_NOTIF_PERIODIC_PRINT);
		vTaskDelay(APP_UART_PERIODIC_WRITER_MS_DELAY); // delay
	}
	
	// undefined behavior fail-safe
	vTaskDelete(NULL);
}


static void task_uart_consumer(void * pvParameters)
{
	unsigned short u8_l_notification_id;
	unsigned short u8_l_periodic_msg_num; // periodic msg number to print
	unsigned char str_l_msg[APP_UART_STR_MAX_LEN] = {'0'};
	
	// init
	u8_l_notification_id = 0;
	u8_l_periodic_msg_num = 0;
	
	for(;;)
	{
		if(pdTRUE == xQueueReceive(gl_uart_queue, &u8_l_notification_id, portMAX_DELAY))
		{
			// new notification dequeued, parse
			
			// btn 1 pressed
			if(APP_NOTIF_BTN_1_PRESSED == u8_l_notification_id)
			{
				sprintf((char *)str_l_msg, APP_MSG_BTN_1_PRESSED);
			}
			
			// btn 2 pressed
			else if(APP_NOTIF_BTN_2_PRESSED == u8_l_notification_id)
			{
				sprintf((char *)str_l_msg, APP_MSG_BTN_2_PRESSED);
			}
			
			// btn 1 released
			else if(APP_NOTIF_BTN_1_RELEASED == u8_l_notification_id)
			{
				sprintf((char *)str_l_msg, APP_MSG_BTN_1_RELEASED);
			}
			
			// btn 2 released
			else if(APP_NOTIF_BTN_2_RELEASED == u8_l_notification_id)
			{
				sprintf((char *)str_l_msg, APP_MSG_BTN_2_RELEASED);
			}
			
			// periodic (100ms) msg print
			else if(APP_NOTIF_PERIODIC_PRINT == u8_l_notification_id)
			{
				// update buffer to be printed with "periodic %num%"
				sprintf((char *)str_l_msg, APP_MSG_PERIODIC, u8_l_periodic_msg_num);
				u8_l_periodic_msg_num = ((u8_l_periodic_msg_num + 1) % 10); // increment %num%
			}
			
			// print msg to UART
			while(pdFALSE == vSerialPutString((char *)str_l_msg, strlen((char *)str_l_msg))); // block until print suceeds
		}
		
	} // end for
	
	// undefined behavior fail-safe
	vTaskDelete(NULL);
}


/**
 * @brief                       :   Button handler, detects rising and falling edge
 *
 * @param[in]   pvParameters    :   Task Parameters
 *
 */
static void task_btn_1_handler(void *pvParameters)
{
	const TickType_t tickType_l_check_ms_delay = 5; // every 5 ticks = 5ms
	
	//notify_led_handler(NULL); // initial notification
	
    /* Task Loop */
    for (;;)
    {
			// check button state
			if(PIN_IS_HIGH == GPIO_read(BTN_1_PORT, BTN_1_PIN))
			{
				// notify rising edge (pressed)
				notify_uart_consumer(APP_NOTIF_BTN_1_PRESSED);
				
				// debounce delay
				vTaskDelay(BTN_MS_DEBOUNCE);
				
				// recheck if button is released
				while(PIN_IS_HIGH == GPIO_read(BTN_1_PORT, BTN_1_PIN)) // wait until btn is released
				{
					vTaskDelay(tickType_l_check_ms_delay);
				}
			
				// button released, notify falling edge (released)
				notify_uart_consumer(APP_NOTIF_BTN_1_RELEASED);
			}
			else
			{
				// button is not pressed
				// Do Nothing
			}
    }

    // control should reach here, if reached delete task to avoid
    // undefined behaviours
    vTaskDelete(NULL);
}


/**
 * @brief                       :   Button handler, detects rising and falling edge
 *
 * @param[in]   pvParameters    :   Task Parameters
 *
 */
static void task_btn_2_handler(void *pvParameters)
{
	const TickType_t tickType_l_check_ms_delay = 5; // every 5 ticks = 5ms
	
	//notify_led_handler(NULL); // initial notification
	
    /* Task Loop */
    for (;;)
    {
			// check button state
			if(PIN_IS_HIGH == GPIO_read(BTN_2_PORT, BTN_2_PIN))
			{
				// notify rising edge (pressed)
				notify_uart_consumer(APP_NOTIF_BTN_2_PRESSED);
				
				// debounce delay
				vTaskDelay(BTN_MS_DEBOUNCE);
				
				// recheck if button is released
				while(PIN_IS_HIGH == GPIO_read(BTN_2_PORT, BTN_2_PIN)) // wait until btn is released
				{
					vTaskDelay(tickType_l_check_ms_delay);
				}
			
				// button released, notify falling edge (released)
				notify_uart_consumer(APP_NOTIF_BTN_2_RELEASED);
			}
			else
			{
				// button is not pressed
				// Do Nothing
			}
    }

    // control should reach here, if reached delete task to avoid
    // undefined behaviours
    vTaskDelete(NULL);
}

static void notify_uart_consumer(unsigned short u8_a_notification)
{
	 xQueueSend(
								gl_uart_queue,
								&u8_a_notification,
								0
						 );
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



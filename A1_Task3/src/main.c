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

/* Peripheral includes. */
#include "serial.h"
#include "GPIO.h"

/* Lib includes */
#include "std.h"

/*-----------------------------------------------------------*/

/* Constants to setup I/O and processor. */
#define mainBUS_CLK_FULL	( ( unsigned char ) 0x01 )

/* Constants for the ComTest demo application tasks. */
#define mainCOM_TEST_BAUD_RATE	( ( unsigned long ) 115200 )

/* Macros */
#define LED_PORT    		PORT_0
#define LED_PIN     		PIN1

#define BTN_PORT				PORT_0
#define BTN_PIN					PIN0
#define BTN_MS_DEBOUNCE 50

#define LED_MS_DELAY_400	400
#define LED_MS_DELAY_100	100

#define BTN_MS_PRESS_FOR_400	2000
#define BTN_MS_PRESS_FOR_100	4000

#define APP_NOTIF_DELAY_400 0x01
#define APP_NOTIF_DELAY_100 0x02
#define APP_NOTIF_STOP			0x03

/* Global Variables */
TaskHandle_t gl_TaskHandle_led_toggle_task_400; // every 400ms (press longer than 2 seconds)
TaskHandle_t gl_TaskHandle_led_toggle_task_100; // every 100ms (press longer than 4 seconds)
TaskHandle_t gl_TaskHandle_led_handler;
TaskHandle_t gl_TaskHandle_btn_handler;

/*
 * Configure the processor for use with the Keil demo board.  This is very
 * minimal as most of the setup is managed by the settings in the project
 * file.
 */
static void prvSetupHardware( void );

// Tasks Functions Prototypes
static void led_toggle_task(void * pvParameters);
static void led_handler_task(void *pvParameters);
static void btn_handler_task(void *pvParameters);

// Notifiers Functions Prototypes
static void notify_led_handler(uint32_t u32_a_notification);
/*-----------------------------------------------------------*/

/*
 * Application entry point:
 * Starts all the other tasks, then starts the scheduler. 
 */
int main( void )
{
	uint32_t u32_delay_400ms = LED_MS_DELAY_400;
	uint32_t u32_delay_100ms = LED_MS_DELAY_100;
	
	/* Setup the hardware for use with the Keil demo board. */
	prvSetupHardware();

	
	/* Create Tasks here */
	
	/* Handlers Tasks*/
	
	// led handler
	xTaskCreate(
		led_handler_task					,	// pvTaskCode		:	Task Function
		"led-hnd"									,	// pcName				:	Task Friendly Name
		configMINIMAL_STACK_SIZE	,	// usStackDepth	:	number of words for task stack size
		NULL											,	// pvParameters	: A value that is passed as the paramater to the created task.
		PRI_HIGH									,	// uxPriority		:	The priority at which the created task will execute.
		&gl_TaskHandle_led_handler	// [out] task handle
	);
	
	// btn handler
	xTaskCreate(
		btn_handler_task					,	// pvTaskCode		:	Task Function
		"btn-hnd"									,	// pcName				:	Task Friendly Name
		configMINIMAL_STACK_SIZE	,	// usStackDepth	:	number of words for task stack size
		NULL											,	// pvParameters	: A value that is passed as the paramater to the created task.
		PRI_HIGH									,	// uxPriority		:	The priority at which the created task will execute.
		&gl_TaskHandle_btn_handler	// [out] task handle
	);
	
	/* LED TOGGLING TASKS */
	// led 400ms toggle task
	xTaskCreate(
		led_toggle_task						,	// pvTaskCode		:	Task Function
		"tog400ms"								,	// pcName				:	Task Friendly Name
		configMINIMAL_STACK_SIZE	,	// usStackDepth	:	number of words for task stack size
		&u32_delay_400ms					,	// pvParameters	: A value that is passed as the paramater to the created task.
		PRI_HIGH									,	// uxPriority		:	The priority at which the created task will execute.
		&gl_TaskHandle_led_toggle_task_400 // [out] task handle
	);
	
	// led 100ms toggle task
	xTaskCreate(
		led_toggle_task						,	// pvTaskCode		:	Task Function
		"tog100ms"								,	// pcName				:	Task Friendly Name
		configMINIMAL_STACK_SIZE	,	// usStackDepth	:	number of words for task stack size
		&u32_delay_100ms					,	// pvParameters	: A value that is passed as the paramater to the created task.
		PRI_HIGH									,	// uxPriority		:	The priority at which the created task will execute.
		&gl_TaskHandle_led_toggle_task_100 // [out] task handle
	);
	
	// set led toggle tasks initially to suspended state
	vTaskSuspend(gl_TaskHandle_led_toggle_task_400);
	vTaskSuspend(gl_TaskHandle_led_toggle_task_100);

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

/**
 * @brief                       :   Led Handler Task Function, handles led state
 *
 * @param[in]   pvParameters    :   Task Parameters
 *
 */
static void btn_handler_task(void *pvParameters)
{
	uint32_t u32_press_ms_duration;
	//TickType_t xLastWakeTime;
	//const TickType_t xFrequency = 5; // every 5 ticks = 5ms
	const uint32_t uint32_check_ms_delay = 5; // every 5 ticks = 5ms
	
	// Initialise the xLastWakeTime variable with the current time.
  //xLastWakeTime = xTaskGetTickCount();
	
    /* Task Loop */
    for (;;)
    {
			// check button state
			if(PIN_IS_HIGH == GPIO_read(BTN_PORT, BTN_PIN))
			{
				// debounce delay
				vTaskDelay(BTN_MS_DEBOUNCE);
				u32_press_ms_duration = BTN_MS_DEBOUNCE; // reset delay
				
				// recheck if button is released
				while(PIN_IS_HIGH == GPIO_read(BTN_PORT, BTN_PIN)) // wait until btn is released
				{
					// Wait for the next cycle.
					//vTaskDelayUntil( &xLastWakeTime, xFrequency );
					vTaskDelay(uint32_check_ms_delay);
					u32_press_ms_duration = GET_MAX(u32_press_ms_duration, u32_press_ms_duration + uint32_check_ms_delay); // prevents overflow of press duration tracking
				}
				
				// button released, check duration and notify led handler task accordingly
				if(BTN_MS_PRESS_FOR_400 > u32_press_ms_duration) // btn pressed for less than 2 seconds
				{
					// led off
					notify_led_handler(APP_NOTIF_STOP);
				}
				else if((BTN_MS_PRESS_FOR_400 <= u32_press_ms_duration) && (BTN_MS_PRESS_FOR_100 > u32_press_ms_duration)) // btn pressed for more than 2 seconds & less than 4 seconds
				{
					// led on with 400ms
					notify_led_handler(APP_NOTIF_DELAY_400);
				}
				else if(BTN_MS_PRESS_FOR_100 <= u32_press_ms_duration) // btn pressed for more than 4 seconds
				{
					// led on with 100ms
					notify_led_handler(APP_NOTIF_DELAY_100);
				}
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
 * @brief                       :   Led Handler Task Function, handles led state
 *
 * @param[in]   pvParameters    :   Task Parameters
 *
 */
static void led_handler_task(void *pvParameters)
{
	uint32_t uint32_notification;
	BaseType_t BaseType_notify_wait_result;
	
    /* Task Loop */
    for (;;)
    {
			BaseType_notify_wait_result = 
			xTaskNotifyWait(
				0x00									, // Don't clear bits on entry
				MAX_32_BIT_VAL				, // Clear Bits on exit
				&uint32_notification	, // notification value
				portMAX_DELAY					 	// ticks to wait in blocked state for notification
			);
			
			if(pdTRUE == BaseType_notify_wait_result)
			{
				// switch led state/timing accordingly
				if(APP_NOTIF_DELAY_100 == uint32_notification)
				{
					// suspend 400ms task and resume 100ms
					vTaskSuspend(gl_TaskHandle_led_toggle_task_400);
					vTaskResume(gl_TaskHandle_led_toggle_task_100);
				}
				else if(APP_NOTIF_DELAY_400 == uint32_notification)
				{
					// suspend 100ms task and resume 400ms
					vTaskSuspend(gl_TaskHandle_led_toggle_task_100);
					vTaskResume(gl_TaskHandle_led_toggle_task_400);
				}
				else if(APP_NOTIF_STOP == uint32_notification)
				{
					// suspend all delay tasks
					vTaskSuspend(gl_TaskHandle_led_toggle_task_100);
					vTaskSuspend(gl_TaskHandle_led_toggle_task_400);
					
					// turn off LED
					GPIO_write(LED_PORT, LED_PIN, PIN_IS_LOW);
				}
			}
    }

    // control should reach here, if reached delete task to avoid
    // undefined behaviours
    vTaskDelete(NULL);
}


/**
 * @brief                       :   Led Task Function, toggles an LED every 1 second
 *
 * @param[in]   pvParameters    :   Task Parameters
 *
 */
static void led_toggle_task(void *pvParameters)
{
	 //UBaseType_t uxHighWaterMark;

	/* Inspect our own high water mark on entering the task. */
	//uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
	uint32_t u32_l_delay;
	
    /* Task Loop */
    for (;;)
    {
			if(NULL_PTR != pvParameters)
			{
				u32_l_delay = * (uint32_t *) pvParameters;
				
				// Turn led on
        GPIO_write(LED_PORT, LED_PIN, PIN_IS_HIGH);
        vTaskDelay(u32_l_delay); // delay task
			
			
				//uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
			
				// Turn led off
        GPIO_write(LED_PORT, LED_PIN, PIN_IS_LOW);
        vTaskDelay(u32_l_delay); // delay task 
			}
    }

    // control should reach here, if reached delete task to avoid
    // undefined behaviours
   vTaskDelete(NULL);
    
}

static void notify_led_handler(uint32_t u32_a_notification)
{
	xTaskNotify(
		gl_TaskHandle_led_handler		, // Task to notify
		u32_a_notification					, // Notification value
		eSetValueWithOverwrite	 			// notify type
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



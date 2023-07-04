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

#include "std.h"

/*-----------------------------------------------------------*/

/* Constants to setup I/O and processor. */
#define mainBUS_CLK_FULL	( ( unsigned char ) 0x01 )

/* Constants for the ComTest demo application tasks. */
#define mainCOM_TEST_BAUD_RATE	( ( unsigned long ) 115200 )


/* Macros */
// LED Ports
#define LED_1_PORT PORT_0
#define LED_2_PORT PORT_0
#define LED_3_PORT PORT_0

// LED Pins 
#define LED_1_PIN		PIN1
#define LED_2_PIN		PIN2
#define LED_3_PIN		PIN3

// LED Delays
#define LED_1_DELAY	100
#define LED_2_DELAY	500
#define LED_3_DELAY	1000

/* Private Types */
typedef struct
{
	portX_t 	portX_led_port;
	pinX_t		pinX_led_pin;
	uint16_t 	uint16_delay;
}st_led_task_param_t;

/*
 * Configure the processor for use with the Keil demo board.  This is very
 * minimal as most of the setup is managed by the settings in the project
 * file.
 */
static void prvSetupHardware( void );
static void led_toggle_task(void * pvParameters);
/*-----------------------------------------------------------*/


/*
 * Application entry point:
 * Starts all the other tasks, then starts the scheduler. 
 */
int main( void )
{
	// tasks parameters
	st_led_task_param_t st_l_led_1_params = 
	{
		LED_1_PORT,
		LED_1_PIN,
		LED_1_DELAY
	};
	
	st_led_task_param_t st_l_led_2_params = 
	{
		LED_2_PORT,
		LED_2_PIN,
		LED_2_DELAY
	};
  
	st_led_task_param_t st_l_led_3_params = 
	{
		LED_3_PORT,
		LED_3_PIN,
		LED_3_DELAY
	}; 
	
	/* Setup the hardware for use with the Keil demo board. */
	prvSetupHardware();
	
  /* Create Tasks here */
	// Led 1 task
	xTaskCreate( 
		led_toggle_task						,	// pvTaskCode		:	Task Function 
		"led1tog"									,	// pcName				:	Task Friendly Name 
		configMINIMAL_STACK_SIZE	,	// usStackDepth	:	number of words for task stack size 
		&st_l_led_1_params		 		,	// pvParameters	: A value that is passed as the paramater to the created task. 
		PRI_HIGH									,	// uxPriority		:	The priority at which the created task will execute. 
		NULL // [out] task handle 
	); 
	
	// Led 2 task
	xTaskCreate( 
		led_toggle_task						,	// pvTaskCode		:	Task Function 
		"led2tog"									,	// pcName				:	Task Friendly Name 
		configMINIMAL_STACK_SIZE	,	// usStackDepth	:	number of words for task stack size 
		&st_l_led_2_params		 		,	// pvParameters	: A value that is passed as the paramater to the created task. 
		PRI_HIGH									,	// uxPriority		:	The priority at which the created task will execute. 
		NULL // [out] task handle 
	); 
	
	// Led 1 task
	xTaskCreate( 
		led_toggle_task						,	// pvTaskCode		:	Task Function 
		"led3tog"									,	// pcName				:	Task Friendly Name 
		configMINIMAL_STACK_SIZE	,	// usStackDepth	:	number of words for task stack size 
		&st_l_led_3_params		 		,	// pvParameters	: A value that is passed as the paramater to the created task. 
		PRI_HIGH									,	// uxPriority		:	The priority at which the created task will execute. 
		NULL // [out] task handle 
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

static void led_toggle_task(void * pvParameters)
{
	for(;;)
	{
		uint16_t u16_l_delay;
		portX_t portX_l_led_port;
		pinX_t	pinX_l_led_pin;
		
		// args check
		if(NULL_PTR != pvParameters)
		{
			portX_l_led_port	= ((st_led_task_param_t *)pvParameters)->portX_led_port;
			pinX_l_led_pin 		= ((st_led_task_param_t *)pvParameters)->pinX_led_pin;
			u16_l_delay 			= ((st_led_task_param_t *)pvParameters)->uint16_delay;
			
			// delay check
			if(u16_l_delay > 0 && u16_l_delay <= 2000)
			{
				// Turn led on 
        GPIO_write(portX_l_led_port, pinX_l_led_pin, PIN_IS_HIGH); 
        vTaskDelay(u16_l_delay); // delay led toggle 
			 
				// Turn led off 
        GPIO_write(portX_l_led_port, pinX_l_led_pin, PIN_IS_LOW);				
				vTaskDelay(u16_l_delay); // delay led toggle
			}
		}
	}
	
	// undefined behavior handling, deleting task
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



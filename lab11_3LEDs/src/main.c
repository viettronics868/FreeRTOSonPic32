/*******************************************************************************\
 the idea of the lab is inspired by the original lab at 
 * https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Tutorials.git
 * and transferring the lab onto the Curiosity 2.0 pic32 mz ef board.
 Author: Nam_K Pham_L. 

  File Name:
    main.c
 * 
 * Setup:
 *	Board: Curiosity 2.0 PIC32MZ EF development board
	Inputs: SW1, SW2, SW3
	Outputs: built-in LEDs (LED1/RED and LED2/GREEN and LED3/YELLOW)
	Lab objects:
 *		DMA + callback
 *		UART6 TX 
 *		FreeRTOS + configuration
 *		Mutex
 *		Binary Semaphore
 *		Software timers + callback
 *		ISR on SWx pins + callback
 *	 Applying Technique:
 *		debounce 50ms on key press
 *		function pointers
  Summary:
   A software timer in FreeRTOS is a mechanism that allows you to execute a callback function 
 * at a specific time in the future, or periodically, without creating a dedicated task to wait for that time.. 
 * these lines of code is implemented/verified and software is released for educational only.

  Description:
    This file contains the "main" function for a project.  The
    "main" function calls the "SYS_Initialize" function to initialize the state
    machines of all modules in the system. Debug messages are showed via UART6. 
 * DMA module is using to make the task/CPU unblock and transmission continues in background.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include "definitions.h"                // SYS function prototypes

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "device_cache.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define SW_PRESS_STATE 0

//declare static variable for identifying LEDs
//0 for Red; 1 for Green; 2 for Yellow
static uint8_t whichLED = 0;

//declare global variable for interval of timing
static uint16_t interval = 1000;

static uint8_t debounce = 50;

static uint8_t count = 0;

//declare buffer for software static timer
static StaticTimer_t xTimerRedBuffer;
static StaticTimer_t xSW1TimerDebounceBuffer;
static StaticTimer_t xSW2TimerDebounceBuffer;
static StaticTimer_t xSW3TimerDebounceBuffer;

//declare timers
static TimerHandle_t xIntervalTimer;
static TimerHandle_t xSW1DebounceTimer;
static TimerHandle_t xSW2DebounceTimer;
static TimerHandle_t xSW3DebounceTimer;

//declare protectors
SemaphoreHandle_t xMutex;
SemaphoreHandle_t xBinarySema;

//declare timer callbacks
static void prvTimerCallbackFunction(TimerHandle_t xTimer);
static void prvSW1DebounceCallbackFunction(TimerHandle_t xTimer);
static void prvSW2DebounceCallbackFunction(TimerHandle_t xTimer);
static void prvSW3DebounceCallbackFunction(TimerHandle_t xTimer);

//declare led toggle functions
static void vRedToggle(void){
	LED_RED_Toggle();
}
static void vGreenToggle(void){
	LED_GREEN_Toggle();
}
static void vYellowToggle(void){
	LED_YELLOW_Toggle();
}

//declare an array of function pointers
static void (*fp_Led[3])(void) = {vRedToggle, vGreenToggle, vYellowToggle};

//declare buffer for UART6
static uint8_t __attribute__ ((aligned (16))) u6TxBuffer[100] = {0};

//declare a function to display the debug messages
static void Debug_msg(char * msg){
	while (*msg){
		while (U6STAbits.UTXBF);
		U6TXREG=*msg++;
		while (!U6STAbits.TRMT);
	}
}

//declare dmac callback function
static void U6D0Callback(DMAC_TRANSFER_EVENT event, uintptr_t context){
	if (event == DMAC_TRANSFER_EVENT_COMPLETE){
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(xBinarySema, &xHigherPriorityTaskWoken);
		portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
	}
}

//declare ISR Callback of SW1, SW2, SW3 and debounce 50ms
static void SW1Callback(GPIO_PIN pin, uintptr_t context){
	if (SW1_Get() == SW_PRESS_STATE){
		count = 0;
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xTimerResetFromISR(xSW1DebounceTimer, &xHigherPriorityTaskWoken);
		portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
	}
}

static void SW2Callback(GPIO_PIN pin, uintptr_t context){
	if (SW2_Get() == SW_PRESS_STATE){
		count = 0;
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xTimerResetFromISR(xSW2DebounceTimer, &xHigherPriorityTaskWoken);
		portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
	}
}

static void SW3Callback(GPIO_PIN pin, uintptr_t context){
	if (SW3_Get() == SW_PRESS_STATE){
		count = 0;
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xTimerResetFromISR(xSW3DebounceTimer, &xHigherPriorityTaskWoken);
		portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
	}
}

//declare the function of application initialize
static void APP_Initialize(void){
	
	//initialize LED
	LED_RED_Clear();
	LED_GREEN_Clear();
	LED_YELLOW_Clear();
	
	//initialize dmac
	DMAC_ChannelCallbackRegister(
				DMAC_CHANNEL_0,
				U6D0Callback,
				0);
	
	//initialize interrupt at pin SWx
	GPIO_PinInterruptCallbackRegister(
				SW1_PIN,
				SW1Callback,
				0);
	GPIO_PinInterruptEnable(SW1_PIN);
	
	GPIO_PinInterruptCallbackRegister(
				SW2_PIN,
				SW2Callback,
				0);
	GPIO_PinInterruptEnable(SW2_PIN);
	
	GPIO_PinInterruptCallbackRegister(
				SW3_PIN,
				SW3Callback, 0);
	GPIO_PinInterruptEnable(SW3_PIN);
}



// *****************************************************************************
// *****************************************************************************
// Section: Main Entry Point
// *****************************************************************************
// *****************************************************************************

int main ( void )
{
    /* Initialize all modules */
    SYS_Initialize ( NULL );

    Debug_msg("lab11-software timer in LEDs blinking\r\n");
    
    APP_Initialize();
    
    //create mutex
    xMutex = xSemaphoreCreateMutex();
    //create binary semaphore
    xBinarySema = xSemaphoreCreateBinary();
    
    //create static timers
    xIntervalTimer = xTimerCreateStatic(
			"interval timer",
			pdMS_TO_TICKS(interval),
			pdFALSE,
			(void *)1,
			prvTimerCallbackFunction,
			&xTimerRedBuffer);
    if (xIntervalTimer == NULL){ 
		Debug_msg("cannot create the timer \r\n");
		return (EXIT_FAILURE);
		}
    
    xSW1DebounceTimer = xTimerCreateStatic(
			"debounce SW1 50ms",
			pdMS_TO_TICKS(debounce),
			pdFALSE,
			(void *)2,
			prvSW1DebounceCallbackFunction,
			&xSW1TimerDebounceBuffer
			);
    if (xSW1DebounceTimer == NULL){
		Debug_msg("cannot create debounce timer for SW1 \r\n");
		return (EXIT_FAILURE);
		}
    
    xSW2DebounceTimer = xTimerCreateStatic(
			"debounce SW2 50ms",
			pdMS_TO_TICKS(debounce),
			pdFALSE,
			(void *)3,
			prvSW2DebounceCallbackFunction,
			&xSW2TimerDebounceBuffer
			);
    if (xSW2DebounceTimer == NULL){
		Debug_msg("cannot create debounce timer for SW2 \r\n");
		return(EXIT_FAILURE);
		}
    
    xSW3DebounceTimer = xTimerCreateStatic(
			"debounce SW3 50ms",
			pdMS_TO_TICKS(debounce),
			pdFALSE,
			(void *)4,
			prvSW3DebounceCallbackFunction,
			&xSW3TimerDebounceBuffer
			);
    if (xSW3DebounceTimer == NULL){
		Debug_msg("cannot create debounce timer for SW3 \r\n");
		return (EXIT_FAILURE);
		}
    
    //start auto-reload timer
    if (xTimerStart(
	    xIntervalTimer,
	    0) == pdFAIL) {
		Debug_msg("cannot start the timer \r\n");
		return (EXIT_FAILURE);
		}
    
    
    vTaskStartScheduler();
    
    while ( true )
    {
        /* Maintain state machines of all polled MPLAB Harmony modules. */
        SYS_Tasks ( );
    }

    /* Execution should not come here during normal operation */

    return ( EXIT_FAILURE );
}

static void prvTimerCallbackFunction(TimerHandle_t xTimer){
	(void)xTimer;
	
	if (count < 15){
		for (uint8_t led=0; led<3; led++){
			if (led == whichLED){
				fp_Led[led]();
				count++;
				//show a message on com port
				if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){
					sprintf((char *)u6TxBuffer, "toggle the led %d at interval of %d in %d times \r\n", 
								whichLED+1,
								interval, 
								count);
					DCACHE_CLEAN_BY_ADDR(
								(uint32_t)u6TxBuffer,
								strlen((const char *)u6TxBuffer));
					DMAC_ChannelTransfer(
							DMAC_CHANNEL_0,
							(const void *)u6TxBuffer,
							strlen((const char *)u6TxBuffer),
							(const void *)&U6TXREG, 1, 1);				
					xSemaphoreTake(xBinarySema, portMAX_DELAY);
					xSemaphoreGive(xMutex);
				}			
				break;
			}
		}
		if (whichLED < 2){
			whichLED++;//prepare the next LED
		} else {
			whichLED = 0; //loop back to the first LED
		}
		//reset timer for toggling the next led
		if (xTimerStart( 
			xIntervalTimer,
			0) == pdFAIL){
				Debug_msg("cannot reset timer \r\n");
				exit(EXIT_FAILURE);
				}
	}
}

static void prvSW1DebounceCallbackFunction(TimerHandle_t xTimer){
	if (SW1_Get() == SW_PRESS_STATE){
		interval = 2000;
		LED_RED_Clear();
		LED_GREEN_Clear();
		LED_YELLOW_Clear();
		xTimerChangePeriod(
			xIntervalTimer, 
			pdMS_TO_TICKS(interval),
			0);
	}
}

static void prvSW2DebounceCallbackFunction(TimerHandle_t xTimer){
	if (SW2_Get() == SW_PRESS_STATE){
		interval = 500;
		LED_RED_Clear();
		LED_GREEN_Clear();
		LED_YELLOW_Clear();
		xTimerChangePeriod(
			xIntervalTimer,
			pdMS_TO_TICKS(interval),
			0);
	}
}

static void prvSW3DebounceCallbackFunction(TimerHandle_t xTimer){
	if (SW3_Get() == SW_PRESS_STATE){
		interval = 250;
		LED_RED_Clear();
		LED_GREEN_Clear();
		LED_YELLOW_Clear();
		xTimerChangePeriod(
			xIntervalTimer,
			pdMS_TO_TICKS(interval),
			0);
	}
}

/*******************************************************************************
 End of File
*/


/*******************************************************************************\
 the idea of the lab is inspired by the original lab at 
 * https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Tutorials.git
 * and transferring the lab onto the Curiosity 2.0 pic32 mz ef board.
 Author: L.K. 

  File Name:
    main.c
 * 
 * Setup:
 *	Board: Curiosity 2.0 PIC32MZ EF development board
	Inputs: SW1, SW2, SW3
	Outputs: LED1, LED2, LED3
	Lab objects:
 *		DMA
 *		UART6 TX
 *		FreeRTOS
 *		Event Group - Mutex
 *		Static Tasks 
 *		ISR on pushbutton and DMAC transfer complete

  Summary:
    Event Group is an important concept in FreeRTOS task synchronization. An Event Group is a collection of bits (like flags).
 * Each bit can represent an event (e.g., ?sensor ready,? ?UART complete,? ?button pressed?). When the event occur and condition 
 * is met (bits set), the task unblocks.
 * . these lines of code is implemented/verified and software is released for educational only

  Description:
 * The lab is for blinking LEDs. if a key is pressed, the period of blinking is changed
    This file contains the "main" function for a project.  The "main" function calls the "SYS_Initialize" function to initialize the state
    machines of all modules in the system, and call LAB15_Initialize function to initialize the modules of the application. 
 * Debug messages are showed via UART6.  DMA module is using to make the task/CPU unblock and transmission continues in background.
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
#include "semphr.h"
#include "device_cache.h"
#include "timers.h"
#include "event_groups.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define KEY_PRESS_STATE 0
#define DEBOUNCE 50
#define RED_INTERVAL_SLOW 3000
#define RED_INTERVAL_FAST 1500
#define RED_INTERVAL_FASTER 700
#define RED_INTERVAL_FASTEST 200
#define RED_INDEX 4

static uint32_t red_interval[RED_INDEX] = {	RED_INTERVAL_SLOW,
				RED_INTERVAL_FAST,
				RED_INTERVAL_FASTER,
				RED_INTERVAL_FASTEST};

//declare variable to count RED_INDEX
static uint8_t red_count = 0;

//declare variables for task RED
static StaticTask_t xTaskRedBuffer;
static StackType_t xTaskRedTcbBuffer[configMINIMAL_STACK_SIZE];
static void prvTaskRedFunction(void * pvParams);

//declare synchronization primitives
static SemaphoreHandle_t xMutex;

//declare buffer for UART6 TX
static uint8_t __attribute__ ((aligned (16)))u6TxBuffer[128] = {0};

//declare software timer for debounce 50ms key press
static TimerHandle_t xDebounceRed;
static StaticTimer_t xDebounceRedBuffer;
static void prvDebounceRedFunction(TimerHandle_t xTimer);

//declare software timer for blinking LED
static TimerHandle_t xBlinkingRed;
static StaticTimer_t xBlinkingRedBuffer;
static void prvBlinkingRedFunction(TimerHandle_t xTimer);

//define event group bit 1 for key 1
#define BIT_PERIOD_RED (1U << 0)
//define event group bit 2 for UART6 completed
#define BIT_UART6_COMPLETE (1U<<1)

//declare event group
static EventGroupHandle_t xEveGr;
static StaticEventGroup_t xEveGrBuffer;

//declare callbak for ISR of key press
static void SW1_ISR_Callback(GPIO_PIN pin, uintptr_t context){
	if (SW1_Get() == KEY_PRESS_STATE){
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xTimerResetFromISR(xDebounceRed, &xHigherPriorityTaskWoken);
		portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
	}
}

//declare callback for UART transfer 
static void U6D0Callback(DMAC_TRANSFER_EVENT event, uintptr_t contextHandle){
	if (event == DMAC_TRANSFER_EVENT_COMPLETE){
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xEventGroupSetBitsFromISR(xEveGr, BIT_UART6_COMPLETE, &xHigherPriorityTaskWoken );
		portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
	}
}

//declare function for debug message
static void Debug_msg(char * msg){
	while (*msg){
		while (U6STAbits.UTXBF);
		U6TXREG = *msg++;
		while (!U6STAbits.TRMT);
	}
}

//create LAB15 initialization
static void LAB15_App(void){
	
	//turn RED on
	LED1_Clear();
	
	Debug_msg("lab15-Event Group-push a button for changing period of blinking LED \r\n");
		
	char redMsg[64];
	sprintf(redMsg,"   RED started blinking at period of %d ms \r\n", red_interval[red_count & (RED_INDEX-1)]);
	Debug_msg(redMsg);

	//register callback for gpio ISR
	GPIO_PinInterruptCallbackRegister(
				SW1_PIN,
				SW1_ISR_Callback,
				0);
	GPIO_PinInterruptEnable(SW1_PIN);
	
	//register callback for DMA
	DMAC_ChannelCallbackRegister(
			DMAC_CHANNEL_0,
			U6D0Callback,
			0);
	
	//create software timer for debounce - one-shot
	xDebounceRed = xTimerCreateStatic(
				"debounce 50ms",
				pdMS_TO_TICKS(DEBOUNCE),
				pdFALSE,
				(void *)1,
				prvDebounceRedFunction,
				&xDebounceRedBuffer);
	if (xDebounceRed == NULL){
		Debug_msg("cannot create timer for debounce \r\n");
		exit(EXIT_FAILURE);
	}
	
	//create software timer for blinking LED - auto reload
	xBlinkingRed = xTimerCreateStatic(
				"blinking RED ",
				pdMS_TO_TICKS(red_interval[red_count & (RED_INDEX-1)]),
				pdTRUE,
				(void *)2,
				prvBlinkingRedFunction,
				&xBlinkingRedBuffer);
	if (xBlinkingRed == NULL){
		Debug_msg("cannot create timer for blinking\r\n");
		exit(EXIT_FAILURE);
	} else {
		if (xTimerStart(xBlinkingRed, 0) == pdFALSE){
			Debug_msg("cannot start Timer for blinking \r\n");
			exit(EXIT_FAILURE);
		}
	}
	
	//create a new event group for UART6 transfer
	xEveGr = xEventGroupCreateStatic(&xEveGrBuffer);
	
	//create a task that wait for BIT_UART6_TRANSFER
	if (xTaskCreateStatic(
		prvTaskRedFunction,
		"show on com port",
		configMINIMAL_STACK_SIZE,
		NULL,
		tskIDLE_PRIORITY,
		xTaskRedTcbBuffer,
		&xTaskRedBuffer) == NULL){
				Debug_msg("cannot create task for com port\r\n");
				exit(EXIT_FAILURE);
				}
	//create mutex
	xMutex = xSemaphoreCreateMutex();
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

    LAB15_App();
    
    vTaskStartScheduler();
    
    while ( true )
    {
        /* Maintain state machines of all polled MPLAB Harmony modules. */
        SYS_Tasks ( );
    }

    /* Execution should not come here during normal operation */

    return ( EXIT_FAILURE );
}

//after debounce 50ms , start a software timer to blink RED
static void prvDebounceRedFunction(TimerHandle_t xTimer){
	(void) xTimer;
	if (SW1_Get() == KEY_PRESS_STATE){
		red_count++;
		
		//set BIT_PERIOD_RED to enable show message on com p ort
		xEventGroupSetBits(
				xEveGr,
				BIT_PERIOD_RED);
		//change period of blinking
		if (xTimerChangePeriod(
			xBlinkingRed,
			pdMS_TO_TICKS(red_interval[red_count & (RED_INDEX-1)]),//wrap around 0, 1, 2, 3
			0) == pdFAIL){
				Debug_msg("cannot change timer period .. \r\n");
				exit(EXIT_FAILURE);
				}
		if (red_count == 4) red_count = 0;//prevent undefine behavior of out of uint8_t
	}
}

//Timer callback for blinking RED
static void prvBlinkingRedFunction(TimerHandle_t xTimer){
	(void) xTimer;
	LED1_Toggle();
}

//a task function to show message on com port
static void prvTaskRedFunction(void * Params){
	(void ) Params;
	for (;;){
		if (xEventGroupWaitBits(//waiting for BIT_PERIOD_RED to start to show message on com port
				xEveGr,
				BIT_PERIOD_RED,
				pdTRUE,
				pdFALSE,
				portMAX_DELAY) == pdTRUE)
		{
			if (xSemaphoreTake(xMutex, pdMS_TO_TICKS(100)))
			{
				sprintf((char *)u6TxBuffer, "  Mayday Mayday RED blinking period %d ms\r\n", 
						red_interval[red_count & (RED_INDEX-1)]);
				DCACHE_CLEAN_BY_ADDR(
					(uint32_t) u6TxBuffer,
					strlen((const char *) u6TxBuffer));
				DMAC_ChannelTransfer(
					DMAC_CHANNEL_0,
					(const void *) u6TxBuffer,
					strlen((const char *)u6TxBuffer),
					(const void *)&U6TXREG, 1, 1);
				xEventGroupWaitBits( //waiting for BIT_UART6_COMPLETE for transfer event complete
					xEveGr,
					BIT_UART6_COMPLETE,
					pdTRUE,
					pdFALSE,
					portMAX_DELAY); 

				xSemaphoreGive(xMutex);
			}			
		}
	}	
}
/*******************************************************************************
 End of File
*/


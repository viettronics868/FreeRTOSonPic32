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
	Inputs: SW1, SW2, SW3, SW4
	Outputs: LED1, LED2, LED3, LED RGB
	Lab objects:
 *		DMA
 *		UART6 TX
 *		FreeRTOS
 *		Task Notification  - Mutex
 *		Static Tasks 
 *		ISR on pushbutton
 *		DMAC transfer and xTaskGetCurrentTaskHandle() to register itself.

  Summary:
   Task Notification is  a "inter-task communication"  in FreeRTOS task . in Task Control Block of the task has a built-in 32-bit 
 * notification field. That means every task has a 32-bit value for storing number, flag or counter and a 'pending' state.
 *  these lines of code is implemented/verified and software is released for educational only

  Description:
 * The lab implement the complete safe pattern (mutex + notification ). 
 * if key press happened, the LED will be toggled and the message will show on com port.
    This file contains the "main" function for a project.  
 * The "main" function calls the "SYS_Initialize" function to initialize the state machines of all modules in the system, and create tasks . 
 * 
 * the files Lab17_DMA.c and Lab17_DMA.h are helper functions
 * the files Lab17_UART6.c and Lab17_UART6.h are showing debug messages on com port
 * the files Lab17_Timers.c and Lab17_Timers.h are for timer-related functions
 * the files Lab17_STAmac.c and Lab17_STAmac.h are for the task of state machine core logic
 * the files Lab17_SWxISR.c and Lab17_SWxISR.h are for the ISR from switches
 * the file Lab17_gpio.h are for the inline functions that wrapped around the MCC macros
 * 
 * Debug messages or notifications are showed via UART6.  DMA module is using to make the task/CPU unblock and transmission continues in background.
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
#include "semphr.h"		// using mutex
#include "timers.h"		//using debounce timers and blinking timers
#include "device_cache.h"
#include "event_groups.h"		//setting bits for Office Admin task
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "Lab17_STAmac.h"
#include "Lab17_DMA.h"
#include "Lab17_UART6.h"
#include "Lab17_Timers.h"
#include "Lab17_SWxISR.h"

TaskHandle_t xTaskSTAHandle;

//declare mutual exclusive
SemaphoreHandle_t xMutex;

static StaticTask_t xSTAmacTcbBuffer;

static StackType_t xSTAmacStack[configMINIMAL_STACK_SIZE];


//declare application of lab 17
static void vLab17_Init(void){
	
	//turn on all leds
	LED0_Clear();
	LED1_Clear();
	LED2_Clear();
	LED_R_Clear();
	LED_G_Clear();
	LED_B_Clear();
	
	//create task Lab17 state machine core and assign task handle
	xTaskSTAHandle = xTaskCreateStatic(
				vLab17STAmac,
				"state machine of lab 17",
				configMINIMAL_STACK_SIZE,
				NULL,
				tskIDLE_PRIORITY,
				xSTAmacStack,
				&xSTAmacTcbBuffer);
	if (xTaskSTAHandle == NULL)
	{
		Debug_msg("cannot create task for state machine ...\r\n");
		exit(EXIT_FAILURE);
	}
		
	//create mutex
	xMutex = xSemaphoreCreateMutex();
	
	Lab17_DMAInit();
	Lab17_TimersInit();
	Lab17_SWxISRInit();
	
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
    
    vLab17_Init();
     
    vTaskStartScheduler();

    while ( true )
    {
        /* Maintain state machines of all polled MPLAB Harmony modules. */
        SYS_Tasks ( );
    }

    /* Execution should not come here during normal operation */

    return ( EXIT_FAILURE );
}

/*******************************************************************************
 End of File
*/


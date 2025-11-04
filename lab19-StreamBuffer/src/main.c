/*******************************************************************************\
 the idea of the lab is inspired by the original lab at 
 * https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Tutorials.git
 * and transferring the lab onto the Curiosity 2.0 pic32 mz ef board (pic32mz2048efm144)
 Author: L.K. 

  File Name:
    main.c
 * 
 * Setup:
 *	Board: Curiosity 2.0 PIC32MZ EF development board
	Inputs: UART6 RX via MPLAB Data Visualizer terminal
	Outputs: LED0, LED1, LED2, LED RGB, UART1 TX via Putty terminal
	Lab objects:
 *		Stream Buffer
 *		UART1 RX / UART6 TX
 *		FreeRTOS
 *		Binary Semaphore  - Mutex
 *		Static Task for the core state machine 
 *		Timers and Timer ISR callback
 *		DMA and DMA FIFO ISR callback
 

  Summary:
 * Stream Buffer is the lightweight communication primitive that is a FIFO byte pipe used for continuous streams of bytes and no 
 * message boundaries.  
 *  these lines of code is implemented/verified and software is released for educational only

  The Conceptual Architecture:
 * 
 * [ UART6 RX Hardware FIFO ]
         | (via UART6CallbackRegister)
    UART6_RxCallback()       <-  fires on RX threshold (UART6 FIFO ISR) or idle timeout (Timer ISR)
         |
    [ Stream Buffer (xStreamBufferCreateStatic) ]
         |
    UART6 Echo Task (prints chars to UART6 TX in real time)
         |
    Detect Enter ("\n") -> forward full message to UART1 TX
 * 
 * *****************************************
 * The lab implement the complete safe pattern (mutex + binary semaphore ). 
 *The user should enter a text message on UART6 RX via MPLAB Data Visualizer terminal 
 * and mark the end of the message by Enter key and the message will show on UART1 TX via Putty.
    This file contains the "main" function for a project.  
 * The "main" function calls the "SYS_Initialize" function to initialize the state machines of all modules in the system, and create tasks . 
 * 
 * the files Lab19_DMA.c and Lab19_DMA.h are for DMA1 and DMA6
 * the files Lab19_UART.c and Lab19_UART.h are for inputting and outputting messages via UART1 TX and UART6 RX
 * the files Lab19_Timers.c and Lab19_Timers.h are for timer-related functions that include to flush partial FIFO data 
 * the files Lab19_STAmac.c and Lab19_STAmac.h are for the task of detecting Enter key and send the whole message to UART1 TX
 * the files Lab19_STREAMBUFFER.c / .h create a static stream buffer and control block to avoid heap allocation (ideal for embedded/medical devices) 
 * the files Lab19_config.c / .h are for the pattern "define once, extern everywhere else"
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
#include "Lab19_STA.h"
#include "Lab19_DMA.h"
#include "Lab19_UART.h"
#include "Lab19_Timer.h"
#include "Lab19_STREAMBUFFER.h"
#include "task.h"
#include "semphr.h"

SemaphoreHandle_t xMutex;
SemaphoreHandle_t xSemBin;

void Lab19_Initialize(void){
	
	
	LED_B_Clear();
	LED_G_Clear();
	LED_R_Clear();
	
	xMutex = xSemaphoreCreateMutex();
	xSemBin = xSemaphoreCreateBinary();
	
	vLab19_STA_init();
	vLab19_DMA0_init();
	vLab19_DMA1_init();
	vLab19_DMA6_init();
	vLab19_UART6_init();
	vLab19_Timer_init();
	vLab19_StrmBuff_init();
	
		
	
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

    Lab19_Initialize();
    
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


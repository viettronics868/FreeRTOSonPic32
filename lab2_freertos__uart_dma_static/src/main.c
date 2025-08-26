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
	Inputs: N/A
	Outputs: built-in debug via com port/UART6
	Labs objects:
		DMA
		Static Task (prvTaskFunction)
 *		UART TX
 *		FreeRTOS

  Summary:
    Task are the primary components of FreeRTOS. They can be dynamic tasks or static tasks. 
 * these lines of code is implemented/verified and software is released for educational only


  Description:
    This file contains the "main" function for a project.  The
    "main" function calls the "SYS_Initialize" function to initialize the state
    machines of all modules in the system
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include <stdlib.h>
#include <string.h>

#include "definitions.h"                // SYS function prototypes
#include "device_cache.h"
#include "FreeRTOS.h"
#include "task.h"

static StackType_t xTaskStackBuffer[configMINIMAL_STACK_SIZE];
static StaticTask_t xTaskTCBBuffer;

static volatile bool isUARTTxComplete = true;
static uint8_t __attribute__ ((aligned (16) )) uartTxBuffer[100 ] = {0};

static void prvTaskFunction(void *pvParams);

static void UARTDmaChannelHandler(DMAC_TRANSFER_EVENT event, uintptr_t contextHandle){
    if (event == DMAC_TRANSFER_EVENT_COMPLETE){
        isUARTTxComplete = true;
    }
}


// ****..*************************************************************************
// ***.**************************************************************************
// Section: Main Entry Point
// *****************************************************************************
// *****************************************************************************

int main ( void )
{
    /* Initialize all modules */
    SYS_Initialize ( NULL );
    
    //configure UART6 and DMA0
    DMAC_ChannelCallbackRegister(
                                                        DMAC_CHANNEL_0, 
                                                        UARTDmaChannelHandler,
                                                        0);
    const char *hello_msg = "lab2- UART6 with DMA0 \r\n";
    const char *task_msg = "cannot create task \r\n";
    
    strcpy((char *) uartTxBuffer, hello_msg);
    DCACHE_CLEAN_BY_ADDR(
                                                        (uint32_t) uartTxBuffer, 
                                                        strlen((const char *) uartTxBuffer));
    isUARTTxComplete = false;
    DMAC_ChannelTransfer(
                                            DMAC_CHANNEL_0, 
                                            (const void *) uartTxBuffer, 
                                            strlen((const char* ) uartTxBuffer), 
                                            (const void *) &U6TXREG, 
                                            1, 
                                            1);
    
    uint32_t timeout = 1000000000;
    while (!isUARTTxComplete && timeout--){
        __asm__("nop");
    }
    
    TaskHandle_t xTaskCreateResult = NULL;
    xTaskCreateResult = xTaskCreateStatic(
                                                                    prvTaskFunction,
                                                                    "Task1",
                                                                    configMINIMAL_STACK_SIZE,
                                                                    NULL,
                                                                    tskIDLE_PRIORITY,
                                                                    (xTaskStackBuffer),
                                                                    &(xTaskTCBBuffer));
    //handle the case of failure of create static task
    if (xTaskCreateResult == NULL){
        strcpy((char *) uartTxBuffer, task_msg);
        DCACHE_CLEAN_BY_ADDR(
                                                            (uint32_t) uartTxBuffer,
                                                            strlen((const char *) uartTxBuffer));
        isUARTTxComplete = false;
        DMAC_ChannelTransfer(
                                                DMAC_CHANNEL_0,
                                                (const void *) uartTxBuffer,
                                                strlen((const char *) uartTxBuffer),
                                                (const void *) &U6TXREG,
                                                1,
                                                1);
	return(EXIT_FAILURE);
    } 

    { vTaskStartScheduler();}
    
   
    
    while ( true )
    {
        /* Maintain state machines of all polled MPLAB Harmony modules. */
        SYS_Tasks ( );
    }

    /* Execution should not come here during normal operation */

    return ( EXIT_FAILURE );
}

//print a message on com port via DMA
static void prvTaskFunction(void * pvParams){
    
    (void ) pvParams;
        
    for (uint8_t tsk = 0; tsk < 5; tsk++) {   
            const char * greet_msg = " Tutorial 2 task 1 is running ... \r\n";
            strcpy((char *) uartTxBuffer, greet_msg);
            DCACHE_CLEAN_BY_ADDR(
                                                                (uint32_t) uartTxBuffer,
                                                                strlen((const char *) uartTxBuffer));
            isUARTTxComplete = false;
            DMAC_ChannelTransfer(
                                                    DMAC_CHANNEL_0,
                                                    (const void *) uartTxBuffer,
                                                    strlen((const char *) uartTxBuffer),
                                                    (const void *)&U6TXREG,
                                                    1,
                                                    1);

            vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/*******************************************************************************
 End of File
*/


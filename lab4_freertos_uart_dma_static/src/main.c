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
 *		Mutex
 *		Binary Semaphore

  Summary:
    Task are the primary components of FreeRTOS. They can be dynamic tasks or static tasks. 
 * these lines of code is implemented/verified and software is released for educational only


  Description:
    This file contains the "main" function for a project.  The "main" function calls the "SYS_Initialize" function to 
 * initialize the state machines of all modules in the system. Debug messages are showed via UART6. 
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
#include <stdint.h>
#include <stdio.h>

#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "device_cache.h"
#include "semphr.h"
#include <xc.h>

static StackType_t xTaskHighTCBBuffer[configMINIMAL_STACK_SIZE];
static StackType_t xTaskLowTCBBuffer[configMINIMAL_STACK_SIZE];

static StaticTask_t xTaskHighStaticBuffer;
static StaticTask_t xTaskLowStaticBuffer;

static void prvTaskHighFunction(void * pvParams);
static void prvTaskLowFunction(void * pvParams);

static SemaphoreHandle_t xU6D2Mutex = NULL;
static SemaphoreHandle_t xU6D2Bin = NULL;

static uint8_t __attribute__ ((aligned (16))) U6TxBuffer[100] = {0};

volatile bool u6d2Error = false;
volatile bool u6d2None = false;
volatile bool u6d2Half = false;

static void U6D2Handler(DMAC_TRANSFER_EVENT event, uintptr_t contextHandle){
    if (event == DMAC_TRANSFER_EVENT_COMPLETE){        
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(xU6D2Bin, &xHigherPriorityTaskWoken);
        portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);        
    } 
    else if ( event == DMAC_TRANSFER_EVENT_ERROR){
        u6d2Error = true;
    } 
    else if (event == DMAC_TRANSFER_EVENT_NONE)
    {
        u6d2None = true;
    } 
        else u6d2Half = true;
}
//debug messages is written directly to U6TXREG
static void Debug_msg(char * msg){
	while (*msg){
		while (U6STAbits.UTXBF);
		U6TXREG= *msg++;
		while (!U6STAbits.TRMT);
	}
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
    
    Debug_msg("Lab04-FreeRTOS on UART6 via DMA2\r\n");
   
    xU6D2Mutex = xSemaphoreCreateMutex();
    xU6D2Bin = xSemaphoreCreateBinary();

     DMAC_ChannelCallbackRegister(
                                                                DMAC_CHANNEL_2,
                                                                U6D2Handler,
                                                                0);
    //create Task High - statically 
    TaskHandle_t xTaskStaticResult = NULL;
    xTaskStaticResult = xTaskCreateStatic(
                                                                prvTaskHighFunction,
                                                                "Task High",
                                                                configMINIMAL_STACK_SIZE,
                                                                NULL,
                                                                tskIDLE_PRIORITY,
                                                                 //0,
                                                                 (xTaskHighTCBBuffer),
                                                                 &(xTaskHighStaticBuffer));
    if (xTaskStaticResult == NULL){
	    Debug_msg("cannot create task \r\n");
    }
    
    //create Task Low
    xTaskStaticResult = xTaskCreateStatic(
                                                                prvTaskLowFunction,
                                                                "Task Low",
                                                                configMINIMAL_STACK_SIZE,
                                                                NULL,
                                                                tskIDLE_PRIORITY,
                                                                //1,
                                                                (xTaskLowTCBBuffer),
                                                                &(xTaskLowStaticBuffer));
    if (xTaskStaticResult == NULL){
	    Debug_msg("cannot create the task\r\n");
    }
    
    vTaskStartScheduler();
    while (1)
    {
        /* Maintain state machines of all polled MPLAB Harmony modules. */
        SYS_Tasks ( );
    }

    /* Execution should not come here during normal operation */

    return ( EXIT_FAILURE );
}

static void prvTaskHighFunction(void * pvParams){
    (void ) pvParams;
    for (uint8_t high=0; high < 5; high++){
                if (xSemaphoreTake(xU6D2Mutex, portMAX_DELAY) == pdTRUE)
                {
                    sprintf((char *)U6TxBuffer, "   task High take mutex and running ..\r\n");                   
                    DCACHE_CLEAN_BY_ADDR(  
                                                                        (uint32_t) U6TxBuffer,
                                                                        strlen((const char *) U6TxBuffer));                
                    DMAC_ChannelTransfer(
                                                            DMAC_CHANNEL_2,
                                                            (const void *) U6TxBuffer,
                                                            strlen((const char *) U6TxBuffer),
                                                            (const void *) &U6TXREG, 1, 1);
                
                    xSemaphoreTake(xU6D2Bin, portMAX_DELAY);

                    xSemaphoreGive(xU6D2Mutex);
                }
               
                vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

static void prvTaskLowFunction(void * pvParams){
    (void) pvParams;
    for (uint8_t low = 0; low < 5 ; low++){
                if (xSemaphoreTake(xU6D2Mutex, portMAX_DELAY) == pdTRUE)
                {                  
                    sprintf((char *)U6TxBuffer, "   task Low take mutex and running ..\r\n");
                    DCACHE_CLEAN_BY_ADDR(
                                                                        (uint32_t) U6TxBuffer,
                                                                        strlen((const char *) U6TxBuffer));                  
                    DMAC_ChannelTransfer(
                                                            DMAC_CHANNEL_2,
                                                            (const void *) U6TxBuffer,
                                                            strlen((const char *) U6TxBuffer),
                                                            (const void *) &U6TXREG, 1, 1);
                    xSemaphoreTake(xU6D2Bin, portMAX_DELAY);

                    xSemaphoreGive(xU6D2Mutex);
                }                
                 vTaskDelay(pdMS_TO_TICKS(2000));
    }
}


/*******************************************************************************
 End of File
*/


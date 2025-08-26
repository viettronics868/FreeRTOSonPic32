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
    Task are the primary components of FreeRTOS. A task can be passed parameter at creation.
 * The parameter can be custom data structure or a pointer or a data.
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

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "device_cache.h"
#include "semphr.h"
#include "task.h"

static StaticTask_t xTaskNPBuffer;
static StaticTask_t xTaskLKBuffer;

static StackType_t xTaskNPTcbBuffer[configMINIMAL_STACK_SIZE];
static StackType_t xTaskLKTcbBuffer[configMINIMAL_STACK_SIZE];

static void prvTaskNPFunctionStatic(void * pvParams);
static void prvTaskLKFunctionStatic(void * pvParams);

static uint8_t __attribute__ ((aligned (16))) u6TxBuffer[100] = {0};

static SemaphoreHandle_t xMutex = NULL;
static SemaphoreHandle_t xBinarySem = NULL;

static char * np = "NP_Msg";
static char * lk = "LK_Msg";

static uint8_t stackMsg[100] ={0};

static void U6D3Handler(DMAC_TRANSFER_EVENT event, uintptr_t contextHandle){
    if (event == DMAC_TRANSFER_EVENT_COMPLETE){
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(xBinarySem, &xHigherPriorityTaskWoken);
        portEND_SWITCHING_ISR( xHigherPriorityTaskWoken);
    }
}

static void Debug_msg(char * msg){
	while (*msg){
		while (U6STAbits.UTXBF);
		U6TXREG=*msg++;
		while (U6STAbits.TRMT);
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
    
    xMutex = xSemaphoreCreateMutex();
    xBinarySem = xSemaphoreCreateBinary();
    
    DMAC_ChannelCallbackRegister(
            DMAC_CHANNEL_3,
            U6D3Handler,
            0);
    
    Debug_msg("Lab5-FreeRTOS-passing pointer to task \r\n");

    TaskHandle_t xTskStaticResult = NULL;
    xTskStaticResult = xTaskCreateStatic(
            prvTaskNPFunctionStatic,
            "task NP",
            configMINIMAL_STACK_SIZE,
            (void *) np,
            tskIDLE_PRIORITY,
            (xTaskNPTcbBuffer),
            &(xTaskNPBuffer));
    
    if (xTskStaticResult == NULL){//handle Null pointer return
	    Debug_msg("cannot create task NP \r\n");
	    return(EXIT_FAILURE);
    }
    
    xTskStaticResult = xTaskCreateStatic(
            prvTaskLKFunctionStatic,
            "task LK",
            configMINIMAL_STACK_SIZE,
            (void *) lk,
            tskIDLE_PRIORITY,
            (xTaskLKTcbBuffer),
            &(xTaskLKBuffer));
    if (xTskStaticResult == NULL){//handle null pointer return
	    Debug_msg("cannot create task LK \r\n");
	    return(EXIT_FAILURE);
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

static void prvTaskNPFunctionStatic(void * pvParams){
 
    char *localNP = (char *)pvParams;
    
    for (uint8_t np=0; np< 5; np++){
        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){
            sprintf((char *) u6TxBuffer, "  task %s take mutex and is running ..\r\n", localNP);
            DCACHE_CLEAN_BY_ADDR(
                                                                (uint32_t)u6TxBuffer,
                                                                strlen((const char * ) u6TxBuffer));
            DMAC_ChannelTransfer(
                                                    DMAC_CHANNEL_3,
                                                    (const void *)u6TxBuffer,
                                                    strlen((const char *) u6TxBuffer),
                                                    (const void *) &U6TXREG, 1, 1);

            xSemaphoreTake(xBinarySem, portMAX_DELAY);

            xSemaphoreGive(xMutex);
        }
//monitoring real usage of taskNP's stack        
        UBaseType_t npHighWater = uxTaskGetStackHighWaterMark(NULL);
        sprintf((char *)stackMsg,"  task NP - stack free %u word\r\n", (unsigned int)npHighWater);
        char *ptrMsg = (char *)stackMsg;
	if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){
		Debug_msg(ptrMsg);
		xSemaphoreGive(xMutex);
	}

        vTaskDelay(pdMS_TO_TICKS(1000));
       
    }
}

static void prvTaskLKFunctionStatic(void * pvParams){
    
    char * localLK = (char *) pvParams;
    
    for (uint8_t lk= 0; lk< 5; lk++){
        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){
            sprintf((char *) u6TxBuffer, "    task %s takes mutex and running .. \r\n", localLK);
            DCACHE_CLEAN_BY_ADDR(
                                                                (uint32_t)u6TxBuffer,
                                                                strlen((const char *) u6TxBuffer));
            DMAC_ChannelTransfer(
                                                    DMAC_CHANNEL_3,
                                                    (const void *)u6TxBuffer,
                                                    strlen((const char *)u6TxBuffer),
                                                    (const void *) &U6TXREG, 1, 1);
            xSemaphoreTake(xBinarySem, portMAX_DELAY);

            xSemaphoreGive(xMutex);
        }
        
// monitoring real usage of taskLK's stack       
        UBaseType_t lkHighWater = uxTaskGetStackHighWaterMark(NULL);
        sprintf((char *)stackMsg, "    task LK - stack free: %u word\r\n", (unsigned int) lkHighWater);
        char *ptrMsg = (char *)stackMsg;
	if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){
		Debug_msg(ptrMsg);
		xSemaphoreGive(xMutex);
	}
        
        vTaskDelay(pdMS_TO_TICKS(1000));
        
    }
}


/*******************************************************************************
 End of File
*/


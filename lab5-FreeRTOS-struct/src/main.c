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
 *		Custom struct

  Summary:
    Task are the primary components of FreeRTOS. A task can be passed parameter at creation.
 * The parameter can be custom data structure like a struct .
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
#include "task.h"
#include "device_cache.h"
#include "semphr.h"

static StackType_t xTaskLKTcbBuffer[configMINIMAL_STACK_SIZE];
static StackType_t xTaskNPTcbBuffer[configMINIMAL_STACK_SIZE];

static StaticTask_t xTaskLKStaticBuffer;
static StaticTask_t xTaskNPStaticBuffer;

static void prvTaskLKFunction(void * pvParams);
static void prvTaskNPFunction(void * pvParams);

static SemaphoreHandle_t xMutex = NULL;
static SemaphoreHandle_t xBinarySem = NULL;

static uint8_t __attribute__ ((aligned (16))) u6TxBuffer[256] = {0};

typedef struct strParams_t{
                                        char msg[100];
                                        char name[20];
                                        bool status;
                                        unsigned int StackFree;
} strParams_t;

static strParams_t lkParams = {
                            .msg = "  we created this task for:",
                            .name = "L.K",
                            .status = pdTRUE,
                            .StackFree = 0};
static strParams_t npParams = {
                            .msg = "    they created that task for:",
                            .name = "N.P",
                            .status = pdTRUE,
                            .StackFree = 0};

static void U6D5Handler(DMAC_TRANSFER_EVENT event, uintptr_t contextHandle){
    if (event == DMAC_TRANSFER_EVENT_COMPLETE){
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(xBinarySem, &xHigherPriorityTaskWoken);
        portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
    }
}

static void Debug_msg(char * msg){
	while (*msg){
		while (U6STAbits.UTXBF);
		U6TXREG = *msg++;
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

    Debug_msg("Lab5-pass a struct to a task at creation \r\n");
        
    DMAC_ChannelCallbackRegister(
            DMAC_CHANNEL_5,
            U6D5Handler,
            0);
    
    xMutex =  xSemaphoreCreateMutex();
    xBinarySem = xSemaphoreCreateBinary();
    
    TaskHandle_t xTaskLKCreateResult = xTaskCreateStatic(
                                                                                            prvTaskLKFunction,
                                                                                            "task LK",
                                                                                            configMINIMAL_STACK_SIZE,
                                                                                            (void *)&lkParams, //pass struct pointer
                                                                                            tskIDLE_PRIORITY,
                                                                                            (xTaskLKTcbBuffer),
                                                                                            &(xTaskLKStaticBuffer));
    if(xTaskLKCreateResult == NULL){
	    Debug_msg("cannot create task LK \r\n");
	return (EXIT_FAILURE);
    }
    
    
    TaskHandle_t xTaskNPCreateResult = xTaskCreateStatic(
                                                                                            prvTaskNPFunction,
                                                                                            "task NP",
                                                                                            configMINIMAL_STACK_SIZE,
                                                                                            (void *)&npParams,  //pass  struct pointer
                                                                                            tskIDLE_PRIORITY,
                                                                                            (xTaskNPTcbBuffer),
                                                                                            &(xTaskNPStaticBuffer));
    if (xTaskNPCreateResult == NULL){
	    Debug_msg("cannot create task NP \r\n");
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

static void prvTaskLKFunction(void * pvParams){
    strParams_t * locallk = (strParams_t *) pvParams;
    for (uint8_t lk=0; lk < 5; lk++){
        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){
            locallk->status = (locallk->status == pdTRUE) ? pdFALSE : pdTRUE; //ternary operator
            sprintf((char *)u6TxBuffer, "%s %s and %d \r\n", locallk->msg, locallk->name, locallk->status);
            DCACHE_CLEAN_BY_ADDR(
                                                                (uint32_t)u6TxBuffer,
                                                                strlen((const char *) u6TxBuffer));
            DMAC_ChannelTransfer(
                                                    DMAC_CHANNEL_5,
                                                    (const void *)u6TxBuffer,
                                                    strlen((const char *)u6TxBuffer),
                                                    (const void *)&U6TXREG, 1, 1);
            
            xSemaphoreTake(xBinarySem, portMAX_DELAY);
            
            xSemaphoreGive(xMutex);
        }
        char lkFreeArray[50] = {0};
        BaseType_t lkStackFree = uxTaskGetStackHighWaterMark(NULL); // monitor stack usage of stack LK
        locallk->StackFree = (unsigned int)lkStackFree;
        char * lkFree = lkFreeArray;
        sprintf(lkFree, "  task LK has left %u words in stack \r\n ", locallk->StackFree);
	if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){
		Debug_msg(lkFree);
		xSemaphoreGive(xMutex);
	}
	
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void prvTaskNPFunction(void * pvParams){
    strParams_t * localnp = (strParams_t *)pvParams;
    
    for (uint8_t np=0; np < 5; np++){
        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){
            localnp->status = (localnp->status == pdTRUE) ? pdFALSE : pdTRUE; //ternary operator (? :)
            sprintf((char *) u6TxBuffer, "%s %s and %s \r\n", localnp->msg, localnp->name, 
                                                                                    (localnp->status == pdTRUE) ? "true" : "false");
            DCACHE_CLEAN_BY_ADDR (
                                                                    (uint32_t) u6TxBuffer,
                                                                    strlen((const char *) u6TxBuffer));
            DMAC_ChannelTransfer(
                                                    DMAC_CHANNEL_5,
                                                    (const void *) u6TxBuffer,
                                                    strlen((const char *)u6TxBuffer),
                                                    (const void *) &U6TXREG, 1, 1);
            
            xSemaphoreTake(xBinarySem, portMAX_DELAY);
            
            xSemaphoreGive(xMutex);
                    
        }
        char npArrayMsg[50] = {0};
        BaseType_t npStackFree = uxTaskGetStackHighWaterMark(NULL);//monitor stack usage of task NP
        localnp->StackFree = (unsigned int) npStackFree;
        char * npStackMsg = npArrayMsg;
        sprintf(npStackMsg, "    task NP has left %u words in stack \r\n", localnp->StackFree);
	if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){
		Debug_msg(npStackMsg);
		xSemaphoreGive(xMutex);
	}
	 
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


/*******************************************************************************
 End of File
*/


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
	Outputs: com port via Putty
	Lab objects:
 *		DMA
 *		UART6 TX
 *		FreeRTOS
 *		Priority Inversion - Priority Inheritance
 *		Static Tasks 
 *		configUSE_MUTEXES 1 : enable Priority Inheritance feature and enable xSemaphoreCreateMutex() macro
 *		uxTaskPriorityGet() built-in function

  Summary:
    Priority Inversion: A higher-priority task is forced to wait because a lower-priority task holds a resource (like a mutex).
If a medium-priority task runs in between, the higher-priority task is delayed longer than necessary.
 * Priority Inheritance: A solution where the low-priority task temporarily inherits the priority of the higher-priority task waiting for the resource.
 * . these lines of code is implemented/verified and software is released for educational only

  Description:
 * There are two tasks, task High and task Low. 
 * At first, task High yields to task Low and task Low occupies the mutex, but doesn't give.
 * Then task High waits for the mutex  and miss the deadline. Hence the scheduler activate Priority Inheritance built-in feature. 
 * The priority of task Low changes and is equal the priority of task High. 
 * After 6 counts, task Low gives the mutex and then is changed back to the original priority. The situation is end and everything is normal.
    This file contains the "main" function for a project.  The "main" function calls the "SYS_Initialize" function to initialize the state
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
#include "semphr.h"
#include "device_cache.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>

//declare static tasks and their variables
static StaticTask_t xTaskHighBuffer;
static StaticTask_t xTaskLowBuffer;

static StackType_t xTaskHighTcbBuffer[configMINIMAL_STACK_SIZE];
static StackType_t xTaskLowTcbBuffer[configMINIMAL_STACK_SIZE];

static void xTaskHighFunction (void * pvParams);//this task High will be blocked by the task Low
static void xTaskLowFunction (void * pvParams);// this task Low will have different priority level because of Priority Inheritance feature

//activate_inheritance is set to 1 will show Priority Inheritance feature 
//activate_inheritance is cleared to 0 will change priority level to original level
static uint8_t activate_inheritance = 1;

//declare the buffer for UART6
static uint8_t __attribute__ ((aligned (16))) u6TxBuffer[100] = {0};

//declare synchronization primitives
static SemaphoreHandle_t xMutex = NULL;
static SemaphoreHandle_t xBinarySema = NULL;

//declare the callback function of dmac transfer process
static void U6D0Callback(DMAC_TRANSFER_EVENT event, uintptr_t context){
	if (event == DMAC_TRANSFER_EVENT_COMPLETE){
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(xBinarySema, &xHigherPriorityTaskWoken);
		portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
	}
}

//declare a function to directly show debug message on UART6 
static void Debug_msg(char * msg){
	while (*msg){
		while (U6STAbits.UTXBF);
		U6TXREG = *msg++;
		while (!U6STAbits.TRMT);
	}
}

static void LAB14_Initialize(void){
	//register callback function of UART6 ISR
	DMAC_ChannelCallbackRegister(
				DMAC_CHANNEL_0,
				U6D0Callback,
				0);
	
	//create primitives
	xMutex = xSemaphoreCreateMutex();
	xBinarySema = xSemaphoreCreateBinary();
	
	//create task High - statically
	if (xTaskCreateStatic(
			xTaskHighFunction,
			"the task High",
			configMINIMAL_STACK_SIZE,
			NULL,
			tskIDLE_PRIORITY+3,
			xTaskHighTcbBuffer,
			&xTaskHighBuffer) == NULL){
		Debug_msg("cannot create task High \r\n");
		exit(EXIT_FAILURE);
	}
	
	//create task Low - statically
	if(xTaskCreateStatic(
			xTaskLowFunction,
			"the task Low",
			configMINIMAL_STACK_SIZE,
			NULL,
			tskIDLE_PRIORITY,
			xTaskLowTcbBuffer,
			&xTaskLowBuffer) == NULL){
		Debug_msg("cannot create task Low \r\n");
		exit(EXIT_FAILURE);
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
	
	Debug_msg("Lab14-Priority Inversion and Priority Inheritance on Pic32MZ \r\n");

	LAB14_Initialize();
	
	//start the scheduler
	vTaskStartScheduler();
    
    
    while ( true )
    {
        /* Maintain state machines of all polled MPLAB Harmony modules. */
        SYS_Tasks ( );
    }

    /* Execution should not come here during normal operation */

    return ( EXIT_FAILURE );
}

static void xTaskHighFunction (void * pvParams){
	(void)pvParams;

	for(;;)//this loop is for mainstream
	{
		//simulate that task High is yielding 
		vTaskDelay(pdMS_TO_TICKS(1000));
		//task High is blocking because of waiting for the mutex
		if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){
			sprintf((char *)u6TxBuffer, "Hello it's OK now, Task High is taking Mutex and running ..\r\n");
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
	}	
}

static void xTaskLowFunction(void * pvParams){
	(void)pvParams;
	for (;;){// this loop is for mainstream
		if (activate_inheritance ==1 ){
		//task Low takes the mutex and doesn't give. 
		//This kind of abuse will automatically activate Priority Inheritance feature
		//and the priority of task Low will be temporarily adjusted and is equal the priority of task High
			if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE)
			{		
				for(uint8_t low =0; low < 6; low++) //this loop is for 6 counts
				{						
					uxTaskPriorityGet(NULL) == tskIDLE_PRIORITY ? sprintf((char *)u6TxBuffer, "Opsss task Low is taking mutex and priority is %lu \r\n", uxTaskPriorityGet(NULL))   
									: sprintf((char *)u6TxBuffer, "	Mayday Mayday its priority is changed to %lu \r\n", uxTaskPriorityGet(NULL));
					DCACHE_CLEAN_BY_ADDR(
								(uint32_t)u6TxBuffer,
								strlen((const char *)u6TxBuffer));
					DMAC_ChannelTransfer(
							DMAC_CHANNEL_0,
							(const void *)u6TxBuffer,
							strlen((const char *)u6TxBuffer),
							(const void *)&U6TXREG, 1, 1);
					xSemaphoreTake(xBinarySema, portMAX_DELAY);
					vTaskDelay(pdMS_TO_TICKS(1000));
				}
				activate_inheritance = 0;//deactivate the situation
				xSemaphoreGive(xMutex);
			}
		} else {
			for (;;){ //this loop is for sub-stream
				if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){
					sprintf((char *)u6TxBuffer, "phusss OK now, task Low is taking mutex and  priority is  back to %lu \r\n", uxTaskPriorityGet(NULL));
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
				vTaskDelay(pdMS_TO_TICKS(1000));
			}
		}
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

/*******************************************************************************
 End of File
*/


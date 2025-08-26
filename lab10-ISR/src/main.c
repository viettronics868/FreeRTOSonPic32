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
	Inputs: built-in push-button ( BTN1/SW1)
	Outputs: built-in LED (LED1/RED)
	Lab objects:
 *		DMA
 *		UART TX
 *		FreeRTOS
 *		Mutex
 *		Binary Semaphore
 *		Custom struct
 *		Static Queue (tenQueue)
		Static Task 
 *		ISR callback (Btn1Handler)

  Summary:
    Queues are the primary mechanism for inter-task communications. They can be used
to send messages between tasks, and between interrupts and tasks. these lines of code is implemented/verified 
 * and software is released for educational only

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
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "device_cache.h"


//define constants of the queue
#define QUEUE_LENGTH 5
#define UNIT_SIZE sizeof(BtnStatus_t) 

//active-low button
#define BTN1_PRESSED_STATE	0

//declare custom data structure to send to the queue
typedef struct BtnStatus_t{
	uint8_t btnID;
	uint32_t btnPress;
	char info_msg[50];
}BtnStatus_t;

//declare variables of a received task - static
static StackType_t xTaskReceiveTcbBuffer[configMINIMAL_STACK_SIZE];
static StaticTask_t xTaskReceiveBuffer;
static void prvTaskReceiveFunction(void * pvParams);

//declare variables of mutex and binary semaphore
SemaphoreHandle_t xMutex;
SemaphoreHandle_t xBinarySema;

//declare variables of uart6 buffer
static uint8_t __attribute__ ((aligned (16))) u6TxBuffer[150] = {0};

//declare a queue
QueueHandle_t tenQueue;
//declare memory of the queue
static uint8_t xQueueQcbBuffer[QUEUE_LENGTH * UNIT_SIZE];
//declare static queue object
static StaticQueue_t xQueueObj;

static void Debug_msg(char * msg){
	while (*msg){
		while (U6STAbits.UTXBF);
		U6TXREG = *msg++;
		while (!U6STAbits.TRMT);
	}
}

//declare handler of dmac transfer event
static void U6D0Handler(DMAC_TRANSFER_EVENT event, uintptr_t contextHandle){
	if (event == DMAC_TRANSFER_EVENT_COMPLETE){
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(xBinarySema, &xHigherPriorityTaskWoken);
		portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
	}
}

BtnStatus_t btnOne = {
	.btnID = 1,
	.btnPress =1,
	.info_msg = "button 1 is pressed and Red is toggled \r\n"
};

//declare callback of Button event
static void Btn1Handler(GPIO_PIN pin, uintptr_t context){
	
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	if (BTN1_Get() == BTN1_PRESSED_STATE){
		if ( xQueueSendFromISR(
			tenQueue,
			&btnOne,
			&xHigherPriorityTaskWoken) == pdFAIL){
				Debug_msg("cannot send object from ISR\r\n");
				exit(EXIT_FAILURE);
		}
		
		portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
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

    //show greeting message-protect uart register with mutex
	Debug_msg("lab10-FreeRTOS-inter-task com \r\n");
	
	LED_1_Clear();
	//create mutex
	xMutex = xSemaphoreCreateMutex();
	//create binary semaphore
	xBinarySema = xSemaphoreCreateBinary();
	
	//register callback for Button event
	GPIO_PinInterruptCallbackRegister(
		BTN1_PIN,
		Btn1Handler,
		0);
	//assign interrupt will fire if mismatch 
	GPIO_PinInterruptEnable(BTN1_PIN);
	
	//register callback for DMAC
	DMAC_ChannelCallbackRegister(
		DMAC_CHANNEL_0,
		U6D0Handler,
		0);
	//create task Receive-static
	if( xTaskCreateStatic(	prvTaskReceiveFunction,
			"receiving task",
			configMINIMAL_STACK_SIZE,
			NULL,
			tskIDLE_PRIORITY,
			xTaskReceiveTcbBuffer,
			&(xTaskReceiveBuffer)) == NULL){
				Debug_msg("cannot create the task \r\n");
				return(EXIT_FAILURE);
	}
	//create the queue - static
	tenQueue = xQueueCreateStatic(
		QUEUE_LENGTH,
		UNIT_SIZE,
		xQueueQcbBuffer,
		&(xQueueObj));
	if (tenQueue == NULL){ 
		Debug_msg("cannot create the queue \r\n");
		return(EXIT_FAILURE);}
	
	vTaskStartScheduler();

	while ( true )
	{
	    /* Maintain state machines of all polled MPLAB Harmony modules. */

	    SYS_Tasks ( );
	}

	/* Execution should not come here during normal operation */

	return ( EXIT_FAILURE );
}

static void prvTaskReceiveFunction(void * pvParams){
	//stop warning of unused parameter
	(void) pvParams;
	if (xSemaphoreTake(xMutex, portMAX_DELAY) ==pdTRUE){
		Debug_msg("hello task Receive is at work-please press button 1 \r\n");
		xSemaphoreGive(xMutex);
	}
	BtnStatus_t localReceive;
	for (;;){
		//receiving data structure from the queue
		if (xQueueReceive(	tenQueue,
				&localReceive,
				portMAX_DELAY) == pdPASS){

			//format string
			sprintf((char *)u6TxBuffer, "%s", localReceive.info_msg);
			//invert RED led
			LED_1_Toggle();
			//show formatted string via uart and dmac-using mutex and binary semaphore for protection
			if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){
				DCACHE_CLEAN_BY_ADDR(
					(uint32_t)u6TxBuffer,
					strlen((char *)u6TxBuffer));
				DMAC_ChannelTransfer(
					DMAC_CHANNEL_0,
					(const void *)u6TxBuffer,
					strlen((const char *)u6TxBuffer),
					(const void *)&U6TXREG, 1, 1);
				xSemaphoreTake(xBinarySema, portMAX_DELAY);
				xSemaphoreGive(xMutex);
			}
		}
	
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}



/*******************************************************************************
 End of File
*/


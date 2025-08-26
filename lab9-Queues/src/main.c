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
		Static Task 
 *		UART TX
 *		FreeRTOS
 *		Mutex
 *		Binary Semaphore
 *		Custom struct
 *		Static Queue

  Summary:
    Queues are the primary mechanism for inter-task communications. They can be used
to send messages between tasks, and between interrupts and tasks.
 * these lines of code is implemented/verified and software is released for educational only

 * 
  Description:
    This file contains the "main" function for a project.  The "main" function calls the "SYS_Initialize" function 
 * to initialize the state  machines of all modules in the system. Debug messages are showed via UART6. 
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
#include "device_cache.h"
#include "semphr.h"
#include "queue.h"

//declare variables for static tasks
static StackType_t xTaskSendTcbBuffer[configMINIMAL_STACK_SIZE];
static StackType_t xTaskReceiveTcbBuffer[configMINIMAL_STACK_SIZE];

static StaticTask_t xTaskSendBuffer;
static StaticTask_t xTaskReceiveBuffer;

static void prvTaskSendFunction(void * pvParams);
static void prvTaskReceiveFunction(void * pvParams);

//declare a struct that is a custom data structure
typedef struct TaskData_t{
	char main_msg[100];
	char task_msg[100];
	unsigned int StackFree;
	uint8_t ID;
} TaskData_t;

//declare xData that is passed to Task Send at creation and then is updated and sent to Task Receive via the queue
static TaskData_t xData ={
	.main_msg = "main pass parameter to Sending task \r\n",
	.task_msg = "Sending task sent to Receive task \r\n",
	.StackFree = 0,
	.ID = 0
};

//declare the Queue
static QueueHandle_t xQueue;

//declare buffer of uart for displaying message in com port via dma
static uint8_t __attribute__ ((aligned (16)))u6TxBuffer[150] = {0};

//delcare mutex and binary semaphore to protect uart buffer and uart transfer event
static SemaphoreHandle_t xMutex;
static SemaphoreHandle_t xBinarySema;

//declare an interrupt for dmac transfer event
static void U6D6Handler(DMAC_TRANSFER_EVENT event, uintptr_t contextHandle){
	if (event == DMAC_TRANSFER_EVENT_COMPLETE){
		BaseType_t xHighPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(xBinarySema, &xHighPriorityTaskWoken);
		portEND_SWITCHING_ISR(xHighPriorityTaskWoken);
	}
}

static void Debug_msg(char * msg){
	while (*msg){
		while (U6STAbits.UTXBF);
		U6TXREG = *msg++;
		while (!U6STAbits.TRMT);
	}
}

//declare constants of the queue
#define QUEUE_LENGTH 5
#define ITEM_SIZE sizeof(TaskData_t)

//declare memory for the queue
static uint8_t xQueueStorage[QUEUE_LENGTH * ITEM_SIZE];

//declare static queue object
static StaticQueue_t xQueueObj;


int main(void){
	SYS_Initialize(NULL);
	
	//display welcome message
	Debug_msg("lab9-FreeRTOS-inter-task communication\r\n");
	
	//initialize DMAC for showing the result via uart+dmac
	DMAC_ChannelCallbackRegister(
		DMAC_CHANNEL_6,
		U6D6Handler,
		0);
	
	//create task Send -  static - pass a struct at creation
	TaskHandle_t xTaskSendResult = xTaskCreateStatic(
		prvTaskSendFunction,
		"Sending Task",
		configMINIMAL_STACK_SIZE,
		(void *) &xData,
		tskIDLE_PRIORITY,
		xTaskSendTcbBuffer,
		&(xTaskSendBuffer));
	//handle the result of the creation of task Send
	if (xTaskSendResult == NULL){
		Debug_msg("cannot create task Send\r\n");
		return (EXIT_FAILURE);
	}
	
	//create task Receive - static - receive the struct from the queue and display via uart+dmac
	TaskHandle_t xTaskReceiveResult = xTaskCreateStatic(
		prvTaskReceiveFunction,
		"Receiving Task",
		configMINIMAL_STACK_SIZE,
		NULL,
		tskIDLE_PRIORITY,
		xTaskReceiveTcbBuffer,
		&(xTaskReceiveBuffer));
	//handle the result of the creation of task Receive
	if (xTaskReceiveResult == NULL){
		Debug_msg("cannot create task Receive\r\n");
		return (EXIT_FAILURE);
	}
	
	
	//create xMutex for protecting uart buffer
	xMutex = xSemaphoreCreateMutex();
	
	//create xBinarySem for protecting uart transfer
	xBinarySema = xSemaphoreCreateBinary();
	
	//create xQueue for inter-task communication
	xQueue = xQueueCreateStatic(
		QUEUE_LENGTH,
		ITEM_SIZE,
		xQueueStorage,
		&(xQueueObj));
	//handle the result of the creation of the Queue
	if (xQueue == NULL){
		Debug_msg("cannot create the Queue\r\n");
		return (EXIT_FAILURE);
	}
	
	vTaskStartScheduler();
	
	for (;;){
		
	}
	
	return (EXIT_FAILURE);
}

static void prvTaskSendFunction (void * pvParams){
	TaskData_t * localSend = (TaskData_t *) pvParams;
	
	BaseType_t xQueueSendResult ;
	//show welcome message from Sending task-using mutex to protect uart transfer register
	if (xSemaphoreTake(xMutex, portMAX_DELAY) ==pdTRUE){
		Debug_msg("hello it is task Send\r\n");
	}
	xSemaphoreGive(xMutex);
	
	for (uint8_t send=0; send < 6 ; send++){
		//get stack water mark and update xData.StackFree and task msg
		BaseType_t FreeOfStack = uxTaskGetStackHighWaterMark(NULL);
		localSend->StackFree = (unsigned int)FreeOfStack;
		strncpy(localSend->task_msg, "Sending task sent to Receiving task ", sizeof(localSend->task_msg));
		localSend->ID = send+1;

		//send xData to xQueue
		xQueueSendResult = xQueueSend(
			xQueue,
			localSend,
			pdMS_TO_TICKS(20));
		//handle the result of the sending
		if (xQueueSendResult == pdFALSE){
			Debug_msg("cannot send data to the Queue\r\n");
			exit(EXIT_FAILURE);

		}

		//task delay
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
	
	
}

static void prvTaskReceiveFunction(void * pvParams){
	//stop warning for unused parameters
	(void )pvParams;
	
	//declare the data structure for receiving from Queue
	TaskData_t localReceive;
	
	//show welcome message from Receiving task
	if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){
		Debug_msg("alo task Receive is running ...\r\n");
	}
	xSemaphoreGive(xMutex);
	
	for (uint8_t rec = 0; rec < 6 ; rec++){
		//read xData from xQueue
		if (xQueueReceive(
			xQueue,
			&localReceive,
			portMAX_DELAY) ==pdFALSE){
				Debug_msg("cannot receive from queue...\r\n");
				exit(EXIT_FAILURE);
		}
		
		if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){
			//format string
			sprintf((char *)u6TxBuffer, "%s in %d time and free of %d words \r\n", localReceive.task_msg, localReceive.ID, localReceive.StackFree);
			//flush from cache to RAM
			DCACHE_CLEAN_BY_ADDR(
				(uint32_t) u6TxBuffer,
				strlen((const char *)u6TxBuffer));
			//dmac transfer
			DMAC_ChannelTransfer(
				DMAC_CHANNEL_6,
				(const void *)u6TxBuffer,
				strlen((const char *)u6TxBuffer),
				(const void *)&U6TXREG, 1, 1);
			
			//waiting for the taking binary semaphore from ISR
			xSemaphoreTake(xBinarySema, portMAX_DELAY);
			//give the mutex
			xSemaphoreGive(xMutex);
		}
		//task delay
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
	
}

/*******************************************************************************
 End of File
*/


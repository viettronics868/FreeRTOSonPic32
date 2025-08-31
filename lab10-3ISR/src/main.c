/*******************************************************************************\
 the idea of the lab is inspired by the original lab at 
 * https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Tutorials.git
 * and transferring the lab onto the Curiosity 2.0 pic32 mz ef board.
 Author: Nam_K Pham_L. 

  File Name:
    main.c
 * 
 * Setup:
 *	Board: Curiosity 2.0 PIC32MZ EF development board
	Inputs: built-in push-buttons ( BTN1/SW1 and BTN2/SW2 and BTN3/SW3)
	Outputs: built-in LEDs (LED1/RED and LED2/GREEN and LED3/YELLOW)
	Lab objects:
 *		DMA
 *		UART TX
 *		FreeRTOS
 *		Mutex
 *		Binary Semaphore
 *		Custom struct
 *		Static Queue (tenQueue)
		Static Task (prvLKFunction)
 *		ISR callback (Btn1Handler and Btn2Handler and Btn3Handler)
 *	Applying technique:  function pointers and debounce on key press

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
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "device_cache.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "timers.h"

//declare debounce timer and buffer and callback
static StaticTimer_t xBtn1DebounceTimerBuffer;
static StaticTimer_t xBtn2DebounceTimerBuffer;
static StaticTimer_t xBtn3DebounceTimerBuffer;

static TimerHandle_t xBtn1DebounceTimer;
static TimerHandle_t xBtn2DebounceTimer;
static TimerHandle_t xBtn3DebounceTimer;

static void prvBtn1DebounceFunction(TimerHandle_t xTimer);
static void prvBtn2DebounceFunction(TimerHandle_t xTimer);
static void prvBtn3DebounceFunction(TimerHandle_t xTimer);

//declare variables of debug task
static StackType_t xLKTcbBuffer[configMINIMAL_STACK_SIZE];
static StaticTask_t xLKBuffer;
static void prvLKFunction(void * pvParams);

#define BUTTON_PRESS_STATE   0
//declare function pointer

//declare protector
SemaphoreHandle_t xMutex;
SemaphoreHandle_t xBinarySema;


//declare the custom data structure
typedef struct BtnData_t{
	uint8_t btnID;
	char color[10];
	char msg[50];
}BtnData_t;

//declare the queue
static QueueHandle_t tenQueue;
#define QUEUE_LENGTH 6
#define UNIT_SIZE sizeof(BtnData_t)
static uint8_t lkQcbBuffer[QUEUE_LENGTH * UNIT_SIZE];
static StaticQueue_t lkQueueBuffer;

//declare variables of uart6 and callback
static uint8_t __attribute__ ((aligned (16))) u6TxBuffer[150] = {0};
static void U6D0Handler(DMAC_TRANSFER_EVENT event, uintptr_t contextHandle){
	if (event == DMAC_TRANSFER_EVENT_COMPLETE){
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(xBinarySema, &(xHigherPriorityTaskWoken));
		portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
	}
}

//declare a function to show debug message
static void Debug_msg(char * msg){
	while (*msg){
		while (U6STAbits.UTXBF);
		U6TXREG = *msg++;
		while (!U6STAbits.TRMT);
	}
}

//data structure of button 1
BtnData_t btnOne = {
	.btnID = 1,
	.color = "RED",
	.msg = "ohmygod"
 };

//data structure of button 2
BtnData_t btnTwo = {
	.btnID = 2,
	.color = "GREEN",
	.msg = "olala"
};

//data structure of button 3
BtnData_t btnThree = {
	.btnID = 3,
	.color = "YELLOW",
	.msg = "shhhh"
};

//declare ISR callback of btn1 for debounce 50ms
static void Btn1Handler(GPIO_PIN pin, uintptr_t context){
	if (BTN_1_Get() == BUTTON_PRESS_STATE){
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xTimerResetFromISR(xBtn1DebounceTimer, &xHigherPriorityTaskWoken);
		portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);		
	}
}

//declare ISR callback of Btn2 for debounce 50ms
static void Btn2Handler(GPIO_PIN pin, uintptr_t context){
	if (BTN_2_Get() == BUTTON_PRESS_STATE){
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xTimerResetFromISR(xBtn2DebounceTimer, &xHigherPriorityTaskWoken);
		portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
	}
}

//declare ISR callback of Btn3 for debounce 50ms
static void Btn3Handler(GPIO_PIN pin, uintptr_t context){
	if (BTN_3_Get() == BUTTON_PRESS_STATE){
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xTimerResetFromISR(xBtn3DebounceTimer, &xHigherPriorityTaskWoken);
		portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
	}
}

//declare three functions for toggling three leds
static void RedInv(void){
	LED_1_Toggle();
}

static void GreenInv(void){
	LED_2_Toggle();
}

static void YellowInv(void){
	LED_3_Toggle();
}
//declare an array of function pointers
static void (*fp_led[3])(void) = {RedInv, GreenInv, YellowInv};

// *****************************************************************************
// *****************************************************************************
// Section: Main Entry Point
// *****************************************************************************
// *****************************************************************************

int main ( void )
{
    /* Initialize all modules */
    SYS_Initialize ( NULL );
    
    Debug_msg("lab10 - queue and 3 ISR \r\n");
    
    //initialize three LEDs - light ON
    LED_1_Clear();
    LED_2_Clear();
    LED_3_Clear();
    
    //create 3 debounce timers
    xBtn1DebounceTimer = xTimerCreateStatic(
	    "BTN1 Debounce ",
	    pdMS_TO_TICKS(50),
	    pdFALSE,
	    (void *)1,
	    prvBtn1DebounceFunction,
	    &xBtn1DebounceTimerBuffer);
    if (xBtn1DebounceTimer == NULL){
	    Debug_msg("cannot create debounce on btn1 \r\n");
	    return (EXIT_FAILURE);
    }
    
    xBtn2DebounceTimer = xTimerCreateStatic(
	    "BTN2 Debounce",
	    pdMS_TO_TICKS(50),
	    pdFALSE,
	    (void *)2,
	    prvBtn2DebounceFunction,
	    &xBtn2DebounceTimerBuffer);
    if (xBtn2DebounceTimer == NULL){
	    Debug_msg("cannot create debounce on btn2 \r\n");
	    return (EXIT_FAILURE);
    }
    
    xBtn3DebounceTimer = xTimerCreateStatic(
	    "BTN3 Debounce",
	    pdMS_TO_TICKS(50),
	    pdFALSE,
	    (void *)3,
	    prvBtn3DebounceFunction,
	    &xBtn3DebounceTimerBuffer);
    if (xBtn3DebounceTimer == NULL){
	    Debug_msg("cannot create debounce on btn3 \r\n");
	    return (EXIT_FAILURE);
    }
    
    //register dmac handler
    DMAC_ChannelCallbackRegister(
	    DMAC_CHANNEL_0,
	    U6D0Handler,
	    0);
    
    //register handler for button 1
    GPIO_PinInterruptCallbackRegister(
	    BTN_1_PIN,
	    Btn1Handler,
	    0);
    GPIO_PinInterruptEnable(BTN_1_PIN);
    
    //register handler for button 2
    GPIO_PinInterruptCallbackRegister(
	    BTN_2_PIN,
	    Btn2Handler,
	    0);
    GPIO_PinInterruptEnable(BTN_2_PIN);
    
    //registfer handler for button 3
    GPIO_PinInterruptCallbackRegister(
	    BTN_3_PIN,
	    Btn3Handler,
	    0);
    GPIO_PinInterruptEnable(BTN_3_PIN);
    
    //create protectors for uart buffer and uart transfer event
    xMutex = xSemaphoreCreateMutex();
    xBinarySema = xSemaphoreCreateBinary();
    
    //create task
    if (xTaskCreateStatic(
	    prvLKFunction,
	    "task LK",
	    configMINIMAL_STACK_SIZE,
	    NULL,
	    tskIDLE_PRIORITY,
	    xLKTcbBuffer,
	    &(xLKBuffer)) == NULL){
				Debug_msg("cannot create task \r\n");
				return (EXIT_FAILURE);
			}
    
    //create the queue
    tenQueue = xQueueCreateStatic(
	    QUEUE_LENGTH,
	    UNIT_SIZE,
	    lkQcbBuffer,
	    &lkQueueBuffer);
    if (tenQueue == NULL){
	    Debug_msg("cannot create tenQueue \r\n");
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

static void prvLKFunction(void * pvParams){
	(void) pvParams;
	
	//show welcome message
	Debug_msg("lk is on the  road-please press a button... \r\n");
	
	for (;;){
		BtnData_t localLK;
		if (xQueueReceive(
			tenQueue,
			&localLK,
			portMAX_DELAY) == pdFAIL){
						Debug_msg("cannot receive object from queue ..\r\n");
						break;}
		for (uint8_t i = 0; i < 3; i++){
			if (localLK.btnID == i+1){
				fp_led[i]();//map button ID to function pointer
				break;
			}
		} 
		if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){		

			//format the string 
			sprintf((char *)u6TxBuffer, "button %d was pressed-debounce 50ms and %s is changed \r\n", localLK.btnID, localLK.color);
			DCACHE_CLEAN_BY_ADDR(
				(uint32_t) u6TxBuffer,
				strlen((const char *) u6TxBuffer));
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

static void prvBtn1DebounceFunction(TimerHandle_t xTimer){
	if (BTN_1_Get() == BUTTON_PRESS_STATE){
		if (xQueueSend(
			tenQueue,
			&btnOne,
			pdMS_TO_TICKS(20)) == pdFAIL){
						Debug_msg("cannot send btn1 to queue \r\n");
						exit(EXIT_FAILURE);
					}
	}
}

static void prvBtn2DebounceFunction(TimerHandle_t xTimer){
	if (BTN_2_Get() == BUTTON_PRESS_STATE){
		if (xQueueSend(
			tenQueue,
			&btnTwo,
			pdMS_TO_TICKS(20)) == pdFAIL){
						Debug_msg("cannot send btn2 to queue \r\n");
						exit(EXIT_FAILURE);
					}
	}
}

static void prvBtn3DebounceFunction(TimerHandle_t xTimer){
	if (BTN_3_Get() == BUTTON_PRESS_STATE){
		if (xQueueSend(
			tenQueue,
			&btnThree,
			pdMS_TO_TICKS(20)) == pdFAIL){
						Debug_msg("cannot send btn3 to queue \r\n");
						exit(EXIT_FAILURE);
					}
	}
}


/*******************************************************************************
 End of File
*/


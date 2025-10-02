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
 *		Task Notification  - Event Group- Mutex
 *		Static Tasks 
 *		ISR on pushbutton
 *		DMAC transfer and xTaskGetCurrentTaskHandle() to register itself.

  Summary:
   Task Notification is  a "inter-task communication"  in FreeRTOS task . in Task Control Block of the task has a built-in 32-bit 
 * notification field. That means every task has a 32-bit value for storing number, flag or counter and a 'pending' state.
 *  these lines of code is implemented/verified and software is released for educational only

  Description:
 * The lab implement the complete safe pattern (mutex + notification + event group). 
 * The Office task will ask for key press including SW1, SW2, SW3, SW4 if no key press after 5 seconds.
 * if key press happened, the LED RGB blinking will change period of blinking.
    This file contains the "main" function for a project.  The "main" function calls the "SYS_Initialize" function to initialize the state
    machines of all modules in the system, and create tasks . 
 * 
 * the files Lab17_DMA.c and Lab17_DMA.h are helper functions
 * the files Lab17_UART6.c and Lab17_UART6.h are showing debug messages on com port
 * the files Lab17_Timers.c and Lab17_Timers.h are for timer-related functions
 * the files Lab17_STAmac.c and Lab17_STAmac.h are for the task of state machine core logic
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

//define interval of timers
#define KEY_PRESSED_EVENT 0
#define DEBOUNCE_PERIOD 50

//#define xHPTW xHigherPriorityTaskWoken

#define LED1_BLINKING 1000
#define LED2_BLINKING 1100
#define LED3_BLINKING 2000

#define LED_R_BLINKING 1000
#define LED_G_BLINKING 2000
#define LED_B_BLINKING 3000

#define BIT_SW1 (1U << 1)
#define BIT_SW2 (1U << 2)
#define BIT_SW3 (1U << 3)
#define BIT_SW4 (1U << 4)

//declare event group
EventGroupHandle_t xLab17EveGr;

//declare task handle to register the task to dma callback
//static TaskHandle_t xTaskSTAHandle;
TaskHandle_t xTaskSTAHandle;

Lab17State_t currentSTA = INIT_STA;

//declare mutual exclusive
SemaphoreHandle_t xMutex;

//declare debounce timers - 50ms
//static TimerHandle_t xDebounceSW1Timer;
//static TimerHandle_t xDebounceSW2Timer;
//static TimerHandle_t xDebounceSW3Timer;
//static TimerHandle_t xDebounceSW4Timer;
//
//static void vDebounceSW1TimerCallback(TimerHandle_t xTimer);
//static void vDebounceSW2TimerCallback(TimerHandle_t xTimer);
//static void vDebounceSW3TimerCallback(TimerHandle_t xTimer);
//static void vDebounceSW4TimerCallback(TimerHandle_t xTimer);
//
////declare blinking timers
//static TimerHandle_t xBlinkingLED1Timer;
//static TimerHandle_t xBlinkingLED2Timer;
//static TimerHandle_t xBlinkingLED3Timer;
//
//static TimerHandle_t xBlinkingLED_RTimer;
//static TimerHandle_t xBlinkingLED_GTimer;
//static TimerHandle_t xBlinkingLED_BTimer;
//
//static void vBlinkingLED1TimerCallback(TimerHandle_t xTimer);
//static void vBlinkingLED2TimerCallback(TimerHandle_t xTimer);
//static void vBlinkingLED3TimerCallback(TimerHandle_t xTimer);
//
//static void vBlinkingLED_RTimerCallback(TimerHandle_t xTimer);
//static void vBlinkingLED_GTimerCallback(TimerHandle_t xTimer);
//static void vBlinkingLED_BTimerCallback(TimerHandle_t xTimer);
//
//
////declare variables of four of tasks
//static StaticTask_t xSW1TcbBuffer;
//static StaticTask_t xSW2TcbBuffer;
//static StaticTask_t xSW3TcbBuffer;
//static StaticTask_t xSW4TcbBuffer;
static StaticTask_t xSTAmacTcbBuffer;
//
//static StackType_t xSW1StackBuffer[configMINIMAL_STACK_SIZE];
//static StackType_t xSW2StackBuffer[configMINIMAL_STACK_SIZE];
//static StackType_t xSW3StackBuffer[configMINIMAL_STACK_SIZE];
//static StackType_t xSW4StackBuffer[configMINIMAL_STACK_SIZE];
static StackType_t xSTAmacStack[configMINIMAL_STACK_SIZE];
//
//static void prvSW1TaskFunc(void * pvParams);
//static void prvSW2TaskFunc(void * pvParams);
//static void prvSW3TaskFunc(void * pvParams);
//static void prvSW4TaskFunc(void * pvParams);

//static void prvOfficeAdminFunc(void * pvParams);
//static StackType_t xTaskOfficeStack[configMINIMAL_STACK_SIZE];
//static StaticTask_t xTaskOfficeBuffer;

//declare application of lab 17
static void vLab17_Init(void){
	
	//turn on all leds
	LED1_Clear();
	LED2_Clear();
	LED3_Clear();
	LED_R_Clear();
	LED_G_Clear();
	LED_B_Clear();
	
	
	//create task OfficeAdmin
//	if (xTaskCreateStatic(
//			prvOfficeAdminFunc,
//			"task of Office Administrator",
//			configMINIMAL_STACK_SIZE,
//			&xLab17EveGr,
//			tskIDLE_PRIORITY,
//			xTaskOfficeStack,
//			&xTaskOfficeBuffer) == NULL){
//		Debug_msg("cannot create Office task \r\n...");
//		exit(EXIT_FAILURE);
//	}
	
	//create task Lab17 state machine core and assign task handle
	xTaskSTAHandle = xTaskCreateStatic(
				vLab17STAmac,
				"state machine of lab 17",
				configMINIMAL_STACK_SIZE,
				&xLab17EveGr,
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
	
	//Debug_msg("oooo \r\n");
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
//
//static void prvOfficeAdminFunc(void * pvParams){
//	for (;;){
//		if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){
//			//register the OfficeAdmin task to DMA callback
//			xTaskSTAHandle =  xTaskGetCurrentTaskHandle();
//			vComPortMsg("lab17 - Task Notification on static task");
//			
//			ulTaskNotifyTakeIndexed(
//					1,
//					pdTRUE,
//					pdMS_TO_TICKS(1000));
//			
//			xSemaphoreGive(xMutex);
//		}
//	}
//}


/*******************************************************************************
 End of File
*/


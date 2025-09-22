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
 *		Event Group-Event Group Synchronization - Mutex
 *		Static Tasks 
 *		ISR on pushbutton and DMAC transfer complete

  Summary:
    Event Group is an important concept in FreeRTOS task synchronization. An Event Group is a collection of bits (like flags).
 * Each bit can represent an event (e.g., sensor ready,UART complete,button pressed). When the event occur and condition 
 * is met (bits set), the task unblocks.
 * . these lines of code is implemented/verified and software is released for educational only

  Description:
 * The lab is for synchronization of three LEDs. 
 * The Office task will ask for key press including SW1, SW2, SW3 if no key press after 5 seconds.
 * if key press happened, the LED RGB will toggle one color, and RED for SW1, GREEN for SW2, BLUE for SW3.
 * if SW1 and SW2 and SW3 are pressed continuously, LED1, LED2 and LED3 reach the synchronization  and the blinking start.
    This file contains the "main" function for a project.  The "main" function calls the "SYS_Initialize" function to initialize the state
    machines of all modules in the system, and call LAB16_Initialize function to initialize the modules of the application. 
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

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "device_cache.h"
#include "semphr.h"
#include "timers.h"
#include "event_groups.h"

//define constant
#define KEY_PRESS_STATE	0
#define DEBOUNCE_PERIOD	50
#define LED1_BLINKING	500
#define LED2_BLINKING	2000
#define LED3_BLINKING	4000
#define LEDRGB_BLINKING	200
static TickType_t TICK_TO_WAIT = 100 / portTICK_PERIOD_MS;

//assign bits for event group
#define BIT_U6_TX_COMPLETE	(1U << 0)
#define BIT_SW1_STATE	(1U << 1)
#define BIT_LED1_SYNC	(1U << 2)
#define BIT_SW2_STATE	(1U << 3)
#define BIT_LED2_SYNC	(1U << 4)
#define BIT_SW3_STATE	(1U << 5 )
#define BIT_LED3_SYNC	(1U << 6)
#define BIT_SW4_STATE	(1U << 7)
#define BIT_1ST		(1U << 8)
#define BIT_2ND		(1U << 9)
#define BIT_3RD		(1U << 10)


//declare event group of the lab 16
static EventGroupHandle_t xLab16EveGr;
static StaticEventGroup_t xLab16EveGrBuffer;

//declare primitives 
static SemaphoreHandle_t xMutex;

//declare software timer for debounce 50ms when key pressing
static TimerHandle_t xDebounceSW1Timer;
static TimerHandle_t xDebounceSW2Timer;
static TimerHandle_t xDebounceSW3Timer;
static TimerHandle_t xDebounceSW4Timer;

//declare debounce timer's buffer
static StaticTimer_t xDebounceSW1Buffer;
static StaticTimer_t xDebounceSW2Buffer;
static StaticTimer_t xDebounceSW3Buffer;
static StaticTimer_t xDebounceSW4Buffer;

//declare interval timers for blinking leds
static TimerHandle_t xLED1BlinkingTimer;
static TimerHandle_t xLED2BlinkingTimer;
static TimerHandle_t xLED3BlinkingTimer;
static TimerHandle_t xLEDRGBBlinkingTimer;

//declare blinking timers' buffers
static StaticTimer_t xLED1BlinkingTimerBuffer;
static StaticTimer_t xLED2BlinkingTimerBuffer;
static StaticTimer_t xLED3BlinkingTimerBuffer;
static StaticTimer_t xLEDRGBBlinkingTimerBuffer;

//declare tasks for synchronization and blinking leds
static StaticTask_t xLed1TaskBuffer;
static StackType_t xLed1TaskStack[configMINIMAL_STACK_SIZE];
static void prvLED1Func(void * pvParams);

static StaticTask_t xLed2TaskBuffer;
static StackType_t xLed2TaskStack[configMINIMAL_STACK_SIZE];
static void prvLED2Func(void * pvParams);

static StaticTask_t xLed3TaskBuffer;
static StackType_t xLed3TaskStack[configMINIMAL_STACK_SIZE];
static void prvLED3Func(void * pvParams);

static StaticTask_t xLedRGBTaskBuffer;
static StackType_t xLedRGBTaskStack[configMINIMAL_STACK_SIZE];
static void prvLEDRGBFunc(void * pvParams);

//declare variables and function of greeting task-static
static StaticTask_t xOfficeTaskBuffer;
static StackType_t xOfficeTaskStack[configMINIMAL_STACK_SIZE];
static void prvOfficeTaskFunc(void * pvParams);



//declare debounce timer's callbacks
static void prvDebounceSW1Callback(TimerHandle_t xTimer){
	if (SW1_Get() == KEY_PRESS_STATE){
		xEventGroupSetBits(
			xLab16EveGr,
			BIT_SW1_STATE | BIT_1ST);//BIT_1ST is only for Office Task
	}
}

static void prvDebounceSW2Callback(TimerHandle_t xTimer){
	if (SW2_Get() == KEY_PRESS_STATE){
		xEventGroupSetBits(
			xLab16EveGr,
			BIT_SW2_STATE | BIT_2ND);//BIT_2ND is only for Office Task
	}
}

static void prvDebounceSW3Callback(TimerHandle_t xTimer){
	if (SW3_Get() == KEY_PRESS_STATE){
		xEventGroupSetBits(
			xLab16EveGr,
			BIT_SW3_STATE | BIT_3RD);//BIT_3RD is only for Office Task
	}
}

static void prvDebounceSW4Callback(TimerHandle_t xTimer){
	if (SW4_Get() == KEY_PRESS_STATE){
		xEventGroupSetBits(
			xLab16EveGr,
			BIT_SW4_STATE);
	}
}

//declare blinking timers' callbacks
static void prvLED1BlinkingTimerCallback(TimerHandle_t xTimer){
	LED1_Toggle();
}

static void prvLED2BlinkingTimerCallback(TimerHandle_t xTimer){
	LED2_Toggle();
}

static void prvLED3BlinkingTimerCallback(TimerHandle_t xTimer){
	LED3_Toggle();
}

static void prvLEDRGBBlinkingTimerCallback(TimerHandle_t xTimer){
	LED_R_Toggle();
	LED_G_Toggle();
	LED_B_Toggle();
}

//callback functions of ISRs on key press
static void prvSW1ISRCallback(GPIO_PIN pin, uintptr_t context){
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xTimerResetFromISR(xDebounceSW1Timer, &xHigherPriorityTaskWoken);
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

static void prvSW2ISRCallback(GPIO_PIN pin, uintptr_t context){
	BaseType_t  xHigherPriorityTaskWoken = pdFALSE;
	xTimerResetFromISR(xDebounceSW2Timer, &xHigherPriorityTaskWoken);
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

static void prvSW3ISRCallback(GPIO_PIN pin, uintptr_t context){
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xTimerResetFromISR(xDebounceSW3Timer, &xHigherPriorityTaskWoken);
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

static void prvSW4ISRCallback(GPIO_PIN pin, uintptr_t context){
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xTimerResetFromISR(xDebounceSW4Timer, &xHigherPriorityTaskWoken);
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

//callback function for UART6 ISR
static void U6D0Callback(DMAC_TRANSFER_EVENT event, uintptr_t contextHandle){
	if (event == DMAC_TRANSFER_EVENT_COMPLETE){
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xEventGroupSetBitsFromISR(
				xLab16EveGr, 
				BIT_U6_TX_COMPLETE,
				&xHigherPriorityTaskWoken);
		portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
	}
}

//declare the buffer of UART6
static uint8_t __attribute__ ((aligned (16))) u6TxBuffer[128] = {0};

//declare a variable that verify the beginning of  Lab 16
static uint8_t startLab16 = 0;

//function for showing debug message
static void Debug_msg(char * msg){
	while (*msg){
		while (U6STAbits.UTXBF);
		U6TXREG = *msg++;
		while (!U6STAbits.TRMT);
	}
}

//lab16 application initialization
static void Lab16_Initialize(void){
	
	LED_R_Clear();
	LED_G_Clear();
	LED_B_Clear();
	
	startLab16 = 0;
	
	//create task for greeting and reminder and power saving
	if (xTaskCreateStatic(
			prvOfficeTaskFunc,
			"Office Admin",
			configMINIMAL_STACK_SIZE,
			NULL,			
			tskIDLE_PRIORITY+1,
			xOfficeTaskStack,
			&xOfficeTaskBuffer) == NULL) {
		Debug_msg("cannot create task Office Admin \r\n");
		exit(EXIT_FAILURE);
	}
	
	//register callback for dmac's irq
	DMAC_ChannelCallbackRegister(
				DMAC_CHANNEL_0,
				U6D0Callback,
				0);
	
	//register callback for gpio's irq
	GPIO_PinInterruptCallbackRegister(
				SW1_PIN,
				prvSW1ISRCallback,
				0);
	GPIO_PinInterruptEnable(SW1_PIN);
	
	GPIO_PinInterruptCallbackRegister(
				SW2_PIN,
				prvSW2ISRCallback,
				0);
	GPIO_PinInterruptEnable(SW2_PIN);
	
	GPIO_PinInterruptCallbackRegister(
				SW3_PIN,
				prvSW3ISRCallback,
				0);
	GPIO_PinInterruptEnable(SW3_PIN);
	
	GPIO_PinInterruptCallbackRegister(
				SW4_PIN,
				prvSW4ISRCallback,
				0);
	GPIO_PinInterruptEnable(SW4_PIN);
	
	//create debounce timers
	xDebounceSW1Timer = xTimerCreateStatic(
		"debounce SW1",
		DEBOUNCE_PERIOD,
		pdFALSE,
		(void *)1,
		prvDebounceSW1Callback,
		&xDebounceSW1Buffer);
	if (xDebounceSW1Timer == NULL){
		Debug_msg("cannot create debounce timer for SW1 \r\n");
		exit(EXIT_FAILURE);
	}
	
	xDebounceSW2Timer = xTimerCreateStatic(
		"debounce SW2",
		DEBOUNCE_PERIOD,
		pdFALSE,
		(void *)2,
		prvDebounceSW2Callback,
		&xDebounceSW2Buffer);
	if (xDebounceSW2Timer == NULL){
		Debug_msg ("cannot create debounce timer for SW2 \r\n");
		exit(EXIT_FAILURE);
	}
	
	xDebounceSW3Timer = xTimerCreateStatic(
		"debounce SW3",
		DEBOUNCE_PERIOD,
		pdFALSE,
		(void *)3,
		prvDebounceSW3Callback,
		&xDebounceSW3Buffer);
	if (xDebounceSW3Timer == NULL){
		Debug_msg("cannot create debounce timer for SW3\r\n");
		exit(EXIT_FAILURE);
	}
	
	xDebounceSW4Timer = xTimerCreateStatic(
		"debounce SW4",
		DEBOUNCE_PERIOD,
		pdFALSE,
		(void *)4,
		prvDebounceSW4Callback,
		&xDebounceSW4Buffer);
	if (xDebounceSW4Timer == NULL){
		Debug_msg("cannot create debounce timer for SW4\r\n");
		exit(EXIT_FAILURE);
	}
	
	//create blinking timers
	xLED1BlinkingTimer = xTimerCreateStatic(
		"blinking LED1",
		LED1_BLINKING,
		pdTRUE,
		(void *)5,
		prvLED1BlinkingTimerCallback,
		&xLED1BlinkingTimerBuffer);
	if (xLED1BlinkingTimer == NULL){
		Debug_msg("cannot create timer for blinking LED1\r\n");
		exit(EXIT_FAILURE);
	}
	
	xLED2BlinkingTimer = xTimerCreateStatic(
		"blinking LED2",
		LED2_BLINKING,
		pdTRUE,
		(void *)6,
		prvLED2BlinkingTimerCallback,
		&xLED2BlinkingTimerBuffer);
	if (xLED2BlinkingTimer == NULL){
		Debug_msg("cannot create timer for blinking LED2\r\n");
		exit(EXIT_FAILURE);
	}
	
	xLED3BlinkingTimer = xTimerCreateStatic(
		"blinking LED3",
		LED3_BLINKING,
		pdTRUE,
		(void *)7,
		prvLED3BlinkingTimerCallback,
		&xLED3BlinkingTimerBuffer);
	if (xLED3BlinkingTimer == NULL){
		Debug_msg("cannot create timer for blinking LED3\r\n");
		exit(EXIT_FAILURE);
	}
	
	xLEDRGBBlinkingTimer = xTimerCreateStatic(
		"blinking LED RGB",
		LEDRGB_BLINKING,
		pdTRUE,
		(void *)8,
		prvLEDRGBBlinkingTimerCallback,
		&xLEDRGBBlinkingTimerBuffer);
	if (xLEDRGBBlinkingTimer == NULL){
		Debug_msg("cannot create timer for blinking LED RGB\r\n");
		exit(EXIT_FAILURE);
	}
	
	//create mutex
	xMutex = xSemaphoreCreateMutex();
	
	//create tasks
	if (xTaskCreateStatic(
			prvLED1Func,
			"task control LED1",
			configMINIMAL_STACK_SIZE,
			NULL,
			tskIDLE_PRIORITY+1,
			xLed1TaskStack,
			&xLed1TaskBuffer) == NULL){
		Debug_msg("cannot create task for LED1\r\n");
		exit(EXIT_FAILURE);
	}
	
	if (xTaskCreateStatic(
			prvLED2Func,
			"task control LED2",
			configMINIMAL_STACK_SIZE,
			NULL,
			tskIDLE_PRIORITY+1,
			xLed2TaskStack,
			&xLed2TaskBuffer) == NULL){
		Debug_msg("cannot create task for LED2\r\n");
		exit(EXIT_FAILURE);
	}
	
	if (xTaskCreateStatic(
			prvLED3Func,
			"task control LED3",
			configMINIMAL_STACK_SIZE,
			NULL,
			tskIDLE_PRIORITY+1,
			xLed3TaskStack,
			&xLed3TaskBuffer) == NULL){
		Debug_msg("cannot create task for LED3\r\n");
		exit(EXIT_FAILURE);
	}
	
	if (xTaskCreateStatic(
			prvLEDRGBFunc,
			"task control LED RGB",
			configMINIMAL_STACK_SIZE,
			NULL,
			tskIDLE_PRIORITY+1,
			xLedRGBTaskStack,
			&xLedRGBTaskBuffer) == NULL){
		Debug_msg("cannot create task for LED RGB\r\n");
		exit(EXIT_FAILURE);
	}
		
	//create event group
	xLab16EveGr = xEventGroupCreateStatic(&xLab16EveGrBuffer);
	if (xLab16EveGr == NULL){
		Debug_msg("cannot create event group\r\n");
		exit(EXIT_FAILURE);
	}
	
	//clear all bits of the event group
	//the handle is valid with xEventGroupCreateStatic() instead of xEventGroupCreate()
	xEventGroupClearBits(
			xLab16EveGr,
			0xFFFFFFFF);
	
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

    Lab16_Initialize();
    
    vTaskStartScheduler();
    
    while ( true )
    {
        /* Maintain state machines of all polled MPLAB Harmony modules. */
        SYS_Tasks ( );
    }

    /* Execution should not come here during normal operation */

    return ( EXIT_FAILURE );
}

//create the function that show a message via DMA0 and UART6
static void prvShowMsg(char * msg){
		
	sprintf((char *)u6TxBuffer, msg);
	DCACHE_CLEAN_BY_ADDR(
				(uint32_t)u6TxBuffer,
				strlen((const char *)u6TxBuffer));
	DMAC_ChannelTransfer(
			DMAC_CHANNEL_0,
			(const void *)u6TxBuffer,
			strlen((const char *)u6TxBuffer),
			(const void *)&U6TXREG, 1, 1);
	xEventGroupWaitBits(
			xLab16EveGr,
			BIT_U6_TX_COMPLETE,
			pdTRUE,
			pdFALSE,
			portMAX_DELAY);			
}

static void prvOfficeTaskFunc(void * pvParams){
	//show greeting message or notification
	for(;;){
		EventBits_t bits = xEventGroupGetBits(xLab16EveGr);
		if (((bits & 0x0700) == 0) && (startLab16 == 0)) {//verify the first time of Office Task
			if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){
				prvShowMsg("Lab16-Event Group Synchronization \r\n");
				startLab16++;
				xSemaphoreGive(xMutex);
			}			
		} else if (((bits & 0x0700) == 0) && (startLab16 ==1)){//verify no action after 10 seconds
			if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){
				prvShowMsg("please press the switches ... \r\n");
				xSemaphoreGive(xMutex);
			}
		}
		else {
			if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){
				prvShowMsg("Awesome!!!\r\n");
				xSemaphoreGive(xMutex);
			}
			//clear bits after checking with period of 10seconds
			if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE)
			{
				xEventGroupClearBits(
						xLab16EveGr,
						0x0700);
				xSemaphoreGive(xMutex);
			}
			
		}
		vTaskDelay(pdMS_TO_TICKS(5000));
	}
}

//task will prepare  LED1 for synchronization with LED2 and LED3
//wait for BIT_SW1_STATE until expire
//set BIT_LED1_SYNC and synchro with bits and start LED1BlinkingTimer
static void prvLED1Func(void * pvParams){
	for (;;){
		EventBits_t bitSW1 = xEventGroupWaitBits(
						xLab16EveGr,
						BIT_SW1_STATE,
						pdTRUE,
						pdFALSE,
						TICK_TO_WAIT);
		if ((bitSW1 & 0x02) == 0x02 ) //@return
		{	
			LED_R_Toggle();
			LED1_Set();
			EventBits_t bitLED1 = xEventGroupSync(
							xLab16EveGr,
							BIT_LED1_SYNC,
							0x54, //BIT_LED1_SYNC | BIT_LED2_SYNC | BIT_LED3_SYNC
							portMAX_DELAY);
			if ((bitLED1 & 0x54) == 0x54) //@return
				{
				//start xLED1BlinkingTimer
				xTimerStart(xLED1BlinkingTimer, 0);
			}
		}
		vTaskDelay(pdMS_TO_TICKS(1000));				
	}	
}

//task will prepare LED2 for synchronization with LED1 and LED3
//wait for BIT_SW2_STATE until expire
//set BIT_LED2_SYNC and synchro with bits and start LED2BlinkingTimer
static void prvLED2Func(void * pvParams){
	for (;;){
		EventBits_t bitSW2 = xEventGroupWaitBits(
						xLab16EveGr,
						BIT_SW2_STATE,
						pdTRUE,
						pdFALSE,
						TICK_TO_WAIT);
		if ((bitSW2 & 0x08 ) == 0x08){ //@return
			LED_G_Toggle();
			LED2_Set();
			EventBits_t bitLED2 = xEventGroupSync(
						xLab16EveGr,
						BIT_LED2_SYNC,
						0x54,
						//TICK_TO_WAIT);
						portMAX_DELAY);
			if ((bitLED2 & 0x54) == 0x54){ //@return
				//start xLED2BlinkingTimer
				xTimerStart(xLED2BlinkingTimer, 0);
			}
		}
		vTaskDelay(pdMS_TO_TICKS(1000));
	}	
}

//task will prepare LED3 for synchronization with LED1 and LED2
//wait for BIT_SW3_STATE until expire
//set BIT_LED3_SYNC and synchro with bits and start LED3BlinkingTimer
static void prvLED3Func(void * pvParams){
	for (;;){
		EventBits_t bitSW3 = xEventGroupWaitBits(
						xLab16EveGr,
						BIT_SW3_STATE,
						pdTRUE,
						pdFALSE,
						TICK_TO_WAIT);
		if ((bitSW3 & 0x20) == 0x20){// @return
			LED_B_Toggle();
			LED3_Set();
			EventBits_t bitLED3 = xEventGroupSync(
						xLab16EveGr,
						BIT_LED3_SYNC,
						0x54,
						portMAX_DELAY);
			if ((bitLED3 & 0x54) == 0x54){ //@return 
				//start xLED3BlinkingTimer
				xTimerStart(xLED3BlinkingTimer, 0);
			}
		}
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

//this task is for service
static void prvLEDRGBFunc(void * pvParams){
	for (;;){
		if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){
			prvShowMsg("LEDRGB and SW4 \r\n");
			xSemaphoreGive(xMutex);
		}
		vTaskDelay(pdMS_TO_TICKS(5000));
	}

}

/*******************************************************************************
 End of File
*/


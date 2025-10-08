/*
 
 */
#include <stdlib.h>		//define EXIT_FAILURE
#include <stdbool.h>		//define TRUE/FALSE
#include <stddef.h>		//define NULL

#include "FreeRTOS.h"
#include "Lab17_Timers.h"
//#include "FreeRTOS.h"
#include "timers.h"
#include "semphr.h"
#include "Lab17_DMA.h"
#include "Lab17_UART6.h"
#include "plib_gpio.h"
#include "Lab17_STAmac.h"
#include "Lab17_gpio.h"

#define KEY_PRESS_STA 0
#define DEBOUNCE_PERIOD 50

extern SemaphoreHandle_t xMutex;
extern Lab17State_t currentSTA;
extern TaskHandle_t xTaskSTAHandle;

TimerHandle_t xSWxDebounceTimer[SW_COUNT];

StaticTimer_t xDebounceTimerBuffer[SW_COUNT];

//create a callback function for debounce timer
//you should create a generic callback for 04 debunce timers for SW1, SW2, SW3, SW4
//you should create static inline functions to wrap around the MCC macro. the library Lab17_gpio.h will store these inline functions
void vDebounceTimerCallback(TimerHandle_t xTimer){
	for (uint8_t dtc = 0; dtc < SW_COUNT; dtc++){
		if (getSw[dtc]() == KEY_PRESS_STA ){
			BaseType_t xHPTW = pdFALSE;
			xTaskNotifyIndexedFromISR(
					xTaskSTAHandle,
					1,
					(1U << dtc),
					eSetBits,
					&xHPTW);
			portEND_SWITCHING_ISR(xHPTW);
			
			break;
		}
	}
}

void Lab17_TimersInit(void){
	
	//create a software timer for debouncing key press
	//you should use a loop to create debounce timers instead of copy/paste the code snippet
	
	for (uint8_t i =0; i < SW_COUNT ; i++){
		xSWxDebounceTimer[i] = xTimerCreateStatic(
					"debounce timer",
					DEBOUNCE_PERIOD,
					pdFALSE,
					(void *)&i,
					vDebounceTimerCallback,
					&xDebounceTimerBuffer[i]);
	
		if (xSWxDebounceTimer[i] == NULL){
			Debug_msg("cannot create timer for deboucing SW1 key press \r\n");
			exit(EXIT_FAILURE);
		} 
	}
}
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
#define LED1_BLG 1000
#define LED2_BLG 2000
#define LED3_BLG 4000

#define LED_R_BLG 500
#define LED_G_BLG 1000
#define LED_B_BLG 2000

#define SW0_STA (1U << 0)
#define SW1_STA (1U << 1)
#define SW2_STA (1U << 2)
#define SW3_STA (1U << 3)



extern SemaphoreHandle_t xMutex;
extern Lab17State_t currentSTA;
extern TaskHandle_t xTaskSTAHandle;

//TimerHandle_t xSW1DebounceTimer;
TimerHandle_t xSWxDebounceTimer[SW_COUNT];
//StaticTimer_t xSW1DebounceTimerBuffer;
StaticTimer_t xDebounceTimerBuffer[SW_COUNT];

//create a callback function for debounce timer
//you should create a generic callback for 04 debunce timers for SW1, SW2, SW3, SW4
//you should create static inline functions to abstract the MCC macro. the library Lab17_gpio.h will store these inline functions
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

//void prvSW1DebounceTimerCallback(TimerHandle_t xTimer){
//	if (SW1_Get() == KEY_PRESS_STA){
//		BaseType_t xHPTW = pdFALSE;
//		xTaskNotifyIndexedFromISR(
//				xTaskSTAHandle,
//				1,
//				SW1_STA,
//				eSetBits,
//				&xHPTW);
//		portEND_SWITCHING_ISR(xHPTW);
//		

		
//		currentSTA = SW1_ENT;
		
//		if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){
//			vComPortMsg("SW1 is pressed \r\n");
//			xSemaphoreGive(xMutex);
//		}
//		LED1_Toggle();
		
//	}
//}


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
	
	
	
//	 xSW1DebounceTimer = xTimerCreateStatic(
//					"debounce timer for key press",
//					DEBOUNCE_PERIOD,
//					pdFALSE,
//					(void *)1,
//					prvSW1DebounceTimerCallback,
//					&xSW1DebounceTimerBuffer); 
		if (xSWxDebounceTimer[i] == NULL){
			Debug_msg("cannot create timer for deboucing SW1 key press \r\n");
			exit(EXIT_FAILURE);
		} 
	}
//	else {
//		Debug_msg("oooo \r\n");
//	}
}
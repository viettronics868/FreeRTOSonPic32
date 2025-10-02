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

#define KEY_PRESS_STA 0
#define DEBOUNCE_PERIOD 50
#define LED1_BLG 1000
#define LED2_BLG 2000
#define LED3_BLG 4000

#define LED_R_BLG 500
#define LED_G_BLG 1000
#define LED_B_BLG 2000

extern SemaphoreHandle_t xMutex;
//extern EventGroupHandle_t xLab17EveGr;
extern Lab17State_t currentSTA;

TimerHandle_t xSW1DebounceTimer;
StaticTimer_t xSW1DebounceTimerBuffer;

//create a callback function for debounce timer
void prvSW1DebounceTimerCallback(TimerHandle_t xTimer){
	if (SW1_Get() == KEY_PRESS_STA){
		
		currentSTA = SW1_ENT;
		
		
//		if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){
//			vComPortMsg("SW1 is pressed \r\n");
//			xSemaphoreGive(xMutex);
//		}
//		LED1_Toggle();
		
	}
}

void Lab17_TimersInit(void){
	
	//create a software timer for debouncing key press
	 xSW1DebounceTimer = xTimerCreateStatic(
					"debounce timer for key press",
					DEBOUNCE_PERIOD,
					pdFALSE,
					(void *)1,
					prvSW1DebounceTimerCallback,
					&xSW1DebounceTimerBuffer); 
	if (xSW1DebounceTimer == NULL){
		Debug_msg("cannot create timer for deboucing SW1 key press \r\n");
		exit(EXIT_FAILURE);
	} 
//	else {
//		Debug_msg("oooo \r\n");
//	}
}
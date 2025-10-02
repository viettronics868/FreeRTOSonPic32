/*
 
 */

#include "FreeRTOS.h"
#include "Lab17_Timers.h"
#include "Lab17_SWxISR.h"
#include "Lab17_UART6.h"
#include "plib_gpio.h"
#include "timers.h"

extern TimerHandle_t xSW1DebounceTimer;

//implement callback for SW1 pressing and ISR firing
void vSW1ISRCallback(GPIO_PIN pin, uintptr_t context){
	//start debounce timer for SW key press
	BaseType_t xHPTW = pdFALSE;
	if (xTimerStartFromISR(
			xSW1DebounceTimer,
			&xHPTW) == pdFALSE){
		Debug_msg("no timer \r\n");
		exit(EXIT_FAILURE);
	}
	portEND_SWITCHING_ISR(xHPTW);
	//Debug_msg("oooo \r\n");
}

void Lab17_SWxISRInit(void){
	GPIO_PinInterruptCallbackRegister(
				SW1_PIN,
				vSW1ISRCallback,
				0);
	GPIO_PinInterruptEnable(SW1_PIN);
}


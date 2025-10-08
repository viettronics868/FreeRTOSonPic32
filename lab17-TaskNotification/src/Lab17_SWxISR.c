/*
 
 */

#include "FreeRTOS.h"
#include "Lab17_Timers.h"
#include "Lab17_SWxISR.h"
#include "Lab17_UART6.h"
#include "plib_gpio.h"
#include "timers.h"

extern TimerHandle_t xSW1DebounceTimer;
extern TimerHandle_t xSWxDebounceTimer[SW_COUNT];

GPIO_PIN SW_[SW_COUNT] = {SW0_PIN, SW1_PIN, SW2_PIN, SW3_PIN};

//implement generic ISR-safe callback
void vSWxISRCallback(GPIO_PIN pin, uintptr_t context){
	BaseType_t xHPTW = pdFALSE;
	for (uint8_t sdt = 0; sdt < SW_COUNT; sdt++){
		if (pin == SW_[sdt]){
			if (xTimerStartFromISR(
					xSWxDebounceTimer[sdt],
					&xHPTW) == pdFALSE){
				Debug_msg("cannot start timer \r\n");
				exit(EXIT_FAILURE);
			}
				
			break;
		}
	}
}


//implement callback for SW1 pressing and ISR firing
//void vSW1ISRCallback(GPIO_PIN pin, uintptr_t context){
//	//start debounce timer for SW key press
//	BaseType_t xHPTW = pdFALSE;
//	if (xTimerStartFromISR(
//			xSW1DebounceTimer,
//			&xHPTW) == pdFALSE){
//		Debug_msg("no debounce timer for SW1 \r\n");
//		exit(EXIT_FAILURE);
//	}
//	portEND_SWITCHING_ISR(xHPTW);
//	
//}

void Lab17_SWxISRInit(void){
	
	for (uint8_t isr = 0; isr < SW_COUNT ; isr++){
		GPIO_PinInterruptCallbackRegister(
					SW_[isr],
					//vSW1ISRCallback,
					vSWxISRCallback,
					0);
		GPIO_PinInterruptEnable(SW_[isr]);
	}
}


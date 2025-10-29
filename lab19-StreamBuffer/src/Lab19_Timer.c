/*
 * Lab19_Timer.c / .h are using an Idle timer to check periodically timeout (50ms) to flush UART6 FIFO buffer 
 * Implementation of the Timer-based "idle flush" mechanism .
 * this Lab is also building a custom timer API layer on top of FreeRTOS. It is the first step of building system frameworks.
 * This is the same principle used in:
 * - Microchip Harmony Framework
 * - STM32 HAL abstraction layers
 * - QP/QXK event frameworks
 * - modern RTOS-based middleware
 */

#include "FreeRTOS.h"
#include "timers.h"
#include "Lab19_Timer.h"
#include "Lab19_DMA.h"
#include "Lab19_config.h"
#include "Lab19_UART.h"
#include "stream_buffer.h"

/**
 * @brief Safely executes the custom FreeRTOS timer api (Start/Stop/Reset)
 * with retry and optional logging.
 *
 * @param func		Pointer to the original timer API function (xTimerStart, xTimerStop, xTimerReset)
 * @param xTimer		Handle of the timer to control
 * @param xTicksToWait	How long to wait if the first attempt fails (bounded wait)
 * @return			pdPASS if successful, pdFAIL otherwise
 */

//implementation the helper of the macro xTimerStart()
BaseType_t TimerStart(TimerHandle_t xTimer, TickType_t xTicksToWait){
	return xTimerStart(xTimer, xTicksToWait);
}
//implementation the helper of the macro xTimerStop()
BaseType_t TimerStop(TimerHandle_t xTimer, TickType_t xTicksToWait){
	return xTimerStop(xTimer, xTicksToWait);
}
//implementation the helper of the macro xTimerReset()
BaseType_t TimerReset(TimerHandle_t xTimer, TickType_t xTicksToWait){
	return xTimerStop(xTimer, xTicksToWait);
}
TimerEvent_t eventTimer;
BaseType_t customTimerHelper(
			//func is pointer to a function that takes (TimerHandle_t, TickType_t) and returns BaseType_t
			BaseType_t (*func)(TimerHandle_t xTimer, TickType_t xTicksToWait),
			TimerHandle_t xTimer,
			TickType_t xTicksToWait){
	BaseType_t result = func(xTimer, 0);
	if (result != pdPASS){
		result = func(xTimer, xTicksToWait);
	}
	
	//the block #ifdef ... #endif makes this line of code is for debug only
#ifdef DEBUG
	if (result != pdPASS){
		Debug6_msg("[WARN] custom Timer API failed\r\n");
	}
#endif

	return result;
}

BaseType_t customTimerAPI(TimerHandle_t xTimer, TimerEvent_t event){
	BaseType_t result = pdFAIL;
	switch (event){
	case EVENT_START:		
		result = customTimerHelper(TimerStart, xTimer, pdMS_TO_TICKS(10));
		break;
	case EVENT_STOP:		
		result = customTimerHelper(TimerStop, xTimer, pdMS_TO_TICKS(10));
		break;
	case EVENT_RESET:		
		result = customTimerHelper(TimerReset, xTimer, pdMS_TO_TICKS(10));
		break;
	default: break;
	}	
	return result;
}


//idle timer 50 ticks a.k.a 50ms
//static timer

TimerHandle_t xRxIdleTimer;
StaticTimer_t xRxIdleTimerBuffer;
extern StreamBufferHandle_t xUart6RxStream;

TimerHandle_t xRequestTimer;
StaticTimer_t xRequestTimerBuffer;

//create Idle Timer here and then start Idle Timer in task State Machine
void vLab19_Timer_init(void){
	xRxIdleTimer = xTimerCreateStatic(
				"Idle Timer for flushing UART6 FIFO buffer",
				IDLE_TIMEOUT,
				pdTRUE,  //auto-reload
				(void *)1,
				vIdleTimerHandler,
				&xRxIdleTimerBuffer
				);
	if (xRxIdleTimer == NULL){
		
		//the block #ifdef ... #endif makes this line of code is for debug only
#ifdef DEBUG
		Debug6_msg("cannot create idle timer \r\n");
#endif
		
		exit(EXIT_FAILURE);
	}
	
	xRequestTimer = xTimerCreateStatic(
				"timer for requesting enter a message",
				REQUEST_TIMEOUT,
				pdFALSE,  //single shot timer
				(void *)2,
				vRequestTimerHandler,
				&xRequestTimerBuffer
				);
	if (xRequestTimer == NULL){
		//the block #ifdef ... #endif makes this line of code is for debug only
#ifdef DEBUG
		Debug6_msg("cannot create request time\r\n");
#endif

		exit(EXIT_FAILURE);
	}
}

void vIdleTimerHandler(TimerHandle_t xTimer){
		
	BaseType_t xHPTW = pdFALSE;
	
	while (_U6STA_URXDA_MASK == (U6STA & _U6STA_URXDA_MASK)){
		uint8_t charac = U6RXREG;
		xStreamBufferSendFromISR(
					xUart6RxStream,
					&charac,
					1,
					&xHPTW);		
	}	
	portEND_SWITCHING_ISR(xHPTW);
}

void vRequestTimerHandler(TimerHandle_t xTimer){
	//using Request Timer's timeout to ask user to enter a message - using DMA1 and UART1 TX
	//then reset xRequestTimer
	vShowMsgD1U1("please enter a message and press Enter\r\n");
	BaseType_t xHPTW = pdFALSE;
	if (xTimerResetFromISR(
			xRequestTimer, 
			&xHPTW) == pdFAIL){
		Debug6_msg("cannot reset request timer\r\n");
	}
	portEND_SWITCHING_ISR(xHPTW);
}


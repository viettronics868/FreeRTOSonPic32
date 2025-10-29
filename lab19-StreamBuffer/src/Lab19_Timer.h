/* 
 * File:   Lab19_Timer.h
 * Author: tuiday
 *
 * Created on October 18, 2025, 12:22 PM
 */

#pragma once

#ifndef LAB19_TIMER_H
#define	LAB19_TIMER_H


#include "FreeRTOS.h"
#include "timers.h"
#include "Lab19_config.h"

/**
 * @brief Safely executes the custom FreeRTOS timer api (Start/Stop/Reset)
 * with retry and optional logging.
 *
 * @param fn		Pointer to the original timer API function (xTimerStart, xTimerStop, xTimerReset)
 * @param xTimer		Handle of the timer to control
 * @param xTicksToWait	How long to wait if the first attempt fails (bounded wait)
 * @return			pdPASS if successful, pdFAIL otherwise
 */

BaseType_t customTimerHelper(
                                            BaseType_t (*func)(TimerHandle_t, TickType_t),
                                            TimerHandle_t xTimer,
                                            TickType_t xTicksToWait);

BaseType_t customTimerAPI(TimerHandle_t xTimer, TimerEvent_t event);

void vLab19_Timer_init(void);
void vIdleTimerHandler(TimerHandle_t xTimer);
void vRequestTimerHandler(TimerHandle_t xTimer);

#ifdef	__cplusplus
extern "C" {
#endif




#ifdef	__cplusplus
}
#endif

#endif	/* LAB19_TIMER_H */


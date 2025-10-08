/* 
 * File:   Lab17_Timers.h
 * Author: tuiday
 *
 * Created on September 29, 2025, 7:42 AM
 */

#ifndef LAB17_TIMERS_H
#define	LAB17_TIMERS_H

#include "timers.h"

#define SW_COUNT 4

void Lab17_TimersInit(void);
void prvSW1DebounceTimerCallback(TimerHandle_t xTimer);

#ifdef	__cplusplus
extern "C" {
#endif




#ifdef	__cplusplus
}
#endif

#endif	/* LAB17_TIMERS_H */


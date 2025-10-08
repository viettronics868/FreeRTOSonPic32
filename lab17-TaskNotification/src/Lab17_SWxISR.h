/* 
 * File:   Lab17_SWxISR.h
 * Author: tuiday
 *
 * Created on September 30, 2025, 8:11 PM
 */

#pragma once

#ifndef LAB17_SWXISR_H
#define	LAB17_SWXISR_H

#include "plib_gpio.h"

void vSW1ISRCallback(GPIO_PIN pin, uintptr_t context);
void Lab17_SWxISRInit();

#ifdef	__cplusplus
extern "C" {
#endif




#ifdef	__cplusplus
}
#endif

#endif	/* LAB17_SWXISR_H */


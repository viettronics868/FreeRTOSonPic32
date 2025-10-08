/* 
 * File:   Lab17_gpio.h
 * Author: tuiday
 *
 * Created on October 6, 2025, 9:11 PM
 */

#pragma once

#ifndef LAB17_GPIO_H
#define	LAB17_GPIO_H

#include "FreeRTOS.h"
#include "plib_gpio.h"
#include <stdint.h>
#include "Lab17_Timers.h"

//create inline wrapper around MCC macros
static inline uint32_t vGetSW0(void) {return SW0_Get();}
static inline uint32_t vGetSW1(void) {return SW1_Get();}
static inline uint32_t vGetSW2(void) {return SW2_Get();}
static inline uint32_t vGetSW3(void) {return SW3_Get();}

//make function pointer types
typedef uint32_t (*GetSW_t)(void);

//declare and initialize an array from the new type
extern GetSW_t getSw[SW_COUNT]; 

static inline void vLED0Toggle(void) {LED0_Toggle();}
static inline void vLED1Toggle(void) {LED1_Toggle();}
static inline void vLED2Toggle(void) {LED2_Toggle();}
static inline void vLEDRGBToggle(void) {LED_R_Toggle();
                                                                LED_B_Toggle();
                                                                LED_G_Toggle();}

typedef void (*LedToggle_t)(void);

extern LedToggle_t ledToggle[SW_COUNT]; 

#ifdef	__cplusplus
extern "C" {
#endif




#ifdef	__cplusplus
}
#endif

#endif	/* LAB17_GPIO_H */


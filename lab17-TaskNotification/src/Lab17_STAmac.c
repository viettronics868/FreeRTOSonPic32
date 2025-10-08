/*
 * File:   Lab17_STAmac.c
 * State Machine core logic
 * 
 * Author: L.K
 *
 * Created on September 27, 2025, 10:20 PM
 
 */

#include "FreeRTOS.h"
#include "Lab17_STAmac.h"
#include "Lab17_Timers.h"
#include "Lab17_gpio.h"
#include "task.h"
#include "semphr.h"
#include "Lab17_DMA.h"
#include "limits.h"
#include "plib_gpio.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>

//#define BIT_SW1 (1U << 1)
//#define BIT_SW2 (1U << 2)
//#define BIT_SW3 (1U << 3)
//#define BIT_SW4 (1U << 4)

Lab17State_t currentSTA = INIT_STA;
extern SemaphoreHandle_t xMutex;

static char dmaStr[50] = {0};

void vLab17STAmac(void * pvParams){
	(void) pvParams;
	
	uint32_t ulEvents;
	for (;;){
		switch (currentSTA){ 
		case (INIT_STA):
			if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){
				vComPortMsg("Lab17 - Task Notification \r\n");
				xSemaphoreGive(xMutex);
			}
			currentSTA = DEPLOY_STA;
			break;
		case (DEPLOY_STA):		
			if (xTaskNotifyWaitIndexed(		
					1,
					0,
					ULONG_MAX,
					&ulEvents,
					portMAX_DELAY) == pdPASS){
				for (uint8_t tnwi = 0; tnwi < SW_COUNT; tnwi++){
					if (ulEvents & (1U << (tnwi))){
						ledToggle[tnwi]();
						sprintf(dmaStr, "switch %d pressed and toggled led %d \r\n ", tnwi, tnwi);
						if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){
							vComPortMsg(dmaStr);
							xSemaphoreGive(xMutex);
						}
					}
				}

			}
			break;
		default: break;
		}
	}
}


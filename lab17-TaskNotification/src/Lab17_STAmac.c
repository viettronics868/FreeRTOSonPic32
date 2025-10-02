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

#include "task.h"
#include "semphr.h"
#include "Lab17_DMA.h"
#include "limits.h"
#include "plib_gpio.h"
#include <string.h>
#include <stdio.h>



extern Lab17State_t currentSTA;
extern SemaphoreHandle_t xMutex;

void vLab17STAmac(void * pvParams){
	(void) pvParams;
	for (;;){
		switch (currentSTA){
		case INIT_STA:
			
			if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){
				vComPortMsg("lab 17 - Task Notification-20250930 \r\n");
//				ulTaskNotifyTakeIndexed(
//						0,
//						pdTRUE,
//						0);
				xSemaphoreGive(xMutex);
			}
			
			currentSTA = IDLE_STA;
			
			break;
			
		case SW1_ENT:
			LED1_Toggle();
			if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){
				vComPortMsg("    SW 1 press \r\n ");
				xSemaphoreGive(xMutex);
			}
			
			currentSTA = IDLE_STA;
			break;
			
		case SW2_ENT:
			break;
			
		case SW3_ENT:
			break;
			
		case SW4_ENT:
			break;
			
		case IDLE_STA:
			
//			UBaseType_t getStack = uxTaskGetStackHighWaterMark(NULL);
//			char idleStr[30];
//			sprintf((char *)idleStr, "stack balance %ld \r\n", getStack);
			
			if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){
				//vComPortMsg(idleStr);
				vComPortMsg("idle state \r\n");
				xSemaphoreGive(xMutex);
			}
			break;
		}
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}


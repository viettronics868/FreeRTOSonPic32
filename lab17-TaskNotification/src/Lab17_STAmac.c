/*
 * File:   Lab17_STAmac.c
 * State Machine core logic
 * 
 * Author: L.K
 *
 * Created on September 27, 2025, 10:20 PM
 
 */

#include "Lab17_STAmac.h"
#include "FreeRTOS.h"
#include "task.h"
#include "third_party/rtos/FreeRTOS/Source/include/semphr.h"
#include "Lab17_DMA.h"
#include "limits.h"



extern Lab17State_t currentSTA;
extern SemaphoreHandle_t xMutex;

void vLab17STAmac(void * pvParams){
	(void) pvParams;
	for (;;){
		switch (currentSTA){
		case INIT_STA:
			
			if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){
				vComPortMsg("lab 17 - Task Notification \r\n ");
				ulTaskNotifyTakeIndexed(
					0,
					pdTRUE,
					0);
				xSemaphoreGive(xMutex);
			}
			
			break;
			
		case SW1_STA:
			break;
			
		case SW2_STA:
			break;
			
		case SW3_STA:
			break;
			
		case SW4_STA:
			break;
			
		case IDLE_STA:
			break;
		}
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}


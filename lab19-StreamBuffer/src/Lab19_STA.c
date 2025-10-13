/*
 
 */
#include "FreeRTOS.h"
#include "task.h"
#include "Lab19_STA.h"
#include "Lab19_UART.h"
#include "plib_gpio.h"

TaskHandle_t xTaskSTAHandle = NULL;
StaticTask_t xTaskSTATcbBuffer;
StackType_t xTaskSTAStackBuffer[configMINIMAL_STACK_SIZE];
Lab19States_t currentState = INIT_STA;

void xTaskSTAma(void * pvParams){
	for (;;){
		switch (currentState){
		case INIT_STA:
			Debug1_msg("INIT- com 13 \r\n");
			currentState = DEPLOY_STA;
			LED_G_Toggle();
			LED_B_Toggle();
			break;
		case DEPLOY_STA:
			Debug6_msg("DEPLOY- com 7 \r\n");
			currentState = INIT_STA;
			LED_R_Toggle();
			LED_G_Toggle();
			break;
		}
		
		vTaskDelay(pdMS_TO_TICKS(2000));
	}
}

void vLab19_STA_init(void){
	xTaskSTAHandle = xTaskCreateStatic(
				xTaskSTAma,
				"task of State Machine core",
				configMINIMAL_STACK_SIZE,
				NULL,
				tskIDLE_PRIORITY,
				xTaskSTAStackBuffer,
				&xTaskSTATcbBuffer);
	if (xTaskSTAHandle == NULL){
		Debug1_msg("cannot create the task of state machine \r\n");
		exit(EXIT_FAILURE);
	}
	
}
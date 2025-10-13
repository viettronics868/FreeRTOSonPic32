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
	for (; ; ){
		switch (currentState){
		case INIT_STA:
			Debug1_msg("\r\n enter character on COM13\r\n");
			for (uint8_t uart1=0; uart1 < 5; uart1++){
				Echo_msg_13_07();
				//Debug1_msg("\r\n");
			}
			currentState = DEPLOY_STA;
			LED_G_Toggle();
			LED_B_Toggle();
			
			break;
		case DEPLOY_STA:
			Debug6_msg("\r\n enter character on COM7 \r\n");
			for (uint8_t uart6=0; uart6 < 5; uart6++){
				Echo_msg_07_13();
				//Debug6_msg("\r\n");
			}
			currentState = INIT_STA;
			LED_R_Toggle();
			LED_G_Toggle();
			break;
		}
		
		//vTaskDelay(pdMS_TO_TICKS(100));
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
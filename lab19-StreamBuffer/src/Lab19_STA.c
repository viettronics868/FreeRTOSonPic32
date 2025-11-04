/*
 * the files Lab19_STA.c /.h implement a non-blocking, full-duplex UART console engine with event-driven output.
 * this design is used in production-grade firmware for routers, drones and medical devices.
 
 */
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "Lab19_STA.h"
#include "Lab19_UART.h"
#include "Lab19_STREAMBUFFER.h"
#include "Lab19_config.h"
#include "Lab19_DMA.h"
#include "plib_gpio.h"
#include "timers.h"
#include "stream_buffer.h"
#include "Lab19_Timer.h"

TaskHandle_t xTaskSTAHandle = NULL;
StaticTask_t xTaskSTATcbBuffer;
StackType_t xTaskSTAStackBuffer[configMINIMAL_STACK_SIZE];

Lab19States_t currentState;

extern uint8_t ucRxStreamBuffer[RX_STREAM_BUFFER_SIZE];
extern TimerHandle_t xRxIdleTimer;
extern TimerHandle_t xRequestTimer;
extern StreamBufferHandle_t xUart6RxStream;
extern uint8_t u1TxBuffer[TX1_BUFFER_SIZE];

uint8_t count_loop = 0;
char count_msg[30];
uint8_t input_charac;
uint8_t cpy_charac;
size_t leng_of_string = 0;
size_t size_of = 0;
uint32_t NotificationValue;

void xTaskSTAma(void * pvParams){	
	
	currentState = INIT_STA;
	
	for (; ; ){
		switch (currentState){
		case INIT_STA:
			
			count_loop++;			
						
			vShowMsgD1U1("Lab19 - Stream Buffer  \r\n");
			
			LED0_Toggle();
#ifdef DEBUG			
			Debug6_msg("ohoh\r\n");//checking UART6 TX 
			sprintf((char *)count_msg, "counted %d state %d \r\n", count_loop, currentState);
			vShowMsgD1U1((char *)count_msg);
#endif			
			currentState = WAIT_MSG_STA;
			//currentState = NOTIFY_STA;
						
			//start Idle Timer auto-reload to check periodically U6STAbits.URXDA
			//using customTimerAPI()
			if (customTimerAPI(xRxIdleTimer, EVENT_START) == pdFAIL){
#ifdef DEBUG
				Debug6_msg("cannot start Idle Timer \r\n");
#endif
				exit(EXIT_FAILURE);
			}
			
			break;
			
//		
			
		case WAIT_MSG_STA:
			
			LED2_Toggle();			
						
			//this variable for debug only
			count_loop++;			
			
			//start REQUEST timer - 5s - single-shot
			//apply defensive pattern
			if (customTimerAPI(xRequestTimer, EVENT_START) == pdFAIL){
#ifdef DEBUG
				Debug6_msg("cannot start Request Timer\r\n");
#endif
				exit(EXIT_FAILURE);
			}			
			
			
			
			//for debug only
#ifdef DEBUG
			sprintf((char *)count_msg, "counted %d state %d \r\n", count_loop, currentState);
			vShowMsgD1U1((char *)count_msg);
#endif											
			//block the task for xStreamBufferReceive()
			//check ENTER key for changing the state into STRM_U1TX_STA
			
			if (xStreamBufferReceive(
					xUart6RxStream,
					&cpy_charac,
					1,
					portMAX_DELAY) >0 )
			
			{					
				//echo the character immediately on Terminal for UART6 
				//using compound literal / inline string built at runtime
				Debug6_msg((char[]){cpy_charac,'\0'});
				
				//check for Enter key press
				if ((cpy_charac == '\n')){	
					//stop all timers - IDLE Timer and REQUEST Timer
					//apply defensive pattern when stopping timers and logging if it fails
					if (xTimerIsTimerActive(xRxIdleTimer) != pdFALSE){
						
						if (customTimerAPI(xRxIdleTimer, EVENT_STOP) == pdFALSE)
						{
							
						//the block #ifdef ... #endif makes this line of code is only for debug
#ifdef DEBUG
							Debug6_msg("[WARN] IDLE timer stop failed\r\n");
#endif
						}
					}
					
					if (xTimerIsTimerActive(xRequestTimer) != pdFALSE){
						
						if (customTimerAPI(xRequestTimer, EVENT_STOP) == pdFALSE)

						{
							
						//the block #ifdef ... #endif makes this line of code is for debug only
#ifdef DEBUG
							Debug6_msg("[WARN] REQUEST timer stop failed\r\n");
#endif
						}
					}
											
					ucRxStreamBuffer[leng_of_string] = cpy_charac;
					
					//copy current string into u1TxBuffer
					//sprintf((char *)u1TxBuffer, "%s" , (char *) ucRxStreamBuffer);
					memcpy((char*)u1TxBuffer, (char *)ucRxStreamBuffer, leng_of_string+1);
					
					//Reset stream buffer before starting the new round
					xStreamBufferReset(xUart6RxStream);
					
					
					//received Enter key and change State into STRM_U1TX_STA
					currentState = STRM_U1TX_STA;
					
					size_of = leng_of_string +1;					
					//size_of = leng_of_string;
					leng_of_string = 0; // reset the length for the next messages
										
				} else {
					ucRxStreamBuffer[leng_of_string++] = cpy_charac;
					
										
				}
			}
				
			break;
		case STRM_U1TX_STA:
			
			count_loop++;
			
			//LED2_Toggle();			
			LED_R_Toggle();
			LED_G_Toggle();
			LED_B_Toggle();
			
			//send Stream Buffer to UART1 TX
			vStrmU1Tx((char *)u1TxBuffer, size_of);
										
			//start IDLE Timer
			customTimerAPI(xRxIdleTimer, EVENT_START);			
			
#ifdef DEBUG
			sprintf((char *)count_msg, "counted %d state %d \r\n", count_loop, currentState);
			vShowMsgD1U1((char *)count_msg);
#endif		
			
			currentState = WAIT_MSG_STA;			
			
			memset((char *)u1TxBuffer, 0, size_of);
			memset((char *)ucRxStreamBuffer, 0, size_of);
			
			break;
			
		default: 
			vTaskDelay(pdMS_TO_TICKS(10));
			break;		
		}
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
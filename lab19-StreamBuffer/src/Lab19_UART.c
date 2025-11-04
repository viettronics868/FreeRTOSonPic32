/*
 * Using MPLAB X IDE v6.25 and MCC v5.2.6 
 * UART1 and UART6 and all pins are configured via MCC configuration
 * Note: UART1 on curiosity 2.0 pic32 mz ef board need an external USB-TTL adaptor with 5V logic instead of 3.3V logic as
 * UART6 on PKoB4
 * Note: the code handles UART register-level operation and will work even if the bit position changes 
 * or if a future header defines it differently
 
 */

#include "FreeRTOS.h"
#include "xc.h"
#include "Lab19_config.h"
#include "timers.h"
#include "Lab19_STREAMBUFFER.h"
#include "stream_buffer.h"
#include "plib_uart6.h"
#include "plib_uart1.h"


extern Lab19States_t currentState; 
extern StreamBufferHandle_t xUart6RxStream;
extern TimerHandle_t xRxIdleTimer;
extern TimerHandle_t xRequestTimer;
extern TaskHandle_t xTaskSTAHandle;

//using UART1 and external USB_TTL adaptor (5V level) for showing messages on COM13
void Debug1_msg(char * msg1){
	while (*msg1){		
		while (_U1STA_UTXBF_MASK == (U1STA & _U1STA_UTXBF_MASK));
		U1TXREG = *msg1++;
		}
	while (!(_U1STA_TRMT_MASK == (U1STA & _U1STA_TRMT_MASK)));
}

//using UART1 and external USB-TTL adaptor for echoing characters on COM13
//for UART1 unit test only
void Echo_msg(void){
	while (!(_U1STA_URXDA_MASK == (U1STA & _U1STA_URXDA_MASK)));
	while (_U1STA_UTXBF_MASK == (U1STA & _U1STA_UTXBF_MASK));
	U1TXREG = U1RXREG;

	while (!(_U1STA_TRMT_MASK == (U1STA & _U1STA_TRMT_MASK)));
}

//using UART6 for inputting character a.k.a COM7
//for integration test UART1 and UART6
void Echo_msg_13_07(void){

	while (!(_U1STA_URXDA_MASK == (U1STA & _U1STA_URXDA_MASK)));
	while (_U6STA_UTXBF_MASK == (U6STA & _U6STA_UTXBF_MASK));
	U6TXREG = U1RXREG;
	while (!(_U6STA_TRMT_MASK == (U6STA & _U6STA_TRMT_MASK)));
}

//using UART6 for inputting character a.k.a COM7
//using UART1 for outputting character a.k.a COM13
//for integration test UART6 and UART1
void Echo_msg_07_13(void){
	
	while (!(_U6STA_URXDA_MASK == (U6STA & _U6STA_URXDA_MASK)));
	
	while (_U1STA_UTXBF_MASK == (U1STA & _U1STA_UTXBF_MASK));
	U1TXREG = U6RXREG;
	
	while( !(_U1STA_TRMT_MASK == (U1STA & _U1STA_TRMT_MASK)));
}

//using UART6 and built-in PKoB4 for showing messages on COM7
void Debug6_msg(char * msg6){
	while (*msg6){
		
		while (_U6STA_UTXBF_MASK == (U6STA & _U6STA_UTXBF_MASK));
		U6TXREG = *msg6++;
				
	}
	while (!(_U6STA_TRMT_MASK == (U6STA & _U6STA_TRMT_MASK)));
}

//declare a callback when ISR getting fire on UART6 RX
void U6RXCallback(uintptr_t context){
	BaseType_t xHPTW = pdFALSE;
	//a small loop inside UART ISRs ? to empty the FIFO completely.
	//It?s safe, deterministic, and required for data reliability.
	
	while (_U6STA_URXDA_MASK == (U6STA & _U6STA_URXDA_MASK)){
		uint8_t chr = U6RXREG;
		xStreamBufferSendFromISR(
					xUart6RxStream,
					&chr,
					1,
					&xHPTW);
		
		//Notify the core state machine task about 4-byte deep receive on UART6 RX
//		xTaskNotifyIndexedFromISR(
//					xTaskSTAHandle,
//					TIMER_U6RX_INDEX,
//					BIT_U6RX_FULL,
//					eSetBits,
//					&xHPTW);
				
	}
	
	//restart idle timer to flush FIFO buffer of UART6 RX
	xTimerResetFromISR(xRxIdleTimer, &xHPTW);
	
	portEND_SWITCHING_ISR(xHPTW);
}

//initializer UART6 for inputting a message via Terminal
void vLab19_UART6_init(void){
	
	//interrupt threshold is full FIFO a.k.a 4 bytes
	U6STAbits.URXISEL0 = 1;
	U6STAbits.URXISEL1 = 1;
	
	UART6_ReadCallbackRegister(
				U6RXCallback,
				0);
}


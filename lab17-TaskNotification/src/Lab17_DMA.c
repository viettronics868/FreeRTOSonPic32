/*
 
 */

#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "plib_cache.h"
#include "device_cache.h"
#include "Lab17_DMA.h"
#include "plib_dmac.h"

extern TaskHandle_t xTaskSTAHandle;

//declare the buffer for DMAC0
static uint8_t __attribute__ ((aligned (16))) u6TxBuffer[128] = {0};

//register callback for DMAC ISR
void Lab17_DMAInit(void){
	DMAC_ChannelCallbackRegister(
				DMAC_CHANNEL_0,
				vDMA0Callback,
				0);
}

//function to transfer string from buffer to UART6
//using index 0 of task notification for signaling DMA completion
void vComPortMsg(char * msg){
	sprintf((char *)u6TxBuffer, msg);
	DCACHE_CLEAN_BY_ADDR(
				(uint32_t)u6TxBuffer,
				strlen((char *)u6TxBuffer)
			);
	DMAC_ChannelTransfer(
				DMAC_CHANNEL_0,
				(const void *)u6TxBuffer,
				strlen((char *)u6TxBuffer),
				(const void *)&U6TXREG, 1, 1);
	ulTaskNotifyTakeIndexed(
				0, 
				pdTRUE,
				0);
}

//DMAC callback
void vDMA0Callback(DMAC_TRANSFER_EVENT event, uintptr_t contextHandle){
	if (event == DMAC_TRANSFER_EVENT_COMPLETE){
		BaseType_t xHPTW = pdFALSE;
		xTaskNotifyIndexedFromISR(
			xTaskSTAHandle,	//task handle of the task of state machine Lab17STAmac
			0,	//index 0 for DMA
			0,	// value is ignored for eIncrement
			eIncrement,
			&xHPTW);
		portEND_SWITCHING_ISR(xHPTW);
		
	}
}
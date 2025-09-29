/*
 
 */

#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "config/default/peripheral/cache/plib_cache.h"
#include "config/default/device_cache.h"
#include "Lab17_DMA.h"
#include "config/default/peripheral/dmac/plib_dmac.h"

extern TaskHandle_t xTaskSTAHandle;
//extern uint8_t __attribute__ ((aligned (16)))u6TxBuffer[128];
extern uint8_t u6TxBuffer[128];

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
}

void vDMA0Callback(DMAC_TRANSFER_EVENT event, uintptr_t contextHandle){
	if (event == DMAC_TRANSFER_EVENT_COMPLETE){
		BaseType_t xHPTW = pdFALSE;
		xTaskNotifyIndexedFromISR(
			xTaskSTAHandle,	//task handle of Lab17STAmac
			0,	//index 0 for DMA
			0,	// value is ignored for eIncrement
			eIncrement,
			&xHPTW);
		portEND_SWITCHING_ISR(xHPTW);
		
	}
}
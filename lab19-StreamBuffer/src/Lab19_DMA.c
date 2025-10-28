/*
 
 */

#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "plib_dmac.h"
#include "device_cache.h"
#include "plib_cache.h"
#include "Lab19_STA.h"
#include "Lab19_config.h"


//buffer for outputting to UART1
extern uint8_t u1TxBuffer[TX1_BUFFER_SIZE] ;
//buffer for inputting to UART6
extern uint8_t u6RxBuffer[RX6_BUFFER_SIZE];

extern SemaphoreHandle_t xSemBin;
extern SemaphoreHandle_t xMutex;

void vLab19DMA1Callback(DMAC_TRANSFER_EVENT event, uintptr_t context){
	if (event == DMAC_TRANSFER_EVENT_COMPLETE){
		BaseType_t xHPTW = pdFALSE;
		xSemaphoreGiveFromISR(xSemBin,&xHPTW);
		portEND_SWITCHING_ISR(xHPTW);
	}
}

void vLab19_DMA1_init(void){
	DMAC_ChannelCallbackRegister(
				DMAC_CHANNEL_1,
				vLab19DMA1Callback,
				0);
}



void vShowMsgD1U1(char * msg){
	
	//sprintf((char *)u1TxBuffer,"%s", msg);
	if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){
		DCACHE_CLEAN_BY_ADDR(
					//(uint32_t)u1TxBuffer,
			(uint32_t)msg,
					//strlen((char *)u1TxBuffer));
			strlen((char *)msg));
		DMAC_ChannelTransfer(
				DMAC_CHANNEL_1,
				//(const void *)u1TxBuffer,
			(const void *)msg,
				//strlen((const char*)u1TxBuffer),
			strlen((const char *)msg),
				(const void *)&U1TXREG, 1, 1);
		xSemaphoreTake(xSemBin, portMAX_DELAY);

		xSemaphoreGive(xMutex);
	}
}

void vStrmU1Tx(char * msg, size_t size){
	//sprintf((char *)u1TxBuffer, "%s", msg);
	if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){
		
		DCACHE_CLEAN_BY_ADDR(
					(uint32_t)msg,
					size+1);
					//size+3);
					//size+5);
		DMAC_ChannelTransfer(
				DMAC_CHANNEL_1,
				(const void *)msg,
				size+1,	
				//size+3,
				//size+5,
				(const void *)&U1TXREG, 1, 1);
		xSemaphoreTake(xSemBin, portMAX_DELAY);
		xSemaphoreGive(xMutex);
	}
}

void vLab19DMA6Callback(DMAC_TRANSFER_EVENT event, uintptr_t context){
	
}

void vLab19_DMA6_init(void){
	DMAC_ChannelCallbackRegister(
				DMAC_CHANNEL_6,
				vLab19DMA6Callback,
				0);
	
}



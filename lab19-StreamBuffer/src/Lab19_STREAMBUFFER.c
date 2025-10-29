/*
 the files Lab19_STREAMBUFFER.c / .h are for initialization code of the stream buffer
 */

#include "FreeRTOS.h"
#include "Lab19_STREAMBUFFER.h"
#include "stream_buffer.h"
#include "Lab19_config.h"
#include "Lab19_UART.h"


uint8_t ucRxStreamBuffer[RX_STREAM_BUFFER_SIZE];
StaticStreamBuffer_t xRxStreamBufferStruct;
StreamBufferHandle_t xUart6RxStream;

void vLab19_StrmBuff_init(void){
	xUart6RxStream = xStreamBufferCreateStatic(
					RX_STREAM_BUFFER_SIZE,
					1,
					ucRxStreamBuffer,
					&xRxStreamBufferStruct);
	if (xUart6RxStream == NULL){
		Debug6_msg("cannot create stream buffer \r\n");
		exit(EXIT_FAILURE);
	}
}
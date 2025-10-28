/*
 
 */

#include <stdint.h>
#include "Lab19_config.h"


uint8_t u6RxBuffer[RX6_BUFFER_SIZE] __attribute__ ((aligned (16))) = {0};
uint8_t u1TxBuffer[TX1_BUFFER_SIZE] __attribute__ ((aligned (16))) = {0};
/*
 the files Lab19_config.c / .h are implementing a central config pattern that can gain control and consistency across the entire lab19 -
 * without touching multiple .C files
 * inside Lab19_config.h , there is a define of DEBUG flag for Global Debug Control
 * When #define DEBUG is active, the debug section of code compiles in. 
 * In the case of commenting out #define DEBUG, the preprocessor removes all debug code during compilation, so no runtime
 * overhead at all.
 */

#include <stdint.h>
#include "Lab19_config.h"


uint8_t u6RxBuffer[RX6_BUFFER_SIZE] __attribute__ ((aligned (16))) = {0};
uint8_t u1TxBuffer[TX1_BUFFER_SIZE] __attribute__ ((aligned (16))) = {0};
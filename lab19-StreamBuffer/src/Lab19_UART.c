/*
 
 */

#include "xc.h"

//using UART1 and external USB_TTL adaptor (5V level) for showing messages on COM13
void Debug1_msg(char * msg1){
	while (*msg1){
		while (U1STAbits.UTXBF);
		U1TXREG = *msg1++;
		while(!U1STAbits.TRMT);
	}
}


//using UART6 and built-in PKoB4 for showing messages on COM7
void Debug6_msg(char * msg6){
	while (*msg6){
		while (U6STAbits.UTXBF);
		U6TXREG = *msg6++;
		while(!U6STAbits.TRMT);
	}
}
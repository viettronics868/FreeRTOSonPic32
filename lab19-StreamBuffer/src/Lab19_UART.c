/*
 * Using MPLAB X IDE v6.25 and MCC v5.2.6 
 * UART1 and UART6 and all pins are configured via MCC configuration
 * Note: UART1 on curiosity 2.0 pic32 mz ef board need an external USB-TTL adaptor with 5V logic instead of 3.3V logic as
 * UART6 on PKoB4
 
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

//using UART1 and external USB-TTL adaptor for echoing characters on COM13
//for UART1 unit test only
void Echo_msg(void){
	while (!U1STAbits.URXDA);
	while (U1STAbits.UTXBF);
	U1TXREG = U1RXREG;
	while (!U1STAbits.TRMT);
}

//using UART1 for inputting character a.k.a COM13
//using UART6 for outputting character a.k.a COM7
//for integration test UART1 and UART6
void Echo_msg_13_07(void){
	while (!U1STAbits.URXDA);
	while (U6STAbits.UTXBF);
	U6TXREG = U1RXREG;
	while (!U6STAbits.TRMT);
}

//using UART6 for inputting character a.k.a COM7
//using UART1 for outputting character a.k.a COM13
//for integration test UART6 and UART1
void Echo_msg_07_13(void){
	while (!U6STAbits.URXDA);
	while (U1STAbits.UTXBF);
	U1TXREG = U6RXREG;
	while (!U1STAbits.TRMT);
}

//using UART6 and built-in PKoB4 for showing messages on COM7
void Debug6_msg(char * msg6){
	while (*msg6){
		while (U6STAbits.UTXBF);
		U6TXREG = *msg6++;
		while(!U6STAbits.TRMT);
	}
}
/*
 
 */

//#include <proc/p32mz2048efm144.h>
#include <xc.h>

#include "Lab17_UART6.h"

void Debug_msg(char * msg){
	while (*msg){
		while (U6STAbits.UTXBF);
		U6TXREG = *msg++;
		while (!U6STAbits.TRMT);
	}
}
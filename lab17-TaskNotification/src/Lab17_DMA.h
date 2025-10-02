/* 
 * File:   Lab17_DMA.h
 * Author: tuiday
 *
 * Created on September 28, 2025, 2:48 PM
 */

#ifndef LAB17_DMA_H
#define	LAB17_DMA_H

#include "plib_dmac.h"

void vDMA0Callback(DMAC_TRANSFER_EVENT event, uintptr_t contextHandle);
void vComPortMsg(char * msg);
void Lab17_DMAInit(void);

#ifdef	__cplusplus
extern "C" {
#endif




#ifdef	__cplusplus
}
#endif

#endif	/* LAB17_DMA_H */


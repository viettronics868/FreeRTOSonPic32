/* 
 * File:   Lab19_DMA.h
 * Author: tuiday
 *
 * Created on October 9, 2025, 9:14 PM
 */

#pragma once

#ifndef LAB19_DMA_H
#define	LAB19_DMA_H

#include "plib_dmac.h"

void vLab19_DMA1_init(void);
void vLab19_DMA6_init(void);

void vLab19DMA1Callback(DMAC_TRANSFER_EVENT event, uintptr_t context);
void vLab19DMA6Callback(DMAC_TRANSFER_EVENT event, uintptr_t context);

void vShowMsgD1U1(char * msg);
void vStrmU1Tx(char * msg, size_t size);

#ifdef	__cplusplus
extern "C" {
#endif




#ifdef	__cplusplus
}
#endif

#endif	/* LAB19_DMA_H */


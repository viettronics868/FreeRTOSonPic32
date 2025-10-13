/* 
 * File:   Lab19_STA.h
 * Author: tuiday
 *
 * Created on October 9, 2025, 9:13 PM
 */

#pragma once

#ifndef LAB19_STA_H
#define	LAB19_STA_H

typedef enum{
            INIT_STA,
            DEPLOY_STA,
} Lab19States_t;

void xTaskSTAma(void * pvParams);
void vLab19_STA_init(void);

#ifdef	__cplusplus
extern "C" {
#endif




#ifdef	__cplusplus
}
#endif

#endif	/* LAB19_STA_H */


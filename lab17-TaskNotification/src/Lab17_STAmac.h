/* 
 * File:   Lab17_STAmac.h
 * Author: tuiday
 *
 * Created on September 27, 2025, 11:22 PM
 */

#ifndef LAB17_STAMAC_H
#define	LAB17_STAMAC_H

//state definition
typedef  enum{
    INIT_STA,
            SW1_STA,
            SW2_STA,
            SW3_STA,
            SW4_STA,
            IDLE_STA
}Lab17State_t;

void vLab17STAmac(void * pvParams);


#ifdef	__cplusplus
extern "C" {
#endif




#ifdef	__cplusplus
}
#endif

#endif	/* LAB17_STAMAC_H */


/* 
 * File:   Lab19_config.h
 * Author: tuiday
 *
 * Created on October 21, 2025, 10:23 PM
 */

#pragma once

#ifndef LAB19_CONFIG_H
#define	LAB19_CONFIG_H

//---------------------------
//Global debug control

//comment this line for production-grade firmware
//#define DEBUG

//--------------------------
//Global project constants
#define RX_STREAM_BUFFER_SIZE 128
#define TX1_BUFFER_SIZE 131
#define RX6_BUFFER_SIZE 128
#define RX_BUFFER_SIZE 128

#define IDLE_TIMEOUT 100
#define REQUEST_TIMEOUT 10000

#define TIMER_U6RX_INDEX (0U)
#define U6RX_INDEX (1U)
#define BIT_U6RX_FULL (1U << 0U)
#define BIT_IDLE_TIMEOUT (1U << 1U)


typedef enum{
            INIT_STA,
            //NOTIFY_STA,
            WAIT_MSG_STA,
            STRM_U1TX_STA,
} Lab19States_t;

typedef enum{
            EVENT_START,
            EVENT_STOP,
            EVENT_RESET
}TimerEvent_t;

extern uint8_t u6RxBuffer[RX6_BUFFER_SIZE] ;
extern uint8_t u1TxBuffer[TX1_BUFFER_SIZE] ;

#ifdef	__cplusplus
extern "C" {
#endif




#ifdef	__cplusplus
}
#endif

#endif	/* LAB19_CONFIG_H */


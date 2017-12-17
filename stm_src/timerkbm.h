/*******************************************************************************
 * FILE PURPOSE:
 *******************************************************************************
 * FILE NAME:       timerkbm.h
 *
 * DESCRIPTION:     Interface KBM timer functions
 *
 * AUTHOR:		      Wah Kung
 *-----------------------------------------------------------------------------
 *
 * (C) Copyright 2013, AASTRA Telecom
******************************************************************************/
#ifndef TIMERKBM_H
#define TIMERKBM_H


#define XKB_TIM1_PRESCALER    1000
#define XKB_TIM1_PERIOD_MS               6
#define XKB_PHONE_POLL_TO_IN_MS          2000
//#define XKB_PHONE_POLL_TO_IN_MS          4500
//    #define XKB_PHONE_POLL_TO_IN_MS          22000
#define XKB_STARTUP_TIM1_PERIOD_MS       600

#define XKB_TIM2_PERIOD_MS               10
#define XKB_MAX_TIM2_TIMEOUT_IN_MS       0x7F
#define XKB_MAX_TIM2_TIMERENTRY_COUNT    20

GLOBAL void TimerKBMInit(void);
GLOBAL void TimerKBM_DeInit(void);
GLOBAL void xkbStopAllTimer(void);
GLOBAL void xkbStopAllPeriodicTimer(void);
/*---------------------------------------------------------+
| Global function: xkbTimerStart 
|                  start a one-short timer in mS.
|
| input     -  iXkbMs, unit in mS, Max 127 mS
|                      when iXkbMs ==0, immediate timeout
|
|              iCallBP, parameter supplied with callback 
|          
|              iCallBF, callback func on timer expired 
+---------------------------------------------------------*/
GLOBAL void xkbTimerStart(unsigned char iXkbMs, void *iCallBP, void (*iCallBF)(void *));


GLOBAL unsigned char xkbTimerStop(unsigned char, void *, void (*)(void *));
GLOBAL void xkbPeriodicTimerStart(unsigned char, void *, void (*)(void *));
GLOBAL void xkbPeriodicTimerStop(unsigned char, void *, void (*)(void *));

#define IS_TIM2_TIMEOUT_MS_OK(ito) ((ito) <= 0x7F)
#endif //TIMERKBM_H

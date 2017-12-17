/*******************************************************************************
 * FILE PURPOSE:    addon HW timers
 *******************************************************************************
 * FILE NAME:       timerkbm.c
 *
 * DESCRIPTION:     Configure HW timers and handle Interrupts
 * AUTHOR:          Heinz Trueb
 *-----------------------------------------------------------------------------
 *
 * (C) Copyright 2006, Aastra Telecom Schweiz
 * (C) Copyright 2013, Aastra Telecom
 ******************************************************************************/
#include "import.h"
#include <string.h>
#include "sysdef.h"
#include "smos.h"
#include "stm8s_tim1.h"
#include "stm8s_tim2.h"
#include "stm8s_tim3.h"
#include "stm8s_clk.h"
#include "stm8s_gpio.h"
#include "timerkbm.h"
#include "stm8s.h"
#include "queue.h"

GLOBAL enum Kbd_states Keyboard_mode;
/*
 *                Timer 1 interrupt related
*/
GLOBAL void Check_status(void);
GLOBAL void matrixSetRepeatCount(void);
GLOBAL void xkb_Tim1_Startup_S(void);
GLOBAL void xkb_Tim1_Config_S(void);
GLOBAL  void (*xkb_TIM1_state_proc[Kbd_State_End])(void) = 
{
   xkb_Tim1_Startup_S,      /* Kbd_Unknown,   0         */
   xkb_Tim1_Startup_S,      /* Kbd_Startup,   1         */
   Check_status,            /* Kbd_WaitDC1Ack,2         */
   xkb_Tim1_Config_S,       /* Kbd_Config,    3         */
   Check_status,            /* Kbd_Running,   4         */
   Check_status,            /* Kbd_Running_PollSent, 5  */
};
/*
 *                Timer 2 interrupt related
*/
/*---------------------------------------------------------+
|  union for TIM2 tick
|  every TIM2 tick is XKB_TIM2_PERIOD_MS
|
+---------------------------------------------------------*/
/*---------------------------------------------------------+
|      0       |       0 
|      7 6 5 4 | 3 2 1 0
|      --------|--------
|      0 0 0 1   0 0 1 0
+---------------------------------------------------------*/
typedef    union   XKB_TIM2_TICKS_T    {
   unsigned short        isUnShort;
   struct      {
      unsigned char        isSpareUnChar;
      unsigned char        isUnChar;
   };
   struct      {
      //---------------------   00 to 07
      unsigned int      eTicks : 7;        // 06-00
      unsigned int      eOverflow :  1;    // 07      
      unsigned int      eReqTicks :  7;    // 14-08   
      unsigned int      ePeriodic :  1;    // 15      
   }                    isSlic;
}           XKB_TIM2_TICKS;
TAILQ_HEAD(xkbFreeHead,xkbTimerE)      xkbFreeTimerQ;
TAILQ_HEAD(xkbActiveHead,xkbTimerE)    xkbActiveTimerQ;
struct      xkbTimerE
{
   TAILQ_ENTRY(xkbTimerE)     entries;	       /* Tail queue. */
   XKB_TIM2_TICKS             itk;
   void                      *xkbTim2TOParm;
   void                     (*xkbTim2TOFunc)(void *);
};
struct  xkbTimerE             xkbTimerL[XKB_MAX_TIM2_TIMERENTRY_COUNT];
LOCAL XKB_TIM2_TICKS          xkbSysTicks;




/*---------------------------------------------------------+
| Global function: xkbd_TIM1_proc
|                  entered every XKB_TIM1_PERIOD_MS
|
+---------------------------------------------------------*/
void xkbd_TIM1_proc(void)
{
   if ( Keyboard_mode >= Kbd_State_End) {
      return;
   }
   xkb_TIM1_state_proc[Keyboard_mode]();
}

LOCAL void xkbPeriodicTimerRestart( unsigned char, void *, void (*)(void *));
/*---------------------------------------------------------+
| Global function: xkbd_TIM2_proc
|                  entered every XKB_TIM2_PERIOD_MS
|
+---------------------------------------------------------*/
GLOBAL void xkbd_TIM2_proc(void)
{
   TAILQ_HEAD( xkbActiveHead, xkbTimerE)  lTIM2TOQ;
   struct xkbTimerE                      *lETimerE = NULL;
   struct xkbTimerE                       lETimerB[XKB_MAX_TIM2_TIMERENTRY_COUNT];
   int                                    i;
   xkbSysTicks.isSlic.eTicks++;
   if ( TAILQ_EMPTY( &xkbActiveTimerQ) ) {
      /*   nothing in active queue                 */
      return;
   }
   lETimerE = TAILQ_FIRST( &xkbActiveTimerQ);
   if ( lETimerE->itk.isSlic.eTicks != xkbSysTicks.isSlic.eTicks) {
      return;
   }
   memset( &lETimerB, 0, sizeof(lETimerB));
   TAILQ_INIT(&lTIM2TOQ);
   i = 0;
   TAILQ_FOREACH(lETimerE, &xkbActiveTimerQ, entries) {
      memcpy( &lETimerB[i], lETimerE, sizeof( struct xkbTimerE));
      TAILQ_INSERT_TAIL(&lTIM2TOQ, &lETimerB[i] , entries);
      if ( ++i >= XKB_MAX_TIM2_TIMERENTRY_COUNT) {
         break;
      }
   }
   TAILQ_FOREACH( lETimerE, &lTIM2TOQ, entries) {
      if ( lETimerE->itk.isSlic.eTicks == xkbSysTicks.isSlic.eTicks) {
         if ( lETimerE->itk.isSlic.ePeriodic) {
            xkbPeriodicTimerRestart(lETimerE->itk.isSlic.eReqTicks,
                                    lETimerE->xkbTim2TOParm,
                                    lETimerE->xkbTim2TOFunc);
         }  else {
            xkbTimerStop( lETimerE->itk.isSlic.eReqTicks,
                          lETimerE->xkbTim2TOParm,
                          lETimerE->xkbTim2TOFunc);
         }
         if ( lETimerE->xkbTim2TOFunc) {
            lETimerE->xkbTim2TOFunc( lETimerE->xkbTim2TOParm);
         }
         continue;
      }
      break;
   }
}
/*---------------------------------------------------------+
| Global function: TimerKBMInit
|
+---------------------------------------------------------*/
GLOBAL void TimerKBMInit(void)
{
   uint32_t             lTfmaster = 0x0;
   int                  lTmrCount = 0;
   int                  i;
   unsigned short       lTimPrescaler = 0x0;
   unsigned long        lTimClk = 0x0;
   unsigned long        lTimPeriod = 0x0;
   struct xkbTimerE    *lTimerE = NULL;

   lTmrCount = sizeof(xkbTimerL) / sizeof( struct xkbTimerE);
   memset( &xkbTimerL, 0, sizeof(xkbTimerL));
   TAILQ_INIT(&xkbFreeTimerQ);
   TAILQ_INIT(&xkbActiveTimerQ);
   lTimerE = &xkbTimerL[0];
   for ( i = 0; i < lTmrCount; i++ ) {
      TAILQ_INSERT_TAIL(&xkbFreeTimerQ, lTimerE , entries);
      lTimerE++;
   }
   /* Get master frequency */
   lTfmaster = CLK_GetClockFreq();
   lTimPrescaler = XKB_TIM1_PRESCALER;
   lTimClk = lTfmaster / lTimPrescaler;

   lTimPeriod = lTimClk;
   lTimPeriod /= 1000;
   lTimPeriod *= XKB_STARTUP_TIM1_PERIOD_MS;

   TIM1_DeInit();
   TIM1_TimeBaseInit(lTimPrescaler, TIM1_COUNTERMODE_UP, lTimPeriod, 0);
   /* Update Interrupt Enable */
   TIM1_ITConfig(TIM1_IT_UPDATE, ENABLE);
   /* ARRPreload Enable */
   TIM1_ARRPreloadConfig(ENABLE);
   /* TIM1 counter enable */
   TIM1_Cmd(ENABLE);


   lTimPrescaler = TIM2_PRESCALER_128;
   lTimClk = lTfmaster;
   lTimClk >>= lTimPrescaler;

   lTimPeriod = lTimClk;
   lTimPeriod /= 1000;
   lTimPeriod *= XKB_TIM2_PERIOD_MS;
   xkbSysTicks.isUnChar = 0;
   TIM2_DeInit();
	TIM2_TimeBaseInit(lTimPrescaler, (unsigned short) lTimPeriod); 
   TIM2_ITConfig(TIM2_IT_UPDATE, ENABLE);
   TIM2_Cmd(ENABLE);
}
/*---------------------------------------------------------+
| Global function: TimerKBMSetRung
|                  set keyboard to running state
|        
+---------------------------------------------------------*/
GLOBAL void TimerKBMSetRung(void)
{
   uint32_t        lTfmaster = 0x0;
   unsigned long   lTim1Prescaler = 0x0;
   unsigned long   lTim1Clk = 0x0;
   unsigned short  lTim1Period = 0x0;

   /* Get master frequency */
   lTfmaster = CLK_GetClockFreq();
   lTim1Prescaler = TIM1_GetPrescaler();
   lTim1Clk = lTfmaster / lTim1Prescaler;
   lTim1Period = lTim1Clk;
   lTim1Period /= 1000;
   lTim1Period *= XKB_TIM1_PERIOD_MS;
   TIM1_SetAutoreload(lTim1Period);
   matrixSetRepeatCount();
}
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
LOCAL  unsigned char       tmrHighWaterMark = 0;
GLOBAL void xkbTimerStart(unsigned char iXkbMs, 
                        void *iCallBP,
                        void (*iCallBF)(void *))
{
   struct xkbTimerE    *lATimerE = NULL;
   struct xkbTimerE    *lCTimerE = NULL;
   unsigned char        lTCount = 0;
   unsigned char        lCurrTicks = xkbSysTicks.isUnChar;
   assert_param(IS_TIM2_TIMEOUT_MS_OK(iXkbMs));
   if ( TAILQ_EMPTY( &xkbFreeTimerQ) ) {
      return;
   }
   lATimerE = TAILQ_FIRST( &xkbFreeTimerQ);
   TAILQ_REMOVE( &xkbFreeTimerQ, lATimerE, entries);
   memset( lATimerE, 0, sizeof( struct xkbTimerE));
   lATimerE->itk.isSlic.eReqTicks = iXkbMs;
   lATimerE->itk.isSlic.eTicks = iXkbMs;
   lATimerE->itk.isUnChar += xkbSysTicks.isUnChar;
   lATimerE->xkbTim2TOParm = iCallBP;
   lATimerE->xkbTim2TOFunc = iCallBF;
   if ( iXkbMs == 0 ) {
      /*  immediate time out, schedule for next clock */
      lATimerE->itk.isSlic.eReqTicks = 1;
      lATimerE->itk.isSlic.eTicks = 1;
      lATimerE->itk.isUnChar += xkbSysTicks.isUnChar;
      TAILQ_INSERT_HEAD(&xkbActiveTimerQ, lATimerE, entries);
      if ( tmrHighWaterMark == 0 ) {
         tmrHighWaterMark++;
      }
      return;
   }

   if ( TAILQ_EMPTY( &xkbActiveTimerQ)) {
      TAILQ_INSERT_HEAD(&xkbActiveTimerQ, lATimerE, entries);
      if ( tmrHighWaterMark == 0 ) {
         tmrHighWaterMark++;
      }
      return;
   }
   
   lTCount = 0;
   TAILQ_FOREACH( lCTimerE, &xkbActiveTimerQ, entries) {
      lTCount++;
   }
   if ( lTCount > tmrHighWaterMark) {
      tmrHighWaterMark = lTCount;
   }
   TAILQ_FOREACH( lCTimerE, &xkbActiveTimerQ, entries) {
      if ( lCTimerE->itk.isUnChar > lATimerE->itk.isUnChar) {
         TAILQ_INSERT_BEFORE(lCTimerE, lATimerE, entries);
         break;
      } 
   }
   if ( lCTimerE == TAILQ_END(&xkbActiveTimerQ)) {
      TAILQ_INSERT_TAIL(&xkbActiveTimerQ, lATimerE, entries);
   }
}
/*---------------------------------------------------------+
| Global function: xkbTimerStop
|                  stop a one-short timer in mS.
|        
| input     -  iXkbMs,        unit in mS, Max 127 mS
|              iCallBP        callback parameter
|              iCallBF        callback func
|
| output    -  0              given timer not found or not stopped
|              not zero       given timer stopped
|
+---------------------------------------------------------*/
GLOBAL unsigned char xkbTimerStop(unsigned char iXkbMs, 
                                  void *iCallBP,
                                  void (*iCallBF)(void *))
{
   struct xkbTimerE    *lOSTimerE = NULL;
   unsigned char        lCRtn = 0;

   assert_param(IS_TIM2_TIMEOUT_MS_OK(iXkbMs));
   if ( TAILQ_EMPTY( &xkbActiveTimerQ) ) {
      return lCRtn;
   }
   TAILQ_FOREACH( lOSTimerE, &xkbFreeTimerQ, entries) {
      if ( lOSTimerE->itk.isSlic.ePeriodic) {
         continue;
      }
      if ( lOSTimerE->itk.isSlic.eReqTicks != iXkbMs) {
         continue;
      } 
      if ( iXkbMs == 0 ) {
         if ( lOSTimerE->itk.isSlic.eReqTicks != 1) {
            continue;
         } 
      }
      if ( lOSTimerE->xkbTim2TOParm != iCallBP) {
         continue;
      }
      if ( lOSTimerE->xkbTim2TOFunc != iCallBF) {
         continue;
      }
      return lCRtn;
   }
   TAILQ_FOREACH( lOSTimerE, &xkbActiveTimerQ, entries) {
      if ( lOSTimerE->itk.isSlic.ePeriodic) {
         continue;
      }
      if ( lOSTimerE->itk.isSlic.eReqTicks != iXkbMs) {
         continue;
      } 
      if ( lOSTimerE->xkbTim2TOParm != iCallBP) {
         continue;
      }
      if ( lOSTimerE->xkbTim2TOFunc != iCallBF) {
         continue;
      }
      TAILQ_REMOVE( &xkbActiveTimerQ, lOSTimerE, entries);
      memset( lOSTimerE, 0, sizeof( struct xkbTimerE));
      TAILQ_INSERT_TAIL(&xkbFreeTimerQ, lOSTimerE, entries);
      lCRtn = 1;
      break;
   }
   return lCRtn;
}
/*---------------------------------------------------------+
| Global function: xkbPeriodicTimerStart
|                  start a periodic timer in mS.
|        
| input     -  iXkbMs, unit in mS, Max 127 mS
|
+---------------------------------------------------------*/
GLOBAL void xkbPeriodicTimerStart(unsigned char iXkbMs, 
                        void *iCallBP,
                        void (*iCallBF)(void *))
{
   struct xkbTimerE    *lPCTimerE = NULL;
   struct xkbTimerE    *lSTimerE = NULL;
   unsigned char        lCurrTicks = xkbSysTicks.isUnChar;
   assert_param(IS_TIM2_TIMEOUT_MS_OK(iXkbMs));
   if ( iXkbMs == 0 ) {
      return;
   }
   if ( TAILQ_EMPTY( &xkbFreeTimerQ) ) {
      return;
   }
   lPCTimerE = TAILQ_FIRST( &xkbFreeTimerQ);
   TAILQ_REMOVE( &xkbFreeTimerQ, lPCTimerE, entries);
   memset( lPCTimerE, 0, sizeof( struct xkbTimerE));
   lPCTimerE->itk.isSlic.eReqTicks = iXkbMs;
   lPCTimerE->itk.isSlic.eTicks = iXkbMs;
   lPCTimerE->itk.isUnChar += xkbSysTicks.isUnChar;
   lPCTimerE->xkbTim2TOParm = iCallBP;
   lPCTimerE->xkbTim2TOFunc = iCallBF;
   lPCTimerE->itk.isSlic.ePeriodic = 1;

   if ( TAILQ_EMPTY( &xkbActiveTimerQ) == 0 ) {
      TAILQ_INSERT_HEAD(&xkbActiveTimerQ, lPCTimerE, entries);
      return;
   }
   
   TAILQ_FOREACH( lSTimerE, &xkbActiveTimerQ, entries) {
      if ( lSTimerE->itk.isUnChar > lPCTimerE->itk.isUnChar) {
         TAILQ_INSERT_BEFORE(lSTimerE, lPCTimerE, entries);
         return;
      } 
   }
   if ( lSTimerE == TAILQ_END(&xkbActiveTimerQ)) {
      TAILQ_INSERT_TAIL(&xkbActiveTimerQ, lPCTimerE, entries);
   }
}
/*---------------------------------------------------------+
| Global function: xkbPeriodicTimerStop
|                  stop a periodic timer in mS.
|        
| input     -  iXkbMs, unit in mS, Max 127 mS
|
+---------------------------------------------------------*/
GLOBAL void xkbPeriodicTimerStop(unsigned char iXkbMs, 
                        void *iCallBP,
                        void (*iCallBF)(void *))
{
   struct xkbTimerE    *lPSTimerE = NULL;
   assert_param(IS_TIM2_TIMEOUT_MS_OK(iXkbMs));
   if ( TAILQ_EMPTY( &xkbActiveTimerQ) ) {
      return;
   }
   TAILQ_FOREACH( lPSTimerE, &xkbActiveTimerQ, entries) {
      if ( lPSTimerE->itk.isSlic.ePeriodic == 0 ) {
         continue;
      }
      if ( lPSTimerE->itk.isSlic.eReqTicks != iXkbMs) {
         continue;
      } 
      if ( lPSTimerE->xkbTim2TOParm != iCallBP) {
         continue;
      }
      if ( lPSTimerE->xkbTim2TOFunc != iCallBF) {
         continue;
      }
      TAILQ_REMOVE( &xkbActiveTimerQ, lPSTimerE, entries);
      memset( lPSTimerE, 0, sizeof( struct xkbTimerE));
      TAILQ_INSERT_TAIL(&xkbFreeTimerQ, lPSTimerE, entries);
      return;
   }
}


/*---------------------------------------------------------+
| Local function: xkbPeriodicTimerRestart
|
+---------------------------------------------------------*/
LOCAL void xkbPeriodicTimerRestart( unsigned char ipmS, 
                                    void *ipCallBP,
                                    void (*ipCallBF)(void *))
{
   struct xkbTimerE    *lPRTimerE = NULL;
   struct xkbTimerE    *lPRWTimerE = NULL;
   TAILQ_FOREACH( lPRTimerE, &xkbActiveTimerQ, entries) {
      if ( lPRTimerE->itk.isSlic.ePeriodic == 0 ) {
         continue;
      }
      if ( lPRTimerE->itk.isSlic.eReqTicks != ipmS) {
         continue;
      } 
      if ( lPRTimerE->xkbTim2TOParm != ipCallBP) {
         continue;
      }
      if ( lPRTimerE->xkbTim2TOFunc != ipCallBF) {
         continue;
      }
      TAILQ_REMOVE( &xkbActiveTimerQ, lPRTimerE, entries);
      lPRTimerE->itk.isUnChar = lPRTimerE->itk.isSlic.eReqTicks;
      lPRTimerE->itk.isUnChar += xkbSysTicks.isUnChar;
      if ( TAILQ_EMPTY( &xkbActiveTimerQ) == 0 ) {
         TAILQ_INSERT_HEAD(&xkbActiveTimerQ, lPRTimerE, entries);
         return;
      }
      TAILQ_FOREACH( lPRWTimerE, &xkbActiveTimerQ, entries) {
         if ( lPRWTimerE->itk.isUnChar > lPRTimerE->itk.isUnChar) {
            TAILQ_INSERT_BEFORE(lPRWTimerE, lPRTimerE, entries);
            return;
         } 
      }
      TAILQ_INSERT_TAIL(&xkbActiveTimerQ, lPRTimerE, entries);
      return;
   }
}
/*---------------------------------------------------------+
| Global function: xkbStopAllTimer
|
+---------------------------------------------------------*/
GLOBAL void xkbStopAllTimer(void)
{
   struct xkbTimerE    *lSATimerE = NULL;
   if ( TAILQ_EMPTY( &xkbActiveTimerQ) ) {
      return;
   }
   TAILQ_FOREACH( lSATimerE, &xkbActiveTimerQ, entries) {
      if ( lSATimerE->itk.isSlic.ePeriodic) {
         continue;
      }
      TAILQ_REMOVE( &xkbActiveTimerQ, lSATimerE, entries);
      memset( lSATimerE, 0, sizeof( struct xkbTimerE));
      TAILQ_INSERT_TAIL(&xkbFreeTimerQ, lSATimerE, entries);
      break;
   }
}
/*---------------------------------------------------------+
| Global function: xkbStopAllPeriodicTimer
|
+---------------------------------------------------------*/
GLOBAL void xkbStopAllPeriodicTimer(void)
{
   struct xkbTimerE    *lSPTimerE = NULL;
   if ( TAILQ_EMPTY( &xkbActiveTimerQ) ) {
      return;
   }
   TAILQ_FOREACH( lSPTimerE, &xkbActiveTimerQ, entries) {
      if ( lSPTimerE->itk.isSlic.ePeriodic ==0) {
         continue;
      }
      TAILQ_REMOVE( &xkbActiveTimerQ, lSPTimerE, entries);
      memset( lSPTimerE, 0, sizeof( struct xkbTimerE));
      TAILQ_INSERT_TAIL(&xkbFreeTimerQ, lSPTimerE, entries);
      break;
   }
}
/*---------------------------------------------------------+
| Global function: xkbTimerSanityCheck
|
+---------------------------------------------------------*/
GLOBAL void       xkbTimerSanityCheck(void) 
{
   struct xkbTimerE    *lSanityTimerE = NULL;
   int                  ls01;
   int                  ls02;

   disableInterrupts();
   ls01 = 0;
   TAILQ_FOREACH( lSanityTimerE, &xkbActiveTimerQ, entries) {
      ls01++;
   }
   ls02 = 0;
   TAILQ_FOREACH( lSanityTimerE, &xkbFreeTimerQ, entries) {
      ls02++;
   }
   if ( (ls01 + ls02) != (sizeof(xkbTimerL)/sizeof(struct xkbTimerE)) ) {
      _asm("nop");
   } 
	enableInterrupts();
}
/*---------------------------------------------------------+
| Global function: TimerKBM_DeInit
|
+---------------------------------------------------------*/
GLOBAL void TimerKBM_DeInit(void)
{
   TAILQ_INIT(&xkbFreeTimerQ);
   TAILQ_INIT(&xkbActiveTimerQ);
}

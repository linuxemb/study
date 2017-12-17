/*******************************************************************************
 * FILE PURPOSE:    External UART keyboard State Machine
 *******************************************************************************
 * FILE NAME:       xkbStateMach.c
 *
 * DESCRIPTION:     
 * AUTHOR:          Heinz Trueb
 *-----------------------------------------------------------------------------
 *
 * (C) Copyright 2006, Aastra Telecom Schweiz
 * (C) Copyright 2013, Aastra Telecom
 ******************************************************************************/
/* imported definitions -------------------------------------------------*/
#include "import.h"
#include "sysdef.h"
#include "smos.h"
#include "xternUartScanCode.h"
#include "uart.h"
#include "timerkbm.h"
#include "stm8s.h"

GLOBAL _SMALL_MEM_ enum Kbd_states Keyboard_mode;
GLOBAL long                 xkbMaxPhoneCfgCountGuard;
GLOBAL long                 xkbPhoneCfgCountGuard;

GLOBAL void    matrixClearRepeatCount(void);

void        xkb_Tim1_Startup_S(void)
{
   // Send DC1 char
   UARTSendCharBuffer(EKBD_DC1);
}

void        xkb_Tim1_Config_S(void)
{
   if ( xkbPhoneCfgCountGuard && --xkbPhoneCfgCountGuard <= 0 ) {
      xkbPhoneCfgCountGuard = xkbMaxPhoneCfgCountGuard;
      Keyboard_mode = Kbd_Startup;
      matrixClearRepeatCount();
      xkbStopAllTimer();
      xkbStopAllPeriodicTimer();
   }
}

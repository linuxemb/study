/*******************************************************************************
 * FILE PURPOSE:    addon serial module
 *******************************************************************************
 * FILE NAME:       uart.c
 *
 * DESCRIPTION:     Send and receive addon messages. CRC check. Length check.
 *
 * AUTHOR:          Heinz Trueb
 *-----------------------------------------------------------------------------
 *
 * (C) Copyright 2006, Aastra Telecom Schweiz
 * (C) Copyright 2013, AASTRA Telecom
 ******************************************************************************/
#include "import.h"
#include <string.h>
#include "smos.h"
#include "timerkbm.h"
#include "stm8s.h"
#include "stm8s_uart2.h"
#include "stm8s_clk.h"
#include "stm8s_gpio.h"
#include "sysdef.h"
#include "xternUartScanCode.h"

/* imported definitions -------------------------------------------------*/
GLOBAL  enum Kbd_states Keyboard_mode;
GLOBAL  char                       Kbd_str[];

GLOBAL void UARTSendBuffer(UINT8 *buf, UINT8 len);
GLOBAL void TimerKBMSetRung(void);
GLOBAL void matrixClearRepeatCount(void);

/* exported definitions -------------------------------------------------*/
#include "export.h"

GLOBAL unsigned char        xkbRxPhonePollGuard(void);
GLOBAL long                 xkbMaxPhoneCfgCountGuard;
GLOBAL long                 xkbPhoneCfgCountGuard;
/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/*******************************************************************************
 * TYPEDEFS
 ******************************************************************************/
#define     XKB_UART2_RUN_MAX_CHAR        100
#define     XKB_UART2_RINGBUF_SIZE        100

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static void xkb_UART2_RX_Startup_proc(void);
static void xkb_UART2_RX_Config_proc(void);
static void xkb_UART2_RX_RunPoll_proc(void);
LOCAL void xkb_UART2_RX_Run_proc(void);
LOCAL unsigned char xkb_UART2_Get_Tx_Count(void);

/*******************************************************************************
 * MODULE VARS
 ******************************************************************************/
char            *xkb_Phone_Poll_msg = "\002Kbd Type\003";
unsigned char    run_rx_byte[XKB_UART2_RUN_MAX_CHAR];
unsigned char    tx_rbuff[XKB_UART2_RINGBUF_SIZE];
unsigned char   *tx_buff;
unsigned char    tx_readIdx;
unsigned char    tx_wrtIdx;
unsigned short          runRxWrtIdx;
LOCAL unsigned char     rx_byte;
LOCAL long                 xkbMaxPhonePollCountGuard;
LOCAL long                 xkbPhonePollCountGuard;

GLOBAL void (*xkb_UART2_state_proc[Kbd_State_End])(void) = 
{
   xkb_UART2_RX_Startup_proc,      /* Kbd_Unknown,   0         */
   xkb_UART2_RX_Startup_proc,      /* Kbd_Startup,   1         */
   xkb_UART2_RX_Startup_proc,      /* Kbd_WaitDC1Ack,2         */
   xkb_UART2_RX_Config_proc,       /* Kbd_Config,    3         */
   xkb_UART2_RX_Run_proc,          /* Kbd_Running,   4         */
   xkb_UART2_RX_RunPoll_proc,      /* Kbd_Running_PollSent,   5 */
};
/*------------------------------------------------------------------------------
 * function     :   InitUART
 * description  :   Initialisation of all registers. Not yet activated.
 *---------------------------------------------------------------------------- */
GLOBAL void InitUART(void)
{
	UART2_DeInit();      // initialize UART2
	/* Set 115200 baud, 8 bits, 1 stop, no parity */
//	UART2_Init(115200, UART2_WORDLENGTH_8D, UART2_STOPBITS_1, UART2_PARITY_NO, UART2_SYNCMODE_CLOCK_DISABLE, UART2_MODE_TXRX_ENABLE);
	UART2_Init(19200, UART2_WORDLENGTH_8D, UART2_STOPBITS_1, UART2_PARITY_NO, UART2_SYNCMODE_CLOCK_DISABLE, UART2_MODE_TXRX_ENABLE);
   /* Set receive interrupt   */
   UART2_ITConfig( UART2_IT_RXNE_OR, ENABLE);
	tx_readIdx = 0;
	tx_wrtIdx = 0;
   memset( tx_rbuff, 0, sizeof( tx_rbuff));
   tx_buff = tx_rbuff;
   memset( run_rx_byte, 0, sizeof( run_rx_byte));
   xkbMaxPhoneCfgCountGuard = 20;
   xkbMaxPhonePollCountGuard = XKB_PHONE_POLL_TO_IN_MS;
   xkbMaxPhonePollCountGuard /= XKB_TIM1_PERIOD_MS;
   xkbPhonePollCountGuard = xkbMaxPhonePollCountGuard;
}

/*------------------------------------------------------------------------------
 * function     :   StartUART
 * description  :   Activate Transmitter and Receiver
 *---------------------------------------------------------------------------- */
GLOBAL void StartUART(void)
{
   UART2->CR2 |= (u8)UART2_CR2_TEN;  /**< Set the Transmitter Enable bit */
   UART2->CR2 |= (u8)UART2_CR2_REN;  /**< Set the Receiver Enable bit */
   // Set receive/overrun interrupt
   UART2_ITConfig(UART2_IT_RXNE_OR, ENABLE);
   tx_readIdx = 0;
   tx_wrtIdx = 0;
   tx_buff = tx_rbuff;
}

/***********************************************************
Interrupt Processing Subroutines
***********************************************************/
/*------------------------------------------------------------------------------
 * function     :   xkbd_UART2_TX_proc
 * description  :   UART2 tx isr
 *---------------------------------------------------------------------------- */
GLOBAL void xkbd_UART2_TX_proc(void)
{
   unsigned char    lTxReadIdx;
   unsigned char    lTxCount;
	if (UART2_GetITStatus(UART2_IT_TXE)==SET) {    // transmit data register empty
      tx_readIdx++;
      if (tx_readIdx >= sizeof( tx_rbuff))  {
         tx_readIdx = 0;
      }
      lTxCount = xkb_UART2_Get_Tx_Count();
      switch( lTxCount) {
         default:
            UART2_SendData8( tx_rbuff[tx_readIdx]);
            lTxReadIdx = tx_readIdx + 1;
            break;
         case 0:
            UART2_ITConfig(UART2_IT_TC, ENABLE);
            UART2_ITConfig(UART2_IT_TXE, DISABLE); // disable tdre interrupt

            break;
         case 1:
            UART2_SendData8( tx_rbuff[tx_readIdx]);
            UART2_ITConfig(UART2_IT_TC, ENABLE);
            UART2_ITConfig(UART2_IT_TXE, DISABLE); // disable tdre interrupt
            break;
      }
	}
	if (UART2_GetITStatus(UART2_IT_TC)==SET) {    // transmit complete
      UART2_ITConfig(UART2_IT_TC, DISABLE);
      lTxCount = xkb_UART2_Get_Tx_Count();
      if ( lTxCount) {
         tx_readIdx++;
      }
	}
}
/*------------------------------------------------------------------------------
 * function     :   xkbd_UART2_RX_proc
 * description  :   UART2 rx isr
 *---------------------------------------------------------------------------- */
void xkbd_UART2_RX_proc(void)
{
   unsigned char     lDiscard;
   if (UART2_GetITStatus(UART2_IT_RXNE)==RESET) {  // received data
      return;
   }
   if ( Keyboard_mode >= Kbd_State_End) {
      lDiscard = UART2_ReceiveData8();
      return;
   }
   xkb_UART2_state_proc[Keyboard_mode]();
}
/*------------------------------------------------------------------------------
 * function     :   xkb_UART2_RX_Startup_proc
 * description  :   UART2 tx in Kbd_Startup
 *---------------------------------------------------------------------------- */
GLOBAL void xkb_UART2_RX_Startup_proc(void)
{
   uint8_t stTmp;
   rx_byte = UART2_ReceiveData8();
   if (UART2_GetITStatus(UART2_IT_OR)==SET) {    // overrun error
      stTmp = UART2_ReceiveData8();
      UART2_ClearITPendingBit(UART2_IT_RXNE);
   }
	switch (rx_byte) {
      case EKBD_ACK:
         Keyboard_mode = Kbd_Config;
         xkbPhoneCfgCountGuard = xkbMaxPhoneCfgCountGuard;
         UARTSendBuffer(Kbd_str, 11);
         break;
      case EKBD_DC2:
         sysStm8Reset();
         break;
      default:
         break;
   
   }
}
/*------------------------------------------------------------------------------
 * function     :   xkb_UART2_RX_Config_proc
 * description  :   UART2 tx in Kbd_Config
 *---------------------------------------------------------------------------- */
GLOBAL void xkb_UART2_RX_Config_proc(void)
{
   uint8_t rTemp;
   rx_byte = UART2_ReceiveData8();
   if (UART2_GetITStatus(UART2_IT_OR)==SET) {    // overrun error
      rTemp = UART2_ReceiveData8();
      UART2_ClearITPendingBit(UART2_IT_RXNE);
   }
	switch (rx_byte) {
	   case EKBD_ACK:      // received ACK
         Keyboard_mode = Kbd_Running;
         TimerKBMSetRung();
         xkbPhonePollCountGuard = xkbMaxPhonePollCountGuard;
         xkbPhoneCfgCountGuard = 0;
         runRxWrtIdx = 0;
         memset( run_rx_byte, 0, sizeof( run_rx_byte));
         break;
	   case EKBD_DC2:      // go to stm8 bootloader
         sysStm8Reset();
         break;
      default:
         break;
   }
}
/*------------------------------------------------------------------------------
 * function     :   xkb_UART2_RX_Run_proc
 * description  :   UART2 rx in Kbd_Running
 *---------------------------------------------------------------------------- */
LOCAL void           xkb_UART2_RX_Run_proc(void)
{
   uint8_t                   ktemp;
   unsigned char             lRxIdx;
#if 0
   FlagStatus    lfs;

   lfs = UART2_GetFlagStatus( UART2_FLAG_TXE); 
   if ( lfs == SET) {
      _asm("nop");
   } else {
      _asm("nop");
   }

   lfs = UART2_GetFlagStatus( UART2_FLAG_RXNE); 
   if ( lfs == SET) {
      _asm("nop");
   } else {
      _asm("nop");
   }

   lfs = UART2_GetFlagStatus( UART2_FLAG_OR_LHE); 
   if ( lfs == SET) {
      _asm("nop");
   } else {
      _asm("nop");
   }

   lfs = UART2_GetFlagStatus( UART2_FLAG_NF); 
   if ( lfs == SET) {
      _asm("nop");
   } else {
      _asm("nop");
   }

   lfs = UART2_GetFlagStatus( UART2_FLAG_FE); 
   if ( lfs == SET) {
      _asm("nop");
   } else {
      _asm("nop");
   }

   lfs = UART2_GetFlagStatus( UART2_FLAG_PE); 
   if ( lfs == SET) {
      _asm("nop");
   } else {
      _asm("nop");
   }
#endif
   ktemp = UART2_ReceiveData8();
   if (UART2_GetITStatus(UART2_IT_OR)==SET) {    // overrun error
      UART2_ClearITPendingBit(UART2_IT_RXNE);
      _asm("nop");
   }
   if ( ktemp == EKBD_DC2 ) { // go to stm8 bootloader
      sysStm8Reset();
   }
   if ( ktemp == EKBD_STX ) {
      runRxWrtIdx = 0;
      memset( run_rx_byte, 0, sizeof( run_rx_byte));
      run_rx_byte[runRxWrtIdx++] = ktemp;
      return;
   }
   if ( runRxWrtIdx == 0 ) {
      runRxWrtIdx = 0;
      memset( run_rx_byte, 0, sizeof( run_rx_byte));
      return;
   }
   if ( (runRxWrtIdx + 1)  >= sizeof( run_rx_byte)) {
      runRxWrtIdx = 0;
      memset( run_rx_byte, 0, sizeof( run_rx_byte));
      return;
   }
   if ( ktemp == EKBD_ETX) {
      unsigned char *lRPp;
      lRPp = run_rx_byte;
      if ( memcmp( lRPp, xkb_Phone_Poll_msg, sizeof( xkb_Phone_Poll_msg)) == 0 ) {
         UARTSendBuffer(Kbd_str, 11);
         Keyboard_mode = Kbd_Running_PollSent;
         xkbPhonePollCountGuard = xkbMaxPhonePollCountGuard;
      }  else {
      _asm("nop");
      }
      runRxWrtIdx = 0;
      memset( run_rx_byte, 0, sizeof( run_rx_byte));
      return;
   }
   run_rx_byte[runRxWrtIdx++] = ktemp;
}
/*------------------------------------------------------------------------------
 * function     :   xkb_UART2_RX_RunPoll_proc
 * description  :   
 *---------------------------------------------------------------------------- */
GLOBAL void xkb_UART2_RX_RunPoll_proc(void)
{
   uint8_t ptemp;

   ptemp = UART2_ReceiveData8();
   if (UART2_GetITStatus(UART2_IT_OR)==SET) {    // overrun error
      UART2_ClearITPendingBit(UART2_IT_RXNE);
   }
   if ( ptemp == EKBD_DC2 ) { // go to stm8 bootloader
      sysStm8Reset();
   }
   if ( ptemp == EKBD_ACK) {
      memset( run_rx_byte, 0, sizeof( run_rx_byte));
      runRxWrtIdx = 0;
      Keyboard_mode = Kbd_Running;
   }  else {
      _asm("nop");
   }
}

/*------------------------------------------------------------------------------
 * function     :   UARTStartTx
 * description  :   write to tx buffer;
 *                  write to uart when this is the first byte
 *---------------------------------------------------------------------------- */
void UARTStartTx(void)
{
   if (tx_wrtIdx == tx_readIdx) {
      return;
   }
   if ( UART2_GetFlagStatus( UART2_FLAG_TXE) == RESET) {
      return;
   }
   UART2_SendData8( tx_rbuff[tx_readIdx]);
   UART2_ITConfig(UART2_IT_TC, DISABLE); // disable tc interrupt
   UART2_ITConfig(UART2_IT_TXE, ENABLE); // enable tdre interrupt
}

/*------------------------------------------------------------------------------
 * function     :   UARTSendBuffer
 * description  :   Send from buffer
 *                  insert ESC 0x1B when there is control characters between
 *                  STX and ETX
 *---------------------------------------------------------------------------- */
GLOBAL void UARTSendBuffer(UINT8 *buf, UINT8 len)
{
   unsigned char *lc;
   unsigned char  ls = 0;
   if ( buf == NULL) {
      return;
   }
   if ( len == 0 ) {
      return;
   }
   lc = buf;
   ls = (tx_wrtIdx == tx_readIdx ? 1 : 0); 
   /*  first char is STX  */
   tx_rbuff[tx_wrtIdx++] = *lc;
   if ( tx_wrtIdx >= sizeof( tx_rbuff))  {
      tx_wrtIdx = 0;
   } 
   lc++;
   len--;
   while( len) {
      if ( len > 1) {
         switch ( *lc) {
            case EKBD_STX:
            case EKBD_ETX:
            case EKBD_ACK:
            case EKBD_BELL:
            case EKBD_DC1:
            case EKBD_DC2:
            case EKBD_DC3:
            case EKBD_DC4:
            case EKBD_ESC:
               tx_rbuff[tx_wrtIdx++] = EKBD_ESC;
               if ( tx_wrtIdx >= sizeof( tx_rbuff))  {
                  tx_wrtIdx = 0;
               } 
               break;
            default:
               break;
         }
      }
      tx_rbuff[tx_wrtIdx++] = *lc;
      lc++;
      if ( tx_wrtIdx >= sizeof( tx_rbuff))  {
         tx_wrtIdx = 0;
      } 
      len--;
   }
   if ( ls) {
      UARTStartTx();
   }
}
/*------------------------------------------------------------------------------
 * function     :   UARTSendCharBuffer
 * description  :   Send from buffer
 *---------------------------------------------------------------------------- */
GLOBAL void UARTSendCharBuffer(unsigned char ic)
{
   unsigned char  ls = 0;
   ls = (tx_wrtIdx == tx_readIdx ? 1 : 0); 
   tx_rbuff[tx_wrtIdx++] = ic;
   if ( tx_wrtIdx >= sizeof( tx_rbuff))  {
      tx_wrtIdx = 0;
   } 
   if ( ls) {
      UARTStartTx();
   }
}

/*------------------------------------------------------------------------------
 * function     :   xkbRxPhonePollGuard
 * description  :   
 *
 * return       :   0 receiving Phone-side guard count reached. ie no
 *                    Phone-side poll message received
 *                  1 Phone-side poll message received
 *---------------------------------------------------------------------------- */
GLOBAL unsigned char xkbRxPhonePollGuard(void)
{
   if ( xkbPhonePollCountGuard && --xkbPhonePollCountGuard <= 0) {
      unsigned long   ll;
      uint32_t        lMfmaster = 0x0;
      unsigned short  lTim1Prescaler = 0x0;
      unsigned long   lTim1Clk = 0x0;
      unsigned short  lTim1Period = 0x0;

      runRxWrtIdx = 0;
      memset( run_rx_byte, 0, sizeof( run_rx_byte));

      memset( tx_rbuff, 0, sizeof( tx_rbuff));
      tx_readIdx = 0;
      tx_wrtIdx = 0;

      /* Get master frequency */
      lMfmaster = CLK_GetClockFreq();
      lTim1Prescaler = TIM1_GetPrescaler();
      lTim1Clk = lMfmaster / lTim1Prescaler;

      lTim1Period = lTim1Clk;
      lTim1Period /= 1000;
      lTim1Period *= XKB_STARTUP_TIM1_PERIOD_MS;

      TIM1_SetAutoreload(lTim1Period);
      Keyboard_mode = Kbd_Startup;
      matrixClearRepeatCount();
      xkbStopAllTimer();
      xkbStopAllPeriodicTimer();
      xkbPhonePollCountGuard = 0;
      return 1;
   }
   return 0;
}
/*------------------------------------------------------------------------------
 * function     :   xkb_UART2_Get_Tx_Count
 * description  :   
 *
 * return       :   0 receiving Phone-side guard count reached. ie no
 *                    Phone-side poll message received
 *                  1 Phone-side poll message received
 *---------------------------------------------------------------------------- */
LOCAL unsigned char xkb_UART2_Get_Tx_Count(void)
{
   unsigned char    lTxCount = 0;
   if ( tx_wrtIdx >= tx_readIdx) {
      lTxCount = tx_wrtIdx - tx_readIdx;
   }  else  {
      lTxCount = sizeof( tx_readIdx);
      lTxCount -= tx_readIdx - tx_wrtIdx;
   }
   return lTxCount;
}
/* end of file */

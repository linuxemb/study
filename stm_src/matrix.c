/*******************************************************************************
 * FILE PURPOSE:    SW version
 *******************************************************************************
 * FILE NAME:       matrix.c
 *
 * AUTHOR:          unknown
 *-----------------------------------------------------------------------------
 *
 * (C) Copyright 2006, AASTRA Telecom Schweiz AG
 * (C) Copyright 2013, AASTRA Telecom
******************************************************************************/

/* imported definitions -------------------------------------------------*/
#include "import.h"
#include <string.h>
#include "smos.h"
#include "sysdef.h"
#include "matrix.h"
#include "uart.h"
#include "stm8s_uart2.h"
#include "timerkbm.h"
#include "stm8s_gpio.h"
#include "xternUartScanCode.h"
#include "xkbVersion.h"

GLOBAL EEPROM xkbVersionU xkbU;
/* exported definitions -------------------------------------------------*/
#include "export.h"

//#define DEBOUNCE_PRESS_AND_RELEASE
//#define DEBOUNCE_RELEASE_ONLY
#define DEBOUNCE_NOTHING
#define MAX_NUMOF_ROWS  8

GLOBAL enum Kbd_states Keyboard_mode = Kbd_Startup;

/* local variable definitions --------------------------------------------*/
/* union for Keyboard type
*/
//     0       |       0 
//     7 6 5 4 | 3 2 1 0
//     --------|--------
//     0 0 0 1   0 0 1 0
typedef    union       {
   unsigned char        isUnChar;
   EKBD_LAYOUT          isXKbdType;
   struct      {
      //---------------------   00 to 07
      unsigned int      tGpio00  :  1;          //    00
      unsigned int      tGpio04  :  1;          //    01
      unsigned int      tGpio07  :  1;          //    02
      unsigned int      tPos03  :  1;          //    03   
      unsigned int      tPos04  :  1;          //    04   
      unsigned int      tPos05  :  1;          //    05   
      unsigned int      tPos06  :  1;          //    06   
      unsigned int      tPos07 :  1;           //    07   
   }                    isBi;
}           GPIO_XKBD_TYPE_DEF;


// Port D pins 2-3 connected to the key column bus
#define PORTD_COLUMN_BITS          0x0C

// Port C bits 2-7 are connected to the column bus. Active state. The bits are active high.
#define PORTC_COLUMN_BITS          0xFC

// Port B bits 0-5 connected to the ROW bus. Idle state. The bits are active low.
#define PORTB_ROW_BITS             0x3F

GLOBAL _SMALL_MEM_ GPIO_XKBD_TYPE_DEF  xKbd_type;  // global for keyboard type
GLOBAL             char  Kbd_str[] = "\002Type 1 \155\155\003";
LOCAL  _SMALL_MEM_ UINT8 row_state = 0;
LOCAL  _SMALL_MEM_ UINT8 row       = 0x01;
LOCAL  _SMALL_MEM_ UINT8 keys[2][MAX_NUMOF_ROWS] = {{ 0, 0, 0, 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0, 0, 0, 0 }};
LOCAL  unsigned char     matrixKeyRepeatCount;

/* keyboard scan codes */
LOCAL UINT16 scancodes[XKBD_MAX_ROW_COUNT][XKBD_MAX_COL_COUNT] = 
{ 
   {    /*   Row 0; Col 0 to 7               */
      EKBD_SW8, EKBD_SW9, EKBD_SW10, EKBD_SW11, 
      EKBD_SW12, EKBD_SW13, EKBD_SW14, EKBD_SW77, 
   },
   {    /*   Row 1, Col 0 to 7               */
      EKBD_SW23, EKBD_SW24, EKBD_SW25, EKBD_SW26, EKBD_SW27, EKBD_SW28, EKBD_SW29, EKBD_SW30, 
   },
   {    /*   Row 2, Col 0 to 7               */
      EKBD_SW38, EKBD_SW39, EKBD_SW40, EKBD_SW41, EKBD_SW42, EKBD_SW43, EKBD_SW44, EKBD_SW45, 
   },
   {    /*   Row 3, Col 0 to 7               */
      EKBD_SW53, EKBD_SW54, EKBD_SW55, EKBD_SW56, EKBD_SW57, EKBD_SW58, EKBD_SW76, EKBD_SW74, 
   },
   {    /*   Row 4, Col 0 to 7               */
      EKBD_SW46, EKBD_SW47, EKBD_SW48, EKBD_SW49, EKBD_SW50, EKBD_SW51, EKBD_SW52, EKBD_SW63, 
   },
   {    /*   Row 5, Col 0 to 7               */
      EKBD_SW31, EKBD_SW33, EKBD_SW34, EKBD_SW35, EKBD_SW36, EKBD_SW37, EKBD_SW62, EKBD_INVALID_SW, 
   },
   {    /*   Row 6, Col 0 to 7               */
      EKBD_SW1, EKBD_SW2, EKBD_SW3, EKBD_SW4, EKBD_SW5, EKBD_SW6, EKBD_SW7, EKBD_SW67,
   },
   {    /*   Row 7, Col 0 to 7               */
      EKBD_SW16, EKBD_SW18, EKBD_SW19, EKBD_SW20, EKBD_SW21, EKBD_SW22, EKBD_INVALID_SW, EKBD_INVALID_SW,
   },
};

LOCAL unsigned char noRepeatScancodes[] = 
{ 
   EKBD_SW31, EKBD_SW46, EKBD_SW47, EKBD_SW58, EKBD_SW59, 
};
LOCAL unsigned char AzertyNoRepeatScancodes[] = 
{ 
   EKBD_SW31, EKBD_SW45, EKBD_SW28, EKBD_SW46, EKBD_SW58, EKBD_SW59,
};
LOCAL unsigned char QwertzNoRepeatScancodesNonShift[] = 
{ 
   EKBD_SW1, EKBD_SW13, EKBD_SW20, EKBD_SW31, 
   EKBD_SW45, EKBD_SW46, EKBD_SW58, EKBD_SW59, 
};
LOCAL unsigned char QwertzNoRepeatScancodesShift[] = 
{ 
   EKBD_SW13, EKBD_SW31, EKBD_SW45, EKBD_SW46, EKBD_SW58, EKBD_SW59, 
};
LOCAL unsigned char *QwertzNoRepeatScancodesP;
LOCAL unsigned char  QwertzNoRepeatScancodesS;
char* Phone_msg = "\002Kbd Type\003";

LOCAL unsigned char        sendKeyBuf[XKBD_MAX_UART_BUF_SIZE_IN_BYTE];

/* local function definitions --------------------------------------------*/
//void DisableRows(void);
LOCAL void mStartRptKeyGHdlr(void *);
LOCAL void mRptKeyUpHdlr(void *);
LOCAL void mRptKeyDownHdlr(void *);
LOCAL unsigned char mGetRpKeyDownTO(void);
LOCAL unsigned char mGetRptKeyUpTO(void);
LOCAL unsigned char mGetRptKeyGTO(void);

/*---------------------------------------------------------+
| Global function: MatrixKeyRead
|
+---------------------------------------------------------*/

// Attention: This function is called out of a timer interrupt.
void MatrixKeyRead(void)
{
   unsigned char     *lBufP = NULL;
   unsigned short     lBufLen = 0;
   unsigned short     lBufCount = 0;
   unsigned char      lRepKeyGuardmS = 0;
   unsigned char      lRepKeyDownIntervalmS = 0;
   unsigned char      lRepKeyUpIntervalmS = 0;
   unsigned char      lIsTmrStop = 0;
   unsigned char      lNeedRepeatTimer = 0;
   unsigned char      m;
   UINT8 curr_key, temp, i, k, k1;
	UINT16 j;
  //read key's, invert -> pressed key = '1':
  // collect bits 3 and 2 from port D and bits 2 to 7 from port C

  curr_key = ((~GPIO_ReadInputData(GPIOD) & 0x0C) << 4) | ((~GPIO_ReadInputData(GPIOC) & 0xFC) >> 2);

#ifdef DEBOUNCE_NOTHING
  temp = keys[0][row_state] ^ curr_key; // exor
#endif
#ifdef DEBOUNCE_PRESS_AND_RELEASE
  temp = (keys[1][row_state] ^ curr_key) & (keys[1][row_state] ^ keys[0][row_state] );
    
#endif
#ifdef DEBOUNCE_RELEASE_ONLY
  // not debounced positive edge:
  temp = (~keys[0][row_state] & curr_key) |
          // debounced negative edge:
         (keys[1][row_state] & keys[0][row_state] & ~curr_key);
#endif

   lBufP = &sendKeyBuf[0];
   lBufP = sendKeyBuf;
   memset( lBufP, 0, sizeof(sendKeyBuf));
   lBufLen = sizeof(sendKeyBuf);
   lBufCount = 0;

  for (i = 0x01, k = 0; k < 8 ; i *= 2, k++) {
   if ( lBufLen == 0 ) {
      break;
   }
    if (temp & i) {  // key changed since last scan
      lNeedRepeatTimer = 1;
      j = scancodes[row_state][k]; // key scancode
	   switch( xKbd_type.isXKbdType) {
         case EKBD_AZERTY:             /*  2              */
            for( m = 0; m < sizeof( AzertyNoRepeatScancodes) /sizeof( unsigned char); m++) {
               if ( AzertyNoRepeatScancodes[m] == j) {
                  lNeedRepeatTimer = 0;
                  break;
               }
            }
            break;
         case EKBD_QWERTZ:              /*  6             */
            switch( j) {
               case EKBD_SW46:    /* shift key down       */
               case EKBD_SW58:
               case EKBD_SW59:
                  QwertzNoRepeatScancodesP = &QwertzNoRepeatScancodesShift[0];
                  QwertzNoRepeatScancodesS = sizeof( QwertzNoRepeatScancodesShift);
                  lNeedRepeatTimer = 0;
                  break;
               default:
                  for( m = 0; m < QwertzNoRepeatScancodesS; m++) {
                     if ( QwertzNoRepeatScancodesP[m] == j) {
                        lNeedRepeatTimer = 0;
                        break;
                     }
                  }
                  break;
            }
            break;
         default:
            for( m = 0; m < sizeof( noRepeatScancodes) /sizeof( unsigned char); m++) {
               if ( noRepeatScancodes[m] == j) {
                  lNeedRepeatTimer = 0;
                  break;
               }
            }
            break;
      }
      if (curr_key & i) {  // pressed
         *lBufP =  j | EKBD_SW_DOWN;
         lBufP++;
         lBufLen--;
         lBufCount++;
         if ( lNeedRepeatTimer) {
            xkbTimerStart(mGetRptKeyGTO(), (void *)j, mStartRptKeyGHdlr);
         }  else {
            _asm("nop");
         }
      }
      else {  // released
	      switch( xKbd_type.isXKbdType) {
            case EKBD_QWERTZ:              /*  6             */
               for( m = 0; m < QwertzNoRepeatScancodesS; m++) {
                  k1 = QwertzNoRepeatScancodesP[m];
                  lIsTmrStop = xkbTimerStop(mGetRptKeyGTO(), (void *)k1, mStartRptKeyGHdlr);
                  lIsTmrStop = xkbTimerStop(mGetRpKeyDownTO(), (void *)k1, mRptKeyDownHdlr);
                  lIsTmrStop = xkbTimerStop(mGetRptKeyUpTO(), (void *) k1, mRptKeyUpHdlr);
               }
               QwertzNoRepeatScancodesP = &QwertzNoRepeatScancodesNonShift[0];
               QwertzNoRepeatScancodesS = sizeof( QwertzNoRepeatScancodesNonShift);
               break;
            default:
               break;
         }
         *lBufP = j & ~EKBD_SW_DOWN;
         lBufP++;
         lBufLen--;
         lBufCount++;
         if ( lNeedRepeatTimer) {
            lIsTmrStop = xkbTimerStop(mGetRptKeyGTO(), (void *)j, mStartRptKeyGHdlr);
            lIsTmrStop = xkbTimerStop(mGetRpKeyDownTO(), (void *)j, mRptKeyDownHdlr);
            if ( lIsTmrStop) {
               lBufP--;
               lBufLen++;
               lBufCount--;
            }
            lIsTmrStop = xkbTimerStop(mGetRptKeyUpTO(), (void *) j, mRptKeyUpHdlr);
         }  else  {
            _asm("nop");
         }
      }
    }
  }

   if ( lBufCount) {
      UARTSendCharBuffer(EKBD_STX);
     *lBufP = EKBD_ETX;
      lBufCount++;
      UARTSendBuffer(sendKeyBuf, lBufCount);
   }

  keys[1][row_state] = keys[0][row_state];
  keys[0][row_state] = curr_key;

   return;
}

/*---------------------------------------------------------+
| Global function: MatrixRowWrite
+---------------------------------------------------------*/

// Attention: This function is called out of a timer interrupt.
void MatrixRowWrite(void)
{
   unsigned char        lGpioDdr;
	// Activate the next row.
	switch (row) {
		case 0x01:
         GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_OUT_PP_LOW_FAST);  // Row0-5 push pull high
         break;
		case 0x02:
         GPIO_Init(GPIOB, GPIO_PIN_1, GPIO_MODE_OUT_PP_LOW_FAST);  // Row0-5 push pull high
         break;
		case 0x04:
         GPIO_Init(GPIOB, GPIO_PIN_2, GPIO_MODE_OUT_PP_LOW_FAST);  // Row0-5 push pull high
         break;
		case 0x08:
         GPIO_Init(GPIOB, GPIO_PIN_3, GPIO_MODE_OUT_PP_LOW_FAST);  // Row0-5 push pull high
         break;
		case 0x10:
         GPIO_Init(GPIOB, GPIO_PIN_4, GPIO_MODE_OUT_PP_LOW_FAST);  // Row0-5 push pull high
         break;
		case 0x20:
         GPIO_Init(GPIOB, GPIO_PIN_5, GPIO_MODE_OUT_PP_LOW_FAST);  // Row0-5 push pull high
			break;
		case 0x40:
         GPIO_Init(GPIOE, GPIO_PIN_5, GPIO_MODE_OUT_PP_LOW_FAST);  // Row0-5 push pull high
			break;
		case 0x80:
         GPIO_Init(GPIOC, GPIO_PIN_1, GPIO_MODE_OUT_PP_LOW_FAST);  // Row0-5 push pull high
			break;
	}
}

/*---------------------------------------------------------+
| Global function: MatrixInit
+---------------------------------------------------------*/
void MatrixInit(void)
{
   matrixClearMem();
	 BitStatus st0 ,st4,st7;
	 uint8_t st00;
  st00 =0;
	 st0=RESET;
	 st4=RESET;
	 st7=RESET;
//	st00 = BitStatus((GPIO_ReadInputPin( GPIOD, GPIO_PIN_0)));
	
 st0=  GPIO_ReadInputPin( GPIOD, GPIO_PIN_0);
 st4= GPIO_ReadInputPin( GPIOD, GPIO_PIN_4);
 // if st4 D4=1, then set xkbd gpiog4=1
 
 st7= GPIO_ReadInputPin( GPIOD, GPIO_PIN_7);
 //st00= GPIO_ReadInputData(GPIOD);
 
 	xKbd_type.isBi.tGpio00 = (BitStatus)(st0 |0xf0);
//	((BitStatus)(GPIOx->IDR & (uint8_t)GPIO_Pin)); //lisa modify
 xKbd_type.isBi.tGpio04 = (BitStatus)(st4 |0xef);
 xKbd_type.isBi.tGpio07 =(BitStatus)(st7 |0xf0);
 
 //=============================
//	xKbd_type.isBi.tGpio00 = GPIO_ReadInputPin( GPIOD, GPIO_PIN_0);
 //  xKbd_type.isBi.tGpio04 = GPIO_ReadInputPin( GPIOD, GPIO_PIN_4);
 //  xKbd_type.isBi.tGpio07 = GPIO_ReadInputPin( GPIOD, GPIO_PIN_7);
   // Adjust keyboard type if necessary
	switch( xKbd_type.isXKbdType) {
      case EKBD_AZERTY:             /*  2              */
	      Kbd_str[6] = '2';
         break;
      case EKBD_AZZERTY:            /*  3  ( Lithuanian) */
	      Kbd_str[6] = '3';
         break;
      case EKBD_QZERTY:             /*  4              */
	      Kbd_str[6] = '4';
         break;
      case EKBD_DVORAK:             /*  5              */
	      Kbd_str[6] = '5';
         break;
      case EKBD_QWERTZ:             /*  6              */
	      Kbd_str[6] = '6';
         QwertzNoRepeatScancodesP = &QwertzNoRepeatScancodesNonShift[0];
         QwertzNoRepeatScancodesS = sizeof( QwertzNoRepeatScancodesNonShift);
         break;
      case EKBD_QWERTY:             /*  1              */
         break;
      case EKBD_LAYOUT_INVALID:     /*  0              */
      default:
	      Kbd_str[6] = '9';
         break;
   }
   /*  set keyboard version */
   Kbd_str[9] = xkbU.bLeast;
   Kbd_str[8] = xkbU.bMost;
 
   GPIO_Init(GPIOB, GPIO_PIN_ALL, GPIO_MODE_IN_PU_NO_IT);  // Row0-5 push pull high
   GPIO_Init(GPIOE, GPIO_PIN_5, GPIO_MODE_IN_PU_NO_IT);  // Row6 push pull high
   GPIO_Init(GPIOC, GPIO_PIN_1, GPIO_MODE_IN_PU_NO_IT);  // Row7 push pull high

   //   Input pull-up, no external interrupt
   GPIO_Init(GPIOC, PORTC_COLUMN_BITS, GPIO_MODE_IN_PU_NO_IT);  // Col0-5
   // Init port D as floating input for key columns.
   //   Input pull-up, no external interrupt
   GPIO_Init(GPIOD, PORTD_COLUMN_BITS, GPIO_MODE_IN_PU_NO_IT);  // Col6-7
	
   Keyboard_mode = Kbd_Startup;
   matrixKeyRepeatCount = 0;
}
void Check_status(void)
{
   FlagStatus    lm;

   switch ( Keyboard_mode) {
      case Kbd_Running:
      case Kbd_Running_PollSent:
         break;
      default:
         return;
         break;
   }
   if ( xkbRxPhonePollGuard()) {
      return;
   }
   lm = UART2_GetFlagStatus( UART2_FLAG_RXNE); 
   if ( lm == SET) {
      _asm("nop");
      return;
   } else {
      _asm("nop");
   }
   MatrixRowWrite();
   MatrixKeyRead();
	switch (row) {
		case 0x01:
         GPIO_WriteReverse(GPIOB, GPIO_PIN_0);
         GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_IN_PU_NO_IT);
         break;
		case 0x02:
         GPIO_WriteReverse(GPIOB, GPIO_PIN_1);
         GPIO_Init(GPIOB, GPIO_PIN_1, GPIO_MODE_IN_PU_NO_IT);
         break;
		case 0x04:
         GPIO_WriteReverse(GPIOB, GPIO_PIN_2);
         GPIO_Init(GPIOB, GPIO_PIN_2, GPIO_MODE_IN_PU_NO_IT);
         break;
		case 0x08:
         GPIO_WriteReverse(GPIOB, GPIO_PIN_3);
         GPIO_Init(GPIOB, GPIO_PIN_3, GPIO_MODE_IN_PU_NO_IT);
         break;
		case 0x10:
         GPIO_WriteReverse(GPIOB, GPIO_PIN_4);
         GPIO_Init(GPIOB, GPIO_PIN_4, GPIO_MODE_IN_PU_NO_IT);
         break;
		case 0x20:
         GPIO_WriteReverse(GPIOB, GPIO_PIN_5);
         GPIO_Init(GPIOB, GPIO_PIN_5, GPIO_MODE_IN_PU_NO_IT);
			break;
		case 0x40:
         GPIO_WriteReverse(GPIOE, GPIO_PIN_5);
         GPIO_Init(GPIOE, GPIO_PIN_5, GPIO_MODE_IN_PU_NO_IT);
			break;
		case 0x80:
         GPIO_WriteReverse(GPIOC, GPIO_PIN_1);
         GPIO_Init(GPIOC, GPIO_PIN_1, GPIO_MODE_IN_PU_NO_IT);
			break;
	}
  // calc next row 
  row <<= 1;
  row_state++;
  if ( row == 0x0 ) {
    row = 0x01;
    row_state = 0;
  }
}

void     matrixClearMem(void)
{
   xKbd_type.isUnChar = 0;
   Keyboard_mode = Kbd_Unknown;
   matrixKeyRepeatCount = 0;
   QwertzNoRepeatScancodesP = 0;
   QwertzNoRepeatScancodesS = 0;
}

LOCAL void     mStartRptKeyGHdlr(void *ip)
{
   if ( matrixKeyRepeatCount == 0 ) {
      return;
   }
   xkbTimerStart(mGetRptKeyUpTO(), ip, mRptKeyUpHdlr);
}

LOCAL unsigned char     mGetRptKeyGTO(void)
{
   unsigned short     lRepGTO;
   lRepGTO = 1000;
   lRepGTO /= matrixKeyRepeatCount;
   lRepGTO *= 2;
   if ( lRepGTO > XKB_MAX_TIM2_TIMEOUT_IN_MS) {
      return XKB_MAX_TIM2_TIMEOUT_IN_MS;
   }
   return (unsigned char) lRepGTO;
}
LOCAL unsigned char     mGetRptKeyUpTO(void)
{
   unsigned short     lRepUpTO;
   lRepUpTO = 1000;
   lRepUpTO /= matrixKeyRepeatCount;
   lRepUpTO /= 2;
   if ( lRepUpTO > XKB_MAX_TIM2_TIMEOUT_IN_MS) {
      return XKB_MAX_TIM2_TIMEOUT_IN_MS;
   }
   return (unsigned char) lRepUpTO;
}
LOCAL unsigned char     mGetRpKeyDownTO(void)
{
   unsigned short     lRepDownTO;
   lRepDownTO = 1000;
   lRepDownTO /= matrixKeyRepeatCount;
   lRepDownTO /= 2;
   if ( lRepDownTO > XKB_MAX_TIM2_TIMEOUT_IN_MS) {
      return XKB_MAX_TIM2_TIMEOUT_IN_MS;
   }
   return (unsigned char) lRepDownTO;
}
LOCAL void     mRptKeyUpHdlr(void *iu)
{
   unsigned short     lu;
   unsigned char      lRepScanUp;

   if ( matrixKeyRepeatCount == 0 ) {
      return;
   }

   xkbTimerStart(mGetRpKeyDownTO(), iu, mRptKeyDownHdlr);
   lu = (unsigned short) iu;
   lRepScanUp = (unsigned char) lu;
   lRepScanUp &= ~EKBD_SW_DOWN;
   UARTSendCharBuffer(EKBD_STX);
   UARTSendCharBuffer(lRepScanUp);
   UARTSendCharBuffer(EKBD_ETX);
}
LOCAL void     mRptKeyDownHdlr(void *id)
{
   unsigned short     ldd;
   unsigned char      lRepScanDown;

   if ( matrixKeyRepeatCount == 0 ) {
      return;
   }

   xkbTimerStart(mGetRptKeyUpTO(), id, mRptKeyUpHdlr);
   ldd = (unsigned short) id;
   lRepScanDown = (unsigned char) ldd;
   lRepScanDown |= EKBD_SW_DOWN;
   UARTSendCharBuffer(EKBD_STX);
   UARTSendCharBuffer(lRepScanDown);
   UARTSendCharBuffer(EKBD_ETX);
}

GLOBAL void    matrixSetRepeatCount(void) 
{
   matrixKeyRepeatCount = XKBD_KEY_REPEAT_RATE_IN_SECOND;
}
GLOBAL void    matrixClearRepeatCount(void) 
{
   matrixKeyRepeatCount = 0;
}

/*******************************************************************************
 * FILE PURPOSE:
 *******************************************************************************
 * FILE NAME:       uart.h
 *
 * DESCRIPTION:     Serial interface
 *
 * AUTHOR:          Heinz Trueb
 *-----------------------------------------------------------------------------
 *
 * (C) Copyright 2005, AASTRA Telecom Schweiz AG
 * (C) Copyright 2013, AASTRA Telecom
 ******************************************************************************/
#ifndef UART_H_
#define UART_H_

GLOBAL void InitUART(void);
GLOBAL void StartUART(void);
GLOBAL void UART_IT_Function(void);
GLOBAL void UARTSendBuffer(UINT8 *buf, UINT8 len);
GLOBAL void UARTSendCharBuffer(unsigned char);
GLOBAL unsigned char xkbRxPhonePollGuard(void);
GLOBAL unsigned char xkbTxRtnReadIdx(void);
GLOBAL unsigned char xkbTxRtnWrtIdx(void);

#endif UART_H

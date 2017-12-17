/*******************************************************************************
 * FILE PURPOSE:    SW version
 *******************************************************************************
 * FILE NAME:       matrix.h
 *
 * AUTHOR:          unknown
 *-----------------------------------------------------------------------------
 *
 * (C) Copyright 2006, AASTRA Telecom Schweiz AG
 * (C) Copyright 2013, AASTRA Telecom
 ******************************************************************************/
#ifndef MATRIX_H
#define MATRIX_H

void     MatrixInit(void);
void     MatrixRowWrite(void);
void     Check_status(void);
void     matrixClearMem(void);
void     MatrixKeyRead(void);

#define     XKBD_MAX_ROW_COUNT     8
#define     XKBD_MAX_COL_COUNT     8

#define     XKBD_MAX_UART_BUF_SIZE_IN_BYTE     10
#define     XKBD_KEY_REPEAT_RATE_IN_SECOND     40


/* -------------------------------------------------------*/
#endif  // MATRIX_H


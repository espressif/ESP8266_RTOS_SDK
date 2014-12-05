/*
 *  Copyright (C) 2010 -2011  Espressif System
 *
 */

#ifndef __UART_H__
#define __UART_H__

#define UART0   0
#define UART1   1

typedef enum {
    FIVE_BITS  = 0x0,
    SIX_BITS   = 0x1,
    SEVEN_BITS = 0x2,
    EIGHT_BITS = 0x3
} UartBitsNum4Char;

typedef enum {
    ONE_STOP_BIT      = 0,
    ONE_HALF_STOP_BIT = BIT(2),
    TWO_STOP_BIT      = BIT(2)
} UartStopBitsNum;

typedef enum {
    NONE_BITS = 0,
    ODD_BITS  = 0,
    EVEN_BITS = BIT(4)
} UartParityMode;

typedef enum {
    STICK_PARITY_DIS = 0,
    STICK_PARITY_EN  = BIT(3) | BIT(5)
} UartExistParity;

typedef enum {
    BIT_RATE_9600   = 9600,
    BIT_RATE_19200  = 19200,
    BIT_RATE_38400  = 38400,
    BIT_RATE_57600  = 57600,
    BIT_RATE_74880  = 74880,
    BIT_RATE_115200 = 115200,
    BIT_RATE_230400 = 230400,
    BIT_RATE_460800 = 460800,
    BIT_RATE_921600 = 921600
} UartBautRate;

typedef enum {
    NONE_CTRL,
    HARDWARE_CTRL,
    XON_XOFF_CTRL
} UartFlowCtrl;

typedef struct {
    UartBautRate     baut_rate;
    UartBitsNum4Char data_bits;
    UartExistParity  exist_parity;
    UartParityMode 	 parity;    // chip size in byte
    UartStopBitsNum  stop_bits;
    UartFlowCtrl     flow_ctrl;
} UartDevice;

#endif

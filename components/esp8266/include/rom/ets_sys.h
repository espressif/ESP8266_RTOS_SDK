/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __ETS_SYS_H__
#define __ETS_SYS_H__

#include <stdint.h>

#include "esp8266/eagle_soc.h"

/* interrupt related */
#define ETS_SPI_INUM        2
#define ETS_GPIO_INUM       4
#define ETS_UART_INUM       5
#define ETS_MAX_INUM        6
#define ETS_SOFT_INUM       7
#define ETS_WDT_INUM        8
#define ETS_FRC_TIMER1_INUM 9

extern char NMIIrqIsOn;
extern uint32_t WDEV_INTEREST_EVENT;

#define INT_ENA_WDEV        0x3ff20c18
#define WDEV_TSF0_REACH_INT (BIT(27))

#define ETS_NMI_LOCK()  \
    do {    \
        char m = 10;    \
        do {    \
            REG_WRITE(INT_ENA_WDEV, 0); \
            m = 10; \
            for (; m > 0; m--) {}   \
            REG_WRITE(INT_ENA_WDEV, WDEV_TSF0_REACH_INT);   \
        } while(0); \
    } while (0)

#define ETS_NMI_UNLOCK()    \
    do {    \
        REG_WRITE(INT_ENA_WDEV, WDEV_INTEREST_EVENT);   \
    } while (0)

#define ETS_INTR_LOCK() do {    \
    if (NMIIrqIsOn == 0) { \
        vPortEnterCritical();   \
        char m = 10;    \
        do {    \
            REG_WRITE(INT_ENA_WDEV, 0); \
            m = 10; \
            for (; m > 0; m--) {}   \
            REG_WRITE(INT_ENA_WDEV, WDEV_TSF0_REACH_INT);   \
        } while(0);  \
    }   \
    } while(0)

#define ETS_INTR_UNLOCK()   do {    \
    if (NMIIrqIsOn == 0) { \
        REG_WRITE(INT_ENA_WDEV, WDEV_INTEREST_EVENT);   \
        vPortExitCritical(); \
    }   \
    } while(0)

#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"

/**
  * @brief  Delay function, maximum value: 65535 us.
  *
  * @param  uint16_t us : delay time, uint: us, maximum value: 65535 us
  *
  * @return null
  */
void os_delay_us(uint16_t us);

/**
  * @brief     Register the print output function.
  *
  * @attention os_install_putc1((void *)uart1_write_char) in uart_init will set
  *            printf to print from UART 1, otherwise, printf will start from
  *            UART 0 by default.
  *
  * @param     void(*p)(char c) - pointer of print function
  *
  * @return    null
  */
void os_install_putc1(void (*p)(char c));

/**
  * @brief  Print a character. Start from from UART0 by default.
  *
  * @param  char c - character to be printed
  *
  * @return null
  */
void os_putc(char c);

#endif /* _ETS_SYS_H */

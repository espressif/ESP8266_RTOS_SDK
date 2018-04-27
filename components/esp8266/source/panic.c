// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <string.h>

#include "c_types.h"

#include "esp8266/ets_sys.h"
#include "esp8266/eagle_soc.h"
#include "esp8266/uart_register.h"

/*
 * Todo: panic output UART ID, we may add it to 'kconfig' to select target UART.
 */
#define PANIC_UART 0

/*
 * @brief output a character
 * 
 * @param c a character
 * 
 * @return none
 */
static IRAM_ATTR void panit_putc(char c)
{
    while (1) {
        uint32_t fifo_cnt = READ_PERI_REG(UART_STATUS(PANIC_UART)) & (UART_TXFIFO_CNT << UART_TXFIFO_CNT_S);

        if ((fifo_cnt >> UART_TXFIFO_CNT_S & UART_TXFIFO_CNT) < 126)
            break;
    }

    WRITE_PERI_REG(UART_FIFO(PANIC_UART) , c);
}

/*
 * @brief output a left-aligned string
 * 
 * @param s string pointer
 * @param left_aligned left-aligned bytes
 * 
 * @return none
 */
static IRAM_ATTR void panic_puts(const char *s, int left_aligned)
{
    int bytes = left_aligned - strlen(s);

    while (bytes-- > 0)
        panit_putc(' ');

    while (*s)
        panit_putc(*s++);
}

/*
 * @brief output a hex value string of 32-bites data
 * 
 * @param hex 32-bites data
 * 
 * @return none
 */
static IRAM_ATTR void panic_puthex(int hex)
{
    char buf[8];
    int bytes = 0;

    while (bytes < 8) {
        char c = (uint32_t)hex % 16;

        if (c < 10)
            c = c + '0';
        else
            c = c - 10 + 'a';

        buf[bytes++] = c;
        hex >>= 4;
    }

    while (bytes)
        panit_putc(buf[--bytes]);
}

/*
 * @brief output xtensa register value map when crash
 * 
 * @param frame xtensa register value map pointer
 * 
 * @return none
 */
void IRAM_ATTR panicHandler(void *frame)
{
    int *regs = (int *)frame;
    int x, y;
    const char *sdesc[] = {
        "PC",   "PS",   "A0",   "A1",
        "A2",   "A3",   "A4",   "A5",
        "A6",   "A7",   "A8",   "A9",
        "A10",  "A11",  "A12",  "A13",
        "A14",  "A15",  "SAR",  "EXCCAUSE"
    };

    /* NMI can interrupt exception. */
    ETS_INTR_LOCK();

    for (x = 0; x < 20; x += 4) {
        for (y = 0; y < 4; y++) {
            panic_puts(sdesc[x + y], 8);
            panic_puts(": 0x", 0);
            panic_puthex(regs[x + y + 1]);
            panic_puts("  ", 0);
        }
        panit_putc('\n');
    }

    /*
     * Todo: add more option to select here to 'Kconfig':
     *     1. blocking
     *     2. restart
     *     3. GBD break
     */
    while (1);
}

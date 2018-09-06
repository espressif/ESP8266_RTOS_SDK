// Copyright 2018-2019 Espressif Systems (Shanghai) PTE LTD
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

#include <stdint.h>

#include "sdkconfig.h"

#include "esp_attr.h"

#include "esp8266/eagle_soc.h"
#include "esp8266/uart_register.h"
#include "esp8266/rom_functions.h"

int IRAM_ATTR ets_putc(int c)
{
    while (1) {
        uint32_t fifo_cnt = READ_PERI_REG(UART_STATUS(CONFIG_CONSOLE_UART_NUM)) & (UART_TXFIFO_CNT << UART_TXFIFO_CNT_S);

        if ((fifo_cnt >> UART_TXFIFO_CNT_S & UART_TXFIFO_CNT) < 126)
            break;
    }

    WRITE_PERI_REG(UART_FIFO(CONFIG_CONSOLE_UART_NUM) , c);

    return c;
}

int IRAM_ATTR ets_vprintf(const char *fmt, va_list ap)
{
    return ets_io_vprintf(ets_putc, fmt, ap);
}

/* Re-write ets_printf in SDK side, since ets_printf in ROM will use a global
 * variable which address is in heap region of SDK side. If use ets_printf in ROM,
 * this variable maybe re-write when heap alloc and modification.*/
int IRAM_ATTR ets_printf(const char* fmt, ...)
{
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = ets_vprintf(fmt, ap);
    va_end(ap);

    return ret;
}

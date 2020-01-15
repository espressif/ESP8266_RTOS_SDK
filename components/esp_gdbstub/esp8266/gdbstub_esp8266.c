// Copyright 2015-2019 Espressif Systems (Shanghai) PTE LTD
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

#include "esp8266/uart_register.h"
#include "esp8266/eagle_soc.h"
#include "sdkconfig.h"
#include "esp_task_wdt.h"

#define UART_NUM CONFIG_ESP_CONSOLE_UART_NUM

void esp_gdbstub_target_init(void)
{
}

int esp_gdbstub_getchar(void)
{
    while (((READ_PERI_REG(UART_STATUS(UART_NUM)) >> UART_RXFIFO_CNT_S) & UART_RXFIFO_CNT) == 0) {
        esp_task_wdt_reset();
    }
    return READ_PERI_REG(UART_FIFO(UART_NUM));
}

void esp_gdbstub_putchar(int c)
{
    while (((READ_PERI_REG(UART_STATUS(UART_NUM)) >> UART_TXFIFO_CNT_S) & UART_TXFIFO_CNT) >= 126) {
        ;
    }

    WRITE_PERI_REG(UART_FIFO(UART_NUM) , c);
}

int esp_gdbstub_readmem(intptr_t addr)
{
    if (addr < 0x3ff00000 || addr >= 0x60010000) {
        /* see cpu_configure_region_protection */
        return -1;
    }
    uint32_t val_aligned = *(uint32_t *)(addr & (~3));
    uint32_t shift = (addr & 3) * 8;
    return (val_aligned >> shift) & 0xff;
}

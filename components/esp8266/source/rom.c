// Copyright 2019-2020 Espressif Systems (Shanghai) PTE LTD
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

#include "rom/uart.h"

void uart_tx_wait_idle(uint8_t uart_no)
{
    uint32_t tx_bytes;
    uint32_t baudrate, byte_delay_us;
    uart_dev_t *const UART[2] = {&uart0, &uart1};
    uart_dev_t *const uart = UART[uart_no];
    
    baudrate = (UART_CLK_FREQ / (uart->clk_div.val & 0xFFFFF));
    byte_delay_us = (uint32_t)(10000000 / baudrate);

    do {
        tx_bytes = uart->status.txfifo_cnt;
        /* either tx count or state is non-zero */
    } while (tx_bytes);

    ets_delay_us(byte_delay_us);
}

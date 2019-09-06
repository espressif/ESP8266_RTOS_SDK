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

#include <stdio.h>
#include <assert.h>
#include "esp_system.h"
#include "esp_rftest.h"
#include "esp_log.h"
#include "esp_vfs_dev.h"
#include "FreeRTOS.h"
#include "driver/uart.h"
#include "nano_console.h"
#include "esp_libc.h"

#ifndef ESP_FACTORY_TEST_EXTRA_COMPONENTS

#define CONFIG_CONSOLE_UART_NUM 0

#define TAG "factory-test"

static void initialize_console()
{
    /* Disable buffering on stdin */
    setvbuf(stdin, NULL, _IONBF, 0);

    /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
    esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
    /* Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

    /* Configure UART. Note that REF_TICK is used so that the baud rate remains
     * correct while APB frequency is changing in light sleep mode.
     */
    uart_config_t uart_config = {
            .baud_rate = CONFIG_CONSOLE_UART_BAUDRATE,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
    };
    ESP_ERROR_CHECK(uart_param_config(CONFIG_CONSOLE_UART_NUM, &uart_config));

    /* Install UART driver for interrupt-driven reads and writes */
    ESP_ERROR_CHECK(uart_driver_install(CONFIG_CONSOLE_UART_NUM,
            256, 0, 0, NULL, 0));

    /* Tell VFS to use UART driver */
    esp_vfs_dev_uart_use_driver(CONFIG_CONSOLE_UART_NUM);

    esp_console_register_rftest_command();

    ESP_ERROR_CHECK(nc_init());
}

void __attribute__((weak)) app_main(void)
{
    ESP_LOGI(TAG, "SDK factory test firmware version:%s\n", esp_get_idf_version());

    initialize_console();
}

#endif /* ESP_FACTORY_TEST_EXTRA_COMPONENTS */

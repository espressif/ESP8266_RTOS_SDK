// Copyright 2018-2025 Espressif Systems (Shanghai) PTE LTD
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

#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Receive SPI data
 *
 * @param data Data buffer to receive
 * @param len Data length
 * @param xTicksToWait Ticks to wait until receive data; use portMAX_DELAY to
 *                      never time out.
 * @return 
 *         - Actual length received
 */
uint32_t hspi_slave_logic_read_data(uint8_t*data, uint32_t len, TickType_t xTicksToWait);

/**
 * @brief Send SPI data
 *
 * @param data Data buffer to send 
 * @param len Data length
 * @param xTicksToWait Ticks to wait until send data; use portMAX_DELAY to
 *                      never time out.
 * @return 
 *         - Actual length received
 */
uint32_t hspi_slave_logic_write_data(uint8_t*data, uint32_t len, TickType_t xTicksToWait);

/**
 * @brief Create a SPI device to transmit SPI data
 *
 * @param trigger_pin The pin used for handshake
 * @param trigger_level The number of bytes that must be in the stream
 * buffer before a task that is blocked on the stream buffer to wait for data is
 * moved out of the blocked state.  For example, if a task is blocked on a read
 * of an empty stream buffer that has a trigger level of 1 then the task will be
 * unblocked when a single byte is written to the buffer or the task's block
 * time expires.  As another example, if a task is blocked on a read of an empty
 * stream buffer that has a trigger level of 10 then the task will not be
 * unblocked until the stream buffer contains at least 10 bytes or the task's
 * block time expires.  If a reading task's block time expires before the
 * trigger level is reached then the task will still receive however many bytes
 * are actually available.  Setting a trigger level of 0 will result in a
 * trigger level of 1 being used.  It is not valid to specify a trigger level
 * that is greater than the buffer size.
 * @param tx_buffer_size The total number of bytes the send stream buffer will be
 * able to hold at any one time.
 * @param rx_buffer_size The total number of bytes the receive stream buffer will be
 * able to hold at any one time.
 * 
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_NO_MEM No memory
 */
esp_err_t hspi_slave_logic_device_create(gpio_num_t trigger_pin, uint32_t trigger_level,uint32_t tx_buffer_size, uint32_t rx_buffer_size);

/**
 * @brief Delete the SPI slave bus
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_STATE SPI slave already deleted
 */
esp_err_t hspi_slave_logic_device_delete(void);

#ifdef __cplusplus
}
#endif
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
#include "spi_ram.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void *spi_ram_fifo_handle_t;

/**
 * @brief spi ram fifo initialization parameter structure type definition
 */
typedef struct {
    spi_ram_num_t ram_num;
    uint32_t start_addr;
    uint32_t total_size;
} spi_ram_fifo_config_t;

/**
  * @brief spi ram fifo write function
  *
  * @note spi ram fifo may block waiting for free space in fifo
  *
  * @param handle spi ram fifo handle
  * @param data Pointer to the write data buffer
  * @param len Length of write data, range: len > 0
  * @param timeout_ticks freertos timeout ticks
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_ERR_TIMEOUT spi ram fifo write timeout
  *     - ESP_FAIL spi ram fifo not created yet
  */
esp_err_t spi_ram_fifo_write(spi_ram_fifo_handle_t handle, uint8_t *data, int len, uint32_t timeout_ticks);

/**
  * @brief spi ram fifo read function
  *
  * @note spi ram fifo may block waiting for data in fifo
  *
  * @param handle spi ram fifo handle
  * @param data Pointer to the read data buffer
  * @param len Length of read data, range: len > 0
  * @param timeout_ticks freertos timeout ticks
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_ERR_TIMEOUT spi ram fifo read timeout
  *     - ESP_FAIL spi ram fifo not created yet
  */
esp_err_t spi_ram_fifo_read(spi_ram_fifo_handle_t handle, uint8_t *data, int len, uint32_t timeout_ticks);

/**
  * @brief Get spi ram fifo filled length
  *
  * @param handle spi ram fifo handle
  * @param len Pointer to the fifo filled length
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_FAIL spi ram fifo not created yet
  */
esp_err_t spi_ram_fifo_get_fill(spi_ram_fifo_handle_t handle, uint32_t *len);

/**
  * @brief Get spi ram fifo free length
  *
  * @param handle spi ram fifo handle
  * @param len Pointer to the fifo free length
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_FAIL spi ram fifo not created yet
  */
esp_err_t spi_ram_fifo_get_free(spi_ram_fifo_handle_t handle, uint32_t *len);

/**
  * @brief Get spi ram fifo total length
  *
  * @param handle spi ram fifo handle
  * @param len Pointer to the fifo total length
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_FAIL spi ram fifo not created yet
  */
esp_err_t spi_ram_fifo_get_total(spi_ram_fifo_handle_t handle, uint32_t *len);

/**
  * @brief Get spi ram fifo overflow number of times

  *
  * @param handle spi ram fifo handle
  * @param num Pointer to the fifo overflow number of times
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_FAIL spi ram fifo not created yet
  */
esp_err_t spi_ram_fifo_get_overflow(spi_ram_fifo_handle_t handle, uint32_t *num);

/**
  * @brief Get spi ram fifo underrun number of times
  *
  * @param handle spi ram fifo handle
  * @param num Pointer to the fifo underrun number of times
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_FAIL spi ram fifo not created yet
  */
esp_err_t spi_ram_fifo_get_underrun(spi_ram_fifo_handle_t handle, uint32_t *num);

/**
  * @brief Delete the spi ram fifo
  *
  * @param handle spi ram fifo handle
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_FAIL spi ram fifo not created yet
  */
esp_err_t spi_ram_fifo_delete(spi_ram_fifo_handle_t handle);

/**
  * @brief Create the spi ram fifo
  *
  * @param config Pointer to the configuration parameter
  *
  * @return
  *     - spi ram fifo handle
  */
spi_ram_fifo_handle_t spi_ram_fifo_create(spi_ram_fifo_config_t *config);

#ifdef __cplusplus
}
#endif
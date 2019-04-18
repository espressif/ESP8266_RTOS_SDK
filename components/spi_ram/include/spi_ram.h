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

#ifdef __cplusplus
extern "C" {
#endif

/* spi ram spi mode default cmd definition */
#define SPI_RAM_SPI_MODE_DEFAULT_CMD   0x000B0200    /* EXIT: 0x00, READ: 0x0B, WRITE: 0x02, START: 0x00*/
/* spi ram spi mode default read wait cycle definition */
#define SPI_RAM_SPI_MODE_DEFAULT_READ_WAIT_CYCLE   8

/* spi ram qpi mode default cmd definition */
#define SPI_RAM_QPI_MODE_DEFAULT_CMD 0xF5EB3835    /* EXIT: 0xF5, READ: 0xEB, WRITE: 0x38, START: 0x35*/
/* spi ram qpi mode default read wait cycle definition */
#define SPI_RAM_QPI_MODE_DEFAULT_READ_WAIT_CYCLE   6

/**
 * @brief spi ram number
 * 
 * @note SPI_RAM_NUM_0 use hardware CS (IO0) and SPI_RAM_NUM_1 use GPIO to simulate CS (IO5).
 */
typedef enum {
    SPI_RAM_NUM_0 = 0x0,
    SPI_RAM_NUM_1,
    SPI_RAM_NUM_MAX
} spi_ram_num_t;

/**
 * @brief spi ram clock division factor enumeration
 */
typedef enum {
    SPI_RAM_2MHz_DIV  = 40,
    SPI_RAM_4MHz_DIV  = 20,
    SPI_RAM_5MHz_DIV  = 16,
    SPI_RAM_8MHz_DIV  = 10,
    SPI_RAM_10MHz_DIV = 8,
    SPI_RAM_16MHz_DIV = 5,
    SPI_RAM_20MHz_DIV = 4,
    SPI_RAM_40MHz_DIV = 2,
    SPI_RAM_80MHz_DIV = 1,
} spi_ram_clk_div_t;

typedef union {
    struct {
        uint32_t start:   8;
        uint32_t write:   8;
        uint32_t read:    8;
        uint32_t exit:    8;
    };
    uint32_t val;
} spi_ram_cmd_t;

/**
 * @brief spi ram initialization parameter structure type definition
 */
typedef struct {
    uint32_t size;
    uint32_t read_wait_cycle;
    spi_ram_cmd_t cmd;
    spi_ram_clk_div_t clk_div;
} spi_ram_config_t;

/**
  * @brief spi ram check function
  *
  * @param num spi ram number
  *     - SPI_RAM_NUM_0
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_FAIL spi ram error
  */
esp_err_t spi_ram_check(spi_ram_num_t num);

/**
  * @brief Set the spi ram clock division factor
  *
  * @param num spi ram number
  *     - SPI_RAM_NUM_0
  * @param data Pointer to the spi ram clock division factor
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_FAIL spi ram not installed yet
  */
esp_err_t spi_ram_set_clk_div(spi_ram_num_t num, spi_ram_clk_div_t *clk_div);

/**
  * @brief spi ram data write function
  *
  * @param num spi ram number
  *     - SPI_RAM_NUM_0
  * @param addr spi ram write address
  * @param data Pointer to the write data buffer
  * @param len Length of write data, range [1, 64]
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_FAIL spi ram not installed yet
  */
esp_err_t spi_ram_write(spi_ram_num_t num, uint32_t addr, uint8_t *data, int len);

/**
  * @brief spi ram data read function
  *
  * @param num spi ram number
  *     - SPI_RAM_NUM_0
  * @param addr spi ram read address
  * @param data Pointer to the read data buffer
  * @param len Length of read data, range [1, 64]
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_FAIL spi ram not installed yet
  */
esp_err_t spi_ram_read(spi_ram_num_t num, uint32_t addr, uint8_t *data, int len);

/**
  * @brief Deinit the spi ram
  *
  * @param num spi ram number
  *     - SPI_RAM_NUM_0
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_FAIL spi ram not installed yet
  */
esp_err_t spi_ram_deinit(spi_ram_num_t num);

/**
  * @brief Initialize the spi ram
  *
  * @param num spi ram number
  *     - SPI_RAM_NUM_0
  * @param config Pointer to deliver initialize configuration parameter
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_NO_MEM malloc fail
  *     - ESP_FAIL spi ram has been initialized
  */
esp_err_t spi_ram_init(spi_ram_num_t num, spi_ram_config_t *config);

#ifdef __cplusplus
}
#endif
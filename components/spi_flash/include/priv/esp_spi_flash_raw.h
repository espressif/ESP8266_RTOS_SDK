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

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#include "esp8266/rom_functions.h"

#ifdef __cplusplus
extern "C" {
#endif

enum GD25Q32C_status {
    GD25Q32C_STATUS1=0,
    GD25Q32C_STATUS2,
    GD25Q32C_STATUS3,
};

typedef enum {
    SPI_TX   = 0x1,
    SPI_RX   = 0x2,
    SPI_WRSR = 0x4,
    SPI_RAW  = 0x8,  /*!< No wait spi idle and no read status */
} spi_cmd_dir_t;

typedef struct {
    uint16_t cmd;
    uint8_t cmd_len;
    uint32_t *addr;
    uint8_t addr_len;
    uint32_t *data;
    uint8_t data_len;
    uint8_t dummy_bits;
} spi_cmd_t;

bool spi_user_cmd_raw(esp_rom_spiflash_chip_t *chip, spi_cmd_dir_t mode, spi_cmd_t *p_cmd);

uint32_t spi_flash_get_id_raw(esp_rom_spiflash_chip_t *chip);

esp_err_t spi_flash_enable_qmode_raw(esp_rom_spiflash_chip_t *chip);

esp_err_t spi_flash_read_status_raw(esp_rom_spiflash_chip_t *chip, uint32_t *status);

esp_err_t spi_flash_write_status_raw(esp_rom_spiflash_chip_t *chip, uint32_t status_value);

esp_err_t spi_flash_read_raw(esp_rom_spiflash_chip_t *chip, size_t src_addr, void *dest, size_t size);

esp_err_t spi_flash_write_raw(esp_rom_spiflash_chip_t *chip, size_t dest_addr, const void *src, size_t size);

esp_err_t spi_flash_erase_sector_raw(esp_rom_spiflash_chip_t *chip, size_t sec, size_t sec_size);

void spi_flash_switch_to_qio_raw(void);

#ifdef __cplusplus
}
#endif

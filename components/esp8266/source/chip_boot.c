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

#include "sdkconfig.h"
#include "esp_attr.h"
#include "spi_flash.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp8266/eagle_soc.h"
#include "esp8266/rom_functions.h"
#include "esp_image_format.h"

#define PERIPHS_SPI_FLASH_USRREG        (0x60000200 + 0x1c)
#define PERIPHS_SPI_FLASH_CTRL          (0x60000200 + 0x08)
#define PERIPHS_IO_MUX_CONF_U           (0x60000800)

#define SPI0_CLK_EQU_SYSCLK             BIT8
#define SPI_FLASH_CLK_EQU_SYSCLK        BIT12

static const char *TAG = "chip_boot";

/*
 * @brief initialize the chip including flash I/O and chip cache according to
 *        boot parameters which are stored at the flash
 */
void chip_boot(size_t start_addr)
{
    int ret;
    uint32_t freqdiv, flash_size;
    uint32_t freqbits;
    esp_image_header_t fhdr;

    uint32_t flash_map_table[FALSH_SIZE_MAP_MAX] = {
        1 * 1024 * 1024,
        2 * 1024 * 1024,
        4 * 1024 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024
    };
    uint32_t flash_map_table_size = sizeof(flash_map_table) / sizeof(flash_map_table[0]);

    extern esp_spi_flash_chip_t flashchip;
    extern void phy_get_bb_evm(void);
    extern void cache_init(uint8_t);
    extern void user_spi_flash_dio_to_qio_pre_init(void);

    phy_get_bb_evm();

    SET_PERI_REG_MASK(PERIPHS_SPI_FLASH_USRREG, BIT5);

    ret = spi_flash_read(start_addr, &fhdr, sizeof(esp_image_header_t));
    if (ret) {
        ESP_EARLY_LOGE(TAG, "SPI flash read result %d\n", ret);
    }

    if (3 > fhdr.spi_speed)
        freqdiv = fhdr.spi_speed + 2;
    else if (0x0F == fhdr.spi_speed)
        freqdiv = 1;
    else
        freqdiv = 2;

    if (fhdr.spi_size < flash_map_table_size) {
        flash_size = flash_map_table[fhdr.spi_size];
        ESP_EARLY_LOGD(TAG, "SPI flash size is %d\n", flash_size);
    } else {
        flash_size = 0; 
        ESP_EARLY_LOGE(TAG, "SPI size error is %d\n", fhdr.spi_size);
    }
    flashchip.chip_size = flash_size;

    if (1 >= freqdiv) {
        freqbits = SPI_FLASH_CLK_EQU_SYSCLK;
        SET_PERI_REG_MASK(PERIPHS_SPI_FLASH_CTRL, SPI_FLASH_CLK_EQU_SYSCLK);
        SET_PERI_REG_MASK(PERIPHS_IO_MUX_CONF_U, SPI0_CLK_EQU_SYSCLK);
    } else {
        freqbits = ((freqdiv - 1) << 8) + ((freqdiv / 2 - 1) << 4) + (freqdiv - 1);
        CLEAR_PERI_REG_MASK(PERIPHS_SPI_FLASH_CTRL, SPI_FLASH_CLK_EQU_SYSCLK);
        CLEAR_PERI_REG_MASK(PERIPHS_IO_MUX_CONF_U, SPI0_CLK_EQU_SYSCLK);
    }
    SET_PERI_REG_BITS(PERIPHS_SPI_FLASH_CTRL, 0xfff, freqbits, 0);

    if (fhdr.spi_mode == ESP_IMAGE_SPI_MODE_QIO) {
        ESP_EARLY_LOGD(TAG, "SPI flash enable QIO mode\n");
        user_spi_flash_dio_to_qio_pre_init();
    }
}

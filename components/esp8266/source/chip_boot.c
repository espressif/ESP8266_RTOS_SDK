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

#define PERIPHS_SPI_FLASH_USRREG        (0x60000200 + 0x1c)
#define PERIPHS_SPI_FLASH_CTRL          (0x60000200 + 0x08)
#define PERIPHS_IO_MUX_CONF_U           (0x60000800)

#define SPI0_CLK_EQU_SYSCLK             BIT8
#define SPI_FLASH_CLK_EQU_SYSCLK        BIT12

typedef struct flash_hdr {
    uint8_t         magic;
    uint8_t         blocks;
    uint8_t         spi_mode;
    uint8_t         spi_speed : 4;
    uint8_t         spi_size_map : 4;
    uint32_t        entry_addr;
} flash_hdr_t;

typedef struct boot_hdr {
    uint8_t     user_bin : 2;
    uint8_t     boot_status : 1;
    uint8_t     to_qio : 1;
    uint8_t     reserve : 4;

    uint8_t     version : 5;
    uint8_t     test_pass_flag : 1;
    uint8_t     test_start_flag : 1;
    uint8_t     enhance_boot_flag : 1;

    uint8_t     test_bin_addr[3];
    uint8_t     user_bin_addr[3];
} boot_hdr_t;

extern int ets_printf(const char *fmt, ...);

static const char *TAG = "chip_boot";

/*
 * @brief initialize the chip including flash I/O and chip cache according to
 *        boot parameters which are stored at the flash
 */
void chip_boot(size_t start_addr, size_t map)
{
    int ret;
    uint32_t freqdiv, flash_size, sect_size;
    uint32_t freqbits;
    flash_hdr_t fhdr;
    boot_hdr_t bhdr;

    uint32_t flash_map_table[FALSH_SIZE_MAP_MAX] = {
        1 * 1024 * 1024,
        2 * 1024 * 1024,
        4 * 1024 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024
    };
    uint32_t flash_map_table_size = sizeof(flash_map_table) / sizeof(flash_map_table[0]);

    extern void phy_get_bb_evm(void);
    extern void cache_init(uint32_t , uint32_t, uint32_t);
    extern void user_spi_flash_dio_to_qio_pre_init(void);
    extern int esp_get_boot_param(uint32_t, uint32_t, void *, uint32_t);

    phy_get_bb_evm();

    SET_PERI_REG_MASK(PERIPHS_SPI_FLASH_USRREG, BIT5);

    ret = spi_flash_read(start_addr, &fhdr, sizeof(flash_hdr_t));
    if (ret) {
        ESP_EARLY_LOGE(TAG, "SPI flash read result %d\n", ret);
    }

    if (3 > fhdr.spi_speed)
        freqdiv = fhdr.spi_speed + 2;
    else if (0x0F == fhdr.spi_speed)
        freqdiv = 1;
    else
        freqdiv = 2;

    if (fhdr.spi_size_map < flash_map_table_size) {
        flash_size = flash_map_table[fhdr.spi_size_map];
    } else {
        flash_size = 0; 
        ESP_EARLY_LOGE(TAG, "SPI size error is %d\n", fhdr.spi_size_map);
    }
    sect_size = 4 * 1024;

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

    ret = esp_get_boot_param(flash_size, sect_size, &bhdr, sizeof(boot_hdr_t));
    if (ret) {
        ESP_EARLY_LOGE(TAG, "Get boot parameters %d\n", ret);
    }

    cache_init(map, 0, 0);

    if (bhdr.to_qio == 0)
        user_spi_flash_dio_to_qio_pre_init();
}

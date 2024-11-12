// Copyright 2024-2026 Espressif Systems (Shanghai) PTE LTD
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

#ifndef _SPI_FLASH_PATCH_H
#define _SPI_FLASH_PATCH_H

#include <stdint.h>
#include <stddef.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FLASH_PATCH_TEXT_ATTR   __attribute__((section(".flash.patch.text")))
#define FLASH_PATCH_RODATA_ATTR __attribute__((section(".flash.patch.rodata")))
#define FLASH_PATCH_BSS_ATTR __attribute__((section(".flash.patch.bss")))

#define FLASH_PATCH_STR(str) (__extension__({static const FLASH_PATCH_RODATA_ATTR char __c[] = (str); (const char *)&__c;}))

#if CONFIG_ENABLE_SPI_FLASH_PATCH_DEBUG
typedef int (*__ets_printf_t)(const char *fmt, ...); 
#define ROM_PRINTF(_fmt, ...)   ((__ets_printf_t)(0x400024cc))(_fmt, ##__VA_ARGS__)
#else
#define ROM_PRINTF(_fmt, ...)
#endif

typedef struct spi_state {
    uint32_t io_mux_reg;
    uint32_t spi_clk_reg;
    uint32_t spi_ctrl_reg;
    uint32_t spi_user_reg;
} spi_state_t;

void spi_enter(spi_state_t *state);
void spi_exit(spi_state_t *state);

void spi_trans(bool write_mode, uint32_t cmd, uint32_t cmd_bits, uint32_t addr, uint32_t addr_bits, uint8_t *data,
                    uint32_t data_bytes, uint32_t dummy_bits);
void patch_delay(int ms);

#ifdef __cplusplus
}
#endif

#endif /* _SPI_FLASH_H */

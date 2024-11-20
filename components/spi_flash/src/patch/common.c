// Copyright 2023 Espressif Systems (Shanghai) PTE LTD
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

#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/errno.h>

#include "esp_log.h"
#include "esp_attr.h"
#include "spi_flash.h"
#include "priv/esp_spi_flash_raw.h"
#include "FreeRTOS.h"

#include "esp8266/eagle_soc.h"
#include "esp8266/pin_mux_register.h"
#include "esp8266/spi_register.h"
#include "esp8266/spi_struct.h"

#include "driver/gpio.h"
#include "esp_attr.h"

#include "spi_flash_patch.h"

#define SPI_FLASH       SPI0
#define SPI_BLOCK_SIZE  32
#define ADDR_SHIFT_BITS 8

extern void Cache_Read_Disable_2(void);
extern void Cache_Read_Enable_2();
extern void vPortEnterCritical(void);
extern void vPortExitCritical(void);

void FLASH_PATCH_TEXT_ATTR patch_delay(int ms)
{
    for (volatile int i = 0; i < ms; i++) {
        for (volatile int j = 0; j < 7800; j++) {
        }
    }
}

void FLASH_PATCH_TEXT_ATTR spi_enter(spi_state_t *state)
{
    vPortEnterCritical();
    Cache_Read_Disable_2(); 

    Wait_SPI_Idle(&g_rom_flashchip);

    state->io_mux_reg = READ_PERI_REG(PERIPHS_IO_MUX_CONF_U);
    state->spi_clk_reg = SPI_FLASH.clock.val;
    state->spi_ctrl_reg = SPI_FLASH.ctrl.val;
    state->spi_user_reg = SPI_FLASH.user.val;

    SPI_FLASH.user.usr_command = 1;
    SPI_FLASH.user.flash_mode = 0;
    SPI_FLASH.user.usr_miso_highpart = 0;

    CLEAR_PERI_REG_MASK(PERIPHS_IO_MUX_CONF_U, SPI0_CLK_EQU_SYS_CLK);

    SPI_FLASH.user.cs_setup = 1;
    SPI_FLASH.user.cs_hold = 1;
    SPI_FLASH.user.usr_mosi = 1;

    SPI_FLASH.user.usr_command = 1;
    SPI_FLASH.user.flash_mode = 0;

    SPI_FLASH.ctrl.fread_qio = 0;
    SPI_FLASH.ctrl.fread_dio = 0;
    SPI_FLASH.ctrl.fread_quad = 0;
    SPI_FLASH.ctrl.fread_dual = 0;

    SPI_FLASH.clock.val = 0;
    SPI_FLASH.clock.clkcnt_l = 3;
    SPI_FLASH.clock.clkcnt_h = 1;
    SPI_FLASH.clock.clkcnt_n = 3;

    SPI_FLASH.ctrl.fastrd_mode = 1;

    while (SPI_FLASH.cmd.usr) {
        ;
    }
}

void FLASH_PATCH_TEXT_ATTR spi_exit(spi_state_t *state)
{
    while (SPI_FLASH.cmd.usr) {
        ;
    }

    WRITE_PERI_REG(PERIPHS_IO_MUX_CONF_U, state->io_mux_reg);

    SPI_FLASH.ctrl.val = state->spi_ctrl_reg;
    SPI_FLASH.clock.val = state->spi_clk_reg;
    SPI_FLASH.user.val = state->spi_user_reg;

    Cache_Read_Enable_2();
    vPortExitCritical();
    patch_delay(1);
}

static void FLASH_PATCH_TEXT_ATTR spi_trans_block(bool write_mode,
                            uint32_t cmd,
                            uint32_t cmd_bits,
                            uint32_t addr,
                            uint32_t addr_bits,
                            uint8_t *data,
                            uint32_t data_bytes,
                            uint32_t dummy_bits)
{
    if ((uint32_t)data & 0x3) {
        ROM_PRINTF(FLASH_PATCH_STR("ERROR: data=%p\n"), data);
        return;
    }

    if (cmd_bits) {
        SPI_FLASH.user.usr_command = 1;
        SPI_FLASH.user2.usr_command_value = cmd;
        SPI_FLASH.user2.usr_command_bitlen = cmd_bits - 1;
    } else {
        SPI_FLASH.user.usr_command = 0;
        SPI_FLASH.user2.usr_command_bitlen = 0;
    }

    if (addr_bits) {
        SPI_FLASH.user.usr_addr = 1;
        SPI_FLASH.addr = addr << ADDR_SHIFT_BITS;
        SPI_FLASH.user1.usr_addr_bitlen = addr_bits - 1;
    } else {
        SPI_FLASH.user.usr_addr = 0;
        SPI_FLASH.user1.usr_addr_bitlen = 0;
    }

    if (dummy_bits) {
        SPI_FLASH.user.usr_dummy = 1;
        SPI_FLASH.user1.usr_dummy_cyclelen = dummy_bits - 1;
    } else {
        SPI_FLASH.user.usr_dummy = 0;
        SPI_FLASH.user1.usr_dummy_cyclelen = 0;
    }

    if (data_bytes) {
        if (write_mode) {
            int words = (data_bytes + 3) / 4;
            uint32_t *p = (uint32_t *)data;

            SPI_FLASH.user.usr_mosi = 1;
            SPI_FLASH.user.usr_miso = 0;
            SPI_FLASH.user1.usr_mosi_bitlen = data_bytes * 8 - 1;
            SPI_FLASH.user1.usr_miso_bitlen = 0;
            for (int i = 0; i < words; i++) {
                SPI_FLASH.data_buf[i] = p[i];
            }
        } else {
            int words = (data_bytes + 3) / 4;

            SPI_FLASH.user.usr_mosi = 0;
            SPI_FLASH.user.usr_miso = 1;
            SPI_FLASH.user1.usr_miso_bitlen = data_bytes * 8 - 1;
            SPI_FLASH.user1.usr_mosi_bitlen = 0;

            for (int i = 0; i < words; i++) {
                SPI_FLASH.data_buf[i] = 0;
            }
        }
    } else {
        SPI_FLASH.user.usr_mosi = 0;
        SPI_FLASH.user1.usr_mosi_bitlen = 0;
        SPI_FLASH.user.usr_miso = 0;
        SPI_FLASH.user1.usr_miso_bitlen = 0;
    }

    SPI_FLASH.cmd.usr = 1;
    while (SPI_FLASH.cmd.usr) {
        ;
    }
    
    if (!write_mode && data_bytes) {
        int words = (data_bytes + 3) / 4;
        uint32_t *p = (uint32_t *)data;

        for (int i = 0; i < words; i++) {
            p[i] = SPI_FLASH.data_buf[i];
        }
    }
}

void FLASH_PATCH_TEXT_ATTR spi_trans(bool write_mode,
                      uint32_t cmd,
                      uint32_t cmd_bits,
                      uint32_t addr,
                      uint32_t addr_bits,
                      uint8_t *data,
                      uint32_t data_bytes,
                      uint32_t dummy_bits)

{
    if (!data_bytes || data_bytes <= SPI_BLOCK_SIZE) {
        return spi_trans_block(write_mode, cmd, cmd_bits, addr,
                               addr_bits, data, data_bytes, dummy_bits);
    }

    for (int i = 0; i < data_bytes; i += SPI_BLOCK_SIZE) {
        uint32_t n = MIN(SPI_BLOCK_SIZE, data_bytes - i);

        spi_trans_block(write_mode, cmd, cmd_bits, addr + i,
                        addr_bits, data + i, n, dummy_bits);
    }
}

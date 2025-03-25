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

#include "spi_flash_patch.h"

#define SPI_BLOCK_SIZE  32

void spi_trans(bool write_mode, uint32_t cmd, uint32_t cmd_bits, uint32_t addr, uint32_t addr_bits, uint8_t *data,
                    uint32_t data_bytes, uint32_t dummy_bits);
void patch_delay(int ms);

#define TOCHAR(_v)              #_v
#define PRINT_STEP(_s)          ROM_PRINTF(FLASH_PATCH_STR("Step %d\n"), (_s));
#define JUMP_TO_STEP(_s)        { ROM_PRINTF(FLASH_PATCH_STR("%d line Jump to " TOCHAR(_s) "\n"), __LINE__); goto _s; }
#define GOTO_FAILED(_s)         { ROM_PRINTF(FLASH_PATCH_STR("ERROR: " TOCHAR(_s) " failed\n")); ret = -EIO; JUMP_TO_STEP(step17); }

#define write_u8_dummy(_c, _a, _d8,_d)          {uint32_t __data = _d8; spi_trans(1, (_c), 8, (_a), 24, (uint8_t *)&__data, 1, (_d));}
#define write_u8(_c, _a, _d8)                   write_u8_dummy((_c), (_a), (_d8), 0)

extern uint32_t spi_flash_get_id(void);

static uint8_t FLASH_PATCH_BSS_ATTR buffer1024[1024];

static void FLASH_PATCH_TEXT_ATTR write_cmd(uint32_t cmd)
{
    spi_trans(1, cmd, 8, 0, 0, NULL, 0, 0);
}

static void FLASH_PATCH_TEXT_ATTR write_buffer(uint32_t addr, uint8_t *buffer, int size)
{
    for (int i = 0; i < size; i += SPI_BLOCK_SIZE) {
        int n = MIN(size - i, SPI_BLOCK_SIZE);

        write_cmd(0x6);
        spi_trans(1, 0x42, 8, addr + i, 24, buffer + i, n, 0);
        patch_delay(3);
    }
}

static void FLASH_PATCH_TEXT_ATTR read_buffer(uint32_t addr, uint8_t *buffer, int n)
{
    spi_trans(0, 0x48, 8, addr, 24, buffer, n, 8);
}

static void FLASH_PATCH_TEXT_ATTR erase_sector(uint32_t addr)
{
    write_cmd(0x6);
    spi_trans(1, 0x44, 8, addr, 24, NULL, 0, 0);
    patch_delay(8);
}

int FLASH_PATCH_TEXT_ATTR th25q16hb_apply_patch_0(void)
{
    int ret = 0;
    uint32_t flash_id;
    int count;
    spi_state_t state;
    uint8_t *buffer256_0;
    uint8_t *buffer256_1;
    uint8_t *buffer256_2;

    flash_id = spi_flash_get_id();
    if (flash_id != 0x1560eb) {
        uint32_t data = 0;
        bool is_th25q16hb = false;
        if (flash_id == 0x0) {
            spi_trans(0, 0x5A, 8, 0x10, 24, &data, 1, 0);
            if (data == 0xEB) {
                spi_trans(0, 0x5A, 8, 0x14, 24, &data, 1, 0);
                if (data == 0x60) {
                    spi_trans(0, 0x5A, 8, 0x34, 24, &data, 4, 0);
                    if (data == 0xFFFFFF00) {
                        is_th25q16hb = true;
                    }
                }
            }
        }

        if (!is_th25q16hb) {
            ROM_PRINTF(FLASH_PATCH_STR("WARN: id=0x%x, is not TH25Q16HB\n"), flash_id);
            return 0;
        }
    }

    buffer256_0 = buffer1024;
    buffer256_1 = buffer1024 + 256;
    buffer256_2 = buffer1024 + 512;

    spi_enter(&state);

    // Step 1
    PRINT_STEP(1);
    write_cmd(0x33);

    // Step 2
    PRINT_STEP(2);
    write_cmd(0xcc);
    
    // Step 3
    PRINT_STEP(3);
    write_cmd(0xaa);

    // Step 4
    PRINT_STEP(4);
    write_u8(0xfa, 0x1200d, 0x3);
    write_u8_dummy(0xfa, 0x2200d, 0x3, 1);

    // Step 5-1
    PRINT_STEP(5);
    read_buffer(0xbed, buffer256_0, 3);
    if (buffer256_0[0] == 0xff &&
        buffer256_0[1] == 0xff &&
        buffer256_0[2] == 0xff) {
        ROM_PRINTF(FLASH_PATCH_STR("INFO: check done 0\n"));
    } else if (buffer256_0[0] == 0x55 &&
               buffer256_0[1] == 0xff &&
               buffer256_0[2] == 0xff) {
        JUMP_TO_STEP(step10);
    } else if (buffer256_0[0] == 0x55 &&
               buffer256_0[1] == 0x55 &&
               buffer256_0[2] == 0xff) {
        JUMP_TO_STEP(step14);
    } else if (buffer256_0[0] == 0x55 &&
               buffer256_0[1] == 0x55 &&
               buffer256_0[2] == 0x55) {
        JUMP_TO_STEP(step17);
    } else {
        ROM_PRINTF(FLASH_PATCH_STR("ERROR: 0xbed=0x%x 0xbee=0x%x 0xbef=0x%x\n"),
                    buffer256_0[0], buffer256_0[1], buffer256_0[2]);
        GOTO_FAILED(5-1);
    }
JUMP_TO_STEP(step17);
    // Step 5-2
    read_buffer(0x50d, buffer256_0, 1);
    buffer256_0[0] &= 0x7f;
    if (buffer256_0[0] == 0x7c) {
        JUMP_TO_STEP(step17);
    } else if (buffer256_0[0] == 0x3c) {
        ROM_PRINTF(FLASH_PATCH_STR("INFO: check done 1\n"));
    } else {
        ROM_PRINTF(FLASH_PATCH_STR("ERROR: 0x50d=0x%x\n"), buffer256_0[0]);
        GOTO_FAILED(5-2);
    }

    // Step 6
    PRINT_STEP(6);
    for (count = 0; count < 3; count++) {
        erase_sector(0);

        bool check_done = true;
        read_buffer(0x0, buffer1024, 1024);
        for (int i = 0; i < 1024; i++) {
            if (buffer1024[i] != 0xff) {
                check_done = false;
                ROM_PRINTF(FLASH_PATCH_STR("ERROR: buffer1024[%d]=0x%x\n"), i, buffer1024[i]);
                break;
            }
        }

        if (check_done) {
            break;
        }
    }
    if (count >= 3) {
        GOTO_FAILED(6)
    }

    // Step 7-1.1
    PRINT_STEP(7);
    read_buffer(0x400, buffer256_0, 256);
    read_buffer(0x400, buffer256_1, 256);
    read_buffer(0x400, buffer256_2, 256);
    if (memcmp(buffer256_0, buffer256_1, 256) ||
        memcmp(buffer256_0, buffer256_2, 256)) {
        GOTO_FAILED(7-1.1);
    }

    write_buffer(0, buffer256_0, 256);
    write_buffer(0x200, buffer256_0, 256);

    // Step 7-1.2
    for (count = 0; count < 3; count++) {
        read_buffer(0, buffer256_1, 256);
        if (memcmp(buffer256_0, buffer256_1, 256)) {
            break;
        }

        read_buffer(0x200, buffer256_1, 256);
        if (memcmp(buffer256_0, buffer256_1, 256)) {
            break;
        }
    }
    if (count < 3) {
        GOTO_FAILED(7-1.2);
    }

    // Step 7-2.1
    read_buffer(0x500, buffer256_0, 256);
    read_buffer(0x500, buffer256_1, 256);
    read_buffer(0x500, buffer256_2, 256);
    if (memcmp(buffer256_0, buffer256_1, 256) ||
        memcmp(buffer256_0, buffer256_2, 256)) {
        GOTO_FAILED(7-2.1);
    }
    write_buffer(0x100, buffer256_0, 256);
    write_buffer(0x300, buffer256_0, 256);

    // Step 7-2.2
    for (count = 0; count < 3; count++) {
        read_buffer(0x100, buffer256_1, 256);
        if (memcmp(buffer256_0, buffer256_1, 256)) {
            break;
        }
        read_buffer(0x300, buffer256_1, 256);
        if (memcmp(buffer256_0, buffer256_1, 256)) {
            break;
        }
    }
    if (count < 3) {
        GOTO_FAILED(7-2.2);
    }

    // Step 8
    PRINT_STEP(8);
    read_buffer(0x0, buffer256_0, 1);
    read_buffer(0x23, buffer256_1, 1);
    if (buffer256_0[0] != 0x13 || buffer256_1[0] != 0x14) {
        ROM_PRINTF(FLASH_PATCH_STR("ERROR: 0x0=0x%x 0x23=0x%x\n"), buffer256_0[0], buffer256_1[0]);
        GOTO_FAILED(8);
    }

    // Step 9-1
    PRINT_STEP(9);
    read_buffer(0x140, buffer256_0, 2);
    if (buffer256_0[0] != 0 || buffer256_0[1] != 0xff) {
        ROM_PRINTF(FLASH_PATCH_STR("ERROR: 0x140=0x%x 0x141=0x%x\n"), buffer256_0[0], buffer256_0[1]);
        GOTO_FAILED(9-1);
    }

    // Step 9-2
    buffer256_0[0] = 0x55;
    write_buffer(0xaed, buffer256_0, 1);
    write_buffer(0xbed, buffer256_0, 1);
    for (count = 0; count < 3; count++) {
        read_buffer(0xaed, buffer256_0, 1);
        if (buffer256_0[0] != 0x55) {
            break;
        }

        read_buffer(0xbed, buffer256_0, 1);
        if (buffer256_0[0] != 0x55) {
            break;
        }
    }
    if (count < 3) {
        GOTO_FAILED(9-2);
    }

step10:
    // Step 10-1
    PRINT_STEP(10);
    for (count = 0; count < 3; count++) {
        erase_sector(0x400);

        bool check_done = true;
        read_buffer(0x400, buffer1024, 1024);
        for (int i = 0; i < 1024; i++) {
            if (buffer1024[i] != 0xff) {
                check_done = false;
                ROM_PRINTF(FLASH_PATCH_STR("ERROR: buffer1024[%d]=0x%x\n"), i, buffer1024[i]);
                break;
            }
        }

        if (check_done) {
            break;
        }
    }
    if (count >= 3) {
        GOTO_FAILED(10-1);
    }

    // Step 10-3.1
    read_buffer(0, buffer256_0, 256);
    read_buffer(0, buffer256_1, 256);
    read_buffer(0, buffer256_2, 256);
    if (memcmp(buffer256_0, buffer256_1, 256) ||
        memcmp(buffer256_0, buffer256_2, 256)) {
        GOTO_FAILED(10-3.1);
    }
    write_buffer(0x400, buffer256_0, 256);
    write_buffer(0x600, buffer256_0, 256);

    // Step 10-3.2
    for (count = 0; count < 3; count++) {
        read_buffer(0x400, buffer256_1, 256);
        if (memcmp(buffer256_0, buffer256_1, 256)) {
            break;
        }

        read_buffer(0x600, buffer256_1, 256);
        if (memcmp(buffer256_0, buffer256_1, 256)) {
            break;
        }
    }
    if (count < 3) {
        GOTO_FAILED(10-3.2);
    }    

    // Step 10-3.3
    read_buffer(0x100, buffer256_0, 256);
    read_buffer(0x100, buffer256_1, 256);
    read_buffer(0x100, buffer256_2, 256);
    if (memcmp(buffer256_0, buffer256_1, 256) ||
        memcmp(buffer256_0, buffer256_2, 256)) {
        GOTO_FAILED(10-3.3);
    }
    buffer256_0[9] = 0x79;
    buffer256_0[13] = (buffer256_0[13] & 0x3f) |
                       ((~buffer256_0[13]) & 0x80) |
                       0x40;
    write_buffer(0x500, buffer256_0, 256);
    write_buffer(0x700, buffer256_0, 256);

    // Step 10-3.4
    for (count = 0; count < 3; count++) {
        read_buffer(0x500, buffer256_1, 256);
        if (memcmp(buffer256_0, buffer256_1, 256)) {
            break;
        }

        read_buffer(0x700, buffer256_1, 256);
        if (memcmp(buffer256_0, buffer256_1, 256)) {
            break;
        }
    }
    if (count < 3) {
        GOTO_FAILED(10-3.4);
    } 

    // Step11-1
    PRINT_STEP(11);

    for (count = 0; count < 3; count++) {
        read_buffer(0x400, buffer256_0, 1);
        read_buffer(0x423, buffer256_1, 1);
        if (buffer256_0[0] != 0x13 || buffer256_1[0] != 0x14) {
            ROM_PRINTF(FLASH_PATCH_STR("ERROR: 0x400=0x%x 0x423=0x%x\n"), buffer256_0[0], buffer256_1[0]);
            break;
        }
    }
    if (count < 3) {
        GOTO_FAILED(11-1);
    }

    // Step11-2.1
    for (count = 0; count < 3; count++) {
        read_buffer(0x540, buffer256_0, 2);
        if (buffer256_0[0] != 0 || buffer256_0[1] != 0xff) {
            ROM_PRINTF(FLASH_PATCH_STR("ERROR: 0x540=0x%x 0x541=0x%x\n"), buffer256_0[0], buffer256_0[1]);
            break;
        }
    }
    if (count < 3) {
        GOTO_FAILED(11-2);
    }

    // Step11-2.2
    for (count = 0; count < 3; count++) {
        read_buffer(0x50d, buffer256_0, 1);
        buffer256_0[0] &= 0x7f;
        if (buffer256_0[0] != 0x7c) {
            ROM_PRINTF(FLASH_PATCH_STR("ERROR: 0x50d=0x%x\n"), buffer256_0[0]);
            break;
        }
    }
    if (count < 3) {
        GOTO_FAILED(11-2);
    }

    // Step 12
    PRINT_STEP(12);
    buffer256_0[0] = 0x55;
    write_buffer(0xaee, buffer256_0, 1);
    write_buffer(0xbee, buffer256_0, 1);

    // Step 13
    PRINT_STEP(13);
    for (count = 0; count < 3; count++) {
        read_buffer(0xaee, buffer256_0, 1);
        if (buffer256_0[0] != 0x55) {
            break;
        }

        read_buffer(0xbee, buffer256_0, 1);
        if (buffer256_0[0] != 0x55) {
            break;
        }
    }
    if (count < 3) {
        GOTO_FAILED(13);
    }

step14:
    // Step 14
    PRINT_STEP(14);
    for (count = 0; count < 3; count++) {
        erase_sector(0);

        bool check_done = true;
        read_buffer(0x0, buffer1024, 1024);
        for (int i = 0; i < 1024; i++) {
            if (buffer1024[i] != 0xff) {
                check_done = false;
                ROM_PRINTF(FLASH_PATCH_STR("ERROR: buffer1024[%d]=0x%x\n"), i, buffer1024[i]);
                break;
            }
        }

        if (check_done) {
            break;
        }
    }
    if (count >= 3) {
        GOTO_FAILED(14);
    }    

    // Step 15
    PRINT_STEP(15);
    buffer256_0[0] = 0x55;
    write_buffer(0xaef, buffer256_0, 1);
    write_buffer(0xbef, buffer256_0, 1);

    // Step 16
    PRINT_STEP(16);
    for (count = 0; count < 3; count++) {
        read_buffer(0xaef, buffer256_0, 1);
        if (buffer256_0[0] != 0x55) {
            break;
        }

        read_buffer(0xbef, buffer256_0, 1);
        if (buffer256_0[0] != 0x55) {
            break;
        }
    }
    if (count < 3) {
        GOTO_FAILED(16);
    }

step17:
    // Step 17
    PRINT_STEP(17);
    write_cmd(0x55);

    // Step 18
    PRINT_STEP(18);
    write_cmd(0x88);

    spi_exit(&state);

    if (!ret) {
        ROM_PRINTF(FLASH_PATCH_STR("INFO: Patch for TH25Q16HB is done\n"));
    }

    return ret;
}

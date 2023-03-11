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

#define SPI_FLASH       SPI0
#define SPI_BLOCK_SIZE  32
#define ADDR_SHIFT_BITS 8

#if 0
typedef int (*__ets_printf_t)(const char *fmt, ...); 
#define ROM_PRINTF(_fmt, ...)   ((__ets_printf_t)(0x400024cc))(_fmt, ##__VA_ARGS__)
#else
#define ROM_PRINTF(_fmt, ...)
#endif

#define TOCHAR(_v)              #_v
#define PRINT_STEP(_s)          ROM_PRINTF("Step %d\n", (_s));
#define JUMP_TO_STEP(_s)        { ROM_PRINTF("Jump to " TOCHAR(_s) "\n"); goto _s; }
#define GOTO_FAILED(_s)         { ROM_PRINTF("ERROR: " TOCHAR(_s) " failed\n"); ret = -EIO; JUMP_TO_STEP(step17); }

#define write_u8_dummy(_c, _a, _d8,_d)          {uint32_t __data = _d8; spi_trans(1, (_c), 8, (_a), 24, (uint8_t *)&__data, 1, (_d));}
#define write_u8(_c, _a, _d8)                   write_u8_dummy((_c), (_a), (_d8), 0)

extern void Cache_Read_Disable_2(void);
extern void Cache_Read_Enable_2();
extern void vPortEnterCritical(void);
extern void vPortExitCritical(void);
extern uint32_t spi_flash_get_id(void);

static void delay(int ms)
{
    for (volatile int i = 0; i < ms; i++) {
        for (volatile int j = 0; j < 7800; j++) {
        }
    }
}

#if 0
static void dump_hex(const uint8_t *ptr, int n)
{
  const uint8_t *s1 = ptr;
  const int line_bytes = 16;

  ROM_PRINTF("\nHex:\n");
  for (int i = 0; i < n ; i += line_bytes)
    {
       int m = MIN(n - i, line_bytes);

      ROM_PRINTF("\t");
      for (int j = 0; j < m; j++)
        {
          ROM_PRINTF("%02x ", s1[i + j]);
        }
      
      ROM_PRINTF("\n");
    }

  ROM_PRINTF("\n");
}

static void dump_hex_compare(const uint8_t *s1, const uint8_t *s2, int n)
{
  const int line_bytes = 16;

  ROM_PRINTF("\nHex:\n");
  for (int i = 0; i < n ; i += line_bytes)
    {
       int m = MIN(n - i, line_bytes);

      ROM_PRINTF("\t");
      for (int j = 0; j < m; j++)
        {
          ROM_PRINTF("%02x:%02x ", s1[i + j], s2[i + j]);
        }
      
      ROM_PRINTF("\n");
    }

  ROM_PRINTF("\n");
}
#endif

typedef struct spi_state {
    uint32_t io_mux_reg;
    uint32_t spi_clk_reg;
    uint32_t spi_ctrl_reg;
    uint32_t spi_user_reg;
} spi_state_t;

static void spi_enter(spi_state_t *state)
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

static void spi_exit(spi_state_t *state)
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
}

static void spi_trans_block(bool write_mode,
                            uint32_t cmd,
                            uint32_t cmd_bits,
                            uint32_t addr,
                            uint32_t addr_bits,
                            uint8_t *data,
                            uint32_t data_bytes,
                            uint32_t dummy_bits)
{
    if ((uint32_t)data & 0x3) {
        ROM_PRINTF("ERROR: data=%p\n", data);
        return;
    }

    if (cmd_bits) {
        SPI_FLASH.user.usr_command = 1;
        SPI_FLASH.user2.usr_command_value = cmd;
        SPI_FLASH.user2.usr_command_bitlen = cmd_bits - 1;
    } else {
        SPI_FLASH.user.usr_command = 0;
    }

    if (addr_bits) {
        SPI_FLASH.user.usr_addr = 1;
        SPI_FLASH.addr = addr << ADDR_SHIFT_BITS;
        SPI_FLASH.user1.usr_addr_bitlen = addr_bits - 1;
    } else {
        SPI_FLASH.user.usr_addr = 0;
    }

    if (dummy_bits) {
        SPI_FLASH.user.usr_dummy = 1;
        SPI_FLASH.user1.usr_dummy_cyclelen = dummy_bits - 1;
    } else {
        SPI_FLASH.user.usr_dummy = 0;
    }

    if (write_mode && data_bytes) {
        int words = (data_bytes + 3) / 4;
        uint32_t *p = (uint32_t *)data;

        SPI_FLASH.user.usr_mosi = 1;
        SPI_FLASH.user.usr_miso = 0;
        SPI_FLASH.user1.usr_mosi_bitlen = data_bytes * 8 - 1;

        for (int i = 0; i < words; i++) {
            SPI_FLASH.data_buf[i] = p[i];
        }
    } else if (!write_mode && data_bytes) {
        int words = (data_bytes + 3) / 4;

        SPI_FLASH.user.usr_mosi = 0;
        SPI_FLASH.user.usr_miso = 1;
        SPI_FLASH.user1.usr_miso_bitlen = data_bytes * 8 - 1;

        for (int i = 0; i < words; i++) {
            SPI_FLASH.data_buf[i] = 0;
        }
    } else {
        SPI_FLASH.user.usr_mosi = 0;
        SPI_FLASH.user.usr_miso = 0;
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

static void spi_trans(bool write_mode,
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

static void write_cmd(uint32_t cmd)
{
    spi_trans(1, cmd, 8, 0, 0, NULL, 0, 0);
}

static void write_buffer(uint32_t addr, uint8_t *buffer, int size)
{
    for (int i = 0; i < size; i += SPI_BLOCK_SIZE) {
        int n = MIN(size - i, SPI_BLOCK_SIZE);

        write_cmd(0x6);
        spi_trans(1, 0x42, 8, addr + i, 24, buffer + i, n, 0);
        delay(3);
    }
}

static void read_buffer(uint32_t addr, uint8_t *buffer, int n)
{
    spi_trans(0, 0x48, 8, addr, 24, buffer, n, 8);
}

static void erase_sector(uint32_t addr)
{
    write_cmd(0x6);
    spi_trans(1, 0x44, 8, addr, 24, NULL, 0, 0);
    delay(8);
}

int th25q16hb_apply_patch_0(void)
{
    int ret = 0;
    uint32_t flash_id;
    int count;
    spi_state_t state;
    uint8_t *buffer256_0;
    uint8_t *buffer256_1;
    uint8_t *buffer256_2;
    uint8_t *buffer1024;

    flash_id = spi_flash_get_id();
    if (flash_id != 0x1560eb) {
        ROM_PRINTF("WARN: id=0x%x, is not TH25Q16HB\n", flash_id);
        return 0;
    }

    buffer1024 = heap_caps_malloc(1024, MALLOC_CAP_8BIT);
    if (!buffer1024) {
        return -ENOMEM;
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
        ROM_PRINTF("INFO: check done 0\n");
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
        ROM_PRINTF("ERROR: 0xbed=0x%x 0xbee=0x%x 0xbef=0x%x\n",
                    buffer256_0[0], buffer256_0[1], buffer256_0[2]);
        GOTO_FAILED(5-1);
    }

    // Step 5-2
    read_buffer(0x50d, buffer256_0, 1);
    buffer256_0[0] &= 0x7f;
    if (buffer256_0[0] == 0x7c) {
        JUMP_TO_STEP(step17);
    } else if (buffer256_0[0] == 0x3c) {
        ROM_PRINTF("INFO: check done 1\n");
    } else {
        ROM_PRINTF("ERROR: 0x50d=0x%x\n", buffer256_0[0]);
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
                ROM_PRINTF("ERROR: buffer1024[%d]=0x%x\n", i, buffer1024[i]);
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
        ROM_PRINTF("ERROR: 0x0=0x%x 0x23=0x%x\n", buffer256_0[0], buffer256_1[0]);
        GOTO_FAILED(8);
    }

    // Step 9-1
    PRINT_STEP(9);
    read_buffer(0x140, buffer256_0, 2);
    if (buffer256_0[0] != 0 || buffer256_0[1] != 0xff) {
        ROM_PRINTF("ERROR: 0x140=0x%x 0x141=0x%x\n", buffer256_0[0], buffer256_0[1]);
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
                ROM_PRINTF("ERROR: buffer1024[%d]=0x%x\n", i, buffer1024[i]);
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
            ROM_PRINTF("ERROR: 0x400=0x%x 0x423=0x%x\n", buffer256_0[0], buffer256_1[0]);
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
            ROM_PRINTF("ERROR: 0x540=0x%x 0x541=0x%x\n", buffer256_0[0], buffer256_0[1]);
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
            ROM_PRINTF("ERROR: 0x50d=0x%x\n", buffer256_0[0]);
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
                ROM_PRINTF("ERROR: buffer1024[%d]=0x%x\n", i, buffer1024[i]);
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

    heap_caps_free(buffer1024);

    if (!ret) {
        ROM_PRINTF("INFO: Patch for TH25Q16HB is done\n");
    }

    return ret;
}

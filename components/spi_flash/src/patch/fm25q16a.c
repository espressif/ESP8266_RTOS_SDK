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
#include "spi_flash_patch.h"

#define DEBUG(fmt,...)  fm_printf(fmt, ##__VA_ARGS__)
#define INFO(fmt,...)   fm_printf(fmt, ##__VA_ARGS__)
#define ERROR(fmt,...)  fm_printf(fmt, ##__VA_ARGS__)

#ifndef IRAM_FUNC_ATTR
#define IRAM_FUNC_ATTR 
#endif

#define fm_printf      ROM_PRINTF

extern bool IRAM_FUNC_ATTR spi_user_cmd(spi_cmd_dir_t mode, spi_cmd_t *p_cmd);
extern uint32_t IRAM_FUNC_ATTR spi_flash_get_id(void);

static bool FLASH_PATCH_TEXT_ATTR double_check_fm25q16a(void);

static void FLASH_PATCH_TEXT_ATTR fm_send_spi_cmd(uint8_t cmd, uint8_t cmd_len, uint32_t addr, uint8_t addr_len, const void* mosi_data, int mosi_len, void* miso_data, int miso_len, uint8_t dummy_bits)
{
    bool write_mode = false;
    uint32_t data_bytes = 0;
    
    if (mosi_len > 0) {
        write_mode = true;
        data_bytes = mosi_len / 8;
    } else if (miso_len > 0) {
        write_mode = false;
        data_bytes = miso_len / 8;
    }
    uint32_t data[(data_bytes+3)/4];
    if (write_mode && mosi_data) {
        memcpy(data, mosi_data, data_bytes);
    }

    spi_trans(write_mode, cmd, cmd_len, addr, addr_len, (uint8_t *)data, data_bytes, dummy_bits);

    if (!write_mode && miso_data) {
        memcpy(miso_data, data, data_bytes);
    }
}

static void FLASH_PATCH_TEXT_ATTR fm_cam_cmd_start()
{
    fm_send_spi_cmd(0x66, 1*8, 0, 0, NULL, 0, NULL, 0, 0);
    fm_send_spi_cmd(0x3C, 1*8, 0, 0, NULL, 0, NULL, 0, 0);
    fm_send_spi_cmd(0xC3, 1*8, 0, 0, NULL, 0, NULL, 0, 0);
}

static void FLASH_PATCH_TEXT_ATTR fm_cam_cmd_end()
{
    fm_send_spi_cmd(0xff, 1*8, 0, 0, NULL, 0, NULL, 0, 0);
}

static void FLASH_PATCH_TEXT_ATTR fm_cam_pre_cmd_generic(uint8_t (*send_list)[5], int lines)
{
    int i;
    fm_cam_cmd_start();
    for(i = 0; i < lines; i++) {
        fm_send_spi_cmd(0x0, 0, 0, 0, &send_list[i][0], 5*8, NULL, 0, 0);
    }
    
    fm_cam_cmd_end();
}

static const uint8_t FLASH_PATCH_RODATA_ATTR read_after_erase_pre_send_list[7][5] = {
    {0x32,0x00,0x03,0xc0,0x88},
    {0x32,0x00,0x00,0x80,0x01},
    {0x32,0x00,0x00,0x84,0x47},
    {0x32,0x00,0x00,0x88,0x47},
    {0x32,0x00,0x00,0x8c,0x04},
    {0x32,0x00,0x00,0x90,0x19},
    {0x32,0x00,0x00,0x94,0x03},
};

static void FLASH_PATCH_TEXT_ATTR fm_cam_read_after_erase_pre(void)
{
    // cmd NO.7 in the doc
    uint8_t send_list[7][5];

    memcpy(send_list, read_after_erase_pre_send_list, 7 * 5);

    fm_cam_pre_cmd_generic(send_list, sizeof(send_list) / sizeof(send_list[0]));
}

static const uint8_t FLASH_PATCH_RODATA_ATTR step_prog_pre_send_list[8][5] = {
    {0x32,0x00,0x03,0xc0,0x88},
    {0x32,0x00,0x00,0x64,0xb7},
    {0x32,0x00,0x00,0x80,0x13},
    {0x32,0x00,0x00,0x84,0x4f},
    {0x32,0x00,0x00,0x88,0x78},
    {0x32,0x00,0x00,0x8c,0x10},
    {0x32,0x00,0x00,0x90,0x40},
    {0x32,0x00,0x00,0x94,0xff},
};

static void FLASH_PATCH_TEXT_ATTR fm_cam_step_prog_pre(void)
{
    // cmd NO.6 in the doc
    uint8_t send_list[8][5];

    memcpy(send_list, step_prog_pre_send_list, 8 * 5);

    fm_cam_pre_cmd_generic(send_list, sizeof(send_list) / sizeof(send_list[0]));
}

static const uint8_t FLASH_PATCH_RODATA_ATTR step_erase_pre_send_list[8][5] = {
    {0x32,0x00,0x00,0x64,0x77},
    {0x32,0x00,0x03,0xc0,0x88},
    {0x32,0x00,0x00,0x80,0x01},
    {0x32,0x00,0x00,0x84,0x46},
    {0x32,0x00,0x00,0x88,0x7e},
    {0x32,0x00,0x00,0x8c,0x06},
    {0x32,0x00,0x00,0x90,0x31},
    {0x32,0x00,0x00,0x94,0x01},
};

static void FLASH_PATCH_TEXT_ATTR fm_cam_step_erase_pre(void)
{
    // cmd NO.5 in the doc
    uint8_t send_list[8][5];

    memcpy(send_list, step_erase_pre_send_list, 8 * 5);

    fm_cam_pre_cmd_generic(send_list, sizeof(send_list) / sizeof(send_list[0]));
}

static const uint8_t FLASH_PATCH_RODATA_ATTR preprog_pre_send_list[8][5] = {
    {0x32,0x00,0x03,0xc0,0x88},
    {0x32,0x00,0x00,0x64,0xf1},
    {0x32,0x00,0x00,0x80,0x53},
    {0x32,0x00,0x00,0x84,0x5c},
    {0x32,0x00,0x00,0x88,0x7c},
    {0x32,0x00,0x00,0x8c,0x04},
    {0x32,0x00,0x00,0x90,0x1f},
    {0x32,0x00,0x00,0x94,0xff},
};

static void FLASH_PATCH_TEXT_ATTR fm_cam_preprog_pre(void)
{
    // cmd NO.4 in the doc
    uint8_t send_list[8][5];

    memcpy(send_list, preprog_pre_send_list, 8 * 5);

    fm_cam_pre_cmd_generic(send_list, sizeof(send_list) / sizeof(send_list[0]));
}

static const uint8_t FLASH_PATCH_RODATA_ATTR prog_pre_send_list[7][5] = {
    {0x32,0x00,0x03,0xc0,0x88},
    {0x32,0x00,0x00,0x80,0x53},
    {0x32,0x00,0x00,0x84,0x7c},
    {0x32,0x00,0x00,0x88,0x7f},
    {0x32,0x00,0x00,0x8c,0x10},
    {0x32,0x00,0x00,0x90,0xff},
    {0x32,0x00,0x00,0x94,0xff},
};

static void FLASH_PATCH_TEXT_ATTR fm_cam_prog_pre(void)
{
    // cmd NO.3 in the doc
    uint8_t send_list[7][5];

    memcpy(send_list, prog_pre_send_list, 7 * 5);

    fm_cam_pre_cmd_generic(send_list, sizeof(send_list) / sizeof(send_list[0]));
}

static const uint8_t FLASH_PATCH_RODATA_ATTR uid_pre_send_list[7][5] = {
    {0x32,0x00,0x03,0xc0,0x48},
    {0x32,0x00,0x00,0x80,0x00},
    {0x32,0x00,0x00,0x84,0x47},
    {0x32,0x00,0x00,0x88,0x47},
    {0x32,0x00,0x00,0x8c,0x04},
    {0x32,0x00,0x00,0x90,0x19},
    {0x32,0x00,0x00,0x94,0x03},
};

static void FLASH_PATCH_TEXT_ATTR fm_cam_uid_pre(void)
{
    // cmd NO.2 in the doc
    uint8_t send_list[7][5];

    memcpy(send_list, uid_pre_send_list, 7 * 5);

    fm_cam_pre_cmd_generic(send_list, sizeof(send_list) / sizeof(send_list[0]));
}

static const uint8_t FLASH_PATCH_RODATA_ATTR read_pre_send_list[7][5] = {
    {0x32,0x00,0x03,0xc0,0x88},
    {0x32,0x00,0x00,0x80,0x00},
    {0x32,0x00,0x00,0x84,0x47},
    {0x32,0x00,0x00,0x88,0x47},
    {0x32,0x00,0x00,0x8c,0x04},
    {0x32,0x00,0x00,0x90,0x19},
    {0x32,0x00,0x00,0x94,0x03},
};

static void FLASH_PATCH_TEXT_ATTR fm_cam_read_pre(void)
{
    // cmd NO.1 in the doc
    uint8_t send_list[7][5];

    memcpy(send_list, read_pre_send_list, 7 * 5);

    // fm_cam_pre_cmd_generic(send_list, sizeof(send_list) / sizeof(send_list[0]));
    fm_cam_pre_cmd_generic(send_list, 7);
}

static bool FLASH_PATCH_TEXT_ATTR fm_flash_wait_idle()
{
    uint8_t status = 0x1;
    while ((status&0x1) == 0x1) {
        fm_send_spi_cmd(0x05, 1*8, 0, 0, NULL, 0, &status, 1*8, 0);
    }
    return true;
}

static void FLASH_PATCH_TEXT_ATTR fm_soft_reset()
{
    fm_send_spi_cmd(0x66, 1*8, 0, 0, NULL, 0, NULL, 0, 0);
    // patch_delay(1);
    fm_send_spi_cmd(0x99, 1*8, 0, 0, NULL, 0, NULL, 0, 0);
    fm_flash_wait_idle();
}

static const uint8_t FLASH_PATCH_RODATA_ATTR uid_cmd[4] = {0x00, 0x03, 0xc0, 0x40};
static const uint8_t FLASH_PATCH_RODATA_ATTR uid_data[4] = {0x00, 0x01, 0x00, 0x8F};

static bool FLASH_PATCH_TEXT_ATTR fm_set_uid_flag(void)
{
    fm_soft_reset();
    fm_cam_cmd_start();
    fm_send_spi_cmd(0x32, 8, 0, 0, uid_cmd, 4 * 8, NULL, 0, 0);
    fm_cam_cmd_end();
    fm_send_spi_cmd(0x06, 1*8, 0, 0, NULL, 0, NULL, 0, 0);
    fm_send_spi_cmd(0x02, 8, 0, 0, uid_data, 4 * 8, 0, 0, 0);
    fm_flash_wait_idle();

    return true;
}

static bool FLASH_PATCH_TEXT_ATTR fm_erase_sector(uint32_t addr)
{
    // write en
    fm_send_spi_cmd(0x06, 1*8, 0, 0, NULL, 0, NULL, 0, 0);
    fm_send_spi_cmd(0x20, 1*8, addr, 24, 0, 0, 0, 0, 0);
    return fm_flash_wait_idle();
}

static bool FLASH_PATCH_TEXT_ATTR fm_cam_erase_and_fix(uint8_t (*buf)[32])
{
    INFO(FLASH_PATCH_STR("Start erase and program cam buf\n"));
    // cmd 4.
    fm_cam_preprog_pre();
    if (!fm_erase_sector(0x0)) {
        ERROR(FLASH_PATCH_STR("ERR in ERASE %d\n"), __LINE__);
        return false;
    }
    int retry = 40;
    while (retry > 0) {
        WDT_FEED();
        // cmd 5.
        fm_cam_step_erase_pre();
        if (!fm_erase_sector(0x0)) {
            ERROR(FLASH_PATCH_STR("ERR in ERASE %d\n"), __LINE__);
            return false;
        }
        // cmd 6.
        fm_cam_step_prog_pre();
        if (!fm_erase_sector(0x0)) {
            ERROR(FLASH_PATCH_STR("ERR in ERASE %d\n"), __LINE__);
            return false;
        }

        // 
        INFO(FLASH_PATCH_STR("Start programming 5 page\n"));
        fm_cam_prog_pre();
        int line = 0;
        for (line = 0; line < 5; line++) {
            // write en
            fm_send_spi_cmd(0x06, 1*8, 0, 0, NULL, 0, NULL, 0, 0);
            // prog
            fm_send_spi_cmd(0x02, 8, 0x20*line, 24, &buf[line][0], 32*8, 0, 0, 0);
            fm_flash_wait_idle();
            WDT_FEED();
            // for (uint32_t loop = 0; loop < 32; loop++) {
            //     DEBUG(FLASH_PATCH_STR("%02x "), buf[line][loop]);
            // }
            // INFO(FLASH_PATCH_STR("\n"));
        }
        INFO(FLASH_PATCH_STR("Programming Done\n"));

        // cmd 7.
        fm_cam_read_after_erase_pre();
        int found_error = false;
        for (line = 0; line < 5; line++) {
            uint32_t cam_rd[8];
            fm_send_spi_cmd(0x03, 8, 0x20 * line, 24, 0, 0, cam_rd, 32*8, 0);
            int idx = 0;
            for (idx = 0;idx < 8; idx++) {
                uint32_t* p = (uint32_t*)buf[line];
                if (cam_rd[idx] != p[idx]) {
                    found_error = true;
                    ERROR(FLASH_PATCH_STR("erase check error, retry...%d, %d, 0x%08x\n"), retry, idx, cam_rd[idx]);
                    break;
                }
            }
            retry -= 1;
        }

        if (!found_error) {
            for (line = 5; line < 20; line++) {
                uint32_t cam_rd[8];
                fm_send_spi_cmd(0x03, 8, 0x20 * line, 24, 0, 0, cam_rd, 32*8, 0);
                int idx = 0;
                for (idx = 0;idx < 8; idx++) {
                    if (cam_rd[idx] != 0x000000ff) {
                        found_error = true;
                        ERROR(FLASH_PATCH_STR("erase check error, retry...%d, %d, 0x%08x\n"), retry, idx, cam_rd[idx]);
                        break;
                    }
                }
                retry -= 1;
            }
        }

        if (! found_error) {
            INFO(FLASH_PATCH_STR("Erase Pass !!!\n"));
            break;
        }
    }
    if (retry <= 0) {
        ERROR(FLASH_PATCH_STR("Erase fail !!!\n"));
        return false;
    }

    // cmd 3.
    INFO(FLASH_PATCH_STR("Start programming\n"));
    fm_cam_prog_pre();
    int line = 0;
    for (line = 5; line < 20; line++) {
        // write en
        fm_send_spi_cmd(0x06, 1*8, 0, 0, NULL, 0, NULL, 0, 0);
        // prog
        fm_send_spi_cmd(0x02, 8, 0x20*line, 24, &buf[line][0], 32*8, 0, 0, 0);
        fm_flash_wait_idle();
        WDT_FEED();
        for (uint32_t loop = 0; loop < 32; loop++) {
            DEBUG(FLASH_PATCH_STR("%02x "), buf[line][loop]);
        }
        INFO(FLASH_PATCH_STR("\n"));
    }

    // read buffer
    INFO(FLASH_PATCH_STR("Prog done, read and check"));
    fm_cam_read_pre();
    for (line = 0;line < 20; line++) {
        WDT_FEED();
        uint8_t cam_check[32];
        fm_send_spi_cmd(0x03, 8, 0x20*line, 24, 0, 0, cam_check, 32*8, 0);
        int j = 0;
        for (j = 0; j < 32; j++) {
            DEBUG(FLASH_PATCH_STR("%02x "), cam_check[j]);
            if ((j + 1) % 16 == 0) {
                DEBUG(FLASH_PATCH_STR("\n"));
            }
        }
        if (memcmp(cam_check, buf[line], 32) != 0) {
            ERROR(FLASH_PATCH_STR("CAM BUF[%d] check error\n"), line);
            patch_delay(50);
            return false;
        }
    }

    fm_set_uid_flag();
    INFO(FLASH_PATCH_STR("CAM prog done !!!\n"));
    return true;
}

static const uint8_t FLASH_PATCH_RODATA_ATTR cam_buf_default_rodata[20][32] = {
    {0x55, 0x00, 0x00, 0x00, 0xaa, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x0b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x00, 0x00, 0x47, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x53, 0x00, 0x00, 0x00, 0x5c, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x53, 0x00, 0x00, 0x00, 0x4f, 0x00, 0x00, 0x00, 0x7a, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x23, 0x00, 0x00, 0x00, 0x4f, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0xe3, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x23, 0x00, 0x00, 0x00, 0x53, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x01, 0x00, 0x00, 0x00, 0x47, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x01, 0x00, 0x00, 0x00, 0x47, 0x00, 0x00, 0x00, 0x67, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x01, 0x00, 0x00, 0x00, 0x47, 0x00, 0x00, 0x00, 0x6c, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x01, 0x00, 0x00, 0x00, 0x47, 0x00, 0x00, 0x00, 0x6e, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x01, 0x00, 0x00, 0x00, 0x47, 0x00, 0x00, 0x00, 0x79, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x01, 0x00, 0x00, 0x00, 0x47, 0x00, 0x00, 0x00, 0x7a, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x01, 0x00, 0x00, 0x00, 0x47, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x01, 0x00, 0x00, 0x00, 0x47, 0x00, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
};

static bool FLASH_PATCH_TEXT_ATTR esp_fm_check_uid()
{
    uint8_t cam_buf_default[20][32];

    memcpy(cam_buf_default, cam_buf_default_rodata, 20*32);

    // cmd 2.
    fm_cam_uid_pre();
    uint8_t uid[6];
    fm_send_spi_cmd(0x03, 1*8, 0x25, 3*8, NULL, 0, uid, 6*8, 0);
    if(uid[0] == ~uid[1] && uid[2] == ~uid[3] && uid[4] == ~uid[5]) {
        INFO(FLASH_PATCH_STR("UID check correct, update buf\n"));
        cam_buf_default[3][12] = uid[0] & 0xff;
        cam_buf_default[5][0] = uid[2] & 0xff;
        cam_buf_default[6][0] = uid[2] & 0xff;
        cam_buf_default[7][0] = uid[4] & 0xff;
        cam_buf_default[9][0] = uid[4] & 0xff;
    } else {
        INFO(FLASH_PATCH_STR("UID check error, use default buf\n"));
    }
    WDT_FEED();
    bool res = fm_cam_erase_and_fix(cam_buf_default);
    WDT_FEED();
    fm_soft_reset();
    return res;
}

static bool FLASH_PATCH_TEXT_ATTR fm_cam_check_buf_valid(uint8_t (*buf)[32])
{
    bool res = false;
    uint8_t count = 3;

    while (!res && count--) {
        // cmd 1.
        fm_cam_read_pre();

        int i = 0, j = 0;
        for (i = 0; i < 20; i++) {
            // read buf
            fm_send_spi_cmd(0x03, 8, 0x20 * i, 24, 0, 0, &buf[i][0], 32*8, 0);
            for (j = 0; j < 32; j++) {
                DEBUG(FLASH_PATCH_STR("%02x "), buf[i][j]);
                if ((j + 1) % 16 == 0) {
                    DEBUG(FLASH_PATCH_STR("\n"));
                    INFO(FLASH_PATCH_STR("\r"));
                }
            }
        }

        if (buf[0][0] == 0x55 && buf[0][4] == 0xaa \
            && buf[4][0] == 0x00 \
            && buf[10][0] == 0x1 && buf[10][20] == 0xff\
            && buf[11][0] == 0x1 && buf[11][20] == 0xff\
            && buf[12][0] == 0x1 && buf[12][20] == 0xff\
            && buf[13][0] == 0x1 && buf[13][20] == 0xff\
            && buf[14][0] == 0x1 && buf[14][20] == 0xff\
            && buf[15][0] == 0x1 && buf[15][20] == 0xff\
            && buf[16][0] == 0x1 && buf[16][20] == 0xff\
            && buf[17][0] == 0x1 && buf[17][20] == 0xff
        ) {
            INFO(FLASH_PATCH_STR("CAM buffer check valid !!!\n"));
            res = true;
        } else {
            INFO(FLASH_PATCH_STR("CAM buffer check IN-Valid !!!\n"));
            res = false;
        }
    }
    // while(1);
    return res;
}

static bool FLASH_PATCH_TEXT_ATTR fm_cam_check_uid(void)
{
    bool res = false;
    uint8_t count = 3;
    uint8_t uid[8];

    while (!res && count--) {
        fm_send_spi_cmd(0x4B, 8, 0, 0, 0, 0, &uid[0], 8*8, 4*8);
        DEBUG(FLASH_PATCH_STR("uid 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n"), uid[0], uid[1], uid[2], uid[3], uid[4], uid[5], uid[6], uid[7]);
        if ((((uid[1]&0xf0) == 0x0) && ((uid[0]&0x70) == 0))
            || (((uid[1]&0xf0) != 0x0))) {
            res = true;
        }
    }

    return res;
}

static bool FLASH_PATCH_TEXT_ATTR fm_fix_cam()
{
    uint8_t check_buf[20][32];

    if (fm_cam_check_uid()) {
        return true;
    }

    if (!double_check_fm25q16a()) {
        return true;
    }

    if (fm_cam_check_buf_valid(check_buf) == false) {
        ERROR(FLASH_PATCH_STR("Cam buf not valid\n"));
        return esp_fm_check_uid();
    } else {
        INFO(FLASH_PATCH_STR("Cam buf valid\n"));

        if ((check_buf[3][0] & 0x08) == 0x08) {
            INFO(FLASH_PATCH_STR("Bit3 == 1, already fixed....\n"));
            fm_set_uid_flag();
            fm_soft_reset();
        } else {
            check_buf[3][0] |= 0x08;
            bool res = fm_cam_erase_and_fix(check_buf);
            fm_soft_reset();
            return res;
        }
    }

    return true;
}

static uint32_t FLASH_PATCH_TEXT_ATTR fm_flash_id(void)
{
    uint32_t data[6];
    uint8_t* id = (uint8_t*)data;
#if 1
    fm_send_spi_cmd(0x9f, 8, 0, 0, 0, 0, id, 24*8, 0);
    return (id[0]<<16 | id[1]<<8 | id[2]);
#else
    uint32_t flash_id = spi_flash_get_id();
    memcpy(id, &flash_id, 3);
    return (id[0]<<16 | id[1]<<8 | id[2]);
#endif
}

static bool FLASH_PATCH_TEXT_ATTR double_check_fm25q16a(void)
{
    uint32_t addr = 0x10;
    uint8_t value[6];
    fm_send_spi_cmd(0x9f, 8, 0, 0, 0, 0, NULL, 0, 0);

    fm_cam_cmd_start();
    fm_send_spi_cmd(0x90, 8, addr, 3 * 8, 0, 0, value, sizeof(value) * 8, 0);
    fm_cam_cmd_end();
    DEBUG(FLASH_PATCH_STR("fm25q16a confirm: "));
    for (uint loop = 0; loop < sizeof(value); loop++) {
        DEBUG(FLASH_PATCH_STR(" 0x%02x"), value[loop]);
        if (value[loop] != 0xA1) {
            DEBUG(FLASH_PATCH_STR("\r\n"));
            return false;
        }
    }
    DEBUG(FLASH_PATCH_STR("\r\n"));

    return true;
}

static bool FLASH_PATCH_TEXT_ATTR is_fm25q16a(void)
{
    uint8_t status = 0x0;
    uint8_t count = 3;
    uint8_t value = 0;

    while (((status&0x20) == 0x0) && count--) {
        fm_send_spi_cmd(0x50, 1*8, 0, 0, NULL, 0, NULL, 0, 0);
        value = 0x80;
        fm_send_spi_cmd(0x01, 1*8, 0, 0, &value, 1*8, NULL, 0, 0);
        fm_send_spi_cmd(0x50, 1*8, 0, 0, NULL, 0, NULL, 0, 0);
        value = 0x03;
        fm_send_spi_cmd(0x31, 1*8, 0, 0, &value, 1*8, NULL, 0, 0);
        fm_send_spi_cmd(0x06, 1*8, 0, 0, NULL, 0, NULL, 0, 0);
        value = 0x02;
        fm_send_spi_cmd(0x31, 1*8, 0, 0, &value, 1*8, NULL, 0, 0);

        fm_flash_wait_idle();
        fm_send_spi_cmd(0x35, 1*8, 0x0, 0, NULL, 0, &status, 1*8, 0);

        fm_soft_reset();
    }

    if ((status&0x20) == 0) {
        DEBUG(FLASH_PATCH_STR("It's FM25Q16A\n"));
        return true;
    }

    DEBUG(FLASH_PATCH_STR("It's FM25Q16B\n"));
    return false;
}

int FLASH_PATCH_TEXT_ATTR fm25q16a_apply_patch_0()
{
    bool res = true;

    spi_state_t state;
    spi_enter(&state);

    uint32_t flash_id = fm_flash_id();
    DEBUG(FLASH_PATCH_STR("Flash id: 0x%x\n"), flash_id);

    WDT_FEED();
    if (flash_id == 0xa14015) {
        fm_soft_reset();
        INFO(DRAM_STR("Found FM25Q16A or FM25Q16B\n"));
        if(is_fm25q16a()) {
            INFO(DRAM_STR("Found FM25Q16A, check CAM buf\n"));
            res = fm_fix_cam();
        }
    } else if ((flash_id&0xffffff) == 0x0 || (flash_id&0xffffff) == 0xffffff) {
        INFO(FLASH_PATCH_STR("Found ID error, recover default CAM buf\n"));
        res = esp_fm_check_uid();
    } else {
        INFO(FLASH_PATCH_STR("Normal flash, continue...\n"));
    }
    WDT_FEED();

    spi_exit(&state);

    if (res != true) {
        fm_printf(FLASH_PATCH_STR("fix fail\n"));
        // we should keep running
        return 0;
    }

    fm_printf(FLASH_PATCH_STR("fix done\n"));
    return 0;
}

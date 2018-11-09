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

#if defined(CONFIG_ESP8266_OTA_FROM_OLD) && defined(BOOTLOADER_BUILD)

#include <string.h>
#include <stdint.h>
#include "esp_flash_data_types.h"
#include "esp_spi_flash.h"
#include "esp_log.h"
#include "esp_image_format.h"
#include "bootloader_config.h"
#include "esp_libc.h"
#include "esp8266/rom_functions.h"
#include "esp8266/eagle_soc.h"

#define PARTITION_DATA_OFFSET   (CONFIG_SPI_FLASH_SIZE / 2)

typedef struct s_sys_param {
    uint8_t     flag;
    uint8_t     reserved1[3];
    uint32_t    reserved2[7];
} sys_param_t;

typedef union s_boot_param {
    struct {
        uint8_t usr_bin : 4;
        uint8_t flag : 4;
    } boot_1;

    struct {
        uint8_t usr_bin : 4;
        uint8_t flag : 4;
        uint8_t version;
    } boot_2;

    struct {
        uint8_t usr_bin : 2;
        uint8_t boot_statue : 1;
        uint8_t to_qio : 1;
        uint8_t reserved1 : 4;

        uint8_t version : 5;
        uint8_t test_pass_flag : 1;
        uint8_t test_start_flag : 1;
        uint8_t enhance_boot_flag : 1;
    } boot_base;

    uint8_t data[4096];
} boot_param_t;

static const char *TAG = "partition_port";
static uint32_t s_partition_offset;
static uint8_t s_cache_buf[SPI_FLASH_SEC_SIZE];
static sys_param_t s_sys_param;
static boot_param_t s_boot_param;
static esp_spi_flash_chip_t s_flash_chip = {
    0x1640ef,
    CONFIG_SPI_FLASH_SIZE,
    64 * 1024,
    4 * 1024,
    256,
    0xffff
};

static inline void esp_hw_reset(void)
{
    CLEAR_WDT_REG_MASK(WDT_CTL_ADDRESS, BIT0);

    WDT_REG_WRITE(WDT_OP_ADDRESS, 8);
    WDT_REG_WRITE(WDT_OP_ND_ADDRESS, 8);

    SET_PERI_REG_BITS(PERIPHS_WDT_BASEADDR + WDT_CTL_ADDRESS, WDT_CTL_RSTLEN_MASK, 7 << WDT_CTL_RSTLEN_LSB, 0);
    // interrupt then reset
    SET_PERI_REG_BITS(PERIPHS_WDT_BASEADDR + WDT_CTL_ADDRESS, WDT_CTL_RSPMOD_MASK, 0 << WDT_CTL_RSPMOD_LSB, 0);
    // start task watch dog1
    SET_PERI_REG_BITS(PERIPHS_WDT_BASEADDR + WDT_CTL_ADDRESS, WDT_CTL_EN_MASK, 1 << WDT_CTL_EN_LSB, 0);

    while (1);
}

static inline int spi_flash_read_data(uint32_t addr, void *buf, size_t n)
{
    int ret;

    ESP_LOGD(TAG, "read buffer %p total %d from 0x%x", buf, n ,addr);

    ret = SPI_read_data(&s_flash_chip, addr, buf, n);

    return ret;
}

static inline int spi_flash_write_data(uint32_t addr, const void *buf, uint32_t n)
{
    int ret;

    ESP_LOGD(TAG, "write buffer %p total %d to 0x%x", buf, n ,addr);

    ret = SPIWrite(addr, (void *)buf, n);

    return ret;
}

static inline int spi_flash_erase(uint32_t addr)
{
    int ret;

    ESP_LOGD(TAG, "erase addr is 0x%x", addr);

    ret = SPIEraseSector(addr / SPI_FLASH_SEC_SIZE);

    return ret;
}

static inline uint32_t esp_get_updated_partition_table_addr(void)
{
    int ret;
    size_t offset;
    uint8_t user_bin;
    const uint32_t sect = CONFIG_SPI_FLASH_SIZE / SPI_FLASH_SEC_SIZE - 3;

    if (s_partition_offset)
        return s_partition_offset;

    ret = spi_flash_read_data((sect + 2) * SPI_FLASH_SEC_SIZE, &s_sys_param, sizeof(sys_param_t));
    if (ret) {
        ESP_LOGE(TAG, "read V2 system param error %d", ret);
        return -1UL;
    }

    ESP_LOGD(TAG, "V2 system flag is %x", s_sys_param.flag);

    offset = s_sys_param.flag ? 1 : 0;

    ret = spi_flash_read_data((sect + offset) * SPI_FLASH_SEC_SIZE, &s_boot_param, sizeof(boot_param_t));
    if (ret) {
        ESP_LOGE(TAG, "read V2 boot param error %d", ret);
        return -1UL;
    }

    if (s_boot_param.boot_base.usr_bin == 1) {
        if (s_boot_param.boot_base.boot_statue == 1)
            user_bin = 1;
        else
            user_bin = 0;
    } else {
        if (s_boot_param.boot_base.boot_statue == 1)
            user_bin = 0;
        else {
            if (s_boot_param.boot_base.version == 4)
                user_bin = 0;
            else
                user_bin = 1;
        }
    }

    if (user_bin)
        s_partition_offset = CONFIG_PARTITION_TABLE_OFFSET + PARTITION_DATA_OFFSET;
    else
        s_partition_offset = CONFIG_PARTITION_TABLE_OFFSET;


    ESP_LOGD(TAG, "Boot info %x %x %x %x %x", s_boot_param.boot_base.usr_bin, s_boot_param.boot_base.boot_statue,
                    s_boot_param.boot_base.version, user_bin, s_partition_offset);

    return  s_partition_offset;
}

static inline int spi_flash_write_data_safe(uint32_t addr, const void *buf, size_t n)
{
    int ret;
    static uint8_t check_buf[SPI_FLASH_SEC_SIZE];

    ret = spi_flash_erase(addr);
    if (ret) {
        ESP_LOGE(TAG, "erase flash 0x%x error", addr);
        return -1;
    }

    ret = spi_flash_write_data(addr, buf, n);
    if (ret) {
        ESP_LOGE(TAG, "write flash %d bytes to 0x%x error", n, addr);
        return 0;
    }

    ret = spi_flash_read_data(addr, check_buf, n);
    if (ret) {
        ESP_LOGE(TAG, "read flash %d bytes from 0x%x error", n, addr);
        return -1;        
    }

    if (memcmp(buf, check_buf, n)) {
        ESP_LOGE(TAG, "check write flash %d bytes to 0x%x error", n, addr);
        return -1;
    }

    return 0;
}

static int esp_flash_sector_copy(uint32_t dest, uint32_t src, uint32_t total_size)
{
    ESP_LOGD(TAG, "Start to copy data from 0x%x to 0x%x total %d", src, dest, total_size);

    for (uint32_t offset = 0; offset < total_size; offset += SPI_FLASH_SEC_SIZE) {
        int ret;

        ret = spi_flash_read_data(src + offset, s_cache_buf, SPI_FLASH_SEC_SIZE);
        if (ret) {
            ESP_LOGE(TAG, "read flash %d bytes from 0x%x error", SPI_FLASH_SEC_SIZE, src + offset);
            return -1;        
        }

        ret = spi_flash_write_data_safe(dest + offset, s_cache_buf, SPI_FLASH_SEC_SIZE);
        if (ret) {
            ESP_LOGE(TAG, "write flash %d bytes to 0x%x error", SPI_FLASH_SEC_SIZE, dest + offset);
            return -1;        
        }        
    }

    return 0;
}

static inline int esp_set_v2boot_app1(void)
{
    int ret;
    size_t offset = s_sys_param.flag ? 1 : 0;
    const uint32_t base_addr = CONFIG_SPI_FLASH_SIZE / SPI_FLASH_SEC_SIZE - 3;
    const uint32_t sys_addr = (base_addr + 2) * SPI_FLASH_SEC_SIZE;
    const uint32_t to_addr = (base_addr + 1 - offset) * SPI_FLASH_SEC_SIZE;

    if (s_boot_param.boot_base.version == 0x2
        || s_boot_param.boot_base.version == 0x1f) {
        if (s_boot_param.boot_base.usr_bin == 1)
            s_boot_param.boot_base.usr_bin = 0;
        else
            s_boot_param.boot_base.usr_bin = 1;
    } else {
        s_boot_param.boot_base.enhance_boot_flag = 1;
        if (s_boot_param.boot_base.boot_statue != 0) {
            if (s_boot_param.boot_base.usr_bin == 1)
                s_boot_param.boot_base.usr_bin = 0;
            else
                s_boot_param.boot_base.usr_bin = 1;            
        }
        s_boot_param.boot_base.boot_statue = 1;
    }

    ESP_LOGD(TAG, "Boot info %x %x %x", s_boot_param.boot_base.usr_bin, s_boot_param.boot_base.boot_statue, s_boot_param.boot_base.version);

    ret = spi_flash_write_data_safe(to_addr, &s_boot_param, sizeof(boot_param_t));
    if (ret) {
        ESP_LOGE(TAG, "write flash %d bytes to 0x%x error", sizeof(boot_param_t), to_addr);
        return -1;
    }

    if (s_sys_param.flag)
        s_sys_param.flag = 0;
    else
        s_sys_param.flag = 1;

    ret = spi_flash_write_data_safe(sys_addr, &s_sys_param, sizeof(sys_param_t));
    if (ret) {
        ESP_LOGE(TAG, "write flash %d bytes to 0x%x error", SPI_FLASH_SEC_SIZE, sys_addr);
        return -1;
    }

    return 0;
}

static inline int esp_sdk_update_from_v2(void)
{
    const int segment_cnt = 3;
    const size_t v2_max_size = 4096;

    uint32_t segment_base = sizeof(esp_image_header_t);
    uint32_t segment_size = 0;

    if (s_partition_offset)
        return 1;

    for (int i = 0 ; i < segment_cnt; i++) {
        int ret;
        esp_image_segment_header_t segment;

        ret = spi_flash_read_data(segment_base, &segment, sizeof(esp_image_segment_header_t));
        if (ret) {
            ESP_LOGE(TAG, "%d read segment @0x%x is %d", i, segment_base, ret);
            return -1UL;
        }

        ESP_LOGD(TAG, "data is %x len is %d", segment.load_addr, segment.data_len);

        segment_size += segment.data_len;

        segment_base += sizeof(esp_image_segment_header_t) + segment.data_len;
    }

    ESP_LOGD(TAG, "boot total segment size is %u", segment_size);

    return segment_size <= v2_max_size;
}

int esp_patition_table_init_location(void)
{
    uint32_t addr;

    if (!esp_sdk_update_from_v2())
        return 0;

    addr = esp_get_updated_partition_table_addr();
    if (addr == CONFIG_PARTITION_TABLE_OFFSET)
        return 0;

    return esp_flash_sector_copy(CONFIG_PARTITION_TABLE_OFFSET, addr, SPI_FLASH_SEC_SIZE);
}

int esp_patition_copy_ota1_to_ota0(const void *partition_info)
{
    int ret;
    bootloader_state_t *bs = (bootloader_state_t *)partition_info;

    ret = esp_flash_sector_copy(bs->ota[0].offset, bs->ota[1].offset, bs->ota[1].size);
    if (ret) {
        ESP_LOGE(TAG, "Fail to copy OTA from 0x%x to 0x%x total %d", bs->ota[1].offset,
                        bs->ota[0].offset, bs->ota[1].size);
        return -1;
    }

    for (uint32_t offset = 0; offset < bs->ota_info.size; offset += SPI_FLASH_SEC_SIZE) {
        ret = spi_flash_erase(bs->ota_info.offset + offset);
        if (ret) {
            ESP_LOGE(TAG, "Fail to erase OTA data from 0x%x", bs->ota_info.offset + offset);
            return -1;
        }
    }  

    return 0;
}

int esp_patition_table_init_data(void *partition_info)
{
    int ret;
    const uint32_t boot_base = 0x1000;
    const uint32_t boot_size = CONFIG_SPI_FLASH_SIZE / 2 - boot_base - 4 * SPI_FLASH_SEC_SIZE;

    if (!esp_sdk_update_from_v2())
        return 0;

    if (esp_get_updated_partition_table_addr() == CONFIG_PARTITION_TABLE_OFFSET)
        return 0;

    ESP_LOGD(TAG, "Copy firmware1 from %d total %d", boot_base + PARTITION_DATA_OFFSET, boot_size);

    ret = esp_flash_sector_copy(boot_base, boot_base + PARTITION_DATA_OFFSET, boot_size);
    if (ret) {
        ESP_LOGE(TAG, "Fail to copy V3 bootloader from 0x%x to 0x%x total %d", boot_base + PARTITION_DATA_OFFSET,
                        boot_base, boot_size);
        return -1;
    }

    ret = esp_set_v2boot_app1();
    if (ret) {
        ESP_LOGE(TAG, "Fail to update V2 bootloader data");
        return -1;        
    }

    esp_hw_reset();

    return 0;
}

#endif /* CONFIG_ESP8266_OTA_FROM_OLD */

// Copyright 2020 Espressif Systems (Shanghai) PTE LTD
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

// #define LOG_LOCAL_LEVEL ESP_LOG_ERROR

#include <stdio.h>
#include <string.h>
#include <sys/errno.h>
#include "esp_log.h"
#include "rom/crc.h"
#include "esp_private/esp_system_internal.h"
#include "esp_fast_boot.h"
#ifndef BOOTLOADER_BUILD
#include "esp_ota_ops.h"
#include "esp_image_format.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp8266/rom_functions.h"
#include "esp8266/eagle_soc.h"
#include "rom/uart.h"
#include "esp_spi_flash.h"
#include "FreeRTOS.h"
#include "task.h"
#endif

static char *TAG = "fast_boot";

#ifndef BOOTLOADER_BUILD
int esp_fast_boot_enable_partition(const esp_partition_t *partition)
{
    int ret;
    esp_image_header_t image;

    if (!partition || partition->type != ESP_PARTITION_TYPE_APP)
        return -EINVAL;

    ret = spi_flash_read(partition->address, &image, sizeof(esp_image_header_t));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "read image head from spi flash error");
        return -EIO;
    }

    rtc_sys_info.fast_boot.image_start = partition->address;
    rtc_sys_info.fast_boot.image_size = partition->size - 4;
    rtc_sys_info.fast_boot.image_entry = image.entry_addr;
    rtc_sys_info.fast_boot.magic = ESP_SYSTEM_FAST_BOOT_IMAGE;
    rtc_sys_info.fast_boot.crc32 = crc32_le(UINT32_MAX, (uint8_t *)&rtc_sys_info.fast_boot, sizeof(rtc_sys_info.fast_boot) - 4);

    return 0;
}

int esp_fast_boot_enable(void)
{
    const esp_partition_t *running = esp_ota_get_running_partition();

    return esp_fast_boot_enable_partition(running);
}

void IRAM_ATTR esp_fast_boot_restart_app(uint32_t image_start, uint32_t entry_addr, uint8_t region, uint8_t sub_region)
{
    const uint32_t sp = DRAM_BASE + DRAM_SIZE - 16;
    void (*user_start)(size_t start_addr);

    Cache_Read_Disable();
    Cache_Read_Enable(sub_region, region, SOC_CACHE_SIZE);

    __asm__ __volatile__(
        "mov    a1, %0\n"
        : : "a"(sp) : "memory"
    );

    user_start = (void *)entry_addr;
    user_start(image_start);
}

int esp_fast_boot_restart(void)
{
    extern void pm_goto_rf_on(void);
    extern void clockgate_watchdog(int on);

    int ret;
    uint8_t region, sub_region;
    uint32_t image_start, image_size, image_entry, image_mask;
    const esp_partition_t *to_boot;
    esp_image_header_t image;

    to_boot = esp_ota_get_boot_partition();
    if (!to_boot) {
        ESP_LOGI(TAG, "no OTA boot partition");
        to_boot = esp_ota_get_running_partition();
        if (!to_boot) {
            ESP_LOGE(TAG, "ERROR: Fail to get running partition");
            return -EINVAL;
        }
    }

    image_start = to_boot->address;
    image_size = to_boot->size - 4;

    ret = spi_flash_read(image_start, &image, sizeof(esp_image_header_t));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ERROR: Fail to read image head from spi flash error=%d", ret);
        return -EIO;
    }

    image_entry = image.entry_addr;

    if (image_start < 0x200000) {
        region = 0;
    } else if (image_start < 0x400000) {
        region = 1;
    } else if (image_start < 0x600000) {
        region = 2;
    } else if (image_start < 0x800000) {
        region = 3;
    } else {
        ESP_LOGE(TAG, "ERROR: App bin error, start_addr 0x%08x image_len %d\n", image_start, image_size);
        return -EINVAL;
    }

    image_mask =  image_start & 0x1fffff;
    if (image_mask < 0x100000) {
        sub_region = 0;
    } else {
        sub_region = 1;
    }

    if (esp_wifi_stop() != ESP_OK) {
        ESP_LOGD(TAG, "ERROR: Fail to stop Wi-Fi");
    }

    uart_tx_wait_idle(1);
    uart_tx_wait_idle(0);

    vTaskDelay(40 / portTICK_RATE_MS);

    pm_goto_rf_on();
    clockgate_watchdog(0);
    REG_WRITE(0x3ff00018, 0xffff00ff);
    SET_PERI_REG_MASK(0x60000D48, BIT1);
    CLEAR_PERI_REG_MASK(0x60000D48, BIT1);

    /* Get startup time */
    //ets_printf("\nets\n");

    uart_disable_swap_io();

    vPortEnterCritical();
    REG_WRITE(INT_ENA_WDEV, 0);
    _xt_isr_mask(UINT32_MAX);

    esp_reset_reason_set_hint(ESP_RST_FAST_SW);

    esp_fast_boot_restart_app(image_start, image_entry, region, sub_region);

    return 0;
}
#endif

void esp_fast_boot_disable(void)
{
    memset(&rtc_sys_info.fast_boot, 0, sizeof(rtc_sys_info.fast_boot));
}

int esp_fast_boot_get_info(uint32_t *image_start, uint32_t *image_size, uint32_t *image_entry)
{
    if (rtc_sys_info.fast_boot.magic != ESP_SYSTEM_FAST_BOOT_IMAGE) {
        ESP_LOGE(TAG, "magic error");
        return -EINVAL;
    }

    if (rtc_sys_info.fast_boot.crc32 != crc32_le(UINT32_MAX, (uint8_t *)&rtc_sys_info.fast_boot, sizeof(rtc_sys_info.fast_boot) - 4)) {
        ESP_LOGE(TAG, "CRC32 error");
        esp_fast_boot_disable();
        return -EINVAL;
    }

    *image_start = rtc_sys_info.fast_boot.image_start;
    *image_size = rtc_sys_info.fast_boot.image_size;
    *image_entry =  rtc_sys_info.fast_boot.image_entry;

    esp_fast_boot_disable();

    return 0;
}

void esp_fast_boot_print_info(void)
{
    ets_printf("\nmagic is %x\n", rtc_sys_info.fast_boot.magic);
    ets_printf("image start is %x\n", rtc_sys_info.fast_boot.image_start);
    ets_printf("image size is %x\n", rtc_sys_info.fast_boot.image_size);
    ets_printf("image entry is %x\n", rtc_sys_info.fast_boot.image_entry);
    ets_printf("CRC32 is %x\n", rtc_sys_info.fast_boot.crc32);
}

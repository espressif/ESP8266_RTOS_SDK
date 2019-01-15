// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
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

#include <string.h>
#include "bootloader_config.h"
#include "bootloader_init.h"
#include "bootloader_utility.h"
#include "bootloader_common.h"
#include "esp_image_format.h"
#include "esp_log.h"

static const char* TAG = "boot";

static esp_err_t select_image (esp_image_metadata_t *image_data);
static int selected_boot_partition(const bootloader_state_t *bs);

void call_start_cpu(void)
{
    // 1. Hardware initialization
    if(bootloader_init() != ESP_OK){
        return;
    }

    // 2. Select image to boot
    esp_image_metadata_t image_data;
    if(select_image(&image_data) != ESP_OK){
        return;
    }

    // 3. Loading the selected image
    bootloader_utility_load_image(&image_data);
}

// Selects image to boot
static esp_err_t select_image (esp_image_metadata_t *image_data)
{
    // 1. Load partition table
    bootloader_state_t bs;// = { 0 };
    memset(&bs, 0, sizeof(bootloader_state_t));
    if (!bootloader_utility_load_partition_table(&bs)) {
        ESP_LOGE(TAG, "load partition table error!");
        return ESP_FAIL;
    }

    // 2. Select boot partition
    int boot_index = selected_boot_partition(&bs);
    if(boot_index == INVALID_INDEX) {
        return ESP_FAIL; // Unrecoverable failure (not due to corrupt ota data or bad partition contents)
    }

    // 3. Load the app image for booting
    if (!bootloader_utility_load_boot_image(&bs, boot_index, image_data)) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

/*
 * Selects a boot partition.
 * The conditions for switching to another firmware are checked.
 */
static int selected_boot_partition(const bootloader_state_t *bs)
{
    int boot_index = bootloader_utility_get_selected_boot_partition(bs);
    if (boot_index == INVALID_INDEX) {
        return boot_index; // Unrecoverable failure (not due to corrupt ota data or bad partition contents)
    } else {
        // Factory firmware.
#ifdef CONFIG_BOOTLOADER_FACTORY_RESET
        if (bootloader_common_check_long_hold_gpio(CONFIG_BOOTLOADER_NUM_PIN_FACTORY_RESET, CONFIG_BOOTLOADER_HOLD_TIME_GPIO) == 1) {
            ESP_LOGI(TAG, "Detect a condition of the factory reset");
            bool ota_data_erase = false;
#ifdef CONFIG_BOOTLOADER_OTA_DATA_ERASE
            ota_data_erase = true;
#endif
            const char *list_erase = CONFIG_BOOTLOADER_DATA_FACTORY_RESET;
            ESP_LOGI(TAG, "Data partitions to erase: %s", list_erase);
            if (bootloader_common_erase_part_type_data(list_erase, ota_data_erase) == false) {
                ESP_LOGE(TAG, "Not all partitions were erased");
            }
            return bootloader_utility_get_selected_boot_partition(bs);
        }
#endif
       // TEST firmware.
#ifdef CONFIG_BOOTLOADER_APP_TEST
        if (bootloader_common_check_long_hold_gpio(CONFIG_BOOTLOADER_NUM_PIN_APP_TEST, CONFIG_BOOTLOADER_HOLD_TIME_GPIO) == 1) {
            ESP_LOGI(TAG, "Detect a boot condition of the test firmware");
#ifdef CONFIG_BOOTLOADER_APP_TEST_IN_OTA_1
            /* In this case, test bin will locate in ota_1 by default.
               This is the solution for small Flash. */
            return 1;
#else
            if (bs->test.offset != 0) {
                boot_index = TEST_APP_INDEX;
                return boot_index;
            } else {
                ESP_LOGE(TAG, "Test firmware is not found in partition table");
                return INVALID_INDEX;
            }
#endif
        }
#endif
        // Customer implementation.
        // if (gpio_pin_1 == true && ...){
        //     boot_index = required_boot_partition;
        // } ...
    }
    return boot_index;
}

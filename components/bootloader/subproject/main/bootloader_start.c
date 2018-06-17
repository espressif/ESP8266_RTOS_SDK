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

#include "bootloader_init.h"
#include "bootloader_config.h"
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
//    bootloader_utility_load_image(&image_data);
}

// Selects image to boot
static esp_err_t select_image (esp_image_metadata_t *image_data)
{
    return ESP_OK;
}

/*
 * Selects a boot partition.
 * The conditions for switching to another firmware are checked.
 */
static int selected_boot_partition(const bootloader_state_t *bs)
{
    int boot_index = 1; //bootloader_utility_get_selected_boot_partition(bs);

    return boot_index;
}

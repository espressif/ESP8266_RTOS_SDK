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

#include <string.h>
#include "esp_libc.h"
#include "esp_attr.h"
#include "esp_system.h"
#include "FreeRTOS.h"

/**
 * Get IDF version
 */
const char* esp_get_idf_version(void)
{
    return IDF_VER;
}

/**
 * @brief Fill an esp_chip_info_t structure with information about the ESP8266 chip
 */
static void get_chip_info_esp8266(esp_chip_info_t* out_info)
{
    memset(out_info, 0, sizeof(*out_info));
    
    out_info->model = CHIP_ESP8266;
    out_info->revision = 1;
    out_info->cores = 1;
    out_info->features = CHIP_FEATURE_WIFI_BGN;
}

/**
 * @brief Fill an esp_chip_info_t structure with information about the chip
 */
void esp_chip_info(esp_chip_info_t* out_info)
{
    // Only ESP8266 is supported now, in the future call one of the
    // chip-specific functions based on sdkconfig choice
    return get_chip_info_esp8266(out_info);
}

/**
  * @brief  Get the size of available heap.
  */
uint32_t esp_get_free_heap_size(void)
{
    return xPortGetFreeHeapSize();
}

/**
  * @brief Get the minimum heap that has ever been available
  */
uint32_t esp_get_minimum_free_heap_size(void)
{
    return xPortGetMinimumEverFreeHeapSize();
}

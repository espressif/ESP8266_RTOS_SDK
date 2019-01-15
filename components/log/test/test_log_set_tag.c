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

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>

#include <unity.h>
#include <esp_spi_flash.h>
#include "esp_attr.h"
#include "esp_log.h"


TEST_CASE("Test set tag level", "[log]")
{
    esp_log_level_t level;
    size_t tag_off;
    const char *TAG[] = {"TAG-0", "TAG-1", "TAG-2", "TAG-3"};
    const char tag_max = sizeof(TAG) / sizeof(TAG[0]);

    esp_log_level_set("*", ESP_LOG_NONE);

    for (tag_off = 0; tag_off < tag_max; tag_off++) {
        esp_log_level_set(TAG[tag_off], ESP_LOG_MAX);
    }

    for (level = ESP_LOG_VERBOSE; level > ESP_LOG_NONE; level--) {
        for (tag_off = 0; tag_off < tag_max; tag_off++) {
            ESP_LOGE(TAG[tag_off], "Test TAG ERROR with level %d", level);
            ESP_LOGW(TAG[tag_off], "Test TAG WARN with level %d", level);
            ESP_LOGI(TAG[tag_off], "Test TAG INFO with level %d", level);
            ESP_LOGD(TAG[tag_off], "Test TAG DEBUG with level %d", level);
            ESP_LOGV(TAG[tag_off], "Test TAG VERBOSE with level %d", level);

            esp_log_level_set(TAG[tag_off], level - 1);
        }
    }

    printf("\n\n");

    esp_log_level_set("*", ESP_LOG_MAX);
    level = ESP_LOG_MAX;

    for (tag_off = 0; tag_off < tag_max; tag_off++) {
        ESP_LOGE(TAG[tag_off], "Test TAG ERROR with global level %d", level);
        ESP_LOGW(TAG[tag_off], "Test TAG WARN with global level %d", level);
        ESP_LOGI(TAG[tag_off], "Test TAG INFO with global level %d", level);
        ESP_LOGD(TAG[tag_off], "Test TAG DEBUG with global level %d", level);
        ESP_LOGV(TAG[tag_off], "Test TAG VERBOSE with global level %d", level);
    }
}

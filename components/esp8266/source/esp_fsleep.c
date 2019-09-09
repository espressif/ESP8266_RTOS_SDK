// Copyright 2019-2020 Espressif Systems (Shanghai) PTE LTD
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

#include "esp_sleep.h"
#include "esp_log.h"

static const char* TAG = "FPM";

void esp_wifi_fpm_open(void)
{
    ESP_LOGE(TAG, "We have removed this api, please do not call");
}

void esp_wifi_fpm_close(void)
{
    ESP_LOGE(TAG, "We have removed this api, please do not call");
}

void esp_wifi_fpm_do_wakeup(void)
{
    ESP_LOGE(TAG, "We have removed this api, please do not call");
}

void esp_wifi_fpm_set_wakeup_cb(fpm_wakeup_cb cb)
{
    ESP_LOGE(TAG, "We have removed this api, please do not call");
}

esp_err_t esp_wifi_fpm_do_sleep(uint32_t sleep_time_in_us)
{
    ESP_LOGE(TAG, "We have removed this api, please do not call");
    return ESP_FAIL;
}

void esp_wifi_fpm_set_sleep_type(wifi_sleep_type_t type)
{
    ESP_LOGE(TAG, "We have removed this api, please do not call");
}

wifi_sleep_type_t esp_wifi_fpm_get_sleep_type(void)
{
    ESP_LOGE(TAG, "We have removed this api, please do not call");
    return WIFI_NONE_SLEEP_T;
}

void esp_wifi_enable_gpio_wakeup(uint32_t gpio_num, gpio_int_type_t intr_status)
{
    ESP_LOGE(TAG, "We have removed this api, please do not call");
}

void esp_wifi_disable_gpio_wakeup(void)
{
    ESP_LOGE(TAG, "We have removed this api, please do not call");
}

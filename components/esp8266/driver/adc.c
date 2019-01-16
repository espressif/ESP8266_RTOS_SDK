// Copyright 2018-2025 Espressif Systems (Shanghai) PTE LTD
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


#include <stdio.h>
#include <stdint.h>

#include "esp_attr.h"
#include "esp_err.h"
#include "esp_log.h"

#include "driver/adc.h"

#define ENTER_CRITICAL() portENTER_CRITICAL()
#define EXIT_CRITICAL() portEXIT_CRITICAL()

static const char *TAG = "adc";

#define ADC_CHECK(a, str, ret_val) \
    if (!(a)) { \
        ESP_LOGE(TAG,"%s(%d): %s", __FUNCTION__, __LINE__, str); \
        return (ret_val); \
    }

extern uint16_t test_tout();
extern void phy_adc_read_fast(uint16_t *adc_addr, uint16_t adc_num, uint8_t adc_clk_div);
extern uint16_t phy_get_vdd33();

uint16_t adc_read()
{
    uint16_t ret = test_tout(0);

    if (ret != 0xFFFF) {
        // The working voltage of ADC is designed according to 1.1v. Later, the actual working voltage of ADC is increased to 1.2v, so this scale is added.
        ret = ret * 12 / 11;

        if (ret > 1023) {
            // 10-bit precision ADC
            ret = 1023;
        }
    }

    return ret;
}
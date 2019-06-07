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
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_attr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_phy_init.h"
#include "esp_heap_caps.h"
#include "driver/adc.h"

static const char *TAG = "adc";

#define ADC_CHECK(a, str, ret_val) \
    if (!(a)) { \
        ESP_LOGE(TAG,"%s(%d): %s", __FUNCTION__, __LINE__, str); \
        return (ret_val); \
    }

extern uint16_t test_tout();
extern void phy_adc_read_fast(uint16_t *adc_addr, uint16_t adc_num, uint8_t adc_clk_div);
extern uint16_t phy_get_vdd33();

typedef struct {
    adc_config_t config;
    SemaphoreHandle_t adc_mux;
} adc_handle_t;

adc_handle_t *adc_handle = NULL;

esp_err_t adc_read(uint16_t *data)
{
    ADC_CHECK(data, "parameter pointer is empty", ESP_ERR_INVALID_ARG);
    ADC_CHECK(adc_handle, "ADC has not been initialized yet.", ESP_FAIL);
    uint16_t ret = 0;
    xSemaphoreTake(adc_handle->adc_mux, portMAX_DELAY);

    if (adc_handle->config.mode == ADC_READ_TOUT_MODE) {
        ret = test_tout(0);

        if (ret != 0xFFFF) {
            // The working voltage of ADC is designed according to 1.1v. Later, the actual working voltage of ADC is increased to 1.2v, so this scale is added.
            ret = ret * 12 / 11;

            if (ret > 1023) {
                // 10-bit precision ADC
                ret = 1023;
            }
        }
    } else if (adc_handle->config.mode == ADC_READ_VDD_MODE) {
        ret = phy_get_vdd33();

        if (ret != 0xFFFF) {
            // The working voltage of ADC is designed according to 1.1v. Later, the actual working voltage of ADC is increased to 1.2v, so this scale is added.
            ret = ret * 12 / 11;
        }
    }

    *data = ret;
    xSemaphoreGive(adc_handle->adc_mux);
    return ESP_OK;
}

esp_err_t adc_read_fast(uint16_t *data, uint16_t len)
{
    ADC_CHECK(data && len > 0, "parameter pointer is empty", ESP_ERR_INVALID_ARG);
    ADC_CHECK(adc_handle, "ADC has not been initialized yet.", ESP_FAIL);
    ADC_CHECK(adc_handle->config.mode == ADC_READ_TOUT_MODE, "adc_read_fast can only be used in ADC_READ_TOUT_MODE mode", ESP_ERR_INVALID_ARG);
    ADC_CHECK(adc_handle->config.clk_div >= 8 && adc_handle->config.clk_div <= 32, "ADC sample collection clock=80M/clk_div, range[8, 32]", ESP_FAIL);
    uint16_t i;
    uint16_t ret;

    xSemaphoreTake(adc_handle->adc_mux, portMAX_DELAY);
    phy_adc_read_fast(data, len, adc_handle->config.clk_div);

    for (i = 0; i < len; i++) {
        ret = data[i];

        if (ret != 0xFFFF) {
            // The working voltage of ADC is designed according to 1.1v. Later, the actual working voltage of ADC is increased to 1.2v, so this scale is added.
            ret = ret * 12 / 11;

            if (ret > 1023) {
                // 10-bit precision ADC
                ret = 1023;
            }
        }

        data[i] = ret;
    }

    xSemaphoreGive(adc_handle->adc_mux);
    return ESP_OK;
}

esp_err_t adc_deinit()
{
    ADC_CHECK(adc_handle, "ADC has not been initialized yet.", ESP_FAIL);

    if (adc_handle->adc_mux) {
        vSemaphoreDelete(adc_handle->adc_mux);
    }

    heap_caps_free(adc_handle);
    adc_handle = NULL;
    return ESP_OK;
}

esp_err_t adc_init(adc_config_t *config)
{
    ADC_CHECK(config, "config error", ESP_ERR_INVALID_ARG);
    ADC_CHECK(NULL == adc_handle, "adc has been initialized", ESP_FAIL);
    uint8_t vdd33_const;
    esp_phy_init_data_t *phy_init_data;

    phy_init_data = (esp_phy_init_data_t *)esp_phy_get_init_data();
    vdd33_const = phy_init_data->params[107];
    
    ADC_CHECK((config->mode == ADC_READ_TOUT_MODE) ? (vdd33_const < 255) : true, "To read the external voltage on TOUT(ADC) pin, vdd33_const need less than 255", ESP_FAIL);
    ADC_CHECK((config->mode == ADC_READ_VDD_MODE) ? (vdd33_const == 255) : true, "When adc measuring system voltage, vdd33_const must be set to 255,", ESP_FAIL);
    ADC_CHECK(config->mode <= ADC_READ_MAX_MODE, "adc mode err", ESP_FAIL);

    adc_handle = heap_caps_malloc(sizeof(adc_handle_t), MALLOC_CAP_8BIT);
    ADC_CHECK(adc_handle, "adc handle malloc error", ESP_ERR_NO_MEM);
    memcpy(&adc_handle->config, config, sizeof(adc_config_t));
    adc_handle->adc_mux = xSemaphoreCreateMutex();

    if (NULL == adc_handle->adc_mux) {
        adc_deinit();
        ADC_CHECK(false, "Semaphore create fail", ESP_ERR_NO_MEM);
    }

    return ESP_OK;
}
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

#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ADC working mode enumeration
 */
typedef enum {
    ADC_READ_TOUT_MODE = 0,
    ADC_READ_VDD_MODE,
    ADC_READ_MAX_MODE
} adc_mode_t;

/**
 * @brief ADC initialization parameter structure type definition
 */
typedef struct {
    adc_mode_t mode;  /*!< ADC mode */
    uint8_t clk_div;  /*!< ADC sample collection clock=80M/clk_div, range[8, 32] */
} adc_config_t;

/**
  * @brief Single measurement of TOUT(ADC) pin, unit : 1/1023 V or VDD pin, uint: 1 mV
  *
  * @note When measuring VDD pin voltage, the TOUT(ADC) pin must be left floating.
  * 
  * @param data Pointer to accept adc value.
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_FAIL adc has not been initialized yet
  */
esp_err_t adc_read(uint16_t *data);

/**
  * @brief Measure the input voltage of TOUT(ADC) pin, unit : 1/1023 V.
  *
  * @note Wi-Fi and interrupts need to be turned off.
  * 
  * @param data Pointer to accept adc value. Input voltage of TOUT(ADC) pin, unit : 1/1023 V
  * @param len Receiving length of ADC value, range [1, 65535]
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_FAIL adc has not been initialized yet
  */
esp_err_t adc_read_fast(uint16_t *data, uint16_t len);

/**
  * @brief Deinit the adc
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_FAIL adc has not been initialized yet
  */
esp_err_t adc_deinit();

/**
  * @brief Initialize the adc
  *
  * @note First modify menuconfig->Component config->PHY->vdd33_const value, vdd33_const provides ADC mode settings,
  *       i.e. selecting system voltage or external voltage measurements.
  *       When measuring system voltage, it must be set to 255.
  *       To read the external voltage on TOUT(ADC) pin, vdd33_const need less than 255
  *       When the ADC reference voltage is set to the actual VDD33 power supply voltage, the value range of vdd33_const is [18,36], the unit is 0.1V.
  *       When the ADC reference voltage is set to the default value of 3.3V as the supply voltage, the range of vdd33_const is [0, 18] or (36, 255).
  *
  * @param config Pointer to deliver initialize configuration parameter
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_NO_MEM malloc fail
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_FAIL adc has been initialized
  */
esp_err_t adc_init(adc_config_t *config);

#ifdef __cplusplus
}
#endif
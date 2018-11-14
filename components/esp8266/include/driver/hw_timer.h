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

#define TIMER_BASE_CLK              APB_CLK_FREQ

typedef void (*hw_timer_callback_t)(void *arg);

typedef enum {
    TIMER_CLKDIV_1 = 0,
    TIMER_CLKDIV_16 = 4,
    TIMER_CLKDIV_256 = 8
} hw_timer_clkdiv_t;

typedef enum {
    TIMER_EDGE_INT = 0,   // edge interrupt
    TIMER_LEVEL_INT = 1   // level interrupt
} hw_timer_intr_type_t;

/**
  * @brief Set the frequency division coefficient of hardware timer
  *
  * @param clkdiv frequency division coefficient
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_FAIL hardware timer has been initialized
  */
esp_err_t hw_timer_set_clkdiv(hw_timer_clkdiv_t clkdiv);

/**
  * @brief Get the frequency division coefficient of hardware timer
  *
  * @return
  *     - 0 TIMER_CLKDIV_1
  *     - 4 TIMER_CLKDIV_16
  *     - 8 TIMER_CLKDIV_256
  */
uint32_t hw_timer_get_clkdiv();

/**
  * @brief Set the interrupt type of hardware timer
  *
  * @param intr_type interrupt type
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_FAIL hardware timer has been initialized
  */
esp_err_t hw_timer_set_intr_type(hw_timer_intr_type_t intr_type);

/**
  * @brief Get the interrupt type of hardware timer
  *
  * @return
  *     - 0 TIMER_EDGE_INT
  *     - 1 TIMER_LEVEL_INT
  */
uint32_t hw_timer_get_intr_type();

/**
  * @brief Enable hardware timer reload
  *
  * @param reload false, one-shot mode; true, reload mode
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_FAIL hardware timer has been initialized
  */
esp_err_t hw_timer_set_reload(bool reload);

/**
  * @brief Get the hardware timer reload status
  *
  * @return
  *     - true reload mode
  *     - false one-shot mode
  */
bool hw_timer_get_reload();

/**
  * @brief Enable hardware timer
  *
  * @param en false, hardware timer disable; true, hardware timer enable
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_FAIL hardware timer has been initialized
  */
esp_err_t hw_timer_enable(bool en);

/**
  * @brief Get the hardware timer enable status
  *
  * @return
  *     - true hardware timer has been enabled
  *     - false hardware timer is not yet enabled
  */
bool hw_timer_get_enable();

/**
  * @brief Set the hardware timer load value
  *
  * @param load_data hardware timer load value
  *     - FRC1 hardware timer, range : less than 0x1000000
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_FAIL hardware timer has been initialized
  */
esp_err_t hw_timer_set_load_data(uint32_t load_data);

/**
  * @brief Get the hardware timer load value
  *
  * @return load value
  */
uint32_t hw_timer_get_load_data();

/**
  * @brief Get the hardware timer count value
  *
  * @return count value
  */
uint32_t hw_timer_get_count_data();

/**
  * @brief deinit the hardware timer
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_FAIL hardware timer has not been initialized yet
  */
esp_err_t hw_timer_deinit(void);

/**
  * @brief Initialize the hardware timer
  *
  * @param callback user hardware timer callback function
  * @param arg parameter for ISR handler
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_FAIL hardware timer has been initialized
  */
esp_err_t hw_timer_init(hw_timer_callback_t callback, void *arg);

/**
  * @brief Set a trigger timer us delay to enable this timer
  *
  * @param value
  *     - If reload is true, range : 50 ~ 0x199999
  *     - If reload is false, range : 10 ~ 0x199999
  * @param reload false, one-shot mode; true, reload mode.
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_FAIL hardware timer has not been initialized yet
  */

esp_err_t hw_timer_alarm_us(uint32_t value, bool reload);

/**
  * @brief disable this timer
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_FAIL hardware timer has not been initialized yet
  */
esp_err_t hw_timer_disarm(void);

#ifdef __cplusplus
}
#endif
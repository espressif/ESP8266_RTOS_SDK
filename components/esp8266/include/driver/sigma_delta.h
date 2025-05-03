/*
 * Copyright (c) 2023 <qb4.dev@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
  * @brief  Setup sigma-delta signal generator
  * The target frequency is defined as:
  * - for 0<target<128   freq = 80,000,000/prescaler * target /256 Hz
  * - for 128<target<256 freq = 80,000,000/prescaler * (256-target) /256
  *
  *  @note The target and prescaler will both affect the freq.
  *  CPU_FREQ has no influence on the sigma-delta frequency.
  *
  * @param[in] prescaler clock divider, range 0-255
  * @param[in] target duty cycle,range 0-255
  * @return
  *     - ESP_OK Success
  */
esp_err_t sigma_delta_init(uint8_t prescale,uint8_t target);

/**
  * @brief  Set sigma-delta signal generator prescale
  *
  * @param[in] prescale clock divider, range 0-255
  * @return
  *     - ESP_OK Success
  */
esp_err_t sigma_delta_set_prescale(uint8_t prescale);

/**
  * @brief  Get sigma-delta signal generator prescale
  *
  * @param[out] prescale clock divider
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG when prescale is NULL
  */
esp_err_t sigma_delta_get_prescale(uint8_t *prescale);

/**
  * @brief  Set sigma-delta signal generator target
  *
  * @param[in] target duty cycle,range 0-255
  * @return
  *     - ESP_OK Success
  */
esp_err_t sigma_delta_set_target(uint8_t target);

/**
  * @brief  Get sigma-delta signal generator target
  *
  * @param[out] target duty cycle
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG when target is NULL
  */
esp_err_t sigma_delta_get_target(uint8_t *target);

/**
  * @brief  Disable sigma-delta signal generator
  *
  * @return
  *     - ESP_OK Success
  */
esp_err_t sigma_delta_deinit(void);

/**
  * @brief  Set sigma-delta signal generator output
  * on selected GPIO
  *
  * @note GPIO should already been configured as output
  *
  * @param[in] gpio_num selected gpio pin
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG GPIO number error
  */
esp_err_t sigma_delta_set_output(gpio_num_t gpio_num);

/**
  * @brief  Clear sigma-delta signal generator output
  * on selected GPIO
  *
  * @param[in] gpio_num selected gpio pin
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG GPIO number error
  */
esp_err_t sigma_delta_clear_output(gpio_num_t gpio_num);

#ifdef __cplusplus
}
#endif

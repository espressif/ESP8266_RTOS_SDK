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
  * @brief  PWM function initialization, including GPIO, frequency and duty cycle.
  *
  * @param  period PWM period, unit: us.
  *         e.g. For 1KHz PWM, period is 1000 us. Do not set the period below 20us.
  * @param  duties duty cycle of each channels.
  * @param  channel_num PWM channel number, maximum is 8
  * @param  pin_num GPIO number of PWM channel
  * 
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_FAIL Init error
  */
esp_err_t pwm_init(uint32_t period, uint32_t *duties, uint8_t channel_num, const uint32_t *pin_num);

/**
  * @brief  PWM function uninstall
  * 
  * @return
  *     - ESP_OK Success
  *     - ESP_FAIL Init error
  */
esp_err_t pwm_deinit(void);

/**
  * @brief   Set the duty cycle of a PWM channel.
  *          Set the time that high level or low(if you invert the output of this channel)
  *          signal will last, the duty cycle cannot exceed the period.
  *
  * @note    After set configuration, pwm_start needs to be called to take effect.
  * 
  * @param   channel_num PWM channel number
  *          the channel_num cannot exceed the value initialized by pwm_init.
  * @param   duty duty cycle
  * 
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  */
esp_err_t pwm_set_duty(uint8_t channel_num, uint32_t duty);

/**
  * @brief   Get the duty cycle of a PWM channel.
  *
  * @param   channel_num PWM channel number
  *          the channel_num cannot exceed the value initialized by pwm_init.
  * @param  duty_p pointer saves the address of the specified channel duty cycle
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  */
esp_err_t pwm_get_duty(uint8_t channel_num, uint32_t *duty_p);

/**
  * @brief   Set PWM period, unit: us.
  *
  * @note After set configuration, pwm_start needs to be called to take effect.
  *
  * @param   period PWM period, unit: us
  *          For example, for 1KHz PWM, period is 1000. Do not set the period below 20us.
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  */
esp_err_t pwm_set_period(uint32_t period);

/**
  * @brief   Get PWM period, unit: us.
  *
  * @param   period_p pointer saves the address of the period
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  */
esp_err_t pwm_get_period(uint32_t *period_p);

/**
  * @brief   Starts PWM.
  *
  * @note    This function needs to be called after PWM configuration is changed.
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  */
esp_err_t pwm_start(void);

/**
  * @brief  Stop all PWM channel.
  *         Stop PWM and set the output of each channel to the specified level.
  *         Calling pwm_start can re-start PWM output.
  *
  * @param  stop_level_mask Out put level after PWM is stoped
  *         e.g. We initialize 8 channels, if stop_level_mask = 0x0f,
  *         channel 0,1,2 and 3 will output high level, and channel 4,5,6 and 7 will output low level.
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  */
esp_err_t pwm_stop(uint32_t stop_level_mask);

/**
  * @brief  Set the duty cycle of all channels.
  *
  * @note   After set configuration, pwm_start needs to be called to take effect.
  *
  * @param  duties An array that store the duty cycle of each channel,
  *         the array elements number needs to be the same as the number of channels.
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  */
esp_err_t pwm_set_duties(uint32_t *duties);

/**
  * @brief   Set the phase of a PWM channel.
  *
  * @note    After set configuration, pwm_start needs to be called to take effect.
  * 
  * @param   channel_num PWM channel number
  *          the channel_num cannot exceed the value initialized by pwm_init.
  * @param   phase The phase of this PWM channel, the phase range is (-180 ~ 180).
  * 
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  */
esp_err_t pwm_set_phase(uint8_t channel_num, int16_t phase);

/**
  * @brief   Set the phase of all channels.
  *
  * @note    After set configuration, pwm_start needs to be called to take effect.
  *
  * @param   phases An array that store the phase of each channel,
  *          the array elements number needs to be the same as the number of channels.
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  */
esp_err_t pwm_set_phases(int16_t *phases);

/**
  * @brief   Get the phase of a PWM channel.
  *
  * @param   channel_num PWM channel number
  *          the channel_num cannot exceed the value initialized by pwm_init.
  * @param  phase_p pointer saves the address of the specified channel phase
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  */
esp_err_t pwm_get_phase(uint8_t channel_num, uint16_t *phase_p);

/**
  * @brief   Set PWM period and duty of each PWM channel.
  *
  * @note    After set configuration, pwm_start needs to be called to take effect.
  *
  * @param   period PWM period, unit: us
  *          For example, for 1KHz PWM, period is 1000.
  * @param   duties An array that store the duty cycle of each channel,
  *          the array elements number needs to be the same as the number of channels.
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  */
esp_err_t pwm_set_period_duties(uint32_t period, uint32_t *duties);

/**
  * @brief  Set the inverting output PWM channel.
  *
  * @note   After set configuration, pwm_start needs to be called to take effect.
  *
  * @param  channel_mask The channel bitmask that used to invert the output
  *         e.g. We initialize 8 channels, if channel_mask = 0x0f, channels 0, 1, 2 and 3 will invert the output.
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  */
esp_err_t pwm_set_channel_invert(uint16_t channel_mask);

/**
  * @brief  Clear the inverting output PWM channel.
  *         This function only works for the PWM channel that is already in the inverted output states.
  *
  * @note   After set configuration, pwm_start needs to be called to take effect.
  *
  * @param  channel_mask The channel bitmask that need to clear
  *         e.g. The outputs of channels 0, 1, 2 and 3 are already in inverted state. If channel_mask = 0x07,
  *         the output of channel 0, 1, and 2 will return to normal, the channel 3 will keep inverting output.
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  */
esp_err_t pwm_clear_channel_invert(uint16_t channel_mask);

#ifdef __cplusplus
}
#endif

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

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#define PERIPH_LEDC_MODULE  (0) 
#define LEDC_APB_CLK_HZ (APB_CLK_FREQ)
#define LEDC_REF_CLK_HZ (1*1000000)
#define LEDC_ERR_DUTY   (0xFFFFFFFF)
#define LEDC_ERR_VAL    (-1)

typedef enum {
    LEDC_HIGH_SPEED_MODE = 0, /*!< LEDC high speed speed_mode */
    LEDC_LOW_SPEED_MODE,      /*!< LEDC low speed speed_mode */
    LEDC_SPEED_MODE_MAX,      /*!< LEDC speed limit */
} ledc_mode_t;

typedef enum {
    LEDC_INTR_DISABLE = 0,    /*!< Disable LEDC interrupt */
    LEDC_INTR_FADE_END,       /*!< Enable LEDC interrupt */
} ledc_intr_type_t;

typedef enum {
    LEDC_TIMER_0 = 0, /*!< LEDC timer 0 */
    LEDC_TIMER_1,     /*!< LEDC timer 1 */
    LEDC_TIMER_2,     /*!< LEDC timer 2 */
    LEDC_TIMER_3,     /*!< LEDC timer 3 */
    LEDC_TIMER_MAX,
} ledc_timer_t;

typedef enum {
    LEDC_CHANNEL_0 = 0, /*!< LEDC channel 0 */
    LEDC_CHANNEL_1,     /*!< LEDC channel 1 */
    LEDC_CHANNEL_2,     /*!< LEDC channel 2 */
    LEDC_CHANNEL_3,     /*!< LEDC channel 3 */
    LEDC_CHANNEL_4,     /*!< LEDC channel 4 */
    LEDC_CHANNEL_5,     /*!< LEDC channel 5 */
    LEDC_CHANNEL_6,     /*!< LEDC channel 6 */
    LEDC_CHANNEL_7,     /*!< LEDC channel 7 */
    LEDC_CHANNEL_MAX,
} ledc_channel_t;

typedef enum {
    LEDC_TIMER_1_BIT = 1,   /*!< LEDC PWM duty resolution of  1 bits */
    LEDC_TIMER_2_BIT,       /*!< LEDC PWM duty resolution of  2 bits */
    LEDC_TIMER_3_BIT,       /*!< LEDC PWM duty resolution of  3 bits */
    LEDC_TIMER_4_BIT,       /*!< LEDC PWM duty resolution of  4 bits */
    LEDC_TIMER_5_BIT,       /*!< LEDC PWM duty resolution of  5 bits */
    LEDC_TIMER_6_BIT,       /*!< LEDC PWM duty resolution of  6 bits */
    LEDC_TIMER_7_BIT,       /*!< LEDC PWM duty resolution of  7 bits */
    LEDC_TIMER_8_BIT,       /*!< LEDC PWM duty resolution of  8 bits */
    LEDC_TIMER_9_BIT,       /*!< LEDC PWM duty resolution of  9 bits */
    LEDC_TIMER_10_BIT,      /*!< LEDC PWM duty resolution of 10 bits */
    LEDC_TIMER_11_BIT,      /*!< LEDC PWM duty resolution of 11 bits */
    LEDC_TIMER_12_BIT,      /*!< LEDC PWM duty resolution of 12 bits */
    LEDC_TIMER_13_BIT,      /*!< LEDC PWM duty resolution of 13 bits */
    LEDC_TIMER_14_BIT,      /*!< LEDC PWM duty resolution of 14 bits */
    LEDC_TIMER_15_BIT,      /*!< LEDC PWM duty resolution of 15 bits */
    LEDC_TIMER_16_BIT,      /*!< LEDC PWM duty resolution of 16 bits */
    LEDC_TIMER_17_BIT,      /*!< LEDC PWM duty resolution of 17 bits */
    LEDC_TIMER_18_BIT,      /*!< LEDC PWM duty resolution of 18 bits */
    LEDC_TIMER_19_BIT,      /*!< LEDC PWM duty resolution of 19 bits */
    LEDC_TIMER_20_BIT,      /*!< LEDC PWM duty resolution of 20 bits */
    LEDC_TIMER_BIT_MAX,
} ledc_timer_bit_t;

typedef enum {
    LEDC_FADE_NO_WAIT = 0,  /*!< LEDC fade function will return immediately */
    LEDC_FADE_WAIT_DONE,    /*!< LEDC fade function will block until fading to the target duty */
    LEDC_FADE_MAX,
} ledc_fade_mode_t;

/**
 * @brief Configuration parameters of LEDC Timer timer for ledc_timer_config function
 */
typedef struct {
    ledc_mode_t speed_mode;                /*!< LEDC speed speed_mode, high-speed mode or low-speed mode */
    union {
        ledc_timer_bit_t duty_resolution;  /*!< LEDC channel duty resolution */
        ledc_timer_bit_t bit_num __attribute__((deprecated)); /*!< Deprecated in ESP-IDF 3.0. This is an alias to 'duty_resolution' for backward compatibility with ESP-IDF 2.1 */
    };
    ledc_timer_t  timer_num;               /*!< The timer source of channel (0 - 3) */
    uint32_t freq_hz;                      /*!< LEDC timer frequency (100Hz ~ 1KHz) */
} ledc_timer_config_t;

/**
 * @brief Configuration parameters of LEDC channel for ledc_channel_config function
 */
typedef struct {  
    int gpio_num;                   /*!< the LEDC output gpio_num, if you want to use gpio16, gpio_num = 16 */
    ledc_mode_t speed_mode;         /*!< Invalid parameter, compatible with esp32 API. Configure interrupt, LEDC speed speed_mode, high-speed mode or low-speed mode */
    ledc_channel_t channel;         /*!< LEDC channel (0 - 7) */
    ledc_intr_type_t intr_type;     /*!< Invalid parameter, compatible with esp32 API. Configure interrupt,Fade interrupt enable  or Fade interrupt disable */
    ledc_timer_t timer_sel;         /*!< Invalid parameter, compatible with esp32 API. Select the timer source of channel (0 - 3) */
    uint32_t duty;                  /*!< LEDC channel duty, the range of duty setting is [0, (2**duty_resolution)] */
    int hpoint;                     /*!< Invalid parameter, compatible with esp32 API.LEDC channel hpoint value, the max value is 0xfffff */
} ledc_channel_config_t;

/**
 * @brief  set ledc duty
 * 
 * @param  speed_mode    unnecessary parameters, just for code unity
 * @param  ledc_channel  ledc channel num
 * @param  ledc_duty     set the ledc duty you want
 * 
 *  @return
 *      - ESP_OK              Success
 *      - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t ledc_set_duty(ledc_mode_t speed_mode, ledc_channel_t ledc_channel, uint32_t ledc_duty);

/**
 * @brief  update ledc duty
 * 
 * @param  speed_mode     unnecessary parameters, just for code unity
 * @param  ledc_channel   ledc channel num
 * 
 * @return
 *     - ESP_OK              Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 *     - ESP_FAIL            IO error
 */
esp_err_t ledc_update_duty(ledc_mode_t speed_mode, ledc_channel_t ledc_channel);

/**
 * @brief  set ledc duty by fade
 * 
 * @param  speed_mode       unnecessary parameters, just for code unity
 * @param  ledc_channel     ledc channel num
 * @param  ledc_duty        set ledc duty
 * @param  ledc_fade_time   set ledc fade time
 * 
 * @return
 *     - ESP_OK               Success
 *     - ESP_ERR_INVALID_ARG  Parameter error
 */
esp_err_t ledc_set_fade_with_time(ledc_mode_t speed_mode, ledc_channel_t ledc_channel, uint32_t ledc_duty, int ledc_fade_time);

/**
 * @brief  start change ledc duty by fade
 * @param  speed_mode       unnecessary parameters, just for code unity
 * @param  ledc_channel     ledc channel num
 * @param  fade_mode        set fade mode, for example set LEDC_FADE_NO_WAIT  means LEDC fade function will return immediately 
 * set LEDC_FADE_WAIT_DONE means LEDC fade function will block until fading to the target duty
 * 
 *@return
 *    - ESP_OK              Success
 *    - ESP_ERR_INVALID_ARG Parameter error
 *    - ESP_FAIL            IO error
 */
esp_err_t ledc_fade_start(ledc_mode_t speed_mode, ledc_channel_t ledc_channel, ledc_fade_mode_t fade_mode);

/**
 * @brief LEDC channel configuration
 *        Configure LEDC channel with the given channel/output gpio_num/interrupt/source timer/frequency(Hz)/LEDC duty resolution
 *
 * @param ledc_conf Pointer of LEDC channel configure struct
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t ledc_channel_config(const ledc_channel_config_t* ledc_conf);

/**
 * @brief LEDC timer configuration
 *        Configure LEDC timer with the given source timer/frequency(Hz)/duty_resolution
 *
 * @param  timer_conf Pointer of LEDC timer configure struct
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t ledc_timer_config(const ledc_timer_config_t* timer_conf);

/**
 * @brief Install LEDC fade function. This function will occupy interrupt of LEDC module.
 * @param intr_alloc_flags unnecessary parameters, just for code unity, maybe will be used later
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_STATE Fade function already installed.
 */
esp_err_t ledc_fade_func_install(int intr_alloc_flags);

/**
 * @brief Uninstall LEDC fade function.
 *
 */
esp_err_t ledc_fade_func_uninstall(void);

/**
 * @brief LEDC stop.
 *        Disable LEDC output, and set idle level
 *
 * @param  speed_mode Select the LEDC speed_mode, high-speed mode and low-speed mode
 * @param  channel LEDC channel (0-7), select from ledc_channel_t
 * @param  idle_level Set output idle level after LEDC stops.
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t ledc_stop(ledc_mode_t speed_mode, ledc_channel_t channel, uint32_t idle_level);

/**
 * it is an empty function
 */
int periph_module_enable(int none);

#ifdef __cplusplus
}
#endif
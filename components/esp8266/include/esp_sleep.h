// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
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
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WIFI_NONE_SLEEP_T    = 0,
    WIFI_LIGHT_SLEEP_T,
    WIFI_MODEM_SLEEP_T
} wifi_sleep_type_t;

typedef enum esp_sleep_mode {
    ESP_CPU_WAIT = 0,
    ESP_CPU_LIGHTSLEEP,
} esp_sleep_mode_t;

typedef void (*fpm_wakeup_cb)(void);

/**
 * @brief Sleep wakeup cause
 */
typedef enum {
    ESP_SLEEP_WAKEUP_UNDEFINED,    //!< In case of deep sleep, reset was not caused by exit from deep sleep
    ESP_SLEEP_WAKEUP_ALL,          //!< Not a wakeup cause, used to disable all wakeup sources with esp_sleep_disable_wakeup_source
    ESP_SLEEP_WAKEUP_TIMER,        //!< Wakeup caused by timer
    ESP_SLEEP_WAKEUP_GPIO,         //!< Wakeup caused by GPIO (light sleep only)
} esp_sleep_source_t;

/**
  * @brief     Enter deep-sleep mode.
  *
  *            The device will automatically wake up after the deep-sleep time set
  *            by the users. Upon waking up, the device boots up from user_init.
  *
  * @attention 1. XPD_DCDC should be connected to EXT_RSTB through 0 ohm resistor
  *               in order to support deep-sleep wakeup.
  * @attention 2. system_deep_sleep(0): there is no wake up timer; in order to wake
  *               up, connect a GPIO to pin RST, the chip will wake up by a falling-edge
  *               on pin RST
  * @attention 3. esp_deep_sleep does not shut down WiFi and higher level protocol
  *               connections gracefully. Make sure esp_wifi_stop are called to close any
  *               connections and deinitialize the peripherals.
  *
  * @param     time_in_us  deep-sleep time, unit: microsecond
  *
  * @return    null
  */
void esp_deep_sleep(uint32_t time_in_us);

/**
 * @brief Set implementation-specific power management configuration
 * @param config pointer to implementation-specific configuration structure (e.g. esp_pm_config_esp32)
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_ARG if the configuration values are not correct
 *      - ESP_ERR_NOT_SUPPORTED if certain combination of values is not supported.
 */
esp_err_t esp_pm_configure(const void* config);

/**
  * @brief  Call this API before esp_deep_sleep and esp_wifi_init to set the activity after the
  *         next deep-sleep wakeup.
  *
  *         If this API is not called, default to be esp_deep_sleep_set_rf_option(1).
  *
  * @param  option  radio option
  *                 0 : Radio calibration after the deep-sleep wakeup is decided by byte
  *                     108 of esp_init_data_default.bin (0~127byte).
  *                 1 : Radio calibration will be done after the deep-sleep wakeup. This
  *                     will lead to stronger current.
  *                 2 : Radio calibration will not be done after the deep-sleep wakeup.
  *                     This will lead to weaker current.
  *                 4 : Disable radio calibration after the deep-sleep wakeup (the same
  *                     as modem-sleep). This will lead to the weakest current, but the device
  *                     can't receive or transmit data after waking up.
  *
  * @return null
  */
void esp_deep_sleep_set_rf_option(uint8_t option);

/**
  * @brief     Enable force sleep function.
  *
  * @attention Force sleep function is disabled by default.
  *
  * @return    null
  */
void esp_wifi_fpm_open(void) __attribute__ ((deprecated));

/**
  * @brief  Disable force sleep function.
  *
  * @return null
  */
void esp_wifi_fpm_close(void) __attribute__ ((deprecated));

/**
  * @brief     Wake ESP8266 up from MODEM_SLEEP_T force sleep.
  *
  * @attention This API can only be called when MODEM_SLEEP_T force sleep function
  *            is enabled, after calling wifi_fpm_open.
  *            This API can not be called after calling wifi_fpm_close.
  *
  * @return    null
  */
void esp_wifi_fpm_do_wakeup(void) __attribute__ ((deprecated));

/**
  * @brief     Set a callback of waken up from force sleep because of time out.
  *
  * @attention 1. This API can only be called when force sleep function is enabled,
  *               after calling wifi_fpm_open. This API can not be called after calling
  *               wifi_fpm_close.
  * @attention 2. fpm_wakeup_cb_func will be called after system woke up only if the
  *               force sleep time out (wifi_fpm_do_sleep and the parameter is not 0xFFFFFFF).
  * @attention 3. fpm_wakeup_cb_func will not be called if woke up by wifi_fpm_do_wakeup
  *               from MODEM_SLEEP_T type force sleep.
  *
  * @param     cb  callback of waken up
  *
  * @return    null
  */
void esp_wifi_fpm_set_wakeup_cb(fpm_wakeup_cb cb) __attribute__ ((deprecated));

/**
  * @brief     Force ESP8266 enter sleep mode, and it will wake up automatically when time out.
  *
  * @attention 1. This API can only be called when force sleep function is enabled, after
  *               calling wifi_fpm_open. This API can not be called after calling wifi_fpm_close.
  * @attention 2. If this API returned 0 means that the configuration is set successfully,
  *               but the ESP8266 will not enter sleep mode immediately, it is going to sleep
  *               in the system idle task. Please do not call other WiFi related function right
  *               after calling this API.
  *
  * @param     sleep_time_in_us  sleep time, ESP8266 will wake up automatically
  *                              when time out. Unit: us. Range: 10000 ~ 268435455(0xFFFFFFF).
  *                              - If sleep_time_in_us is 0xFFFFFFF, the ESP8266 will sleep till
  *                              - if wifi_fpm_set_sleep_type is set to be LIGHT_SLEEP_T, ESP8266 can wake up by GPIO.
  *                              - if wifi_fpm_set_sleep_type is set to be MODEM_SLEEP_T, ESP8266 can wake up by wifi_fpm_do_wakeup.
  *
  * @return  ESP_OK, setting succeed;
  * @return  ESP_ERR_WIFI_FPM_MODE, fail to sleep, force sleep function is not enabled.
  * @return  ESP_ERR_WIFI_PM_MODE_OPEN, fail to sleep, Please call esp_wifi_set_ps(WIFI_PS_NONE) first.
  * @return  ESP_ERR_WIFI_MODE, fail to sleep, Please call esp_wifi_set_mode(WIFI_MODE_NULL) first.
  */
esp_err_t esp_wifi_fpm_do_sleep(uint32_t sleep_time_in_us) __attribute__ ((deprecated));

/**
  * @brief     Set sleep type for force sleep function.
  *
  * @attention This API can only be called before wifi_fpm_open.
  *
  * @param     type  sleep type
  *
  * @return    null
  */
void esp_wifi_fpm_set_sleep_type(wifi_sleep_type_t type) __attribute__ ((deprecated));

/**
  * @brief  Get sleep type of force sleep function.
  *
  * @return sleep type
  */
wifi_sleep_type_t esp_wifi_fpm_get_sleep_type(void) __attribute__ ((deprecated));

/**
  * @brief  Set a GPIO to wake the ESP8266 up from light-sleep mode 
  *         ESP8266 will be wakened from Light-sleep, when the GPIO is in low-level.
  * 
  * If the ESP8266 enters light-sleep automatically(esp_wifi_set_sleep_type(LIGHT_SLEEP_T);), 
  * after being waken up by GPIO, when the chip attempts to sleep again, it will check the status of the GPIO:
  * Note:
  * • If the GPIO is still in the wakeup status, the EP8266 will enter modem-sleep mode instead;
  * • If the GPIO is NOT in the wakeup status, the ESP8266 will enter light-sleep mode
  * 
  * @param gpio_num     GPIO number, range: [0, 15].
  *                     gpio_int_type_t intr_status: status of GPIO interrupt to trigger the wakeup process.
  *                     - if esp_wifi_fpm_set_sleep_type is set to be LIGHT_SLEEP_T, ESP8266 can wake up by GPIO.
  *                     - if esp_wifi_fpm_set_sleep_type is set to be MODEM_SLEEP_T, ESP8266 can wake up by esp_wifi_fpm_do_wakeup.
  * @param intr_status  GPIO interrupt type
  *
  * @return   null
  */
void esp_wifi_enable_gpio_wakeup(uint32_t gpio_num, gpio_int_type_t intr_status) __attribute__ ((deprecated));

/**
  * @brief  Disable the function that the GPIO can wake the ESP8266 up from light-sleep mode.
  */
void esp_wifi_disable_gpio_wakeup(void) __attribute__ ((deprecated));

/**
 * @brief Enable wakeup by timer
 * @param time_in_us  time before wakeup, in microseconds
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_ARG if value is out of range (TBD)
 */
esp_err_t esp_sleep_enable_timer_wakeup(uint32_t time_in_us);

/**
 * @brief Enter light sleep with the configured wakeup options
 *
 * @attention esp_deep_sleep does not shut down WiFi and higher level protocol
 *               connections gracefully. Make sure esp_wifi_stop are called to close any
 *               connections and deinitialize the peripherals.
 * @return
 *  - ESP_OK on success (returned after wakeup)
 *  - ESP_ERR_INVALID_STATE if WiFi is not stopped
 */
esp_err_t esp_light_sleep_start(void);

/**
 * @brief Operation system start check time and enter sleep
 * 
 * @note This function is called by system, user should not call this
 */
void esp_sleep_start(void);

/**
 * @brief Enable wakeup from light sleep using GPIOs
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_STATE if wakeup triggers conflict
 */
esp_err_t esp_sleep_enable_gpio_wakeup(void);

/**
 * @brief Disable wakeup source
 *
 * This function is used to deactivate wake up trigger for source
 * defined as parameter of the function.
 *
 * @note This function does not modify wake up configuration in RTC.
 *       It will be performed in esp_sleep_start function.
 *
 * @param source - number of source to disable of type esp_sleep_source_t
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_STATE if trigger was not active
 */
esp_err_t esp_sleep_disable_wakeup_source(esp_sleep_source_t source);

#ifdef __cplusplus
}
#endif

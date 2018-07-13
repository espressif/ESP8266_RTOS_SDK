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

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup System_Sleep_APIs System Sleep APIs
  * @brief System Sleep APIs
  */

/** @addtogroup System_Sleep_APIs
  * @{
  */

/**
  * @brief     Set the chip to deep-sleep mode.
  *
  *            The device will automatically wake up after the deep-sleep time set
  *            by the users. Upon waking up, the device boots up from user_init.
  *
  * @attention 1. XPD_DCDC should be connected to EXT_RSTB through 0 ohm resistor
  *               in order to support deep-sleep wakeup.
  * @attention 2. system_deep_sleep(0): there is no wake up timer; in order to wake
  *               up, connect a GPIO to pin RST, the chip will wake up by a falling-edge
  *               on pin RST
  *
  * @param     uint32 time_in_us : deep-sleep time, unit: microsecond
  *
  * @return    null
  */
void esp_deep_sleep(uint32_t time_in_us);


/**
  * @brief  Call this API before esp_deep_sleep and esp_wifi_init to set the activity after the
  *         next deep-sleep wakeup.
  *
  *         If this API is not called, default to be esp_deep_sleep_set_rf_option(1).
  *
  * @param  uint8 option :
  * @param  0 : Radio calibration after the deep-sleep wakeup is decided by byte
  *             108 of esp_init_data_default.bin (0~127byte).
  * @param  1 : Radio calibration will be done after the deep-sleep wakeup. This
  *             will lead to stronger current.
  * @param  2 : Radio calibration will not be done after the deep-sleep wakeup.
  *             This will lead to weaker current.
  * @param  4 : Disable radio calibration after the deep-sleep wakeup (the same
  *             as modem-sleep). This will lead to the weakest current, but the device
  *             can't receive or transmit data after waking up.
  *
  * @return null
  */
void esp_deep_sleep_set_rf_option(uint8_t option);

/** \defgroup WiFi_Sleep_Type_APIs Sleep Type APIs
  * @brief WiFi Sleep APIs
  */

/** @addtogroup WiFi_Sleep_Type_APIs
  * @{
  */

typedef enum {
    WIFI_NONE_SLEEP_T    = 0,
    WIFI_LIGHT_SLEEP_T,
    WIFI_MODEM_SLEEP_T
} wifi_sleep_type_t;

/**
  * @}
  */


/** \defgroup WiFi_Force_Sleep_APIs Force Sleep APIs
  * @brief WiFi Force Sleep APIs
  */

/** @addtogroup WiFi_Force_Sleep_APIs
  * @{
  */

/**
  * @brief     Enable force sleep function.
  *
  * @attention Force sleep function is disabled by default.
  *
  * @param     null
  *
  * @return    null
  */
void esp_wifi_fpm_open(void);

/**
  * @brief  Disable force sleep function.
  *
  * @param  null
  *
  * @return null
  */
void esp_wifi_fpm_close(void);

/**
  * @brief     Wake ESP8266 up from MODEM_SLEEP_T force sleep.
  *
  * @attention This API can only be called when MODEM_SLEEP_T force sleep function
  *            is enabled, after calling wifi_fpm_open.
  *            This API can not be called after calling wifi_fpm_close.
  *
  * @param     null
  *
  * @return    null
  */
void esp_wifi_fpm_do_wakeup(void);

typedef void (*fpm_wakeup_cb)(void);

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
  * @param     void (*fpm_wakeup_cb_func)(void) : callback of waken up
  *
  * @return    null
  */
void esp_wifi_fpm_set_wakeup_cb(fpm_wakeup_cb cb);

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
  * @param     uint32 sleep_time_in_us : sleep time, ESP8266 will wake up automatically
  *                                      when time out. Unit: us. Range: 10000 ~ 268435455(0xFFFFFFF).
  *    - If sleep_time_in_us is 0xFFFFFFF, the ESP8266 will sleep till
  *    - if wifi_fpm_set_sleep_type is set to be LIGHT_SLEEP_T, ESP8266 can wake up by GPIO.
  *    - if wifi_fpm_set_sleep_type is set to be MODEM_SLEEP_T, ESP8266 can wake up by wifi_fpm_do_wakeup.
  *
  * @return   0, setting succeed;
  * @return  -1, fail to sleep, sleep status error;
  * @return  -2, fail to sleep, force sleep function is not enabled.
  */
esp_err_t esp_wifi_fpm_do_sleep(uint32_t sleep_time_in_us);

/**
  * @brief     Set sleep type for force sleep function.
  *
  * @attention This API can only be called before wifi_fpm_open.
  *
  * @param     wifi_sleep_type_t type : sleep type
  *
  * @return    null
  */
void esp_wifi_fpm_set_sleep_type(wifi_sleep_type_t type);

/**
  * @brief  Get sleep type of force sleep function.
  *
  * @param  null
  *
  * @return sleep type
  */
wifi_sleep_type_t esp_wifi_fpm_get_sleep_type(void);

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

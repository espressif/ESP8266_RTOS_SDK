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

#pragma once

#include <stdint.h>
#include "esp8266/eagle_soc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Wakeup option
 */
#define RTC_GPIO_TRIG_EN    BIT(2)  //!< GPIO wakeup (light sleep only)
#define RTC_TIMER_TRIG_EN   BIT(3)  //!< Timer wakeup

/**
 * @brief CPU frequency values
 */
typedef enum {
    RTC_CPU_FREQ_80M = 0,           //!< 80 MHz
    RTC_CPU_FREQ_160M = 1,          //!< 160 MHz
} rtc_cpu_freq_t;

/**
 * @brief Open RF hardware
 */
void phy_open_rf(void);

/**
 * @brief Close RF hardware
 */
void phy_close_rf(void);

/**
 * @brief Initialize RTC hardware
 */
void rtc_init_clk(uint8_t *init_param);

/**
 * @brief Initialize light sleep hardware
 */
void rtc_lightsleep_init(void);

/**
 * @brief Configure CPU sleep mode
 */
void pm_set_sleep_mode(uint32_t mode);

/**
 * @brief Initialize hardware when CPU wakes up from light sleep
 */
void rtc_wakeup_init(void);

/**
 * @brief Get the currently used CPU frequency configuration
 *
 * @return CPU frequency
 */
rtc_cpu_freq_t rtc_clk_cpu_freq_get(void);

/**
 * @brief Switch CPU frequency
 *
 * This function sets CPU frequency according to the given configuration
 * structure. It enables PLLs, if necessary.
 *
 * @note This function in not intended to be called by applications in FreeRTOS
 * environment. This is because it does not adjust various timers based on the
 * new CPU frequency.
 *
 * @param cpu_freq  CPU frequency
 */
void rtc_clk_cpu_freq_set(rtc_cpu_freq_t cpu_freq);

/**
 * @brief Enter light sleep mode
 * 
 * @note  CPU wakeup has 2672 ms time cost, so the real sleeping time is to_sleep_time_in_us - 2672
 *
 * @param wakeup_opt  bit mask wake up reasons to enable (RTC_xxx_TRIG_EN flags
 *                    combined with OR)
 * @param reject_opt  bit mask of sleep reject reasons:
 *                      - RTC_CNTL_GPIO_REJECT_EN
 *                      - RTC_CNTL_SDIO_REJECT_EN
 *                    These flags are used to prevent entering sleep when e.g.
 *                    an external host is communicating via SDIO slave
 * @return non-zero if sleep was rejected by hardware
 */
uint32_t rtc_light_sleep_start(uint32_t wakeup_opt, uint32_t reject_opt);
/**
 * @brief Convert time interval from microseconds to RTC_CLK cycles
 *
 * @param time_in_us  Time interval in microseconds
 * @param period      Period of clock in microseconds (as returned by esp_clk_cal_get)
 *
 * @return number of clock cycles
 */
uint32_t rtc_us_to_clk(uint32_t time_in_us, uint32_t period);

/**
 * @brief Convert time interval from RTC_CLK to microseconds
 *
 * @param rtc_cycles  Time interval in RTC_CLK cycles
 * @param period      Period of clock in microseconds (as returned by esp_clk_cal_get)
 *
 * @return time interval in microseconds
 */
uint32_t rtc_clk_to_us(uint32_t rtc_cycles, uint32_t period);

/**
 * @brief Get the calibration value of RTC clock
 *
 * @return the calibration value
 */
uint32_t pm_rtc_clock_cali_proc();

/**
 * @brief Configure   CPU sleep time by RTC clock ticks
 * 
 * @param rtc_cycles  Time interval in RTC_CLK cycles
 */
void pm_set_sleep_cycles(uint32_t rtc_cycles);

/**
 * @brief Get current value of RTC counter
 *
 * @return current value of RTC counter
 */
uint32_t rtc_time_get(void);

#ifdef __cplusplus
}
#endif

// Copyright 2018-2019 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RTC_SYS_RAM_SIZE            256

/**
 * The size of structure must not be larger than 256 bytes and all member varible must be uint32_t type
 */
struct _rtc_sys_info {
    uint32_t        hint;   // software reset reason
};

extern struct _rtc_sys_info rtc_sys_info;

/**
 * @brief  Internal function to get SoC reset reason at system initialization
 */
void esp_reset_reason_init(void);

/**
 * @brief  Internal function to set reset reason hint
 *
 * The hint is used do distinguish different reset reasons when software reset
 * is performed.
 *
 * The hint is stored in RTC store register, RTC_RESET_CAUSE_REG.
 *
 * @param hint  Desired esp_reset_reason_t value for the real reset reason
 */
void esp_reset_reason_set_hint(esp_reset_reason_t hint);

#ifdef __cplusplus
}
#endif

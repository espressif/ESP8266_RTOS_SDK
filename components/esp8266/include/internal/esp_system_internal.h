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
#include <esp_system.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RTC_SYS_RAM_SIZE            256

/**
 * @brief Station's AP base information of old SDK
 */
struct old_ap_ssid {
    uint32_t                    len;                    //!< SSID length
    uint8_t                     ssid[32];               //!< SSID data
    uint8_t                     passwd[64];             //!< password data
} __attribute__((packed));

/**
 * @brief System information of old SDK
 */
struct old_sysconf {
    uint8_t                     reserved_1[0x13C];      //!< reserved data
    uint8_t                     ap_number;              //!< number of stored AP
    uint8_t                     ap_index;               //!< index of current used AP
    uint8_t                     reserved_2[0x2];        //!< reserved data
    struct old_ap_ssid          ap_ssid[5];             //!< station's AP base information
} __attribute__((packed));

/**
 * The size of structure must not be larger than 256 bytes and all member varible must be uint32_t type
 */
struct _rtc_sys_info {
    uint32_t        hint;   // software reset reason
    uint32_t        old_sysconf_addr;   /*<! old SDK system configuration parameters base address, 
                                             if your bootloader is older than v3.2, please don't use this */
};

/**
 * @brief System information
 */
typedef struct esp_sys_info {
    uint32_t  version;                                  //!< system version
    uint32_t  reserved[3];                              //!< reserved data
} esp_sys_info_t;

_Static_assert(sizeof(esp_sys_info_t) == 16, "esp_sys_info_t should be 16 bytes");

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

/**
 * @brief  Get reason of last reset but not clear it for next reset
 *
 * @return See description of esp_reset_reason_t for explanation of each value.
 */
esp_reset_reason_t esp_reset_reason_early(void);

/**
 * @brief Get old SDK configuration parameters base address
 *
 * @return 0 if it is not upgraded from old SDK or the absolute address of the flash 
 */
uint32_t esp_get_old_sysconf_addr(void);

#ifdef __cplusplus
}
#endif

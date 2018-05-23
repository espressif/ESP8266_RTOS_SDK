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

#ifndef __ESP_SYSTEM_H__
#define __ESP_SYSTEM_H__

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CPU frequency values
 */
typedef enum {
    RTC_CPU_FREQ_80M = 1,       //!< 80 MHz
    RTC_CPU_FREQ_160M = 2,      //!< 160 MHz
} rtc_cpu_freq_t;

/**
 * @brief Switch CPU frequency
 *
 * If a PLL-derived frequency is requested (80, 160), this function
 * will enable the PLL. Otherwise, PLL will be disabled.
 * Note: this function is not optimized for switching speed. It may take several
 * hundred microseconds to perform frequency switch.
 *
 * @param cpu_freq  new CPU frequency
 */
void rtc_clk_cpu_freq_set(rtc_cpu_freq_t cpu_freq);

/**
  * @brief  Reset to default settings.
  */
void system_restore(void)  __attribute__ ((noreturn));

/**
  * @brief  Restart CPU.
  *
  * This function does not return.
  */
void esp_restart(void) __attribute__ ((noreturn));

/**
  * @brief  Get the size of available heap.
  *
  * Note that the returned value may be larger than the maximum contiguous block
  * which can be allocated.
  *
  * @return Available heap size, in bytes.
  */
uint32_t esp_get_free_heap_size(void);

/**
  * @brief Get the minimum heap that has ever been available
  *
  * @return Minimum free heap ever available
  */
uint32_t esp_get_minimum_free_heap_size( void );

/**
 * @brief  Get one random 32-bit word from hardware RNG
 *
 * @return Random value between 0 and UINT32_MAX
 */
uint32_t esp_random(void);

/**
 * Get IDF version
 *
 * @return constant string from IDF_VER
 */
const char* esp_get_idf_version(void);

typedef enum {
    FLASH_SIZE_4M_MAP_256_256 = 0,  /**<  Flash size : 4Mbits. Map : 256KBytes + 256KBytes */
    FLASH_SIZE_2M,                  /**<  Flash size : 2Mbits. Map : 256KBytes */
    FLASH_SIZE_8M_MAP_512_512,      /**<  Flash size : 8Mbits. Map : 512KBytes + 512KBytes */
    FLASH_SIZE_16M_MAP_512_512,     /**<  Flash size : 16Mbits. Map : 512KBytes + 512KBytes */
    FLASH_SIZE_32M_MAP_512_512,     /**<  Flash size : 32Mbits. Map : 512KBytes + 512KBytes */
    FLASH_SIZE_16M_MAP_1024_1024,   /**<  Flash size : 16Mbits. Map : 1024KBytes + 1024KBytes */
    FLASH_SIZE_32M_MAP_1024_1024,    /**<  Flash size : 32Mbits. Map : 1024KBytes + 1024KBytes */
    FLASH_SIZE_32M_MAP_2048_2048,    /**<  attention: don't support now ,just compatible for nodemcu;
                                           Flash size : 32Mbits. Map : 2048KBytes + 2048KBytes */
    FLASH_SIZE_64M_MAP_1024_1024,     /**<  Flash size : 64Mbits. Map : 1024KBytes + 1024KBytes */
    FLASH_SIZE_128M_MAP_1024_1024,    /**<  Flash size : 128Mbits. Map : 1024KBytes + 1024KBytes */

    FALSH_SIZE_MAP_MAX
} flash_size_map;

/**
 * @brief Chip models
 */
typedef enum {
    CHIP_ESP8266 = 1, //!< ESP8266
} esp_chip_model_t;

/**
 * Chip feature flags, used in esp_chip_info_t
 */
#define CHIP_FEATURE_WIFI_BGN       (1 << 0)

/**
 * @brief The structure represents information about the chip
 */
typedef struct {
    esp_chip_model_t model;  //!< chip model, one of esp_chip_model_t
    uint32_t features;       //!< bit mask of CHIP_FEATURE_x feature flags
    uint8_t cores;           //!< number of CPU cores
    uint8_t revision;        //!< chip revision number
} esp_chip_info_t;

/**
 * @brief Fill an esp_chip_info_t structure with information about the chip
 * @param[out] out_info structure to be filled
 */
void esp_chip_info(esp_chip_info_t* out_info);

/**
  * @brief  Get the current Flash size and Flash map.
  *
  *         Flash map depends on the selection when compiling, more details in document
  *         "2A-ESP8266__IOT_SDK_User_Manual"
  *
  * @param  null
  *
  * @return enum flash_size_map
  */
flash_size_map system_get_flash_size_map(void);

#ifdef __cplusplus
}
#endif

#endif /* __ESP_SYSTEM_H__ */

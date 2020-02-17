// Copyright 2020-2021 Espressif Systems (Shanghai) PTE LTD
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

#ifndef BOOTLOADER_BUILD
#include "esp_partition.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BOOTLOADER_BUILD
/**
 * @brief Setting target partition as fast boot partition and enable fast boot function.
 * 
 * @param partition partition which has image to be booted
 *
 * @return
 *      - 0 on success
 *      - -EINVAL parameter error
 *      - -EIO read flash error
 */
int esp_fast_boot_enable_partition(const esp_partition_t *partition);

/**
 * @brief Setting current running partition as fast boot partition and enable fast boot function.
 *
 * @return
 *      - 0 on success
 *      - -EINVAL current running partition information error
 *      - -EIO read flash error
 */
int esp_fast_boot_enable(void);

/**
 * @brief Directly running the image which is to be booted after restart.
 * 
 * @note It is just like jumping directly from one APP to another one without running ROM bootloader and level 2 bootloader.
 *       Using this API, system starting up is fastest.
 *
 * @return
 *      - 0 on success
 *      - -EINVAL booted partition information error
 *      - -EIO read flash error
 */
int esp_fast_boot_restart(void);
#endif

/**
 * @brief Disabling fast boot function and bootloader will not boot fast.
 */
void esp_fast_boot_disable(void);

/**
 * @brief Getting fast boot information.
 * 
 * @param image_start image startting address in the SPI Flash
 * @param image_size  image max size in the SPI Flash
 * @param image_entry image entry address in the SPI Flash
 * 
 * @return
 *      - 0 on success
 *      - -EINVAL fast boot information error
 */
int esp_fast_boot_get_info(uint32_t *image_start, uint32_t *image_size, uint32_t *image_entry);

/**
 * @brief Printing fast boot information.
 */
void esp_fast_boot_print_info(void);

#ifdef __cplusplus
}
#endif

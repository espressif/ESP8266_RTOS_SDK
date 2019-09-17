// Copyright 2018-2019 Espressif Systems (Shanghai) PTE LTD
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

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define _heap_caps_lock(_num)               \
{                                           \
    extern void vPortETSIntrLock(void);     \
    vPortETSIntrLock();                     \
}

#define _heap_caps_unlock(_num)             \
{                                           \
    extern void vPortETSIntrUnlock(void);   \
    vPortETSIntrUnlock();                   \
}

#define _heap_caps_feed_wdt(_num)           \
{                                           \
    extern void esp_task_wdt_reset(void);   \
    esp_task_wdt_reset();                   \
}

/**
 * @brief Get the total free size of DRAM region
 *
 * @return Amount of free bytes in DRAM region
 */
size_t heap_caps_get_dram_free_size(void);

#ifdef __cplusplus
}
#endif

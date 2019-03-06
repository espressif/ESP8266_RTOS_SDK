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

#define _heap_caps_lock(_num)               \
{                                           \
    extern int esp_wifi_is_sniffer(void);   \
    extern void vPortETSIntrLock(void);     \
    extern void vPortEnterCritical(void);   \
                                            \
    if (esp_wifi_is_sniffer())              \
        vPortETSIntrLock();                 \
    else                                    \
        vPortEnterCritical();               \
}

#define _heap_caps_unlock(_num)             \
{                                           \
    extern int esp_wifi_is_sniffer(void);   \
    extern void vPortETSIntrUnlock(void);   \
    extern void vPortExitCritical(void);    \
                                            \
    if (esp_wifi_is_sniffer())              \
        vPortETSIntrUnlock();               \
    else                                    \
        vPortExitCritical();                \
}

#define _heap_caps_feed_wdt(_num)           \
{                                           \
    extern void esp_task_wdt_reset(void);   \
    esp_task_wdt_reset();                   \
}


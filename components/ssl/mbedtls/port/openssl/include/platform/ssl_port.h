// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
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

#ifndef _SSL_PORT_H_
#define _SSL_PORT_H_

#include <string.h>
#include <stdio.h>

#include "esp_system.h"

#ifdef __cplusplus
 extern "C" {
#endif

#ifdef MEMLEAK_DEBUG

extern void *pvPortMalloc( size_t xWantedSize, const char * file, unsigned line, bool use_iram);
extern void *pvPortZalloc( size_t xWantedSize, const char * file, unsigned line);
extern void vPortFree(void *pv, const char * file, unsigned line);

#define ssl_mem_malloc(s)    \
    ({  \
        pvPortMalloc(s, __FILE__, __LINE__, false);  \
    })

#define ssl_mem_zalloc(s)    \
    ({  \
        pvPortZalloc(s, __FILE__, __LINE__);  \
    })

#define ssl_mem_free(s) \
do{\
    vPortFree(s, __FILE__, __LINE__);\
}while(0)


#else

extern void *pvPortMalloc( size_t xWantedSize );
extern void *pvPortZalloc( size_t xWantedSize );
extern void vPortFree(void *pv);

#define ssl_mem_zalloc(s) pvPortZalloc(s)
#define ssl_mem_malloc(s) pvPortMalloc(s)
#define ssl_mem_free(p) vPortFree(p)

#endif

#define ssl_memcpy memcpy
#define ssl_strlen strlen

#define ssl_speed_up_enter() rtc_clk_cpu_freq_set(RTC_CPU_FREQ_160M)
#define ssl_speed_up_exit()  rtc_clk_cpu_freq_set(RTC_CPU_FREQ_80M)

#define SSL_DEBUG_LOG printf

#endif


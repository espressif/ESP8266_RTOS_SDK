// Copyright 2019-2020 Espressif Systems (Shanghai) PTE LTD
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

#define ETS_INT_MASK    0x00003FFF
#define ESP_TICKS_MAX   UINT32_MAX

typedef uint32_t esp_tick_t;
typedef uint32_t esp_irqflag_t;

static inline esp_irqflag_t soc_save_local_irq(void)
{
    esp_irqflag_t flag;

    __asm__ __volatile__(
            "rsil   %0, 1\n"
            : "=a"(flag)
            :
            : "memory"
    );

    return flag;
}

static inline void soc_restore_local_irq(esp_irqflag_t flag)
{
    __asm__ __volatile__(
            "wsr    %0, ps\n"
            : 
            : "a"(flag)
            : "memory"
    );
}

static inline void soc_set_ccompare(uint32_t ticks)
{
    __asm__ __volatile__(
            "wsr    %0, ccompare0\n"
            : 
            : "a"(ticks)
            : "memory"
    );   
}

static inline uint32_t soc_get_ccompare(void)
{
    uint32_t ticks;

    __asm__ __volatile__(
            "rsr    %0, ccompare0\n"
            : "=a"(ticks)
            :
            : "memory"
    );

    return ticks;
}

static inline uint32_t soc_get_ccount(void)
{
    uint32_t ticks;

    __asm__ __volatile__(
            "rsr    %0, ccount\n"
            : "=a"(ticks)
            :
            : "memory"
    );

    return ticks;
}

static inline void soc_set_ccount(uint32_t ticks)
{
    __asm__ __volatile__(
            "wsr    %0, ccount\n"
            :
            : "a"(ticks)
            : "memory"
    );
}

static inline void soc_clear_int_mask(uint32_t mask)
{
    __asm__ __volatile__(
            "wsr    %0, intclear\n"
            : 
            : "a"(mask)
            : "memory"
    );   
}

static inline uint32_t soc_get_int_mask(void)
{
    uint32_t mask, enable;

    __asm__ __volatile__(
            "rsr    %0, interrupt\n"
            "rsr    %1, intenable\n"
            "rsync\n"
            : "=a"(mask), "=a"(enable)
            :
            : "memory"
    );

    return mask & enable & ETS_INT_MASK;
}

static inline void soc_wait_int(void)
{
    __asm__ __volatile__(
            "waiti  0\n"
            :
            :
            : "memory"
    );
}

#ifdef __cplusplus
}
#endif

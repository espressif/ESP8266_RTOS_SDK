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

#include <stddef.h>
#include "lwip/opt.h"
#include "lwip/memp.h"
#include "FreeRTOS.h"
#include "esp8266/eagle_soc.h"

#ifdef ESP_LWIP
/*
 * @brief allocate an only DRAM memory block for LWIP pbuf
 * 
 * @param s memory size
 * 
 * @return memory block pointer
 */
void *mem_malloc_ll(size_t s)
{
    void *return_addr = (void *)__builtin_return_address(0);

    return _heap_caps_malloc(s, MALLOC_CAP_8BIT, return_addr, 0);
}

void *memp_malloc_ll(size_t type)
{
    extern const struct memp_desc* const memp_pools[MEMP_MAX];

    uint8_t *p;
    const struct memp_desc *desc = memp_pools[type];

    p = (uint8_t *)mem_malloc_ll(MEMP_SIZE + MEMP_ALIGN_SIZE(desc->size));
    if (p)
        p += MEMP_SIZE;

    return p;
}

#endif

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

#include "esp_heap_caps.h"
#include "esp_attr.h"

#define HEAP_REGION_IRAM_MIN 512
#define HEAP_REGION_IRAM_MAX 0x00010000

heap_region_t g_heap_region[HEAP_REGIONS_MAX];

size_t IRAM_ATTR heap_caps_get_dram_free_size(void)
{
#ifndef CONFIG_HEAP_DISABLE_IRAM
    extern size_t g_heap_region_num;

    return g_heap_region[g_heap_region_num - 1].free_bytes;
#else
    return g_heap_region[0].free_bytes;
#endif
}

/**
 * @brief Initialize the capability-aware heap allocator.
 */
void heap_caps_init(void)
{
    extern char _bss_end;
    size_t heap_region_num = 0;

#ifndef CONFIG_HEAP_DISABLE_IRAM
    extern char _iram_end;
    const size_t iram_size = 0x40100000 + CONFIG_SOC_IRAM_SIZE - ((size_t)&_iram_end);

    if (iram_size > HEAP_REGION_IRAM_MIN && iram_size < HEAP_REGION_IRAM_MAX) {
        g_heap_region[heap_region_num].start_addr = (uint8_t *)&_iram_end;
        g_heap_region[heap_region_num].total_size = iram_size;
        g_heap_region[heap_region_num].caps = MALLOC_CAP_32BIT;
        heap_region_num++;
    }
#endif

    g_heap_region[heap_region_num].start_addr = (uint8_t *)&_bss_end;
    g_heap_region[heap_region_num].total_size = ((size_t)(0x40000000 - (uint32_t)&_bss_end));
    g_heap_region[heap_region_num].caps = MALLOC_CAP_8BIT | MALLOC_CAP_32BIT | MALLOC_CAP_DMA;
    heap_region_num++;

    esp_heap_caps_init_region(g_heap_region, heap_region_num);
}

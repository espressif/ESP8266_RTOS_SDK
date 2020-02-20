// Copyright 2019-2020 Espressif Systems (Shanghai) PTE LTD
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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/param.h>

#include <unity.h>

#include "esp_attr.h"
#include "esp8266/eagle_soc.h"

#include "esp_heap_caps.h"

#define MIXED_HEAP_LIST_NOTES 64

#define TIME_US_REG 0x3ff20c00

static inline uint32_t get_us(void)
{
    return REG_READ(TIME_US_REG);
}

static void* IRAM_ATTR test_alloc_time_in_us(size_t n, uint32_t *time)
{
    void *p;
    uint32_t t;

    t = get_us();

    p = heap_caps_malloc(n, MALLOC_CAP_8BIT);
    if (!p)
        return NULL;

    *time += get_us() - t;

    return p;
}

static void IRAM_ATTR test_free_time_in_us(void *p, uint32_t *time)
{
    uint32_t t;

    t = get_us();

    heap_caps_free(p);

    *time += get_us() - t;
}

static void IRAM_ATTR test_heap_effectivity(uint32_t *time, uint32_t *count)
{
    int cnt = 0;
    uint32_t alloc_us = 0, free_us = 0;

    for (; cnt < INT32_MAX; cnt++) { 
        void *p = test_alloc_time_in_us(cnt + sizeof(void *) + 1, &alloc_us);
        if (!p)
            break;
        test_free_time_in_us(p, &free_us);
    }

    time[0] = alloc_us;
    time[1] = free_us;
    count[0] = cnt;
}

static void test_heap_init(uint32_t *pbuf)
{
    for (int i = 0; i < MIXED_HEAP_LIST_NOTES; i++) {
        pbuf[i] = (uint32_t)heap_caps_malloc(1, MALLOC_CAP_8BIT);
        assert(pbuf[i]);
    }

    for (int i = 0; i < MIXED_HEAP_LIST_NOTES; i += 2) {
        heap_caps_free((void *)pbuf[i]);
        pbuf[i] = 0;
    }
}

static void test_heap_deinit(uint32_t *pbuf)
{
    for (int i = 0; i < MIXED_HEAP_LIST_NOTES; i++) {
        if (pbuf[i]) {
            heap_caps_free((void *)pbuf[i]);
            pbuf[i] = 0;
        }
    }
}

TEST_CASE("Test Heap alloc/free effectivity", "[Heap]")
{
    uint32_t time_in_us[2], count;
    uint32_t buf[MIXED_HEAP_LIST_NOTES];

    test_heap_init(buf);

    test_heap_effectivity(time_in_us, &count);

    printf("Each alloc costs time %u us, each free costs time %u us\n", time_in_us[0] / count, time_in_us[1] / count);

    test_heap_deinit(buf);
}

// Enable Heap Trace  :  Each alloc costs time 27 us, each free costs time 5 us
// Disable Heap Trace :  Each alloc costs time 18 us, each free costs time 4 us

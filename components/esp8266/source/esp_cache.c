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

#include "sdkconfig.h"
#include <stdint.h>
#include "esp_attr.h"
#include "esp8266/rom_functions.h"

#ifdef CONFIG_SOC_FULL_ICACHE
#define SOC_CACHE_SIZE 1 // 32KB
#else
#define SOC_CACHE_SIZE 0 // 16KB
#endif

static uint8_t s_cache_map;
static uint8_t s_cache_size = SOC_CACHE_SIZE;

void IRAM_ATTR Cache_Read_Enable_New(void)
{
    Cache_Read_Enable(s_cache_map, 0, s_cache_size);
}

void cache_init(int map)
{
    s_cache_map = map;

    Cache_Read_Enable_New();
}

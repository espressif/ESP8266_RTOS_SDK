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
#if MEMP_SIZE != 0
#error "MEMP_SIZE must be 0"
#else /* MEMP_SIZE != 0 */
size_t memp_malloc_get_size(size_t type)
{
    return memp_pools[type]->size;
}
#endif /* MEMP_SIZE != 0 */
#endif /* ESP_LWIP */

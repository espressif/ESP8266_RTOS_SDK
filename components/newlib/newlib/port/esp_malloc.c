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
#include <string.h>
#include <stdlib.h>
#include "FreeRTOS.h"

void *malloc(size_t n)
{
    void *return_addr = (void *)__builtin_return_address(0);

    return pvPortMalloc_trace(n, return_addr, (unsigned)-1, true);
}

void *realloc(void *old_ptr, size_t n)
{
    void *return_addr = (void *)__builtin_return_address(0);

    void *p = pvPortMalloc_trace(n, return_addr, (unsigned)-1, true);
    if (p && old_ptr) {
        memcpy(p, old_ptr, n);
        vPortFree_trace(old_ptr, return_addr, 0);
    }

    return p;
}

void *zalloc(size_t n)
{
    void *return_addr = (void *)__builtin_return_address(0);

    char *p = pvPortMalloc_trace(n, return_addr, (unsigned)-1, true);
    if (p)
        memset(p, 0, n);

    return p;
}

void *calloc(size_t c, size_t s)
{
    void *return_addr = (void *)__builtin_return_address(0);

    char *p = pvPortMalloc_trace(c * s, return_addr, (unsigned)-1, true);
    if (p)
        memset(p, 0, c * s);

    return p;
}

void free(void *ptr)
{
    void *return_addr = (void *)__builtin_return_address(0);

    vPortFree_trace(ptr, return_addr, (unsigned)-1);
}


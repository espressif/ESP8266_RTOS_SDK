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

    return _heap_caps_malloc(n, MALLOC_CAP_32BIT, return_addr, 0);
}

void *realloc(void *old_ptr, size_t n)
{
    void *return_addr = (void *)__builtin_return_address(0);

    return _heap_caps_realloc(old_ptr, n, MALLOC_CAP_32BIT, return_addr, 0);
}

void *zalloc(size_t n)
{
    void *return_addr = (void *)__builtin_return_address(0);

    return _heap_caps_zalloc(n, MALLOC_CAP_32BIT, return_addr, 0);
}

void *calloc(size_t c, size_t s)
{
    void *return_addr = (void *)__builtin_return_address(0);

    return _heap_caps_calloc(c, s, MALLOC_CAP_32BIT, return_addr, 0);
}

void free(void *ptr)
{
    void *return_addr = (void *)__builtin_return_address(0);

    _heap_caps_free(ptr, return_addr, 0);
}


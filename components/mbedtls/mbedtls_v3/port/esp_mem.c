/*
 * SPDX-FileCopyrightText: 2018-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <esp_attr.h>
#include <esp_heap_caps.h>
#include <sdkconfig.h>
#include "esp_mem.h"

#ifndef CONFIG_MBEDTLS_CUSTOM_MEM_ALLOC

IRAM_ATTR void *esp_mbedtls_mem_calloc(size_t n, size_t size)
{
    return calloc(n, size);
}

IRAM_ATTR void esp_mbedtls_mem_free(void *ptr)
{
    return heap_caps_free(ptr);
}

#endif /* !CONFIG_MBEDTLS_CUSTOM_MEM_ALLOC */

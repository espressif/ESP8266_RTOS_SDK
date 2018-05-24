/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __ESP_LIBC_H__
#define __ESP_LIBC_H__

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t os_random(void);
int32_t os_get_random(unsigned char *buf, size_t len);

#ifndef os_printf
#define os_printf   printf
#endif

/* Note: check_memleak_debug_enable is a weak function inside SDK.
 * please copy following codes to user_main.c.
#include "esp_libc.h"

bool check_memleak_debug_enable(void)
{
    return MEMLEAK_DEBUG_ENABLE;
}
*/

#ifndef MEMLEAK_DEBUG
#define MEMLEAK_DEBUG_ENABLE    0
#ifndef os_free
#define os_free(s)        free(s)
#endif

#ifndef os_malloc
#define os_malloc(s)      malloc(s)
#endif

#ifndef os_calloc
#define os_calloc(p, s)   calloc(p, s)
#endif

#ifndef os_realloc
#define os_realloc(p, s)  realloc(p, s)
#endif

#ifndef os_zalloc
#define os_zalloc(s)      zalloc(s)
#endif
#else
#define MEMLEAK_DEBUG_ENABLE    1

#ifndef os_free
#define os_free(s) \
do{\
    vPortFree_trace(s, __FILE__, __LINE__);\
}while(0)
#endif

#ifndef os_malloc
#define os_malloc(s)    \
    ({  \
        pvPortMalloc_trace(s, __FILE__, __LINE__, false);  \
    })
#endif

#ifndef os_malloc_iram
#define os_malloc_iram(s)    \
    ({  \
        pvPortMalloc_trace(s, __FILE__, __LINE__, true);  \
    })
#endif

#ifndef os_calloc
#define os_calloc(p, s)    \
    ({  \
        pvPortCalloc_trace(p, s, __FILE__, __LINE__);  \
    })
#endif

#ifndef os_realloc
#define os_realloc(p, s)    \
    ({  \
        pvPortRealloc_trace(p, s, __FILE__, __LINE__);  \
    })
#endif

#ifndef os_zalloc
#define os_zalloc(s)    \
    ({  \
        pvPortZalloc_trace(s, __FILE__, __LINE__);  \
    })
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif /* __LIBC_H__ */

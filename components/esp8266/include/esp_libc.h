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
#include <stdarg.h>
#include "rom/ets_sys.h"
#ifndef BOOTLOADER_BUILD
#include "esp_heap_caps.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

int32_t os_get_random(unsigned char *buf, size_t len);

/**
  * @brief  put a character to uart or other devices, similar with putc.
  *
  * @param  c : a character.
  *
  * @return int : the character written as an unsigned char cast to an int or EOF on error.
  */
int ets_putc(int c);

/**
  * @brief  Printf the strings to uart or other devices, similar with vprintf, simple than vprintf.
  *         Can not print float point data format, or longlong data format.
  *
  * @param  const char *fmt : See vprintf.
  *
  * @param  ap : parameter list, see vprintf.
  *
  * @return int : the length printed to the output device.
  */
int ets_vprintf(const char *fmt, va_list ap);

#ifndef os_printf
#define os_printf   printf
#endif

#ifndef os_free
#define os_free(s)        heap_caps_free(s)
#endif

#ifndef os_malloc
#define os_malloc(s)      heap_caps_malloc(s, MALLOC_CAP_32BIT)
#endif

#ifndef os_calloc
#define os_calloc(p, s)   heap_caps_calloc(p, s, MALLOC_CAP_32BIT)
#endif

#ifndef os_realloc
#define os_realloc(p, s)  heap_caps_realloc(p, s, MALLOC_CAP_32BIT)
#endif

#ifndef os_zalloc
#define os_zalloc(s)      heap_caps_zalloc(s, MALLOC_CAP_32BIT)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __LIBC_H__ */

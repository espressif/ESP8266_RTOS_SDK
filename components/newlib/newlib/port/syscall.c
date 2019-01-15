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

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "esp_libc.h"
#include "FreeRTOS.h"
#include "esp_log.h"

int _open_r(struct _reent *r, const char *filename, int flags, int mode)
{
    return 0;
}

_ssize_t _read_r(struct _reent *r, int fd, void *buf, size_t len)
{
    return 0;
}

_ssize_t _write_r(struct _reent *r, int fd, const void *buf, size_t len)
{
    int i;
    const char *cbuf = buf;

    for (i = 0; i < len; i++)
        ets_putc(cbuf[i]);

    return len;
}

_off_t _lseek_r(struct _reent *r, int fd, _off_t where, int whence)
{
    return 0;
}

int _close_r(struct _reent *r, int fd)
{
    return 0;
}

int _rename_r(struct _reent *r, const char *from, const char *to)
{
    return 0;
}

int _unlink_r(struct _reent *r, const char *filename)
{
    return 0;
}

int _fstat_r(struct _reent *r, int fd, struct stat *s)
{
    s->st_mode = S_IFCHR;

    return 0;
}

void *_sbrk_r(struct _reent *r, ptrdiff_t incr)
{
    return NULL;
}

void *_malloc_r(struct _reent *r, size_t n)
{
    void *return_addr = (void *)__builtin_return_address(0);

    return _heap_caps_malloc(n, MALLOC_CAP_32BIT, return_addr, 0);
}

void *_realloc_r(struct _reent *r, void *old_ptr, size_t n)
{
    void *return_addr = (void *)__builtin_return_address(0);

    return _heap_caps_realloc(old_ptr, n, MALLOC_CAP_32BIT, return_addr, 0);
}

void *_calloc_r(struct _reent *r, size_t c, size_t s)
{
    void *return_addr = (void *)__builtin_return_address(0);

    return _heap_caps_calloc(c, s, MALLOC_CAP_32BIT, return_addr, 0);
}

void _free_r(struct _reent *r, void *ptr)
{
    void *return_addr = (void *)__builtin_return_address(0);

    _heap_caps_free(ptr, return_addr, 0);
}

void _exit(int status)
{
    while (1);
}

void abort(void)
{
    ESP_LOGE("ABORT","Error found and abort!");

    /* cause a exception to jump into panic function */
    while (1) {
        *((int *)NULL) = 0;
    }
}
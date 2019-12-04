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
#include <sys/errno.h>

#include "esp_libc.h"
#include "FreeRTOS.h"
#include "esp_log.h"

#ifdef CONFIG_USING_ESP_VFS

#include "esp_vfs.h"

int _open_r(struct _reent *r, const char *filename, int flags, int mode)
{
    return esp_vfs_open(r, filename, flags, mode);
}

_ssize_t _read_r(struct _reent *r, int fd, void *buf, size_t len)
{
    return esp_vfs_read(r, fd, buf, len);
}

_ssize_t _write_r(struct _reent *r, int fd, const void *buf, size_t len)
{
    return esp_vfs_write(r, fd, buf, len);
}

_off_t _lseek_r(struct _reent *r, int fd, _off_t where, int whence)
{
    return esp_vfs_lseek(r, fd, where, whence);
}

int _close_r(struct _reent *r, int fd)
{
    return esp_vfs_close(r, fd);
}

int _rename_r(struct _reent *r, const char *from, const char *to)
{
    return esp_vfs_rename(r, from, to);
}

int _unlink_r(struct _reent *r, const char *filename)
{
    return esp_vfs_unlink(r, filename);
}

int _fstat_r(struct _reent *r, int fd, struct stat *s)
{
    return esp_vfs_fstat(r, fd, s);
}

int _stat_r(struct _reent *r, const char *path, struct stat *st)
{
    return esp_vfs_stat(r, path, st);
}

#else

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

int _stat_r(struct _reent *r, const char *path, struct stat *st)
{
    return 0;
}

#endif /* CONFIG_USING_ESP_VFS */

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

void abort(void)
{
#ifndef CONFIG_ESP_PANIC_SILENT_REBOOT
    ets_printf("abort() was called at PC %p on core %d\r\n", __builtin_return_address(0) - 3, xPortGetCoreID());
#endif

    /* cause a exception to jump into panic function */
    while (1) {
        *((int *)NULL) = 0;
    }
}

void _exit(int status)
{
    abort();
}

void *_sbrk_r(struct _reent *r, ptrdiff_t incr)
{
    abort();
}

int _getpid_r(struct _reent *r)
{
    __errno_r(r) = ENOSYS;
    return -1;
}

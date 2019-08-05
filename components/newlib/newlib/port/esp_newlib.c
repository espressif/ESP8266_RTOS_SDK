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
#include <reent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef CONFIG_USING_ESP_VFS
#include "esp_vfs_dev.h"
#endif

#define _STR(_s)    #_s
#define STR(_s)     _STR(_s)

/*
 * @brief Initialize global and thread's private reent object data. We add this instead of
 *        newlib's initialization function to avoid some unnecessary cost and unused function.
 */
void esp_reent_init(struct _reent* r)
{
    extern void _cleanup(struct _reent *r);

    memset(r, 0, sizeof(*r));

    r->_stdout = _GLOBAL_REENT->_stdout;
    r->_stderr = _GLOBAL_REENT->_stderr;
    r->_stdin  = _GLOBAL_REENT->_stdin;

    r->__cleanup = _cleanup;
    r->__sglue._next = NULL;
    r->__sglue._niobs = 0;
    r->__sglue._iobs = NULL;
    r->_current_locale = "C";
    r->__sdidinit = 1;
}

/*
 * @brief Initialize newlib's platform object data
 */
int esp_newlib_init(void)
{
    const char *default_uart_dev = "/dev/uart/" STR(CONFIG_CONSOLE_UART_NUM);

    esp_reent_init(_global_impure_ptr);

#ifdef CONFIG_USING_ESP_VFS
    esp_vfs_dev_uart_register();
#endif

    _GLOBAL_REENT->_stdout = fopen(default_uart_dev, "w");
    if (!_GLOBAL_REENT->_stdout)
        goto err;

    _GLOBAL_REENT->_stderr = fopen(default_uart_dev, "w");
    if (!_GLOBAL_REENT->_stderr)
        goto err_fail;

    _GLOBAL_REENT->_stdin = fopen(default_uart_dev, "r");
    if (!_GLOBAL_REENT->_stdin)
        goto err_in;

    environ = malloc(sizeof(char*));
    if (!environ)
        goto environ_in;
    environ[0] = NULL;

    return 0;

environ_in:
    fclose(_GLOBAL_REENT->_stdin);
err_in:
    fclose(_GLOBAL_REENT->_stderr);
err_fail:
    fclose(_GLOBAL_REENT->_stdout);
err:
    return -1;
}

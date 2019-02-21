// Copyright 2019-2020 Espressif Systems (Shanghai) PTE LTD
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

#define LOG_LOCAL_LEVEL ESP_LOG_NONE

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/errno.h>

#include "esp_log.h"
#include "nano_console.h"

#define TAG             "nano_console"
#define NC_READ_BUF     128

static SLIST_HEAD(_nc_cmd_head , _nc_cmd) s_nc_cmd = SLIST_HEAD_INITIALIZER(s_nc_cmd);

static inline nc_cmd_t *get_cmd(const char *buf)
{
    nc_cmd_t *it;

    SLIST_FOREACH(it, &s_nc_cmd, entries) {
        if (!strcmp(it->name, buf)) {
            ESP_LOGD(TAG, "%s finds command %s", __func__, buf);
            return it;
        }
    }

    ESP_LOGD(TAG, "%s has not found command %s", __func__, buf);

    return NULL;
}

static void free_args(uintptr_t *argv)
{
    int num = (int)argv[0];

    for (int i = 1; i < num; i++) {
        if (argv[i])
            free((void *)argv[i]);
        else {
            ESP_LOGD(TAG, "%s checks command input arguments number %d is empty", __func__, i);
        }
    }

    free(argv);
}

static uintptr_t *alloc_args(const char *buf, int num)
{
    const char *p = buf;
    uintptr_t *argv_buf;
    int cnt = 0;

    for (int i = 0; i < num; i++) {
        if (p[i] == '\0')
            cnt++;
    }

    argv_buf = calloc(1, (cnt + 1) * sizeof(uintptr_t));
    if (!argv_buf) {
        ESP_LOGE(TAG, "%s allocates memory for arguments table fail", __func__);
        return NULL;
    }
    argv_buf[0] = (uintptr_t)cnt + 1;

    for (int i = 0; i < cnt; i++) {
        int len = strlen(p) + 1;
        char *s = malloc(len);
        if (!s) {
            ESP_LOGE(TAG, "%s allocates memory for arguments %d fail", __func__, i);
            goto fail;
        }
        memcpy(s, p, len);

        p += len;
        argv_buf[i + 1] = (uintptr_t)s;
    }

    return argv_buf;

fail:
    free_args(argv_buf);
    return NULL;
}

static void *nc_thread_entry(void *p)
{
    int num = 0;
    char *pbuf;

    pbuf = malloc(NC_READ_BUF);
    if (!pbuf) {
        ESP_LOGE(TAG, "%s malloc %d bytes buffer fail", __func__, NC_READ_BUF);
        goto nomem;
    }

    while (1) {
        int ret;
        size_t rbytes;
        char c;

        rbytes = fread(&c, 1, 1, stdin);
        if (rbytes != 1) {
            ESP_LOGE(TAG, "%s read character fail", __func__);
            goto io_err;
        }

        if (num >= NC_READ_BUF - 1) {
            ESP_LOGD(TAG, "%s input stream overflows, reset the buffer", __func__);
            num = 0;
            continue;
        }

        if (!isascii(c)) {
            ESP_LOGD(TAG, "%s input character is not ASCII", __func__);
            continue;
        }

        if (c == '\r' || c == '\n') {
            nc_cmd_t *cmd;

#ifdef CONFIG_NC_ECHO_CMD
            ret = fwrite(&c, 1, 1, stdout);
            if (ret != 1) {
                ESP_LOGE(TAG, "%s %d write character fail %d", __func__, __LINE__, ret);
                goto io_err;                
            }
#endif

            if (!num) {
                ESP_LOGD(TAG, "%s gets command %s argument fail", __func__, cmd->name);
                continue;
            }
            
            if (pbuf[num - 1] != '\0')
                pbuf[num++] = '\0';

            cmd = get_cmd(pbuf);
            if (cmd) {
                uintptr_t *argv;

                argv = alloc_args(pbuf, num);
                if (!argv) {
                    ESP_LOGE(TAG, "%s gets command %s argument fail", __func__, cmd->name);
                    num = 0;
                    continue;
                }
                
                cmd->func((int)argv[0] - 1, (char **)(&argv[1]));

                free_args(argv);
            }

            num = 0;
        } else if (c == ' ') {
            if (num && pbuf[num] != ' ') {
                pbuf[num++] = '\0';
#ifdef CONFIG_NC_ECHO_CMD
                ret = fwrite(&c, 1, 1, stdout);
                if (ret != 1) {
                    ESP_LOGE(TAG, "%s %d write character fail %d", __func__, __LINE__, ret);
                    goto io_err;                
                }
#endif
            }
        } else if (c == 0x8 || c == 0x7f) {
            if (num) {
                num--;
#ifdef CONFIG_NC_ECHO_CMD
                char tmp[3] = {c, ' ', c};

                ret = fwrite(&tmp, 1, 3, stdout);
                if (ret != 3) {
                    ESP_LOGE(TAG, "%s %d write character fail %d", __func__, __LINE__, ret);
                    goto io_err;                
                }
#endif
            }
        } else {
            pbuf[num++] = c;
#ifdef CONFIG_NC_ECHO_CMD
            ret = fwrite(&c, 1, 1, stdout);
            if (ret != 1) {
                ESP_LOGE(TAG, "%s %d write character fail %d", __func__, __LINE__, ret);
                goto io_err;
            }
#endif
        }
    }

io_err:
    free(pbuf);
nomem:
    return (void *)(-ENOMEM);
}

/**
 * @brief Initialize nano console
 */
int nc_init(void)
{
    int ret;
    pthread_t tid;

    ret = pthread_create(&tid, NULL, nc_thread_entry, NULL);
    if (ret) {
        ESP_LOGE(TAG, "%s creates thread fail %d", __func__, ret);
        return -1;
    }

    return 0;
}

/**
 * @brief Register a command to nano console core
 */
int nc_register_cmd(nc_cmd_handle_t *handle, const char *name, nc_func_t func)
{
    int len;
    va_list ap;
    nc_cmd_t *cmd;

    cmd = malloc(sizeof(nc_cmd_t));
    if (!cmd) {
        ESP_LOGE(TAG, "%s alloc memory %d fail", __func__, __LINE__);
        return -ENOMEM;
    }

    len = strlen(name) + 1;
    cmd->name = malloc(len);
    if (!cmd->name) {
        ESP_LOGE(TAG, "%s alloc memory %d fail", __func__, __LINE__);
        goto nomem;
    }

    memcpy((char *)cmd->name, name, len);
    cmd->func = func;

    SLIST_INSERT_HEAD(&s_nc_cmd, cmd, entries);

    *handle = cmd;

    va_end(ap);

    return 0;

nomem:
    free(cmd);
    return -ENOMEM;
}

/**
 * @brief Output formated string through nano console I/O stream
 */
int nc_printf(const char *fmt, ...)
{
    va_list ap;
    char *pbuf;

    va_start(ap, fmt);

    int ret = vasprintf(&pbuf, fmt, ap);
    if (ret < 0)
        goto fail;

    ret = fwrite(pbuf, 1, ret, stdout);

    free(pbuf);

    va_end(ap);

    return ret;

fail:
    va_end(ap);
    return -ENOMEM;
}

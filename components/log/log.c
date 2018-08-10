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

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/lock.h>

#include "esp_libc.h"
#include "esp_attr.h"

#include "xtensa/hal.h"

#include "esp_log.h"

#ifdef CONFIG_LOG_COLORS
#define LOG_COLOR           "\033[0;%dm"
#define LOG_BOLD            "\033[1;%dm"
#define LOG_RESET_COLOR     "\033[0m"

static const uint32_t s_log_color[ESP_LOG_MAX] = {
    0,  //  ESP_LOG_NONE
    31, //  ESP_LOG_ERROR
    33, //  ESP_LOG_WARN
    32, //  ESP_LOG_INFO
    0,  //  ESP_LOG_DEBUG
    0,  //  ESP_LOG_VERBOSE
};
#endif

static const char s_log_prefix[ESP_LOG_MAX] = {
    'N', //  ESP_LOG_NONE
    'E', //  ESP_LOG_ERROR
    'W', //  ESP_LOG_WARN
    'I', //  ESP_LOG_INFO
    'D', //  ESP_LOG_DEBUG
    'V', //  ESP_LOG_VERBOSE
};

static uint32_t IRAM_ATTR esp_log_early_timestamp()
{
    return xthal_get_ccount() / (80 * 1000);
}

#ifndef BOOTLOADER_BUILD
static _lock_t s_lock;
static putchar_like_t s_putchar_func = &putchar;

static int esp_log_write_str(const char *s)
{
    int ret;

    do {
        ret = s_putchar_func(*s);
    } while (ret != EOF && *++s);

    return ret;
}

static uint32_t esp_log_timestamp()
{
    return clock() * (1000 / CLOCKS_PER_SEC) + esp_log_early_timestamp() % (1000 / CLOCKS_PER_SEC);
}
#endif

/**
 * @brief Write message into the log at system startup or critical state
 */
void IRAM_ATTR esp_early_log_write(esp_log_level_t level, const char *tag, const char *fmt, ...)
{
    va_list va;
    char prefix = level >= ESP_LOG_MAX ? 'N' : s_log_prefix[level];

#ifdef CONFIG_LOG_COLORS
    uint32_t color = level >= ESP_LOG_MAX ? 0 : s_log_color[level];

    if (color)
        ets_printf(LOG_COLOR, color);
#endif

    if (ets_printf("%c (%d) %s: ", prefix, esp_log_early_timestamp(), tag) < 0)
        goto out;

    va_start(va, fmt);
    ets_vprintf(fmt, va);
    va_end(va);

out:
#ifdef CONFIG_LOG_COLORS
    if (color)
        ets_printf(LOG_RESET_COLOR);
#endif
    ets_printf("\n");
}

#ifndef BOOTLOADER_BUILD
/**
 * @brief Write message into the log
 */
void esp_log_write(esp_log_level_t level, const char *tag,  const char *fmt, ...)
{
    int ret;
    va_list va;
    char *pbuf;
    char prefix = level >= ESP_LOG_MAX ? 'N' : s_log_prefix[level];

    _lock_acquire(&s_lock);
#ifdef CONFIG_LOG_COLORS
    static char buf[16];
    uint32_t color = level >= ESP_LOG_MAX ? 0 : s_log_color[level];

    if (color) {
        sprintf(buf, LOG_COLOR, color);
        ret = esp_log_write_str(buf);
        if (ret == EOF)
            goto exit;
    }
#endif

    ret = asprintf(&pbuf, "%c (%d) %s: ", prefix, esp_log_timestamp(), tag);
    if (ret < 0)
        goto out;
    ret = esp_log_write_str(pbuf);
    free(pbuf);
    if (ret == EOF)
        goto exit;

    va_start(va, fmt);
    ret = vasprintf(&pbuf, fmt, va);
    va_end(va);
    if (ret < 0)
        goto out;
    ret = esp_log_write_str(pbuf);
    free(pbuf);
    if (ret == EOF)
        goto exit;

out:
#ifdef CONFIG_LOG_COLORS
    if (color) {
        ret = esp_log_write_str(LOG_RESET_COLOR);
        if (ret == EOF)
            goto exit;
    }
#endif
    if (ret > 0)
        s_putchar_func('\n');

exit:
    _lock_release(&s_lock);
}

/**
 * @brief Set function used to output log entries
 */
putchar_like_t esp_log_set_putchar(putchar_like_t func)
{
    putchar_like_t tmp;

    _lock_acquire(&s_lock);
    tmp = s_putchar_func;
    s_putchar_func = func;
    _lock_release(&s_lock);

    return tmp;
}
#endif

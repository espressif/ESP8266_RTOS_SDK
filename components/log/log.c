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
#include <stdbool.h>
#include <string.h>
#include <sys/queue.h>
#include <sys/lock.h>

#include "esp_libc.h"
#include "esp_attr.h"

#include "xtensa/hal.h"

#include "esp_log.h"
#include "esp_system.h"

#ifndef BOOTLOADER_BUILD
#include "FreeRTOS.h"
#endif

#ifdef CONFIG_LOG_COLORS
#define LOG_COLOR_HEAD      "\033[0;%dm"
#define LOG_BOLD_HEAD       "\033[1;%dm"
#define LOG_COLOR_END       "\033[0m"

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

uint32_t IRAM_ATTR esp_log_early_timestamp()
{
#ifndef BOOTLOADER_BUILD
    extern uint64_t g_esp_os_us;
    extern uint32_t g_esp_boot_ccount;

    const uint32_t ticks_per_ms = g_esp_ticks_per_us * 1000;
    const uint32_t ms = g_esp_os_us / 1000 + g_esp_boot_ccount / ((CRYSTAL_USED * 2) * 1000);
#else
    const uint32_t ticks_per_ms = ((CRYSTAL_USED * 2) * 1000);
    const uint32_t ms = 0;
#endif

    return soc_get_ccount() / ticks_per_ms + ms;
}

#ifndef BOOTLOADER_BUILD

#ifdef CONFIG_LOG_SET_LEVEL
#define GLOBAL_TAG "*"

typedef struct uncached_tag_entry_{
    SLIST_ENTRY(uncached_tag_entry_) entries; 
    uint8_t level;  // esp_log_level_t as uint8_t
    char tag[0];    // beginning of a zero-terminated string
} uncached_tag_entry_t;

static esp_log_level_t s_global_tag_level = ESP_LOG_VERBOSE;
static SLIST_HEAD(log_tags_head , uncached_tag_entry_) s_log_uncached_tags = SLIST_HEAD_INITIALIZER(s_log_uncached_tags);
static uncached_tag_entry_t *s_uncached_tag_entry_prev;
#endif /* CONFIG_LOG_SET_LEVEL */

static _lock_t s_lock;
static putchar_like_t s_putchar_func = &putchar;

#ifdef CONFIG_LOG_SET_LEVEL
/**
 * @brief get entry by inputting tag
 */
static bool esp_log_get_tag_entry(const char *tag, uncached_tag_entry_t **entry)
{
    uncached_tag_entry_t *it = NULL;

    SLIST_FOREACH(it, &s_log_uncached_tags, entries) {
        if (!strcmp(it->tag, tag)) {
            //one tag in the linked list match, update the level
            *entry = it;
            //quit with it != NULL
            return true;
        }
    }

    return false;
}

static void clear_log_level_list(void)
{
    uncached_tag_entry_t *it = NULL;

    SLIST_FOREACH(it, &s_log_uncached_tags, entries) {
        SLIST_REMOVE(&s_log_uncached_tags, it, uncached_tag_entry_, entries);
        free(it);
    }
}

/**
 * @brief get level by inputting tag
 */
static esp_log_level_t esp_log_get_level(const char *tag)
{
    esp_log_level_t out_level;
    uncached_tag_entry_t *entry;

    _lock_acquire_recursive(&s_lock);

    if (s_uncached_tag_entry_prev && !strcmp(s_uncached_tag_entry_prev->tag, tag)) {
        out_level = (esp_log_level_t)s_uncached_tag_entry_prev->level;
        goto exit;
    }

    if (esp_log_get_tag_entry(tag, &entry) == true) {
        out_level = (esp_log_level_t)entry->level;
        s_uncached_tag_entry_prev = entry;
        goto exit;
    } else
        out_level = s_global_tag_level;

exit:
    _lock_release_recursive(&s_lock);
    return out_level;
}

/**
 * @brief check if system should output data
 */
static inline bool should_output(esp_log_level_t user_level, esp_log_level_t system_level)
{
    return user_level <= system_level;
}

/**
 * @brief set log level for given tag
 */
void esp_log_level_set(const char *tag, esp_log_level_t level)
{
    size_t bytes;
    uncached_tag_entry_t *entry, *new_entry;

    _lock_acquire_recursive(&s_lock);

    if (!strcmp(tag, GLOBAL_TAG)) {
        s_global_tag_level = level;
        clear_log_level_list();
        goto exit;
    }

    if (esp_log_get_tag_entry(tag, &entry) == true) {
        entry->level = level;
        goto exit;
    }

    bytes = strlen(tag) + 1;

    new_entry = malloc(sizeof(uncached_tag_entry_t) + bytes);
    if (!new_entry)
        goto exit;

    new_entry->level = level;
    memcpy(new_entry->tag, tag, bytes);

    SLIST_INSERT_HEAD(&s_log_uncached_tags, new_entry, entries);

exit:
    _lock_release_recursive(&s_lock);
}
#endif /* CONFIG_LOG_SET_LEVEL */

static int esp_log_write_str(const char *s)
{
    int ret;

    do {
        ret = s_putchar_func(*s);
    } while (ret != EOF && *++s);

    return ret;
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
        ets_printf(LOG_COLOR_HEAD, color);
#endif

    if (ets_printf("%c (%d) %s: ", prefix, esp_log_early_timestamp(), tag) < 0)
        goto out;

    va_start(va, fmt);
    ets_vprintf(fmt, va);
    va_end(va);

out:
#ifdef CONFIG_LOG_COLORS
    if (color)
        ets_printf(LOG_COLOR_END);
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
    char prefix;

    _lock_acquire_recursive(&s_lock);

#ifdef CONFIG_LOG_SET_LEVEL
    if (!should_output(level, esp_log_get_level(tag)))
        goto exit;
#endif

#ifdef CONFIG_LOG_COLORS
    static char buf[16];
    uint32_t color = level >= ESP_LOG_MAX ? 0 : s_log_color[level];

    if (color) {
        sprintf(buf, LOG_COLOR_HEAD, color);
        ret = esp_log_write_str(buf);
        if (ret == EOF)
            goto exit;
    }
#endif
    prefix = level >= ESP_LOG_MAX ? 'N' : s_log_prefix[level];
    ret = asprintf(&pbuf, "%c (%d) %s: ", prefix, esp_log_early_timestamp(), tag);
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
        ret = esp_log_write_str(LOG_COLOR_END);
        if (ret == EOF)
            goto exit;
    }
#endif
    if (ret > 0)
        s_putchar_func('\n');

exit:
    _lock_release_recursive(&s_lock);
}

/**
 * @brief Set function used to output log entries
 */
putchar_like_t esp_log_set_putchar(putchar_like_t func)
{
    putchar_like_t tmp;

    _lock_acquire_recursive(&s_lock);
    tmp = s_putchar_func;
    s_putchar_func = func;
    _lock_release_recursive(&s_lock);

    return tmp;
}
#endif

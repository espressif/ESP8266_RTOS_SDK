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

#include <stdint.h>
#include <reent.h>
#include <time.h>
#include <sys/times.h>
#include <sys/time.h>
#include <sys/errno.h>

#include "esp_system.h"
#include "esp_timer.h"
#include "rom/ets_sys.h"

#include "FreeRTOS.h"
#include "task.h"
#include "driver/soc.h"

#include "sdkconfig.h"

#ifdef CONFIG_ESP8266_TIME_SYSCALL_USE_FRC1
#define WITH_FRC 1
#endif

static uint64_t s_boot_time;

static inline void set_boot_time(uint64_t time_us)
{
    esp_irqflag_t flag;

    flag = soc_save_local_irq();
    s_boot_time = time_us;
    soc_restore_local_irq(flag);
}

static inline uint64_t get_boot_time()
{
    uint64_t result;
    esp_irqflag_t flag;

    flag = soc_save_local_irq();
    result = s_boot_time;
    soc_restore_local_irq(flag);

    return result;
}

int _gettimeofday_r(struct _reent* r, struct timeval* tv, void* tz)
{
    (void) tz;

    if (tv) {
        uint64_t microseconds = get_boot_time() + (uint64_t)esp_timer_get_time();
        tv->tv_sec = microseconds / 1000000;
        tv->tv_usec = microseconds % 1000000;
    }

    return 0;
}

int settimeofday(const struct timeval* tv, const struct timezone* tz)
{
    (void) tz;

    if (tv) {
        uint64_t now = ((uint64_t) tv->tv_sec) * 1000000LL + tv->tv_usec;
        uint64_t since_boot = (uint64_t)esp_timer_get_time();
        set_boot_time(now - since_boot);
    }

    return 0;
}

clock_t _times_r(struct _reent *r, struct tms *tms)
{
    tms->tms_utime = xTaskGetTickCount();
    tms->tms_stime = 0;
    tms->tms_cutime = 0;
    tms->tms_cstime = 0;

    return 0;
}

int usleep(useconds_t us)
{
    const int us_per_tick = portTICK_PERIOD_MS * 1000;

    if (us < us_per_tick) {
        ets_delay_us((uint32_t) us);
    } else {
        /* since vTaskDelay(1) blocks for anywhere between 0 and portTICK_PERIOD_MS,
         * round up to compensate.
         */
        vTaskDelay((us + us_per_tick - 1) / us_per_tick);
    }

    return 0;
}

unsigned int sleep(unsigned int seconds)
{
    vTaskDelay(seconds * (1000 / portTICK_PERIOD_MS));

    return 0;
}

int clock_gettime (clockid_t clock_id, struct timespec *tp)
{
#if defined( WITH_FRC ) || defined( WITH_RTC )
    if (tp == NULL) {
        errno = EINVAL;
        return -1;
    }
    struct timeval tv;
    uint64_t monotonic_time_us = 0;
    switch (clock_id) {
        case CLOCK_REALTIME:
            _gettimeofday_r(NULL, &tv, NULL);
            tp->tv_sec = tv.tv_sec;
            tp->tv_nsec = tv.tv_usec * 1000L;
            break;
        case CLOCK_MONOTONIC:
#if defined( WITH_FRC )
            monotonic_time_us = (uint64_t) esp_timer_get_time();
#elif defined( WITH_RTC )
            monotonic_time_us = get_rtc_time_us();
#endif // WITH_FRC
            tp->tv_sec = monotonic_time_us / 1000000LL;
            tp->tv_nsec = (monotonic_time_us % 1000000LL) * 1000L;
            break;
        default:
            errno = EINVAL;
            return -1;
    }
    return 0;
#else
    errno = ENOSYS;
    return -1;
#endif
}

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

#include "limits.h"
#include "sdkconfig.h"

#ifdef CONFIG_ESP8266_TIME_SYSCALL_USE_FRC1
#define WITH_FRC 1
#endif

static uint64_t s_boot_time;

#if defined(WITH_RTC) || defined(WITH_FRC)
// stores the start time of the slew
static uint64_t adjtime_start = 0;
// is how many microseconds total to slew
static int64_t adjtime_total_correction = 0;
#define ADJTIME_CORRECTION_FACTOR 6
static uint64_t get_time_since_boot(void);
#endif
// Offset between FRC timer and the RTC.
// Initialized after reset or light sleep.
#if defined(WITH_RTC) && defined(WITH_FRC)
uint64_t s_microseconds_offset;
#endif

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

// This function gradually changes boot_time to the correction value and immediately updates it.
static uint64_t adjust_boot_time(void)
{
    uint64_t boot_time = get_boot_time();
    if ((boot_time == 0) || (get_time_since_boot() < adjtime_start)) {
        adjtime_start = 0;
    }
    if (adjtime_start > 0) {
        uint64_t since_boot = get_time_since_boot();
        // If to call this function once per second, then (since_boot - adjtime_start) will be 1_000_000 (1 second),
        // and the correction will be equal to (1_000_000us >> 6) = 15_625 us.
        // The minimum possible correction step can be (64us >> 6) = 1us.
        // Example: if the time error is 1 second, then it will be compensate for 1 sec / 0,015625 = 64 seconds.
        int64_t correction = (since_boot >> ADJTIME_CORRECTION_FACTOR) - (adjtime_start >> ADJTIME_CORRECTION_FACTOR);
        if (correction > 0) {
            adjtime_start = since_boot;
            if (adjtime_total_correction < 0) {
                if ((adjtime_total_correction + correction) >= 0) {
                    boot_time = boot_time + adjtime_total_correction;
                    adjtime_start = 0;
                } else {
                    adjtime_total_correction += correction;
                    boot_time -= correction;
                }
            } else {
                if ((adjtime_total_correction - correction) <= 0) {
                    boot_time = boot_time + adjtime_total_correction;
                    adjtime_start = 0;
                } else {
                    adjtime_total_correction -= correction;
                    boot_time += correction;
                }
            }
            set_boot_time(boot_time);
        }
    }
    return boot_time;
}

#if defined( WITH_FRC ) || defined( WITH_RTC )
static uint64_t get_time_since_boot(void)
{
    uint64_t microseconds = 0;
#ifdef WITH_FRC
#ifdef WITH_RTC
    microseconds = s_microseconds_offset + esp_timer_get_time();
#else
    microseconds = esp_timer_get_time();
#endif // WITH_RTC
#elif defined(WITH_RTC)
    microseconds = get_rtc_time_us();
#endif // WITH_FRC
    return microseconds;
}
#endif // defined( WITH_FRC ) || defined( WITH_RTC

int adjtime(const struct timeval *delta, struct timeval *outdelta)
{
#if defined( WITH_FRC ) || defined( WITH_RTC )
    esp_irqflag_t flag;
    if(delta != NULL){
        int64_t sec  = delta->tv_sec;
        int64_t usec = delta->tv_usec;
        if(llabs(sec) > ((INT_MAX / 1000000L) - 1L)) {
            return -1;
        }
        /*
        * If adjusting the system clock by adjtime () is already done during the second call adjtime (),
        * and the delta of the second call is not NULL, the earlier tuning is stopped,
        * but the already completed part of the adjustment is not canceled.
        */
        flag = soc_save_local_irq();
        // If correction is already in progress (adjtime_start != 0), then apply accumulated corrections.
        adjust_boot_time();
        adjtime_start = get_time_since_boot();
        adjtime_total_correction = sec * 1000000L + usec;
        soc_restore_local_irq(flag);
    }
    if(outdelta != NULL){
        flag = soc_save_local_irq();
        adjust_boot_time();
        if (adjtime_start != 0) {
            outdelta->tv_sec    = adjtime_total_correction / 1000000L;
            outdelta->tv_usec   = adjtime_total_correction % 1000000L;
        } else {
            outdelta->tv_sec    = 0;
            outdelta->tv_usec   = 0;
        }
        soc_restore_local_irq(flag);
    }
  return 0;
#else
  return -1;
#endif

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

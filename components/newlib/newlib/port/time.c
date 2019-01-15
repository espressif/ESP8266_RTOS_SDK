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
#include <sys/times.h>
#include <sys/time.h>

#include "esp_system.h"
#include "esp_timer.h"
#include "rom/ets_sys.h"

#include "FreeRTOS.h"
#include "task.h"

extern uint32_t esp_get_time();

static uint64_t s_boot_time;

static os_timer_t microsecond_overflow_timer;
static uint32_t microsecond_last_overflow_tick;
static uint32_t microsecond_overflow_count;

static bool microsecond_overflow_timer_start_flag = false;

static void microsecond_overflow_tick(void* arg)
{
    uint32_t m = esp_get_time();

    vPortEnterCritical();

    if (m < microsecond_last_overflow_tick) {
        microsecond_overflow_count ++;
    }

    microsecond_last_overflow_tick = m;

    vPortExitCritical();
}

/* Start a timer which period is 60s to check the overflow of microsecond. */
static void microsecond_overflow_set_check_timer(void)
{
    if (microsecond_overflow_timer_start_flag == false) {
        os_timer_disarm(&microsecond_overflow_timer);
        os_timer_setfn(&microsecond_overflow_timer, (os_timer_func_t*)microsecond_overflow_tick, 0);
        os_timer_arm(&microsecond_overflow_timer, 60 * 1000, 1);

        microsecond_overflow_timer_start_flag = true;
    }
}

static void set_boot_time(uint64_t time_us)
{
    vPortEnterCritical();

    s_boot_time = time_us;

    vPortExitCritical();
}

static uint64_t get_boot_time()
{
    uint64_t result;

    vPortEnterCritical();

    result = s_boot_time;

    vPortExitCritical();

    return result;
}

static uint64_t get_time_since_boot()
{
    uint32_t m;
    uint32_t c;
    uint64_t microseconds;

    m = esp_get_time();

    vPortEnterCritical();

    c = microsecond_overflow_count + ((m < microsecond_last_overflow_tick) ? 1 : 0);

    vPortExitCritical();

    microseconds = c * (1LL << 32) + m;

    return microseconds;
}

int _gettimeofday_r(struct _reent* r, struct timeval* tv, void* tz)
{
    (void) tz;

    /* ToDo: This can be moved to system start up. */
    microsecond_overflow_set_check_timer();

    if (tv) {
        uint64_t microseconds = get_boot_time() + get_time_since_boot();
        tv->tv_sec = microseconds / 1000000;
        tv->tv_usec = microseconds % 1000000;
    }

    return 0;
}

int settimeofday(const struct timeval* tv, const struct timezone* tz)
{
    (void) tz;

    microsecond_overflow_set_check_timer();

    if (tv) {
        uint64_t now = ((uint64_t) tv->tv_sec) * 1000000LL + tv->tv_usec;
        uint64_t since_boot = get_time_since_boot();
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

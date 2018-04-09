// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
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

#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "espos_timer.h"
#include "espos_scheduler.h"

#define portYIELD_FROM_ISR() taskYIELD()

typedef struct espos_timer_os {
    /* FreeRTOS handle */
    TimerHandle_t       timer;

    /* ESPOS timer callback function */
    espos_timer_cb_t    cb;

    /* ESPOS timer private parameter*/
    void                *priv;
} espos_timer_os_t;

/**
 * @brief FreeRTOS callback function
 */
static void os_timer_callback(TimerHandle_t xTimer)
{
    espos_timer_os_t *os_timer =
        (espos_timer_os_t *)pvTimerGetTimerID(xTimer);
    espos_timer_t timer = (espos_timer_t)os_timer;

    os_timer->cb(timer, os_timer->priv);
}

/**
 * @brief create a timer
 */
esp_err_t espos_timer_create (
    espos_timer_t *timer,
    const char *name,
    espos_timer_cb_t cb,
    void *arg,
    espos_tick_t period_ticks,
    espos_opt_t opt
)
{
    espos_timer_os_t *os_timer;
    UBaseType_t auto_load;
    esp_err_t ret;

    if (!timer || !name || !cb || !period_ticks) {
        return -EINVAL;
    }

    if (opt == ESPOS_TIMER_AUTO_RUN) {
        auto_load = pdTRUE;
    } else if (opt == ESPOS_TIMER_NO_AUTO_RUN) {
        auto_load = pdFALSE;
    } else {
        return -EINVAL;
    }

    if (espos_in_isr() == true) {
        return -EINTR;
    }

    os_timer = malloc(sizeof(espos_timer_os_t));
    if (!os_timer) {
        return -ENOMEM;
    }

    os_timer->timer = xTimerCreate(name, period_ticks, auto_load, os_timer, os_timer_callback);
    if (os_timer->timer) {
        ret = 0;

        os_timer->cb = cb;
        os_timer->priv = arg;

        *timer = (espos_timer_t)os_timer;
    } else {
        ret = -ENOMEM;

        free(os_timer);
    }

    return ret;
}

/**
 * @brief start a timer
 */
esp_err_t espos_timer_start (
    espos_timer_t timer
)
{
    esp_err_t ret;
    espos_timer_os_t *os_timer = (espos_timer_os_t *)timer;

    if (!os_timer) {
        return -EINVAL;
    }

    if (espos_in_isr() == true) {
        BaseType_t xHigherPrioritTaskWoken = pdFALSE;

        ret = xTimerStartFromISR(os_timer->timer, &xHigherPrioritTaskWoken);
        if (pdTRUE == ret && pdTRUE == xHigherPrioritTaskWoken) {
            portYIELD_FROM_ISR();
        }

        if (ret == pdTRUE) {
            ret = 0;
        } else {
            ret = -ECANCELED;
        }
    } else {
        ret = xTimerStart(os_timer->timer, ESPOS_MAX_DELAY);
        if (ret == pdTRUE) {
            ret = 0;
        } else {
            ret = -ECANCELED;
        }
    }

    return ret;
}

/**
 * @brief stop a timer
 */
esp_err_t espos_timer_stop (
    espos_timer_t timer
)
{
    esp_err_t ret;
    espos_timer_os_t *os_timer = (espos_timer_os_t *)timer;

    if (!os_timer) {
        return -EINVAL;
    }

    if (espos_in_isr() == true) {
        BaseType_t xHigherPrioritTaskWoken = pdFALSE;

        ret = xTimerStopFromISR(os_timer->timer, &xHigherPrioritTaskWoken);
        if (pdTRUE == ret && pdTRUE == xHigherPrioritTaskWoken) {
            portYIELD_FROM_ISR();
        }

        if (ret == pdTRUE) {
            ret = 0;
        } else {
            ret = -ECANCELED;
        }
    } else {
        ret = xTimerStop(os_timer->timer, ESPOS_MAX_DELAY);
        if (ret == pdTRUE) {
            ret = 0;
        } else {
            ret = -ECANCELED;
        }
    }

    return ret;
}

/**
 * @brief configure a timer
 */
esp_err_t espos_timer_change (
    espos_timer_t timer,
    espos_opt_t opt,
    void *arg
)
{
    esp_err_t ret;
    espos_timer_os_t *os_timer = (espos_timer_os_t *)timer;

    if (!os_timer) {
        return -EINVAL;
    }

    if (ESPOS_TIMER_CHANGE_PERIOD == opt) {
        TickType_t period = (TickType_t)arg;

        if (espos_in_isr() == true) {
            BaseType_t xHigherPrioritTaskWoken = pdFALSE;

            ret = xTimerChangePeriodFromISR(os_timer->timer, period, &xHigherPrioritTaskWoken);
            if (pdTRUE == ret && pdTRUE == xHigherPrioritTaskWoken) {
                portYIELD_FROM_ISR();
            }

            if (ret == pdTRUE) {
                ret = 0;
            } else {
                ret = -ECANCELED;
            }
        } else {
            ret = xTimerChangePeriod(os_timer->timer, period, ESPOS_MAX_DELAY);
            if (ret == pdTRUE) {
                ret = 0;
            } else {
                ret = -ECANCELED;
            }
        }
    } else if (ESPOS_TIMER_CHANGE_ONCE == opt) {
        return -EINVAL;
    } else if (ESPOS_TIMER_CHANGE_AUTO == opt) {
        return -EINVAL;
    } else {
        return -EINVAL;
    }

    return -ret;
}

/**
 * @brief delete a timer
 */
esp_err_t espos_timer_del (
    espos_timer_t timer
)
{
    esp_err_t ret;
    espos_timer_os_t *os_timer = (espos_timer_os_t *)timer;

    if (!os_timer) {
        return -EINVAL;
    }

    if (espos_in_isr() == true) {
        return -EINTR;
    }

    ret = xTimerDelete(os_timer->timer, ESPOS_MAX_DELAY);
    if (ret == pdTRUE) {
        ret = 0;
        free(os_timer);
    } else {
        ret = -ECANCELED;
    }

    return ret;
}

/**
 * @brief get current timer name point
 */
esp_err_t espos_get_timer_name (
    espos_timer_t timer,
    char **tname
)
{
    espos_timer_os_t *os_timer = (espos_timer_os_t *)timer;

    if (!os_timer) {
        return -EINVAL;
    }

    *tname = NULL;

    return -ENODEV;
}


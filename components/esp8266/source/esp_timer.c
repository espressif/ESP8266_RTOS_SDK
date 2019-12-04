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

#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/soc.h"

#define ESP_TIMER_HZ CONFIG_FREERTOS_HZ

typedef enum {
    ESP_TIMER_INIT = 0,
    ESP_TIMER_ONCE,
    ESP_TIMER_CYCLE,
    ESP_TIMER_STOP,
    ESP_TIMER_DELETE,
} esp_timer_state_t;

struct esp_timer {
    TimerHandle_t       os_timer;

    esp_timer_cb_t      cb;

    void                *arg;

    esp_timer_state_t   state;
};

static const char *TAG = "esp_timer";

static esp_err_t delete_timer(esp_timer_handle_t timer)
{
    BaseType_t ret = xTimerDelete(timer->os_timer, portMAX_DELAY);
    if (ret == pdPASS)
        heap_caps_free(timer);

    return ret == pdPASS ? ESP_OK : ESP_ERR_NO_MEM;
}

/**
 * @brief FreeRTOS callback function
 */
static void esp_timer_callback(TimerHandle_t xTimer)
{
    struct esp_timer *timer = (struct esp_timer *)pvTimerGetTimerID(xTimer);

    timer->cb(timer->arg);

    switch (timer->state) {
        case ESP_TIMER_INIT:
        case ESP_TIMER_STOP:
            break;
        case ESP_TIMER_CYCLE: {
            BaseType_t ret = xTimerReset(timer->os_timer, portMAX_DELAY);
            if (ret != pdPASS) {
                ESP_LOGE(TAG, "start timer at callback error");
            } else {
                ESP_LOGD(TAG, "start timer at callback OK");
            }
            break;
        }
        case ESP_TIMER_ONCE: {
            BaseType_t ret = xTimerStop(timer->os_timer, portMAX_DELAY);
            if (ret != pdPASS) {
                ESP_LOGE(TAG, "stop timer at callback error");
            } else {
                timer->state = ESP_TIMER_STOP;
                ESP_LOGD(TAG, "stop timer at callback OK");
            }
            break;
        }
        case ESP_TIMER_DELETE: {
            esp_err_t ret = delete_timer(timer);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "delete timer at callback error");
            } else {
                ESP_LOGD(TAG, "delete timer at callback OK");
            }
            break;
        }
        default:
            ESP_LOGE(TAG, "timer state error is %d", timer->state);
            break;
    }
}

/**
 * @brief Initialize esp_timer library
 */
esp_err_t esp_timer_init(void)
{
    return ESP_OK;
}

/**
 * @brief De-initialize esp_timer library
 */
esp_err_t esp_timer_deinit(void)
{
    return ESP_OK;
}

/**
 * @brief Create an esp_timer instance
 */
esp_err_t esp_timer_create(const esp_timer_create_args_t* create_args,
                           esp_timer_handle_t* out_handle)
{
    assert(create_args);
    assert(out_handle);

    TimerHandle_t os_timer;
    esp_timer_handle_t esp_timer;

    esp_timer = heap_caps_malloc(sizeof(struct esp_timer), MALLOC_CAP_32BIT);
    if (!esp_timer)
        return ESP_ERR_NO_MEM;

    os_timer = xTimerCreate(create_args->name,
                            portMAX_DELAY,
                            pdFALSE,
                            esp_timer,
                            esp_timer_callback);
    if (os_timer) {
        esp_timer->os_timer = os_timer;
        esp_timer->cb = create_args->callback;
        esp_timer->arg = create_args->arg;
        esp_timer->state = ESP_TIMER_INIT;
        *out_handle = (esp_timer_handle_t)esp_timer;
    } else {
        heap_caps_free(esp_timer);
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

/**
 * @brief Start one-shot timer
 */
esp_err_t esp_timer_start_once(esp_timer_handle_t timer, uint64_t timeout_us)
{
    assert(timer);

    TimerHandle_t os_timer = timer->os_timer;
    BaseType_t os_ret;
    uint64_t last_us = timeout_us % (1000 * (1000 / ESP_TIMER_HZ));
    uint32_t ticks = timeout_us / (1000 * (1000 / ESP_TIMER_HZ));

    if (last_us || !ticks)
        return ESP_ERR_INVALID_ARG;

    os_ret = xTimerChangePeriod(os_timer, ticks, portMAX_DELAY);
    if (os_ret == pdPASS) {
        timer->state = ESP_TIMER_ONCE;
    } else {
        return ESP_ERR_INVALID_STATE;
    }

    return ESP_OK;
}

/**
 * @brief Start a periodic timer
 */
esp_err_t esp_timer_start_periodic(esp_timer_handle_t timer, uint64_t period)
{
    assert(timer);

    TimerHandle_t os_timer = timer->os_timer;
    BaseType_t os_ret;
    uint64_t last_us = period % (1000 * (1000 / ESP_TIMER_HZ));
    uint32_t ticks = period / (1000 * (1000 / ESP_TIMER_HZ));

    if (last_us || !ticks)
        return ESP_ERR_INVALID_ARG;

    os_ret = xTimerChangePeriod(os_timer, ticks, portMAX_DELAY);
    if (os_ret == pdPASS) {
        timer->state = ESP_TIMER_CYCLE;
    } else {
        return ESP_ERR_INVALID_STATE;
    }

    return ESP_OK;
}

/**
 * @brief Stop the timer
 */
esp_err_t esp_timer_stop(esp_timer_handle_t timer)
{
    assert(timer);

    TimerHandle_t os_timer = timer->os_timer;
    BaseType_t os_ret;

    os_ret = xTimerStop(os_timer, portMAX_DELAY);
    if (os_ret == pdPASS)
        timer->state = ESP_TIMER_STOP;

    return os_ret == pdPASS ? ESP_OK : ESP_ERR_INVALID_STATE;   
}

/**
 * @brief Delete an esp_timer instance
 */
esp_err_t esp_timer_delete(esp_timer_handle_t timer)
{
    esp_err_t ret;

    assert(timer);

    if (xTimerGetTimerDaemonTaskHandle() == xTaskGetCurrentTaskHandle()) {
        timer->state = ESP_TIMER_DELETE;
        ret = ESP_OK;
    } else {
        UBaseType_t prio = uxTaskPriorityGet(NULL);
        if (prio >= configTIMER_TASK_PRIORITY)
            vTaskPrioritySet(NULL, configTIMER_TASK_PRIORITY - 1);
        else
            prio = 0;

        ret = delete_timer(timer);

        if (prio)
            vTaskPrioritySet(NULL, prio);
    }

    return ret;
}

int64_t esp_timer_get_time(void)
{
    extern uint64_t g_esp_os_us;

    return (int64_t)(g_esp_os_us + soc_get_ccount() / g_esp_ticks_per_us);
}

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

#define ESP_TIMER_HZ CONFIG_FREERTOS_HZ

struct esp_timer {
    TimerHandle_t       os_timer;

    esp_timer_cb_t      cb;

    void                *arg;

    TickType_t          period_ticks;
};

static const char *TAG = "esp_timer";

/**
 * @brief FreeRTOS callback function
 */
static void esp_timer_callback(TimerHandle_t xTimer)
{
    BaseType_t os_ret;
    struct esp_timer *timer = (struct esp_timer *)pvTimerGetTimerID(xTimer);

    timer->cb(timer->arg);

    if (!timer->period_ticks) {
        os_ret = xTimerStop(timer->os_timer, 0);
        if (os_ret != pdPASS) {
            ESP_LOGE(TAG, "Set timer from periodic to once error");
        }
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

    esp_timer->cb = create_args->callback;
    esp_timer->arg = create_args->arg;
    esp_timer->period_ticks = 0;

    os_timer = xTimerCreate(create_args->name,
                            portMAX_DELAY,
                            pdTRUE,
                            esp_timer,
                            esp_timer_callback);
    if (os_timer) {
        esp_timer->os_timer = os_timer;
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
        TickType_t period_ticks = timer->period_ticks;

        timer->period_ticks = 0;
        os_ret = xTimerStart(os_timer, portMAX_DELAY);
        if (os_ret != pdPASS) {
            timer->period_ticks = period_ticks;
            return ESP_ERR_INVALID_STATE;
        }
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
        TickType_t period_ticks = timer->period_ticks;

        timer->period_ticks = ticks;
        os_ret = xTimerStart(os_timer, portMAX_DELAY);
        if (os_ret != pdPASS) {
            timer->period_ticks = period_ticks;
            return ESP_ERR_INVALID_STATE;
        }
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

    return os_ret == pdPASS ? ESP_OK : ESP_ERR_INVALID_STATE;   
}

/**
 * @brief Delete an esp_timer instance
 */
esp_err_t esp_timer_delete(esp_timer_handle_t timer)
{
    assert(timer);

    TimerHandle_t os_timer = timer->os_timer;
    BaseType_t os_ret;

    os_ret = xTimerDelete(os_timer, portMAX_DELAY);
    if (os_ret == pdPASS)
        heap_caps_free(timer);

    return os_ret == pdPASS ? ESP_OK : ESP_ERR_INVALID_STATE; 
}

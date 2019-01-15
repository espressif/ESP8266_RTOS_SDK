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
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "espos_mutex.h"
#include "espos_scheduler.h"

/* real ESPOS mutex structure */
typedef struct espos_mutex_os {
    espos_opt_t         opt;
    SemaphoreHandle_t   mutex;
} espos_mutex_os_t;

/**
 * @brief create a mutex
 */
esp_err_t espos_mutex_create (
    espos_mutex_t *mutex,
    espos_mutex_type_opt_t opt
)
{
    esp_err_t ret;
    espos_mutex_os_t *os_mutex;

    if (!mutex || (ESPOS_MUTEX_TYPE(opt) >= ESPOS_MUTEX_TYPE_MAX)) {
        return -EINVAL;
    }

    if (espos_in_isr() == true) {
        return -EINTR;
    }

    os_mutex = malloc(sizeof(espos_mutex_os_t));
    if (!os_mutex) {
        return -ENOMEM;
    }

    switch (ESPOS_MUTEX_TYPE(opt)) {
    case ESPOS_MUTEX_NORMAL:
        os_mutex->mutex = xSemaphoreCreateMutex();
        break;
    case ESPOS_MUTEX_RECURSIVE:
        os_mutex->mutex = xSemaphoreCreateRecursiveMutex();
        break;
    default :
        os_mutex->mutex = NULL;
        break;
    }

    if (os_mutex->mutex) {
        ret = 0;
        os_mutex->opt = opt;
        *mutex = (espos_mutex_t)os_mutex;
    } else {
        free(os_mutex);
        ret = -ENOMEM;
    }

    return ret;
}

/**
 * @brief set a mutex name
 */
esp_err_t espos_mutex_set_name(espos_mutex_t mutex, const char *name)
{
    return 0;
}

/**
 * @brief lock a mutex
 */
esp_err_t espos_mutex_lock (
    espos_mutex_t mutex,
    espos_tick_t wait_ticks
)
{
    esp_err_t ret;
    espos_mutex_os_t *os_mutex = (espos_mutex_os_t *)mutex;

    if (!os_mutex) {
        return -EINVAL;
    }

    if (espos_in_isr() == true) {
        return -EINTR;
    }

    switch (ESPOS_MUTEX_TYPE(os_mutex->opt)) {
    case ESPOS_MUTEX_NORMAL:
        ret = xSemaphoreTake(os_mutex->mutex, wait_ticks);
        break;
    case ESPOS_MUTEX_RECURSIVE:
        ret = xSemaphoreTakeRecursive(os_mutex->mutex, wait_ticks);
        break;
    default:
        ret = -EINVAL;
        break;
    }

    if (ret == pdTRUE) {
        ret = 0;
    } else {
        ret = -ETIMEDOUT;
    }

    return ret;
}

/**
 * @brief unlock a mutex
 */
esp_err_t espos_mutex_unlock (
    espos_mutex_t mutex
)
{
    esp_err_t ret;
    espos_mutex_os_t *os_mutex = (espos_mutex_os_t *)mutex;

    if (!os_mutex) {
        return -EINVAL;
    }

    if (espos_in_isr() == true) {
        return -EINTR;
    }

    if (xSemaphoreGetMutexHolder(os_mutex->mutex) != xTaskGetCurrentTaskHandle()) {
        return -EPERM;
    }

    switch (ESPOS_MUTEX_TYPE(os_mutex->opt)) {
    case ESPOS_MUTEX_NORMAL:
        ret = xSemaphoreGive(os_mutex->mutex);
        break;
    case ESPOS_MUTEX_RECURSIVE:
        ret = xSemaphoreGiveRecursive(os_mutex->mutex);
        break;
    default:
        ret = -EINVAL;
        break;
    }

    if (ret == pdTRUE) {
        ret = 0;
    } else {
        ret = -EPERM;
    }

    return ret;
}

/**
 * @brief get task handle whick lock the mutex
 */
espos_task_t espos_mutex_get_holder (
    espos_mutex_t mutex
)
{
    espos_task_t tmp;
    espos_mutex_os_t *os_mutex = (espos_mutex_os_t *)mutex;

    if (!os_mutex) {
        return -EINVAL;
    }

    tmp = (espos_task_t)xSemaphoreGetMutexHolder(os_mutex->mutex);

    return tmp;
}

/**
 * @brief delete the mutex
 */
esp_err_t espos_mutex_del (
    espos_mutex_t mutex
)
{
    espos_mutex_os_t *os_mutex = (espos_mutex_os_t *)mutex;

    if (!os_mutex) {
        return -EINVAL;
    }

    if (espos_in_isr() == true) {
        return -EINTR;
    }

    vSemaphoreDelete(os_mutex->mutex);
    free(os_mutex);

    return 0;
}


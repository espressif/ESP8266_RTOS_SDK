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

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "espos_semaphore.h"
#include "espos_scheduler.h"

#define portYIELD_FROM_ISR() taskYIELD()

/**
 * @brief create a semaphore
 */
esp_err_t espos_sem_create (
    espos_sem_t *sem,
    espos_sem_count_t max_count,
    espos_sem_count_t init_count
)
{
    esp_err_t ret;
    SemaphoreHandle_t os_mutex;

    if (!sem || !max_count) {
        return -EINVAL;
    }

    if (espos_in_isr() == true) {
        return -EINTR;
    }

    os_mutex = xSemaphoreCreateCounting(max_count, init_count);
    if (os_mutex) {
        ret = 0;
        *sem = (espos_sem_t)os_mutex;
    } else {
        ret = -ENOMEM;
    }

    return ret;
}

/**
 * @brief set a semaphore name
 */
esp_err_t espos_sem_set_name(espos_sem_t sem, const char *name)
{
    return 0;
}

/**
 * @brief take semaphore
 */
esp_err_t espos_sem_take (
    espos_sem_t sem,
    espos_tick_t wait_ticks
)
{
    esp_err_t ret;
    SemaphoreHandle_t os_sem = (SemaphoreHandle_t)sem;

    if (!os_sem) {
        return -EINVAL;
    }

    if (espos_in_isr() == true) {
        BaseType_t xHigherPrioritTaskWoken = pdFALSE;

        ret = xSemaphoreTakeFromISR(os_sem, &xHigherPrioritTaskWoken);
        if (pdPASS == ret && pdTRUE == xHigherPrioritTaskWoken) {
            portYIELD_FROM_ISR();
        }

        if (ret == pdPASS) {
            ret = 0;
        } else {
            ret = -ETIMEDOUT;
        }
    } else {
        ret = xSemaphoreTake(os_sem, wait_ticks);
        if (ret == pdPASS) {
            ret = 0;
        } else {
            ret = -ETIMEDOUT;
        }
    }

    return ret;
}

/**
 * @brief give up semaphore
 */
esp_err_t espos_sem_give (
    espos_sem_t sem
)
{
    BaseType_t ret;
    SemaphoreHandle_t os_sem = (SemaphoreHandle_t)sem;

    if (!os_sem) {
        return -EINVAL;
    }

    if (espos_in_isr() == true) {
        BaseType_t xHigherPrioritTaskWoken = pdFALSE;

        ret = xSemaphoreGiveFromISR(os_sem, &xHigherPrioritTaskWoken);
        if (pdPASS == ret && pdTRUE == xHigherPrioritTaskWoken) {
            portYIELD_FROM_ISR();
        }

        if (ret == pdPASS) {
            ret = 0;
        } else {
            ret = -EAGAIN;
        }
    } else {
        ret = xSemaphoreGive(os_sem);
        if (ret == pdPASS) {
            ret = 0;
        } else {
            ret = -EAGAIN;
        }
    }

    return ret;
}

/**
 * @brief delete the semaphore
 */
esp_err_t espos_sem_del (
    espos_sem_t sem
)
{
    SemaphoreHandle_t os_sem = (SemaphoreHandle_t)sem;

    if (!os_sem) {
        return -EINVAL;
    }

    if (espos_in_isr() == true) {
        return -EINTR;
    }

    vSemaphoreDelete(os_sem);

    return 0;
}


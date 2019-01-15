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
#include "freertos/task.h"

#include "espos_task.h"
#include "espos_scheduler.h"

#define portYIELD_FROM_ISR() taskYIELD()

/**
 * @brief create a task
 */
esp_err_t espos_task_create_on_cpu (
    espos_task_t *task,
    const char *name,
    void *arg,
    espos_prio_t prio,
    espos_tick_t ticks,
    espos_size_t stack_size,
    espos_task_entry_t entry,
    espos_task_opt_t opt,
    espos_cpu_t cpu_id
)
{
    BaseType_t ret;
    TaskHandle_t os_task;

    if (!task || !name || !stack_size || !entry) {
        return -EINVAL;
    }

    if (espos_in_isr() == true) {
        return -EINTR;
    }

    ret = xTaskCreate(entry, name, stack_size, arg, prio, &os_task);
    if (ret == pdPASS) {
        ret = 0;

        *task = (espos_task_t)os_task;
    } else {
        ret = -ENOMEM;
    }

    return ret;
}

/**
 * @brief delete a task
 */
esp_err_t espos_task_del (
    espos_task_t task
)
{
    TaskHandle_t os_task = (TaskHandle_t)task;

    if (espos_in_isr() == true) {
        return -EINTR;
    }

    vTaskDelete(os_task);

    return 0;
}

/**
 * @brief let current task sleep for some ticks
 */
esp_err_t espos_task_delay (
    const espos_tick_t delay_ticks
)
{
    vTaskDelay(delay_ticks);

    return 0;
}

/**
 * @brief suspend target task
 */
esp_err_t espos_task_suspend (
    espos_task_t task
)
{
    TaskHandle_t os_task = (TaskHandle_t)task;

    if (!os_task) {
        return -EINVAL;
    }

    vTaskSuspend(os_task);

    return 0;
}

/**
 * @brief resume target task
 */
esp_err_t espos_task_resume (
    espos_task_t task
)
{
    esp_err_t ret;
    TaskHandle_t os_task = (TaskHandle_t)task;

    if (!os_task) {
        return -EINVAL;
    }

    if (espos_in_isr() == true) {
        ret = xTaskResumeFromISR(os_task);
        if (ret == pdTRUE) {
            ret = 0;
            portYIELD_FROM_ISR();
        } else {
            ret = -ECANCELED;
        }

    } else {
        vTaskResume(os_task);
        ret = 0;
    }

    return 0;
}

/**
 * @brief yield the cpu once
 */
esp_err_t espos_task_yield(void)
{
    taskYIELD();

    return 0;
}

/**
 * @brief get current task handle
 */
espos_task_t espos_task_get_current(void)
{
    return (espos_task_t)xTaskGetCurrentTaskHandle();
}

/**
 * @brief get current task name point
 */
esp_err_t espos_task_get_name (
    espos_task_t task,
    char **pname
)
{
    TaskHandle_t os_task = (TaskHandle_t)task;

    if (!os_task) {
        return -EINVAL;
    }

    *pname = (char *)pcTaskGetTaskName(os_task);

    return 0;
}

#if 0

/**
 * @brief set task private data
 */
esp_err_t espos_task_set_private_data (
    espos_task_t task,
    int idx,
    void *info
)
{
    TaskHandle_t os_task = (TaskHandle_t)task;

    if (!os_task) {
        return -EINVAL;
    }

    vTaskSetThreadLocalStoragePointer(os_task, idx, info);

    return 0;
}

/**
 * @brief get task private data
 */
esp_err_t espos_task_get_private_data (
    espos_task_t task,
    int idx,
    void **info
)
{
    TaskHandle_t os_task = (TaskHandle_t)task;

    if (!os_task || !info) {
        return -EINVAL;
    }

    *info = pvTaskGetThreadLocalStoragePointer(os_task, idx);

    return 0;
}

/**
 * @brief get cpu affinity of task
 */
espos_cpu_t espos_task_get_affinity(espos_task_t task)
{
    TaskHandle_t os_task = (TaskHandle_t)task;

    if (!os_task) {
        return -EINVAL;
    }

    return xTaskGetAffinity(os_task);
}

#endif

/**
 * @brief get ESPOS task priority number
 */
espos_size_t espos_task_prio_num(void)
{
    return configMAX_PRIORITIES + 1;
}

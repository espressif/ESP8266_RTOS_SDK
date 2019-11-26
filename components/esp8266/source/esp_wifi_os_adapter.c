// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
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

#include <string.h>

#include "esp_libc.h"
#include "esp_system.h"
#include "esp_attr.h"
#include "esp_wifi_osi.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

#include "nvs.h"

void *__wifi_task_create(void *task_func, const char *name, uint32_t stack_depth, void *param, uint32_t prio)
{
    portBASE_TYPE ret;
    xTaskHandle handle;
    ret = xTaskCreate(task_func, name, stack_depth, param, prio, &handle);

    return ret == pdPASS ? handle : NULL;
}

void __wifi_task_delete(void *task)
{
    vTaskDelete(task);
}

void __wifi_task_yield_from_isr(void)
{
    portYIELD();
}

void __wifi_task_delay(uint32_t tick)
{
    vTaskDelay(tick);
}

uint32_t __wifi_task_get_max_priority(void)
{
    return (uint32_t)(configMAX_PRIORITIES);
}

uint32_t __wifi_task_ms_to_ticks(uint32_t ms)
{
    return (uint32_t)(ms / portTICK_RATE_MS);
}

void __wifi_task_suspend_all(void)
{
    vTaskSuspendAll();
}

void __wifi_task_resume_all(void)
{
    xTaskResumeAll();
}

void *__wifi_queue_create(uint32_t queue_len, uint32_t item_size)
{
    return (void *)xQueueCreate(queue_len, item_size);
}

void __wifi_queue_delete(void *queue)
{
    vQueueDelete(queue);
}

int __wifi_queue_send(void *queue, void *item, uint32_t block_time_tick, uint32_t pos)
{
    signed portBASE_TYPE ret;
    BaseType_t os_pos;

    if (pos == OSI_QUEUE_SEND_BACK)
        os_pos = queueSEND_TO_BACK;
    else if (pos == OSI_QUEUE_SEND_FRONT)
        os_pos = queueSEND_TO_FRONT;
    else
        os_pos = queueOVERWRITE;

    if (block_time_tick == OSI_FUNCS_TIME_BLOCKING) {
        ret = xQueueGenericSend(queue, item, portMAX_DELAY, os_pos);
    } else {
        ret = xQueueGenericSend(queue, item, block_time_tick, os_pos);
    }

    return ret == pdPASS ? true : false;
}

int __wifi_queue_send_from_isr(void *queue, void *item, int *hptw)
{
    signed portBASE_TYPE ret;

    ret = xQueueSendFromISR(queue, item, (signed portBASE_TYPE *)hptw);

    return ret == pdPASS ? true : false;
}

int __wifi_queue_recv(void *queue, void *item, uint32_t block_time_tick)
{
    signed portBASE_TYPE ret;

    if (block_time_tick == OSI_FUNCS_TIME_BLOCKING) {
        ret = xQueueReceive(queue, item, portMAX_DELAY);
    } else {
        ret = xQueueReceive(queue, item, block_time_tick);
    }

    return ret == pdPASS ? true : false;
}

void *__wifi_timer_create(const char *name, uint32_t period_ticks, bool auto_load, void *arg, void (*cb)(void *timer))
{
    return xTimerCreate(name, period_ticks, auto_load, arg, (tmrTIMER_CALLBACK)cb);
}

int __wifi_timer_reset(void *timer, uint32_t ticks)
{
    return xTimerReset(timer, ticks);
}

int __wifi_timer_stop(void *timer, uint32_t ticks)
{
    return xTimerStop(timer, ticks);
}

void *__wifi_task_top_sp(void)
{
    extern uint32_t **pxCurrentTCB;

    return pxCurrentTCB[0];
}

void* __wifi_semphr_create(uint32_t max, uint32_t init)
{
    return (void*)xSemaphoreCreateCounting(max, init);
}

void __wifi_semphr_delete(void* semphr)
{
    vSemaphoreDelete(semphr);
}

int32_t __wifi_semphr_take(void* semphr, uint32_t block_time_tick)
{
    if (block_time_tick == OSI_FUNCS_TIME_BLOCKING) {
        return (int32_t)xSemaphoreTake(semphr, portMAX_DELAY);
    } else {
        return (int32_t)xSemaphoreTake(semphr, block_time_tick);
    }
}

int32_t __wifi_semphr_give(void* semphr)
{
    return (int32_t)xSemaphoreGive(semphr);
}
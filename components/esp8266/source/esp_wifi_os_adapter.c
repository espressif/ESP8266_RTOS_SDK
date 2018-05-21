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

#include "esp_wifi_os_adapter.h"
#include "esp_system.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

#if defined(CONFIG_NEWLIB_LIBRARY_LEVEL_NORMAL) || defined(CONFIG_NEWLIB_LIBRARY_LEVEL_NANO)
#include "esp_newlib.h"
#endif

static uint32_t enter_critical_wrapper(void)
{
    taskENTER_CRITICAL();

    return 0;
}

static void exit_critical_wrapper(uint32_t tmp)
{
    taskEXIT_CRITICAL();
}

static void *task_create_wrapper(void *task_func, const char *name, uint32_t stack_depth, void *param, uint32_t prio)
{
    portBASE_TYPE ret;
    xTaskHandle handle;

    ret = xTaskCreate(task_func, name, stack_depth, param, prio, &handle);

    return ret == pdPASS ? handle : NULL;
}

static void task_delete_wrapper(void *task_handle)
{
    vTaskDelete(task_handle);
}

static void task_yield_wrapper(void)
{
    portYIELD();
}

static void task_yield_from_isr_wrapper(void)
{
    portYIELD();
}

static void task_delay_wrapper(uint32_t tick)
{
    vTaskDelay(tick);
}

static void* task_get_current_task_wrapper(void)
{
    return (void *)xTaskGetCurrentTaskHandle();
}

static uint32_t task_get_max_priority_wrapper(void)
{
    return (uint32_t)(configMAX_PRIORITIES);
}

static uint32_t task_ms_to_tick_wrapper(uint32_t ms)
{
    return (uint32_t)(ms / portTICK_RATE_MS);
}

static void task_suspend_all_wrapper(void)
{
    vTaskSuspendAll();
}

static void task_resume_all_wrapper(void)
{
    xTaskResumeAll();
}

static void os_init_wrapper(void)
{
#if defined(CONFIG_NEWLIB_LIBRARY_LEVEL_NORMAL) || defined(CONFIG_NEWLIB_LIBRARY_LEVEL_NANO)
    esp_newlib_init();
#endif
}

static void os_start_wrapper(void)
{
    vTaskStartScheduler();
}

static void *semphr_create_wrapper(uint32_t max, uint32_t init)
{
    return (void *)xSemaphoreCreateCounting(max, init);
}

static void semphr_delete_wrapper(void *semphr)
{
    vSemaphoreDelete(semphr);
}

static bool semphr_take_from_isr_wrapper(void *semphr, int *hptw)
{
    signed portBASE_TYPE ret;

    ret = xSemaphoreTakeFromISR(semphr, (signed portBASE_TYPE *)hptw);
    
    return ret == pdPASS ? true : false;
}

static bool semphr_give_from_isr_wrapper(void *semphr, int *hptw)
{
    signed portBASE_TYPE ret;

    ret = xSemaphoreGiveFromISR(semphr, (signed portBASE_TYPE *)hptw);

    return ret == pdPASS ? true : false;
}

static bool semphr_take_wrapper(void *semphr, uint32_t block_time_tick)
{
    signed portBASE_TYPE ret;

    if (block_time_tick == OSI_FUNCS_TIME_BLOCKING) {
        ret = xSemaphoreTake(semphr, portMAX_DELAY);
    } else {
        ret = xSemaphoreTake(semphr, block_time_tick);
    }

    return ret == pdPASS ? true : false;
}

static bool semphr_give_wrapper(void *semphr)
{
    signed portBASE_TYPE ret;

    ret = xSemaphoreGive(semphr);

    return ret == pdPASS ? true : false;
}

static void *mutex_create_wrapper(void)
{
    return (void *)xSemaphoreCreateRecursiveMutex();
}

static void mutex_delete_wrapper(void *mutex)
{
    vSemaphoreDelete(mutex);
}

static bool mutex_lock_wrapper(void *mutex)
{
    signed portBASE_TYPE ret;

    ret = xSemaphoreTakeRecursive(mutex, portMAX_DELAY);

    return ret == pdPASS ? true : false;
}

static bool mutex_unlock_wrapper(void *mutex)
{
    signed portBASE_TYPE ret;

    ret = xSemaphoreGiveRecursive(mutex);

    return ret == pdPASS ? true : false;
}

static void *queue_create_wrapper(uint32_t queue_len, uint32_t item_size)
{
    return (void *)xQueueCreate(queue_len, item_size);
}

static void queue_delete_wrapper(void *queue)
{
    vQueueDelete(queue);
}

static bool queue_send_wrapper(void *queue, void *item, uint32_t block_time_tick, uint32_t pos)
{
    signed portBASE_TYPE ret;

    if (block_time_tick == OSI_FUNCS_TIME_BLOCKING) {
        ret = xQueueGenericSend(queue, item, portMAX_DELAY, pos);
    } else {
        ret = xQueueGenericSend(queue, item, block_time_tick, pos);
    }

    return ret == pdPASS ? true : false;
}

static bool queue_send_from_isr_wrapper(void *queue, void *item, int *hptw)
{
    signed portBASE_TYPE ret;

    ret = xQueueSendFromISR(queue, item, (signed portBASE_TYPE *)hptw);

    return ret == pdPASS ? true : false;
}

static bool queue_recv_wrapper(void *queue, void *item, uint32_t block_time_tick)
{
    signed portBASE_TYPE ret;

    if (block_time_tick == OSI_FUNCS_TIME_BLOCKING) {
        ret = xQueueReceive(queue, item, portMAX_DELAY);
    } else {
        ret = xQueueReceive(queue, item, block_time_tick);
    }

    return ret == pdPASS ? true : false;
}

static bool queue_recv_from_isr_wrapper(void *queue, void *item, int *hptw)
{
    signed portBASE_TYPE ret;

    ret = xQueueReceiveFromISR(queue, item, (signed portBASE_TYPE *)hptw);

    return ret == pdPASS ? true : false;
}

static uint32_t queue_msg_waiting_wrapper(void *queue)
{
    return (uint32_t)uxQueueMessagesWaiting(queue);
}

static uint32_t get_free_heap_size_wrapper(void)
{
    return (uint32_t)system_get_free_heap_size();
}

static void *timer_create_wrapper(const char *name, uint32_t period_ticks, bool auto_load, void *arg, void (*cb)(void *timer))
{
    return xTimerCreate(name, period_ticks, auto_load, arg, (tmrTIMER_CALLBACK)cb);
}

static void *timer_get_arg_wrapper(void *timer)
{
    return pvTimerGetTimerID(timer);
}

static bool timer_reset_wrapper(void *timer, uint32_t ticks)
{
    return xTimerReset(timer, ticks);
}

static bool timer_stop_wrapper(void *timer, uint32_t ticks)
{
    return xTimerStop(timer, ticks);
}

static bool timer_delete_wrapper(void *timer, uint32_t ticks)
{
    return xTimerDelete(timer, ticks);
}

static void srand_wrapper(uint32_t seed)
{
    /* empty function */
}

static int32_t rand_wrapper(void)
{
    return (int32_t)os_random();
}

wifi_osi_funcs_t s_wifi_osi_funcs = {
    .enter_critical = enter_critical_wrapper,
    .exit_critical = exit_critical_wrapper,
    
    .task_create = task_create_wrapper,
    .task_delete = task_delete_wrapper,
    .task_yield = task_yield_wrapper,
    .task_yield_from_isr = task_yield_from_isr_wrapper,
    .task_delay = task_delay_wrapper,
    .task_get_current_task = task_get_current_task_wrapper,
    .task_get_max_priority = task_get_max_priority_wrapper,
    
    .task_ms_to_tick = task_ms_to_tick_wrapper,

    .task_suspend_all = task_suspend_all_wrapper,
    .task_resume_all = task_resume_all_wrapper,

    .os_init = os_init_wrapper,
    .os_start = os_start_wrapper,
    
    .semphr_create = semphr_create_wrapper,
    .semphr_delete = semphr_delete_wrapper,
    .semphr_take_from_isr = semphr_take_from_isr_wrapper,
    .semphr_give_from_isr = semphr_give_from_isr_wrapper,
    .semphr_take = semphr_take_wrapper,
    .semphr_give = semphr_give_wrapper,
    
    .mutex_create = mutex_create_wrapper,
    .mutex_delete = mutex_delete_wrapper,
    .mutex_lock = mutex_lock_wrapper,
    .mutex_unlock = mutex_unlock_wrapper,
    
    .queue_create = queue_create_wrapper,
    .queue_delete = queue_delete_wrapper,
    .queue_send = queue_send_wrapper,
    .queue_send_from_isr = queue_send_from_isr_wrapper,
    .queue_recv = queue_recv_wrapper,
    .queue_recv_from_isr = queue_recv_from_isr_wrapper,
    .queue_msg_waiting = queue_msg_waiting_wrapper,

    .timer_create = timer_create_wrapper,
    .timer_get_arg = timer_get_arg_wrapper,
    .timer_reset = timer_reset_wrapper,
    .timer_stop = timer_stop_wrapper,
    .timer_delete = timer_delete_wrapper,

    .malloc = malloc,
    .free = free,
    .get_free_heap_size = get_free_heap_size_wrapper,

    .srand = srand_wrapper,
    .rand = rand_wrapper,
};

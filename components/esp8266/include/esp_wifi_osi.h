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

#ifndef ESP_WIFI_OSI_H_
#define ESP_WIFI_OSI_H_

#include "esp_wifi_os_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

extern wifi_osi_funcs_t s_wifi_osi_funcs;

#define wifi_task_create(func, name, depth, param, pri) \
    s_wifi_osi_funcs.task_create(func, name, depth, param, pri)

#define wifi_task_delete(h) \
    s_wifi_osi_funcs.task_delete(h)

#define wifi_task_yield() \
    s_wifi_osi_funcs.task_yield()

#define wifi_task_yield_from_isr() \
    s_wifi_osi_funcs.task_yield_from_isr()

#define wifi_task_delay(t) \
    s_wifi_osi_funcs.task_delay(t)

#define wifi_task_get_current_task() \
    s_wifi_osi_funcs.task_get_current_task()

#define wifi_task_get_max_priority() \
    s_wifi_osi_funcs.task_get_max_priority()

#define wifi_task_ms_to_ticks(t) \
    s_wifi_osi_funcs.task_ms_to_tick(t)

#define wifi_task_suspend_all() \
    s_wifi_osi_funcs.task_suspend_all()

#define wifi_task_resume_all() \
    s_wifi_osi_funcs.task_resume_all()

#define wifi_os_init() \
    s_wifi_osi_funcs.os_init()

#define wifi_os_start() \
    s_wifi_osi_funcs.os_start()

#define wifi_semphr_create(m, i) \
    s_wifi_osi_funcs.semphr_create(m, i)

#define wifi_semphr_delete(s) \
    s_wifi_osi_funcs.semphr_delete(s)

#define wifi_semphr_take_from_isr(s, r) \
    s_wifi_osi_funcs.semphr_take_from_isr(s, r)

#define wifi_semphr_give_from_isr(s, r) \
    s_wifi_osi_funcs.semphr_give_from_isr(s, r)

#define wifi_semphr_take(s, t) \
    s_wifi_osi_funcs.semphr_take(s, t)

#define wifi_semphr_give(s) \
    s_wifi_osi_funcs.semphr_give(s)

#define wifi_mutex_create() \
    s_wifi_osi_funcs.mutex_create()

#define wifi_mutex_delete(m) \
    s_wifi_osi_funcs.mutex_delete(m)

#define wifi_mutex_lock(m) \
    s_wifi_osi_funcs.mutex_lock(m)

#define wifi_mutex_unlock(m) \
    s_wifi_osi_funcs.mutex_unlock(m)

#define wifi_queue_create(ql, is) \
    s_wifi_osi_funcs.queue_create(ql, is)

#define wifi_queue_delete(q) \
    s_wifi_osi_funcs.queue_delete(q)

#define wifi_queue_send(q, i, t, p) \
    s_wifi_osi_funcs.queue_send(q, i, t, p)

#define wifi_queue_send_from_isr(q, i, r) \
    s_wifi_osi_funcs.queue_send_from_isr(q, i, r)

#define wifi_queue_recv(q, i, t) \
    s_wifi_osi_funcs.queue_recv(q, i, t)

#define wifi_queue_recv_from_isr(q, i, r) \
    s_wifi_osi_funcs.queue_recv_from_isr(q, i, r)

#define wifi_queue_msg_waiting(q) \
    s_wifi_osi_funcs.queue_msg_waiting(q)

#define wifi_timer_create(n, p, al, ag, cb) \
    s_wifi_osi_funcs.timer_create(n, p, al, ag, cb)

#define wifi_timer_get_arg(t) \
    s_wifi_osi_funcs.timer_get_arg(t)

#define wifi_timer_reset(t, tk) \
    s_wifi_osi_funcs.timer_reset(t, tk)

#define wifi_timer_stop(t, tk) \
    s_wifi_osi_funcs.timer_stop(t, tk)

#define wifi_timer_delete(t, tk) \
    s_wifi_osi_funcs.timer_delete(t, tk)

#define wifi_malloc(s, c) \
    s_wifi_osi_funcs.malloc(s, c, __ESP_FILE__, __LINE__)

#define wifi_zalloc(s, c) \
    s_wifi_osi_funcs.zalloc(s, c, __ESP_FILE__, __LINE__)

#define wifi_calloc(cnt, s, c) \
    s_wifi_osi_funcs.calloc(cnt, s, c, __ESP_FILE__, __LINE__)

#define wifi_realloc(ptr, s, c) \
    s_wifi_osi_funcs.realloc(ptr, s, c, __ESP_FILE__, __LINE__)

#define wifi_free(p) \
    s_wifi_osi_funcs.free(p, __ESP_FILE__, __LINE__)

#define wifi_get_free_heap_size() \
    s_wifi_osi_funcs.get_free_heap_size()

#define wifi_srand(s) \
    s_wifi_osi_funcs.srand(s)

#define wifi_rand() \
    s_wifi_osi_funcs.rand()

void *osi_task_top_sp(void);

#ifdef __cplusplus
}
#endif

#endif /* ESP_WIFI_OSI_H_ */

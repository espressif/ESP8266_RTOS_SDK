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

#ifndef ESP_WIFI_OS_ADAPTER_H_
#define ESP_WIFI_OS_ADAPTER_H_

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_WIFI_OS_ADAPTER_VERSION  0x00000001
#define ESP_WIFI_OS_ADAPTER_MAGIC    0xDEADBEAF

#define OSI_FUNCS_TIME_BLOCKING      0xffffffff

#define OSI_QUEUE_SEND_FRONT         0
#define OSI_QUEUE_SEND_BACK          1
#define OSI_QUEUE_SEND_OVERWRITE     2

typedef struct {
    int32_t _version;
    xt_handler (*_set_isr)(int32_t n, xt_handler f, void *arg);
    void (*_ints_on)(uint32_t mask);
    void (*_ints_off)(uint32_t mask);
    uint32_t (*_interrupt_disable)(void);
    void (*_interrupt_restore)(uint32_t tmp);
    void (*_task_yield)(void);
    void (*_task_yield_from_isr)(void);
    void *(*_semphr_create)(uint32_t max, uint32_t init);
    void (*_semphr_delete)(void *semphr);
    int32_t (*_semphr_take_from_isr)(void *semphr, void *hptw);
    int32_t (*_semphr_give_from_isr)(void *semphr, void *hptw);
    int32_t (*_semphr_take)(void *semphr, uint32_t block_time_tick);
    int32_t (*_semphr_give)(void *semphr);
    void *(*_mutex_create)(void);
    void (*_mutex_delete)(void *mutex);
    int32_t (*_mutex_lock)(void *mutex);
    int32_t (*_mutex_unlock)(void *mutex);
    void *(* _queue_create)(uint32_t queue_len, uint32_t item_size);
    void (* _queue_delete)(void *queue);
    int32_t (* _queue_send)(void *queue, void *item, uint32_t block_time_tick, uint32_t pos);
    int32_t (* _queue_send_from_isr)(void *queue, void *item, void *hptw);
    int32_t (* _queue_recv)(void *queue, void *item, uint32_t block_time_tick);
    int32_t (* _queue_recv_from_isr)(void *queue, void *item, void *hptw);
    int32_t (* _queue_msg_waiting)(void *queue);
    void *(* _event_group_create)(void);
    void (* _event_group_delete)(void *event);
    uint32_t (* _event_group_set_bits)(void *event, uint32_t bits);
    uint32_t (* _event_group_clear_bits)(void *event, uint32_t bits);
    uint32_t (* _event_group_wait_bits)(void *event, uint32_t bits_to_wait_for, int clear_on_exit, int wait_for_all_bits, uint32_t block_time_tick);
    int32_t (* _task_create_pinned_to_core)(void *task_func, const char *name, uint32_t stack_depth, void *param, uint32_t prio, void *task_handle, uint32_t core_id);
    int32_t (* _task_create)(void *task_func, const char *name, uint32_t stack_depth, void *param, uint32_t prio, void *task_handle);
    void (* _task_delete)(void *task_handle);
    void (* _task_delay)(uint32_t tick);
    int32_t (* _task_ms_to_tick)(uint32_t ms);
    void *(* _task_get_current_task)(void);
    int32_t (* _task_get_max_priority)(void);
    bool (* _is_in_isr)(void);
    void *(* _malloc)(uint32_t size);
    void (* _free)(void *p);
    int32_t (* _get_free_heap_size)(void);
    int32_t (* _read_efuse_mac)(uint8_t mac[6]);
    void (* _srand)(uint32_t seed);
    int32_t (* _rand)(void);
    int32_t _magic;
} wifi_osi_funcs_t;

extern wifi_osi_funcs_t g_wifi_osi_funcs;

#ifdef __cplusplus
}
#endif

#endif /* ESP_WIFI_OS_ADAPTER_H_ */
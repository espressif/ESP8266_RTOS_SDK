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

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_WIFI_OS_ADAPTER_VERSION  0x00000001
#define ESP_WIFI_OS_ADAPTER_MAGIC    0xDEADBEAF

#define OSI_FUNCS_TIME_BLOCKING      0xffffffff

#define OSI_QUEUE_SEND_FRONT         0
#define OSI_QUEUE_SEND_BACK          1
#define OSI_QUEUE_SEND_OVERWRITE     2

#define OSI_MALLOC_CAP_32BIT         (1 << 1)
#define OSI_MALLOC_CAP_8BIT          (1 << 2)
#define OSI_MALLOC_CAP_DMA           (1 << 3)

typedef struct {
    int32_t version;

    void *(*task_create)(void *task_func, const char *name, uint32_t stack_depth, void *param, uint32_t prio);
    void (*task_delete)(void *task_handle);
    void (*task_yield)(void);
    void (*task_yield_from_isr)(void);
    void (*task_delay)(uint32_t tick);
    void *(*task_get_current_task)(void);
    uint32_t (*task_get_max_priority)(void);

    uint32_t (*task_ms_to_tick)(uint32_t ms);

    void (*task_suspend_all)(void);
    void (*task_resume_all)(void);

    void (*os_init)(void);
    void (*os_start)(void);

    void *(*semphr_create)(uint32_t max, uint32_t init);
    void (*semphr_delete)(void *semphr);
    bool (*semphr_take_from_isr)(void *semphr, int *hptw);
    bool (*semphr_give_from_isr)(void *semphr, int *hptw);
    bool (*semphr_take)(void *semphr, uint32_t block_time_tick);
    bool (*semphr_give)(void *semphr);

    void *(*mutex_create)(void);
    void (*mutex_delete)(void *mutex);
    bool (*mutex_lock)(void *mutex);
    bool (*mutex_unlock)(void *mutex);

    void *(*queue_create)(uint32_t queue_len, uint32_t item_size);
    void (*queue_delete)(void *queue);
    bool (*queue_send)(void *queue, void *item, uint32_t block_time_tick, uint32_t pos);
    bool (*queue_send_from_isr)(void *queue, void *item, int *hptw);
    bool (*queue_recv)(void *queue, void *item, uint32_t block_time_tick);
    bool (*queue_recv_from_isr)(void *queue, void *item, int *hptw);
    uint32_t (*queue_msg_waiting)(void *queue);

    void *(*timer_create)(const char *name, uint32_t period_ticks, bool auto_load, void *arg, void (*cb)(void *timer));
    void *(*timer_get_arg)(void *timer);
    bool (*timer_reset)(void *timer, uint32_t ticks);
    bool (*timer_stop)(void *timer, uint32_t ticks);
    bool (*timer_delete)(void *timer, uint32_t ticks);

    void *(*malloc)(uint32_t size, uint32_t cap, const char *file, size_t line);
    void *(*zalloc)(uint32_t size, uint32_t cap, const char *file, size_t line);
    void *(*realloc)(void *ptr, uint32_t size, uint32_t cap, const char *file, size_t line);
    void *(*calloc)(uint32_t cnt, uint32_t size, uint32_t cap, const char *file, size_t line);
    void (*free)(void *p, const char *file, size_t line);
    uint32_t (*get_free_heap_size)(void);

    void (*srand)(uint32_t seed);
    int32_t (*rand)(void);

    int32_t (* nvs_set_i8)(uint32_t handle, const char* key, int8_t value);
    int32_t (* nvs_get_i8)(uint32_t handle, const char* key, int8_t* out_value);
    int32_t (* nvs_set_u8)(uint32_t handle, const char* key, uint8_t value);
    int32_t (* nvs_get_u8)(uint32_t handle, const char* key, uint8_t* out_value);
    int32_t (* nvs_set_u16)(uint32_t handle, const char* key, uint16_t value);
    int32_t (* nvs_get_u16)(uint32_t handle, const char* key, uint16_t* out_value);
    int32_t (* nvs_open)(const char* name, uint32_t open_mode, uint32_t *out_handle);
    void (* nvs_close)(uint32_t handle); 
    int32_t (* nvs_commit)(uint32_t handle);
    int32_t (* nvs_set_blob)(uint32_t handle, const char* key, const void* value, size_t length);
    int32_t (* nvs_get_blob)(uint32_t handle, const char* key, void* out_value, size_t* length);
    int32_t (* nvs_erase_key)(uint32_t handle, const char* key);

    int32_t magic;
} wifi_osi_funcs_t;

#ifdef __cplusplus
}
#endif

#endif /* ESP_WIFI_OS_ADAPTER_H_ */

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

#ifndef _ESPOS_TYPES_H_
#define _ESPOS_TYPES_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "arch/espos_arch.h"

#define ESPOS_OBJ_NONE  0

typedef uintptr_t       espos_obj_t;

typedef espos_obj_t     espos_task_t;
typedef espos_obj_t     espos_queue_t;
typedef espos_obj_t     espos_mutex_t;
typedef espos_obj_t     espos_sem_t;
typedef espos_obj_t     espos_timer_t;
typedef espos_obj_t     espos_critical_t;

typedef size_t          espos_size_t;
typedef size_t          espos_pos_t;
typedef uint8_t         espos_prio_t;
typedef uint32_t        espos_opt_t;
typedef int32_t         espos_cpu_t;

typedef espos_size_t    espos_tick_t;
typedef espos_size_t    espos_time_t;
typedef espos_size_t    espos_sem_count_t;

typedef void (*espos_task_entry_t)(void *p);
typedef void (*espos_timer_cb_t)(espos_timer_t timer, void *arg);

#endif /* _ESPOS_TYPES_H_ */

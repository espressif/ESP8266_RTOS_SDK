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

#include <reent.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static struct _reent impure_data;
struct _reent *_global_impure_ptr = &impure_data;

struct _reent *__getreent()
{
#if configUSE_NEWLIB_REENTRANT == 1
    /*
     * Locking mutex(only mutex, not ISR mutex) at the following three
     * state may cause OS death. So we use a extra _reent data instead
     * of task private reent data.
     * 
     * But although at the state none of "taskSCHEDULER_RUNNING",  exception
     * can cause CPU swicth context, then Locking mutex may cause OS death.
     * So we command that use ets_printf(ROM function) instead of "printf"
     * at exception and system kernal critical state.
     */
    if (xPortInIsrContext() 
        || !xTaskGetCurrentTaskHandle()
        || xTaskGetSchedulerState() != taskSCHEDULER_RUNNING)
        return &impure_data;

    /*
     * When scheduler starts, _global_impure_ptr = pxCurrentTCB->xNewLib_reent.
     */
#endif
    return _global_impure_ptr;
}

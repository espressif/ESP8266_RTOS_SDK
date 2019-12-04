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

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	HEAP_TRACE_NONE = 0,

    HEAP_TRACE_LEAKS,
} heap_trace_mode_t;

/**
 * @brief heap trace record information(not used)
 */
typedef struct {
    char    buf[1];     /*!< record buffer */
} heap_trace_record_t;

/**
 * @brief Check if heap trace is on
 *
 * @return true if on or false
 */
int heap_trace_is_on(void);

/**
 * @brief Empty function just for passing compiling some place.
 */
esp_err_t heap_trace_init_standalone(heap_trace_record_t *record_buffer, size_t num_records);

/**
 * @brief Start heap tracing. All heap allocations will be traced, until heap_trace_stop() is called.
 *
 * @param mode Mode for tracing.
 * - HEAP_TRACE_LEAKS means only suspected memory leaks are traced. (When memory is freed, the record is removed from the trace buffer.)
 * @return
 * - ESP_OK Tracing is started.
 */
esp_err_t heap_trace_start(heap_trace_mode_t mode);

/**
 * @brief Stop heap tracing.
 *
 * @return
 * - ESP_OK Heap tracing stopped..
 */
esp_err_t heap_trace_stop(void);

/**
 * @brief Resume heap tracing which was previously stopped.
 *
 * @return
 * - ESP_ERR_NOT_SUPPORTED Project was compiled without heap tracing enabled in menuconfig.
 * - ESP_OK Heap tracing resumed.
 */
esp_err_t heap_trace_resume(void);

/**
 * @brief Dump heap trace record data to stdout
 *
 * @note It is safe to call this function while heap tracing is running, however in HEAP_TRACE_LEAK mode the dump may skip
 * entries unless heap tracing is stopped first.
 */
void heap_trace_dump(void);

#ifdef __cplusplus
}
#endif

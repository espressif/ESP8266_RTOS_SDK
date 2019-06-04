// Copyright 2019-2020 Espressif Systems (Shanghai) PTE LTD
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

#include <stddef.h>
#include <sys/queue.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*nc_func_t)(int argc, char **argv);

/**
 * @brief Command data structure
 */
typedef struct _nc_cmd {
    SLIST_ENTRY(_nc_cmd)    entries;                    //!< queue entry
    const char              *name;                      //!< command name
    nc_func_t               func;                       //!< command callback function
} nc_cmd_t;

typedef nc_cmd_t* nc_cmd_handle_t;

/**
 * @brief Initialize nano console
 * 
 * @return 0 if success or others if fail
 */
int nc_init(void);

/**
 * @brief Register a command to nano console core
 * 
 * @param[out] handle   created command handle to users
 * @param[in]  name     command name
 * @param[in]  func     command callback function
 * 
 * @return 0 if success or others if fail
 * 
 * @note The function only can be called before "nc_init"
 */
int nc_register_cmd(nc_cmd_handle_t *handle, const char *name, nc_func_t func);

/**
 * @brief Output formated string through nano console I/O stream
 * 
 * @param[in] fmt  format string
 * @param[in] ...  command arguments' value list
 * 
 * @return output string number or negative value if fail
 */
int nc_printf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

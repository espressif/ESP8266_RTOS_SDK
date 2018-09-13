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

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/**
  * @brief  Initialize the Task Watchdog Timer (TWDT)
  *
  * @return
  *     - ESP_OK: Initialization was successful
  *     - ESP_ERR_NO_MEM: Initialization failed due to lack of memory
  *
  * @note  esp_task_wdt_init() must only be called after the scheduler
  *        started
  */
esp_err_t esp_task_wdt_init(void);

/**
  * @brief  Reset(Feed) the Task Watchdog Timer (TWDT) on behalf of the currently
  *         running task
  */
void esp_task_wdt_reset(void);

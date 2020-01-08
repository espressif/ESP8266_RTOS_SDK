// Copyright 2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <stdint.h>
#include "sdkconfig.h"
#include "esp_wifi_types.h"
#include "tcpip_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup ESP_NETIF_INIT_API ESP-NETIF Initialization API
 * @brief Add these APIs to make code compatible with esp-idf's newer branch
 *
 */

/** @addtogroup ESP_NETIF_INIT_API
 * @{
 */

/**
 * @brief  Wrapper API to call "tcpip_adapter_init()" really
 *
 * @return
 *         - ESP_OK on success

 * @note This function should be called exactly once from application code, when the application starts up.
 */
esp_err_t esp_netif_init(void);

/**
 * @brief  Empty function
 *
 *          Note: Deinitialization is not supported yet
 *
 * @return
 *         - ESP_OK on success
 */
esp_err_t esp_netif_deinit(void);

#ifdef __cplusplus
}
#endif

// Copyright 2018-2025 Espressif Systems (Shanghai) PTE LTD
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

/* Currently only supports infrared NEC code */
/* NEC time parameter configuration */
#define IR_RX_NEC_BIT_NUM      8
#define IR_RX_NEC_HEADER_US    13500
#define IR_RX_NEC_DATA0_US     1120
#define IR_RX_NEC_DATA1_US     2250
#define IR_RX_NEC_TM1_REP_US   20000
#define IR_RX_NEC_TM2_REP_US   11250
#define IR_RX_ERROR_US         200 // Used to compensate errors

/**
 * @brief ir rx initialization parameter structure type definition
 */
typedef struct {
    uint32_t io_num;
    uint32_t buf_len;
} ir_rx_config_t;

/**
 * @brief ir rx nec data union type definition
 */
typedef union {
    struct {
        uint32_t addr1: 8;    
        uint32_t addr2: 8;    
        uint32_t cmd1:  8;    
        uint32_t cmd2:  8;     
    };
    uint32_t val;             /*!< union fill */ 
} ir_rx_nec_data_t;

/**
  * @brief Disable the ir rx
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_FAIL ir rx has not been initialized yet
  */
esp_err_t ir_rx_disable();

/**
  * @brief Enable the ir rx
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_FAIL ir rx has not been initialized yet
  */
esp_err_t ir_rx_enable();

/**
 * @brief Receive infrared data
 *
 * @param data Pointer to the rx data buffer
 * @param len Length of ir_rx_data, range: 0 < len < (uint16_t)
 * @param timeout_ticks freertos timeout ticks
 * 
 * @return
 *     - -1 error
 *     - length The actual length of data received
 */
int ir_rx_recv_data(ir_rx_nec_data_t *data, size_t len, uint32_t timeout_ticks);

/**
  * @brief Deinit the ir rx
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_FAIL ir rx has not been initialized yet
  */
esp_err_t ir_rx_deinit();

/**
  * @brief Initialize the ir rx
  *
  * @param config Pointer to deliver initialize configuration parameter
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_NO_MEM malloc fail
  *     - ESP_FAIL ir rx has been initialized
  */
esp_err_t ir_rx_init(ir_rx_config_t *config);

#ifdef __cplusplus
}
#endif

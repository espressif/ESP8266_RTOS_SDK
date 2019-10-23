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
#define IR_TX_NEC_BIT_NUM         8
#define IR_TX_NEC_HEADER_HIGH_US  9000
#define IR_TX_NEC_HEADER_LOW_US   4500
#define IR_TX_NEC_DATA_HIGH_US    560
#define IR_TX_NEC_DATA_LOW_1_US   1690
#define IR_TX_NEC_DATA_LOW_0_US   560
#define IR_TX_NEC_REP_HIGH_US     9000
#define IR_TX_NEC_REP_LOW_US      2250
#define IR_TX_NEC_REP_STOP_US     562
#define IR_TX_NEC_REP_CYCLE       108000
#define IR_TX_WDEV_TIMER_ERROR_US 5 // Timing in advance to reduce errors
#define IR_TX_HW_TIMER_ERROR_US   40 // Timing in advance to reduce errors

typedef enum {
    IR_TX_WDEV_TIMER,
    IR_TX_HW_TIMER,
} ir_tx_timer_t;

/**
 * @brief ir tx initialization parameter structure type definition
 */
typedef struct {
    uint32_t io_num; // 2 or 14, 2: I2SO_WS 14: I2SI_WS
    uint32_t freq;
    ir_tx_timer_t timer; // WDEV timer will be more accurate, but PWM will not work
} ir_tx_config_t;

/**
 * @brief ir tx data union type definition
 */
typedef union {
    struct {
        uint32_t addr1: 8;    
        uint32_t addr2: 8;    
        uint32_t cmd1:  8;    
        uint32_t cmd2:  8;     
    };
    uint32_t val;             /*!< union fill */ 
} ir_tx_nec_data_t;

/**
 * @brief Send ir data
 * 
 * @note If multiple data are identical, repeat signals will be used.
 * Infrared data consumes more than 100 ms per transmission, so note the timeout_ticks parameter
 *
 * @param data Pointer to the tx data buffer
 * @param len Length of ir_tx_data, range: 0 < len < (uint16_t)
 * @param timeout_ticks freertos timeout ticks
 *
 * @return
 *     - -1 error
 *     - length The actual length of data sent
 */
int ir_tx_send_data(ir_tx_nec_data_t *data, size_t len, uint32_t timeout_ticks);

/**
  * @brief Deinit the ir tx
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_FAIL ir tx has not been initialized yet
  */
esp_err_t ir_tx_deinit();

/**
  * @brief Initialize the ir tx
  * 
  * @note WDEV timer will be more accurate, but PWM will not work.
  *
  * @param config Pointer to deliver initialize configuration parameter
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_NO_MEM malloc fail
  *     - ESP_FAIL ir tx has been initialized
  */
esp_err_t ir_tx_init(ir_tx_config_t *config);

#ifdef __cplusplus
}
#endif

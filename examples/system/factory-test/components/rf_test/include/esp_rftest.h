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

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize RF test module
 */
void rftest_init(void);

/**
 * @brief Set TX testing mode
 *
 * @param mode testing mode, 1 means continuous Wi-Fi packets transmission with 92% duty cycle
 *             0 means default mode for iqview testing
 */
void tx_contin_func(uint8_t mode);

/**
 * @brief TX testing function, continuously sending Wi-Fi packets at "while(1)" loop
 * 
 * @param channel Wi-Fi TX channel, it ranges from 1 to 14
 * @param rate Wi-Fi TX rate, it ranges from 1 to 23
 * @param attenuation Wi-Fi TX power attenuation, it ranges from 1 to 127 and its unit is 0.25dB.
 *                    For example, 1 means 0.25dB, 2 means 0.5 dB and so on.
 */
void esp_tx_func(uint32_t channel, uint32_t rate, uint32_t attenuation);

/**
 * @brief RX testing function, continuously receiving Wi-Fi packets at "while(1)" loop
 * 
 * @param channel Wi-Fi RX channel, it ranges from 1 to 14
 * @param rate Wi-Fi RX rate, it ranges from 1 to 23
 */
void esp_rx_func(uint32_t channel, uint32_t rate);

/**
 * @brief Single carrier TX testing function
 *
 * @param enable enable signal, 1 means starting sending, 0 means stopping sending
 * @param channel Wi-Fi RX channel, it ranges from 1 to 14
 * @param attenuation Wi-Fi TX power attenuation, it ranges from 1 to 127 and its unit is 0.25dB.
 *                    For example, 1 means 0.25dB, 2 means 0.5 dB and so on.
 */
void wifiscwout_func(uint32_t enable, uint32_t channel, uint32_t attenuation);

/**
 * @brief Stop sending or recieving Wi-Fi packets
 * 
 * @return 
 *      - 0 receiving stop TX commands "cmdstop" or "CmdStop" and TX is stopped
 *      - 2 receiving error commands and TX will not stop
 *      - 3 receiving no commands
 */
int cmdstop_callback(void);

/**
 * @brief Register RF test command for console
 */
void esp_console_register_rftest_command(void);

#ifdef __cplusplus
}
#endif

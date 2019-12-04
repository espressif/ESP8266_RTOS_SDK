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

#include "esp_phy_init.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_CAL_DATA_CHECK_FAIL 1

uint8_t phy_check_data_table(uint32_t *table, uint8_t, uint8_t);

void phy_afterwake_set_rfoption(uint8_t);

void phy_set_powerup_option(uint8_t);

void write_data_to_rtc(uint8_t *);

void get_data_from_rtc(uint8_t *);
int register_chipv6_phy(uint8_t* );
void phy_disable_agc();

uint8_t chip_init(uint8_t* init_data, uint8_t *mac, uint32_t uart_baudrate);

#ifdef __cplusplus
}
#endif


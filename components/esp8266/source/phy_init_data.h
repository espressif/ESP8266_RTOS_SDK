// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
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
#include "sdkconfig.h"

// constrain a value between 'low' and 'high', inclusive
#define LIMIT(val, low, high) ((val < low) ? low : (val > high) ? high : val)
#define PHY_INIT_MAGIC "PHYINIT"

static const char phy_init_magic_pre[] = PHY_INIT_MAGIC;

/**
 * @brief Structure containing default recommended PHY initialization parameters.
 */
static const esp_phy_init_data_t phy_init_data= { {
        0x05,
        0x00,
        0x04,
        0x02,
        0x05,
        0x05,
        0x05,
        0x02,
        0x05,
        0x00,
        0x04,
        0x05,
        0x05,
        0x04,
        0x05,
        0x05,
        0x04,
        0xfe,
        0xfd,
        0xff,
        0xf0,
        0xf0,
        0xf0,
        0xe0,
        0xe0,
        0xe0,
        0xe1,
        0x0a,
        0xff,
        0xff,
        0xf8,
        0x00,
        0xf8,
        0xf8,
        LIMIT(CONFIG_ESP8266_PHY_MAX_WIFI_TX_POWER * 4, 0, 0x52),
        LIMIT(CONFIG_ESP8266_PHY_MAX_WIFI_TX_POWER * 4, 0, 0x4e),
        LIMIT(CONFIG_ESP8266_PHY_MAX_WIFI_TX_POWER * 4, 0, 0x4a),
        LIMIT(CONFIG_ESP8266_PHY_MAX_WIFI_TX_POWER * 4, 0, 0x44),
        LIMIT(CONFIG_ESP8266_PHY_MAX_WIFI_TX_POWER * 4, 0, 0x40),
        LIMIT(CONFIG_ESP8266_PHY_MAX_WIFI_TX_POWER * 4, 0, 0x38),
        0x00,
        0x00,
        0x01,
        0x01,
        0x02,
        0x03,
        0x04,
        0x05,
        0x01,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x02,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0xe1,
        0x0a,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x01,
        0x93,
        0x43,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
#ifdef CONFIG_ESP_PHY_INIT_DATA_VDD33_CONST
        CONFIG_ESP_PHY_INIT_DATA_VDD33_CONST,
#else   
        0x00,
#endif
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
} };

static const char phy_init_magic_post[] = PHY_INIT_MAGIC;


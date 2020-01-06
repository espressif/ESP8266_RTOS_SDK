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

#include "sdkconfig.h"
#include "esp_libc.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "internal/esp_wifi_internal.h"
#include "phy.h"

const size_t _g_esp_wifi_ppt_task_stk_size = CONFIG_WIFI_PPT_TASKSTACK_SIZE;

#if CONFIG_ESP8266_WIFI_CONNECT_OPEN_ROUTER_WHEN_PWD_IS_SET
const bool _g_esp_wifi_connect_open_router_when_pwd_is_set = true;
#else
const bool _g_esp_wifi_connect_open_router_when_pwd_is_set = false;
#endif

esp_err_t esp_wifi_init_internal(const wifi_init_config_t *config);

static void esp_wifi_set_debug_log()
{
    /* set WiFi log level and module */
#if CONFIG_ESP8266_WIFI_DEBUG_LOG_ENABLE
    uint32_t wifi_log_level = WIFI_LOG_ERROR;
    uint32_t wifi_log_submodule = WIFI_LOG_SUBMODULE_NULL;
#if CONFIG_ESP8266_WIFI_DEBUG_LOG_WARNING
    wifi_log_level = WIFI_LOG_WARNING;
#endif
#if CONFIG_ESP8266_WIFI_DEBUG_LOG_INFO
    wifi_log_level = WIFI_LOG_INFO;
#endif
#if CONFIG_ESP8266_WIFI_DEBUG_LOG_DEBUG
    wifi_log_level = WIFI_LOG_DEBUG;
#endif
#if CONFIG_ESP8266_WIFI_DEBUG_LOG_VERBOSE
    wifi_log_level = WIFI_LOG_VERBOSE;
#endif
#if CONFIG_ESP8266_WIFI_DEBUG_LOG_SUBMODULE_CORE
    wifi_log_submodule |= WIFI_LOG_SUBMODULE_CORE;
#endif
#if CONFIG_ESP8266_WIFI_DEBUG_LOG_SUBMODULE_SCAN
    wifi_log_submodule |= WIFI_LOG_SUBMODULE_SCAN;
#endif
#if CONFIG_ESP8266_WIFI_DEBUG_LOG_SUBMODULE_PM
    wifi_log_submodule |= WIFI_LOG_SUBMODULE_PM;
#endif
#if CONFIG_ESP8266_WIFI_DEBUG_LOG_SUBMODULE_NVS
    wifi_log_submodule |= WIFI_LOG_SUBMODULE_NVS;
#endif
#if CONFIG_ESP8266_WIFI_DEBUG_LOG_SUBMODULE_TRC
    wifi_log_submodule |= WIFI_LOG_SUBMODULE_TRC;
#endif
#if CONFIG_ESP8266_WIFI_DEBUG_LOG_SUBMODULE_EBUF
    wifi_log_submodule |= WIFI_LOG_SUBMODULE_EBUF;
#endif
#if CONFIG_ESP8266_WIFI_DEBUG_LOG_SUBMODULE_NET80211
    wifi_log_submodule |= WIFI_LOG_SUBMODULE_NET80211;
#endif
#if CONFIG_ESP8266_WIFI_DEBUG_LOG_SUBMODULE_TIMER
    wifi_log_submodule |= WIFI_LOG_SUBMODULE_TIMER;
#endif
#if CONFIG_ESP8266_WIFI_DEBUG_LOG_SUBMODULE_ESPNOW
    wifi_log_submodule |= WIFI_LOG_SUBMODULE_ESPNOW;
#endif
#if CONFIG_ESP8266_WIFI_DEBUG_LOG_SUBMODULE_MAC
    wifi_log_submodule |= WIFI_LOG_SUBMODULE_MAC;
#endif
#if CONFIG_ESP8266_WIFI_DEBUG_LOG_SUBMODULE_WPA
    wifi_log_submodule |= WIFI_LOG_SUBMODULE_WPA;
#endif
#if CONFIG_ESP8266_WIFI_DEBUG_LOG_SUBMODULE_WPS
    wifi_log_submodule |= WIFI_LOG_SUBMODULE_WPS;
#endif
#if CONFIG_ESP8266_WIFI_DEBUG_LOG_SUBMODULE_AMPDU
    wifi_log_submodule |= WIFI_LOG_SUBMODULE_AMPDU;
#endif
#if CONFIG_ESP8266_WIFI_DEBUG_LOG_SUBMODULE_AMSDU
    wifi_log_submodule |= WIFI_LOG_SUBMODULE_AMSDU;
#endif
#if CONFIG_ESP8266_WIFI_DEBUG_LOG_SUBMODULE_FRAG
    wifi_log_submodule |= WIFI_LOG_SUBMODULE_FRAG;
#endif
#if CONFIG_ESP8266_WIFI_DEBUG_LOG_SUBMODULE_WPA2
    wifi_log_submodule |= WIFI_LOG_SUBMODULE_WPA2;
#endif
    esp_wifi_internal_set_log_level(wifi_log_level);
    esp_wifi_internal_set_log_mod(wifi_log_submodule);
#else
    esp_wifi_internal_set_log_level(WIFI_LOG_ERROR);
    esp_wifi_internal_set_log_mod(WIFI_LOG_SUBMODULE_NULL);
#endif /* CONFIG_ESP8266_WIFI_DEBUG_LOG_ENABLE*/
}

/**
  * @brief  Init WiFi
  *         Alloc resource for WiFi driver, such as WiFi control structure, RX/TX buffer,
  *         WiFi NVS structure etc, this WiFi also start WiFi task
  *
  * @attention 1. This API must be called before all other WiFi API can be called
  * @attention 2. Always use WIFI_INIT_CONFIG_DEFAULT macro to init the config to default values, this can
  *               guarantee all the fields got correct value when more fields are added into wifi_init_config_t
  *               in future release. If you want to set your owner initial values, overwrite the default values
  *               which are set by WIFI_INIT_CONFIG_DEFAULT, please be notified that the field 'magic' of 
  *               wifi_init_config_t should always be WIFI_INIT_CONFIG_MAGIC!
  *
  * @param  config pointer to WiFi init configuration structure; can point to a temporary variable.
  *
  * @return
  *    - ESP_OK: succeed
  *    - ESP_ERR_NO_MEM: out of memory
  *    - others: refer to error code esp_err.h
  */
esp_err_t esp_wifi_init(const wifi_init_config_t *config)
{
    esp_event_set_default_wifi_handlers();
    esp_err_t result = esp_wifi_init_internal(config);
    if (result == ESP_OK) {
        esp_wifi_set_debug_log();
    }
    return result;
}

void esp_deep_sleep_set_rf_option(uint8_t option)
{
    phy_afterwake_set_rfoption(option);
}

size_t __attribute__((weak)) esp_wifi_scan_get_ap_num_max(void)
{
    return CONFIG_SCAN_AP_MAX;
}

bool IRAM_ATTR esp_wifi_try_rate_from_high(void) {
#if CONFIG_WIFI_TX_RATE_SEQUENCE_FROM_HIGH
    int8_t rssi;
    rssi = esp_wifi_get_ap_rssi();
    wifi_mode_t mode;
    esp_wifi_get_mode( &mode );
    if (rssi < -26 && mode == WIFI_MODE_STA) {
        return true;
    }
#endif
    return false;
}
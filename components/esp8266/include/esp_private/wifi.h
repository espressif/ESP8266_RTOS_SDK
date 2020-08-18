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

#ifndef _ESP_WIFI_INTERNAL_H
#define _ESP_WIFI_INTERNAL_H

#include "esp_wifi_types.h"
#include "esp_event.h"
#include "esp_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WIFI_RX_PBUF_IRAM,   /** save rx buffer to iram and upload to tcpip*/
    WIFI_RX_PBUF_DRAM,   /** save rx buffer to dram and upload to tcpip */
} wifi_rx_pbuf_mem_type_t;

/**
  * @brief WiFi log level
  *
  */
typedef enum {
    WIFI_LOG_ERROR = 0,   /*enabled by default*/
    WIFI_LOG_WARNING,     /*can be set in menuconfig*/
    WIFI_LOG_INFO,        /*can be set in menuconfig*/
    WIFI_LOG_DEBUG,       /*can be set in menuconfig*/
    WIFI_LOG_VERBOSE,     /*can be set in menuconfig*/
} wifi_log_level_t;
  
/**
  * @brief WiFi log submodule definition
  *
  */
#define WIFI_LOG_SUBMODULE_NULL      (0)
#define WIFI_LOG_SUBMODULE_CORE      (1<<0)
#define WIFI_LOG_SUBMODULE_SCAN      (1<<1)
#define WIFI_LOG_SUBMODULE_PM        (1<<2)
#define WIFI_LOG_SUBMODULE_NVS       (1<<3)
#define WIFI_LOG_SUBMODULE_TRC       (1<<4)
#define WIFI_LOG_SUBMODULE_EBUF      (1<<5)
#define WIFI_LOG_SUBMODULE_NET80211  (1<<6)
#define WIFI_LOG_SUBMODULE_TIMER     (1<<7)
#define WIFI_LOG_SUBMODULE_ESPNOW    (1<<8)
#define WIFI_LOG_SUBMODULE_MAC       (1<<9)
#define WIFI_LOG_SUBMODULE_WPA       (1<<10)
#define WIFI_LOG_SUBMODULE_WPS       (1<<11)
#define WIFI_LOG_SUBMODULE_AMPDU     (1<<12)
#define WIFI_LOG_SUBMODULE_AMSDU     (1<<13)
#define WIFI_LOG_SUBMODULE_FRAG      (1<<14)
#define WIFI_LOG_SUBMODULE_WPA2      (1<<15)


/**
 * @brief Initialize Wi-Fi Driver
 *     Alloc resource for WiFi driver, such as WiFi control structure, RX/TX buffer,
 *     WiFi NVS structure among others.
 *
 * For the most part, you need not call this function directly. It gets called
 * from esp_wifi_init().
 *
 * This function may be called, if you only need to initialize the Wi-Fi driver
 * without having to use the network stack on top.
 *
 * @param  config provide WiFi init configuration
 *
 * @return
 *    - ESP_OK: succeed
 *    - ESP_ERR_NO_MEM: out of memory
 *    - others: refer to error code esp_err.h
 */
esp_err_t esp_wifi_init_internal(const wifi_init_config_t *config);

/**
 * @brief Deinitialize Wi-Fi Driver
 *     Free resource for WiFi driver, such as WiFi control structure, RX/TX buffer,
 *     WiFi NVS structure among others.
 *
 * For the most part, you need not call this function directly. It gets called
 * from esp_wifi_deinit().
 *
 * This function may be called, if you call esp_wifi_init_internal to initialize
 * WiFi driver.
 *
 * @return
 *    - ESP_OK: succeed
 *    - others: refer to error code esp_err.h
 */
esp_err_t esp_wifi_deinit_internal(void);

/**
  * @brief     Set WIFI received TCP/IP data cache ram type
  *
  * @param     type if use dram
  */
void esp_wifi_set_rx_pbuf_mem_type(wifi_rx_pbuf_mem_type_t type);

/**
  * @brief     get WIFI received TCP/IP data cache ram type
  *
  * @return    true if use dram or false
  */
wifi_rx_pbuf_mem_type_t esp_wifi_get_rx_pbuf_mem_type(void);

int8_t esp_wifi_get_ap_rssi(void);

/**
  * @brief The RX callback function when receive probe request packet. 
  *        When probe request packet is received, the callback function will be called.
  *
  * @param frame  Data of received probe request.
  * @param len  length of received probe request.
  * @param rssi  rssi of received probe request.
  */
typedef void (*wifi_sta_rx_probe_req_t)(const uint8_t *frame, int len, int rssi);

/**
  * @brief Register the RX callback function when receive probe request.
  *
  * When probe request packet is received, the registered callback function will be called.
  *
  * @param cb  callback
  *
  * @return
  *    - ESP_OK: succeed
  *    - ESP_ERR_WIFI_NOT_INIT: WiFi is not initialized by esp_wifi_init
  */
esp_err_t esp_wifi_set_sta_rx_probe_req(wifi_sta_rx_probe_req_t cb);

/**
  * @brief  free the rx buffer which allocated by wifi driver
  *
  * @param  void* buffer: rx buffer pointer
  */
void esp_wifi_internal_free_rx_buffer(void* buffer);

/**
  * @brief  transmit the buffer via wifi driver
  *
  * @param  wifi_interface_t wifi_if : wifi interface id
  * @param  void *buffer : the buffer to be tansmit
  * @param  uint16_t len : the length of buffer
  *
  * @return
  *    - ERR_OK  : Successfully transmit the buffer to wifi driver
  *    - ERR_MEM : Out of memory
  *    - ERR_IF : WiFi driver error
  *    - ERR_ARG : Invalid argument
  */
int esp_wifi_internal_tx(wifi_interface_t wifi_if, void *buffer, uint16_t len);

/**
  * @brief     The WiFi RX callback function
  *
  *            Each time the WiFi need to forward the packets to high layer, the callback function will be called
  */
typedef esp_err_t (*wifi_rxcb_t)(void *buffer, uint16_t len, void *eb);

/**
  * @brief     Set the WiFi RX callback
  *
  * @attention 1. Currently we support only one RX callback for each interface
  *
  * @param     wifi_interface_t ifx : interface
  * @param     wifi_rxcb_t fn : WiFi RX callback
  *
  * @return
  *     - ESP_OK : succeed
  *     - others : fail
  */
esp_err_t esp_wifi_internal_reg_rxcb(wifi_interface_t ifx, wifi_rxcb_t fn);

/**
  * @brief     Set current WiFi log level     
  *
  * @param     level   Log level.
  *
  * @return
  *    - ESP_OK: succeed
  *    - ESP_FAIL: level is invalid
  */
esp_err_t esp_wifi_internal_set_log_level(wifi_log_level_t level);

/**
  * @brief     Set current log module and submodule
  *
  * @param     module      Log module
  * @param     submodule   Log submodule
  *
  * @return
  *    - ESP_OK: succeed
  *    - ESP_ERR_WIFI_NOT_INIT: WiFi is not initialized by esp_wifi_init
  *    - ESP_ERR_WIFI_ARG: invalid argument
  */
esp_err_t esp_wifi_internal_set_log_mod(uint32_t submodule);

/**
  * @brief     Get current WiFi log info     
  *
  * @param     log_level  the return log level.
  * @param     log_mod    the return log module and submodule
  *
  * @return
  *    - ESP_OK: succeed
  */
esp_err_t esp_wifi_internal_get_log(wifi_log_level_t *log_level, uint32_t *log_mod);

/**
  * @brief     get wifi power management config.
  * 
  * @param     ps_config    power management config
  */
void esp_wifi_set_pm_config(esp_pm_config_t *pm_config);

/**
  * @brief     set wifi power management config.
  * 
  * @param     ps_config    power management config
  */
void esp_wifi_get_pm_config(esp_pm_config_t *pm_config);

#ifdef __cplusplus
}
#endif

#endif /* _ESP_WIFI_INTERNAL_H */

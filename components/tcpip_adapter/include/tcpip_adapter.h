// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
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

#ifndef _TCPIP_ADAPTER_H_
#define _TCPIP_ADAPTER_H_

#include "sdkconfig.h"
/**
 * @brief TCPIP adapter library
 *
 * The aim of this adapter is to provide an abstract layer upon TCPIP stack.
 * With this layer, switch to other TCPIP stack is possible and easy in ESP8266_RTOS_SDK.
 *
 * If users want to use other TCPIP stack, all those functions should be implemented
 * by using the specific APIs of that stack.
 *
 * tcpip_adapter_init should be called in the start of app_main for only once.
 *
 * Currently most adapter APIs are called in event_default_handlers.c.
 *
 * We recommend users only use set/get IP APIs, DHCP server/client APIs,
 * get free station list APIs in application side. Other APIs are used in ESP8266_RTOS_SDK internal,
 * otherwise the state maybe wrong.
 *
 */

typedef enum {
    TCPIP_ADAPTER_IF_STA = 0,     /**< ESP8266 station interface */
    TCPIP_ADAPTER_IF_AP,          /**< ESP8266 soft-AP interface */
    TCPIP_ADAPTER_IF_MAX
} tcpip_adapter_if_t;

struct netif *esp_netif[TCPIP_ADAPTER_IF_MAX];
char* hostname;
bool default_hostname;

#define TCPIP_ADAPTER_IF_VALID(fd) ((fd < TCPIP_ADAPTER_IF_MAX) ? 1 : 0)

/* Define those to better describe your network interface. */
#define IFNAME0 'e'
#define IFNAME1 'n'

#ifdef CONFIG_TCPIP_ADAPER_DEBUG
#define TAG ""
#define TCPIP_ATAPTER_LOG(str, ...) printf(TAG __FILE__ " line: %d " str, __LINE__, ##__VA_ARGS__)
#else
#define TCPIP_ATAPTER_LOG(str, ...)
#endif


#endif /*  _TCPIP_ADAPTER_H_ */
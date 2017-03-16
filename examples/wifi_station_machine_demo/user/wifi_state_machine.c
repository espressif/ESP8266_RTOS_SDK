/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2017 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <stddef.h>
#include "espressif/c_types.h"
#include "lwipopts.h"
#include "lwip/ip_addr.h"
#include "espressif/esp_libc.h"
#include "espressif/esp_misc.h"
#include "espressif/esp_common.h"
#include "espressif/esp_wifi.h"
#include "espressif/esp_sta.h"
#include "espressif/esp_softap.h"
#include "wifi_state_machine.h"

typedef void (* wifi_state_cb_t)();

wifi_state_cb_t on_station_first_connect = NULL;
wifi_state_cb_t on_station_connect = NULL;
wifi_disco_cb_t on_station_disconnect = NULL;

wifi_state_cb_t on_client_connect = NULL;
wifi_state_cb_t on_client_disconnect = NULL;

volatile bool wifi_station_static_ip = false;
volatile bool wifi_station_is_connected = false;

void ICACHE_FLASH_ATTR wifi_event_handler_cb(System_Event_t *event)
{
    static bool station_was_connected = false;
    if (event == NULL) {
        return;
    }

    //os_printf("[WiFi] event %u\n", event->event_id);

    switch (event->event_id) {
        case EVENT_STAMODE_DISCONNECTED:
            wifi_station_is_connected = false;
            Event_StaMode_Disconnected_t *ev = (Event_StaMode_Disconnected_t *)&event->event_info;
            if(on_station_disconnect){
                on_station_disconnect(ev->reason);
            }
            break;
        case EVENT_STAMODE_CONNECTED:
            if(wifi_station_static_ip){
                wifi_station_is_connected = true;
                if(!station_was_connected){
                    station_was_connected = true;
                    if(on_station_first_connect){
                        on_station_first_connect();
                    }
                }
                if(on_station_connect){
                    on_station_connect();
                }
            }
            break;
        case EVENT_STAMODE_DHCP_TIMEOUT:
            if(wifi_station_is_connected){
                wifi_station_is_connected = false;
                if(on_station_disconnect){
                    on_station_disconnect(REASON_UNSPECIFIED);
                }
            }
            break;
        case EVENT_STAMODE_GOT_IP:
            wifi_station_is_connected = true;
            if(!station_was_connected){
                station_was_connected = true;
                if(on_station_first_connect){
                    on_station_first_connect();
                }
            }
            if(on_station_connect){
                on_station_connect();
            }
            break;

        case EVENT_SOFTAPMODE_STACONNECTED:
            if(on_client_connect){
                on_client_connect();
            }
            break;
        case EVENT_SOFTAPMODE_STADISCONNECTED:
            if(on_client_disconnect){
                on_client_disconnect();
            }
            break;
        default:
            break;
    }
}

void ICACHE_FLASH_ATTR set_on_station_first_connect(wifi_state_cb_t cb){
    on_station_first_connect = cb;
}

void ICACHE_FLASH_ATTR set_on_station_connect(wifi_state_cb_t cb){
    on_station_connect = cb;
}

void ICACHE_FLASH_ATTR set_on_station_disconnect(wifi_disco_cb_t cb){
    on_station_disconnect = cb;
}

void ICACHE_FLASH_ATTR set_on_client_connect(wifi_state_cb_t cb){
    on_client_connect = cb;
}

void ICACHE_FLASH_ATTR set_on_client_disconnect(wifi_state_cb_t cb){
    on_client_disconnect = cb;
}

bool ICACHE_FLASH_ATTR wifi_set_mode(WIFI_MODE mode){
    if(!mode){
        bool s = wifi_set_opmode(mode);
        wifi_fpm_open();
        wifi_fpm_set_sleep_type(MODEM_SLEEP_T);
        wifi_fpm_do_sleep(0xFFFFFFFF);
        return s;
    }
    wifi_fpm_close();
    return wifi_set_opmode(mode);
}

WIFI_MODE ICACHE_FLASH_ATTR init_esp_wifi(){
    wifi_set_event_handler_cb(wifi_event_handler_cb);
    WIFI_MODE mode = wifi_get_opmode_default();
    wifi_set_mode(mode);
    return mode;
}

bool ICACHE_FLASH_ATTR start_wifi_station(const char * ssid, const char * pass){
    WIFI_MODE mode = wifi_get_opmode();
    if((mode & STATION_MODE) == 0){
        mode |= STATION_MODE;
        if(!wifi_set_mode(mode)){
            os_printf("Failed to enable Station mode!\n");
            return false;
        }
    }
    if(!ssid){
        os_printf("No SSID Given. Will connect to the station saved in flash\n");
        return true;
    }
    struct station_config config;
    memset(&config, 0, sizeof(struct station_config));
    strcpy(config.ssid, ssid);
    if(pass){
        strcpy(config.password, pass);
    }
    if(!wifi_station_set_config(&config)){
        os_printf("Failed to set Station config!\n");
        return false;
    }

    if(!wifi_station_dhcpc_status()){
        os_printf("DHCP is not started. Starting it...\n");
        if(!wifi_station_dhcpc_start()){
            os_printf("DHCP start failed!\n");
            return false;
        }
    }
    return wifi_station_connect();
}

bool ICACHE_FLASH_ATTR stop_wifi_station(){
    WIFI_MODE mode = wifi_get_opmode();
    mode &= ~STATION_MODE;
    if(!wifi_set_mode(mode)){
        os_printf("Failed to disable Station mode!\n");
        return false;
    }
    return true;
}

bool ICACHE_FLASH_ATTR start_wifi_ap(const char * ssid, const char * pass){
    WIFI_MODE mode = wifi_get_opmode();
    if((mode & SOFTAP_MODE) == 0){
        mode |= SOFTAP_MODE;
        if(!wifi_set_mode(mode)){
            os_printf("Failed to enable AP mode!\n");
            return false;
        }
    }
    if(!ssid){
        os_printf("No SSID Given. Will start the AP saved in flash\n");
        return true;
    }
    struct softap_config config;
    bzero(&config, sizeof(struct softap_config));
    sprintf(config.ssid, ssid);
    if(pass){
        sprintf(config.password, pass);
    }
    return wifi_softap_set_config(&config);
}

bool ICACHE_FLASH_ATTR stop_wifi_ap(){
    WIFI_MODE mode = wifi_get_opmode();
    mode &= ~SOFTAP_MODE;
    if(!wifi_set_mode(mode)){
        os_printf("Failed to disable AP mode!\n");
        return false;
    }
    return true;
}

bool ICACHE_FLASH_ATTR wifi_station_connected(){
    if(!wifi_station_is_connected){
        return false;
    }
    WIFI_MODE mode = wifi_get_opmode();
    if((mode & STATION_MODE) == 0){
        return false;
    }
    STATION_STATUS wifistate = wifi_station_get_connect_status();
    wifi_station_is_connected = (wifistate == STATION_GOT_IP || (wifi_station_static_ip && wifistate == STATION_CONNECTING));
    return wifi_station_is_connected;
}

bool ICACHE_FLASH_ATTR wifi_ap_enabled(){
    return !!(wifi_get_opmode() & SOFTAP_MODE);
}


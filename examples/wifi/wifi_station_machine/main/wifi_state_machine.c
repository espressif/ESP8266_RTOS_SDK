/* WiFi station machine example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "esp_misc.h"
#include "esp_sta.h"
#include "esp_softap.h"
#include "esp_system.h"
#include "esp_timer.h"

#include "wifi_state_machine.h"

#include "user_config.h"

typedef void (* wifi_state_cb_t)();

wifi_state_cb_t on_station_first_connect = NULL;
wifi_state_cb_t on_station_connect = NULL;
wifi_disco_cb_t on_station_disconnect = NULL;

wifi_state_cb_t on_client_connect = NULL;
wifi_state_cb_t on_client_disconnect = NULL;

volatile bool wifi_station_static_ip = false;
volatile bool wifi_station_is_connected = false;

void wifi_event_handler_cb(System_Event_t* event)
{
    static bool station_was_connected = false;

    if (event == NULL) {
        return;
    }

    //printf("[WiFi] event %u\n", event->event_id);

    switch (event->event_id) {
        case EVENT_STAMODE_DISCONNECTED:
            wifi_station_is_connected = false;
            Event_StaMode_Disconnected_t* ev = (Event_StaMode_Disconnected_t*)&event->event_info;

            if (on_station_disconnect) {
                on_station_disconnect(ev->reason);
            }

            break;

        case EVENT_STAMODE_CONNECTED:
            if (wifi_station_static_ip) {
                wifi_station_is_connected = true;

                if (!station_was_connected) {
                    station_was_connected = true;

                    if (on_station_first_connect) {
                        on_station_first_connect();
                    }
                }

                if (on_station_connect) {
                    on_station_connect();
                }
            }

            break;

        case EVENT_STAMODE_DHCP_TIMEOUT:
            if (wifi_station_is_connected) {
                wifi_station_is_connected = false;

                if (on_station_disconnect) {
                    on_station_disconnect(REASON_UNSPECIFIED);
                }
            }

            break;

        case EVENT_STAMODE_GOT_IP:
            wifi_station_is_connected = true;

            if (!station_was_connected) {
                station_was_connected = true;

                if (on_station_first_connect) {
                    on_station_first_connect();
                }
            }

            if (on_station_connect) {
                on_station_connect();
            }

            break;

        case EVENT_SOFTAPMODE_STACONNECTED:
            if (on_client_connect) {
                on_client_connect();
            }

            break;

        case EVENT_SOFTAPMODE_STADISCONNECTED:
            if (on_client_disconnect) {
                on_client_disconnect();
            }

            break;

        default:
            break;
    }
}

void set_on_station_first_connect(wifi_state_cb_t cb)
{
    on_station_first_connect = cb;
}

void set_on_station_connect(wifi_state_cb_t cb)
{
    on_station_connect = cb;
}

void set_on_station_disconnect(wifi_disco_cb_t cb)
{
    on_station_disconnect = cb;
}

void set_on_client_connect(wifi_state_cb_t cb)
{
    on_client_connect = cb;
}

void set_on_client_disconnect(wifi_state_cb_t cb)
{
    on_client_disconnect = cb;
}

bool wifi_set_mode(WIFI_MODE mode)
{
    if (!mode) {
        bool s = wifi_set_opmode(mode);
        wifi_fpm_open();
        wifi_fpm_set_sleep_type(MODEM_SLEEP_T);
        wifi_fpm_do_sleep(0xFFFFFFFF);
        return s;
    }

    wifi_fpm_close();
    return wifi_set_opmode(mode);
}

WIFI_MODE init_esp_wifi(void)
{
    wifi_set_event_handler_cb(wifi_event_handler_cb);
    WIFI_MODE mode = wifi_get_opmode_default();
    wifi_set_mode(mode);
    return mode;
}

bool start_wifi_station(const char* ssid, const char* pass)
{
    WIFI_MODE mode = wifi_get_opmode();

    if ((mode & STATION_MODE) == 0) {
        mode |= STATION_MODE;

        if (!wifi_set_mode(mode)) {
            printf("Failed to enable Station mode!\n");
            return false;
        }
    }

    if (!ssid) {
        printf("No SSID Given. Will connect to the station saved in flash\n");
        return true;
    }

    struct station_config config;

    memset((void*)&config, 0, sizeof(struct station_config));

    strcpy((char*)config.ssid, ssid);

    if (pass) {
        strcpy((char*)config.password, pass);
    }

    if (!wifi_station_set_config(&config)) {
        printf("Failed to set Station config!\n");
        return false;
    }

    if (!wifi_station_dhcpc_status()) {
        printf("DHCP is not started. Starting it...\n");

        if (!wifi_station_dhcpc_start()) {
            printf("DHCP start failed!\n");
            return false;
        }
    }

    return wifi_station_connect();
}

bool stop_wifi_station(void)
{
    WIFI_MODE mode = wifi_get_opmode();
    mode &= ~STATION_MODE;

    if (!wifi_set_mode(mode)) {
        printf("Failed to disable Station mode!\n");
        return false;
    }

    return true;
}

bool start_wifi_ap(const char* ssid, const char* pass)
{
    WIFI_MODE mode = wifi_get_opmode();

    if ((mode & SOFTAP_MODE) == 0) {
        mode |= SOFTAP_MODE;

        if (!wifi_set_mode(mode)) {
            printf("Failed to enable AP mode!\n");
            return false;
        }
    }

    if (!ssid) {
        printf("No SSID Given. Will start the AP saved in flash\n");
        return true;
    }

    struct softap_config config;

    bzero(&config, sizeof(struct softap_config));

    sprintf((char*)config.ssid, ssid);

    if (pass) {
        sprintf((char*)config.password, pass);
    }

    return wifi_softap_set_config(&config);
}

bool stop_wifi_ap(void)
{
    WIFI_MODE mode = wifi_get_opmode();
    mode &= ~SOFTAP_MODE;

    if (!wifi_set_mode(mode)) {
        printf("Failed to disable AP mode!\n");
        return false;
    }

    return true;
}

bool wifi_station_connected(void)
{
    if (!wifi_station_is_connected) {
        return false;
    }

    WIFI_MODE mode = wifi_get_opmode();

    if ((mode & STATION_MODE) == 0) {
        return false;
    }

    STATION_STATUS wifistate = wifi_station_get_connect_status();
    wifi_station_is_connected = (wifistate == STATION_GOT_IP || (wifi_station_static_ip && wifistate == STATION_CONNECTING));
    return wifi_station_is_connected;
}

bool wifi_ap_enabled(void)
{
    return !!(wifi_get_opmode() & SOFTAP_MODE);
}

static os_timer_t timer;

/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32_t user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();
    uint32_t rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        case FLASH_SIZE_64M_MAP_1024_1024:
            rf_cal_sec = 2048 - 5;
            break;

        case FLASH_SIZE_128M_MAP_1024_1024:
            rf_cal_sec = 4096 - 5;
            break;

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

static void wait_for_connection_ready(uint8_t flag)
{
    os_timer_disarm(&timer);

    if (wifi_station_connected()) {
        printf("connected\n");
    } else {
        printf("reconnect after 2s\n");
        os_timer_setfn(&timer, (os_timer_func_t*)wait_for_connection_ready, NULL);
        os_timer_arm(&timer, 2000, 0);
    }
}

static void on_wifi_connect(void)
{
    os_timer_disarm(&timer);
    os_timer_setfn(&timer, (os_timer_func_t*)wait_for_connection_ready, NULL);
    os_timer_arm(&timer, 100, 0);
}

static void on_wifi_disconnect(uint8_t reason)
{
    printf("disconnect %d\n", reason);
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
    printf("SDK version:%s\n", system_get_sdk_version());

    set_on_station_connect(on_wifi_connect);
    set_on_station_disconnect(on_wifi_disconnect);
    init_esp_wifi();
    stop_wifi_ap();
    start_wifi_station(SSID, PASSWORD);
}

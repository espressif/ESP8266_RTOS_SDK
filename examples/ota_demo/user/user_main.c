/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "lwip/ip_addr.h"
#include "esp_sta.h"
#include "esp_wifi.h"

#include "ota.h"

int got_ip_flag = 0;

extern void local_ota_task(void *pvParameter);

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

// WiFi callback function
void event_handler(System_Event_t *event)
{
    switch (event->event_id) {
    case EVENT_STAMODE_GOT_IP:
        printf("WiFi connected\n");
        got_ip_flag = 1;
        break;

    case EVENT_STAMODE_DISCONNECTED:
        printf("WiFi disconnected, try to connect...\n");
        got_ip_flag = 0;
        wifi_station_connect();
        break;

    default:
        break;
    }
}

void initialize_wifi(void)
{
    wifi_set_opmode(STATION_MODE);

    // set AP parameter
    struct station_config config;
    bzero(&config, sizeof(struct station_config));
    sprintf(config.ssid, WIFI_SSID);
    sprintf(config.password, WIFI_PASSWORD);
    wifi_station_set_config(&config);

    wifi_station_set_auto_connect(true);
    wifi_station_set_reconnect_policy(true);
    wifi_set_event_handler_cb(event_handler);
}

void user_init(void)
{
    initialize_wifi();

    if (xTaskCreate(local_ota_task, "main_process", 1024, NULL, 5, NULL) != pdPASS) {
        printf("lota create failed\n");
    }
}



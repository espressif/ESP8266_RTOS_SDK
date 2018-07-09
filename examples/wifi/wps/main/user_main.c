/* WPS example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_sta.h"
#include "esp_system.h"
#include "esp_wps.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static void user_wps_status_cb(int status)
{
    printf("scan status %d\n", status);

    switch (status) {
        case WPS_CB_ST_SUCCESS:
            wifi_wps_disable();
            wifi_station_connect();
            break;

        case WPS_CB_ST_FAILED:
        case WPS_CB_ST_TIMEOUT:
            wifi_wps_start();
            break;
    }
}

static void user_wps_start(void)
{
    wifi_wps_disable();
    wifi_wps_enable(WPS_TYPE_PBC);
    wifi_set_wps_cb(user_wps_status_cb);
    wifi_wps_start();
}

static void wps_task(void* pvParameters)
{
    wifi_set_opmode(STATION_MODE);
    user_wps_start();
    vTaskDelete(NULL);
}

void user_init(void)
{
    xTaskCreate(wps_task, "wps_task", 4096, NULL, 4, NULL);
}

/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_main.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 *     2015/7/3, v1.0 create this file.
*******************************************************************************/

#include "esp_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_wps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

LOCAL void user_wps_status_cb(int status)
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

LOCAL void user_wps_start(void)
{
    wifi_wps_disable();
    wifi_wps_enable(WPS_TYPE_PBC);
    wifi_set_wps_cb(user_wps_status_cb);
    wifi_wps_start();
}

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
uint32 user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

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
LOCAL void wps_task(void *pvParameters)
{
    wifi_set_opmode(STATION_MODE);
    user_wps_start();
    vTaskDelete(NULL);
}
void user_init(void)
{

     xTaskCreate(wps_task, "wps_task", 1024, NULL, 4, NULL);
    

}
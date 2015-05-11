/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_main.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 *     2014/12/1, v1.0 create this file.
*******************************************************************************/
#include "esp_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "udhcp/dhcpd.h"

#define server_ip "192.168.101.142"
#define server_port 9669

void ICACHE_FLASH_ATTR
smartconfig_done(void *data)
{
	struct station_config *sta_conf = data;
	
	wifi_station_set_config(sta_conf);
	wifi_station_disconnect();
	wifi_station_connect();
}

void ICACHE_FLASH_ATTR
smartconfig_task(void *pvParameters)
{
    smartconfig_start(SC_TYPE_ESPTOUCH, smartconfig_done);//SC_TYPE_AIRKISS

    vTaskDelete(NULL);
}

void ICACHE_FLASH_ATTR
sc_smartconfig_check(void)
{
    if(SC_STATUS_LINK_OVER == smartconfig_get_status()) {
        smartconfig_stop();
    }
    
}
/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_init(void)
{
    printf("SDK version:%s\n", system_get_sdk_version());

    /* need to set opmode before you set config */
    wifi_set_opmode(STATION_MODE);

    xTaskCreate(smartconfig_task, "smartconfig_task", 256, NULL, 2, NULL);
}


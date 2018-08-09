/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *******************************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "MQTTClient.h"

/* The examples use simple WiFi configuration that you can set via
   'make menuconfig'.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_WIFI_SSID CONFIG_WIFI_SSID
#define EXAMPLE_WIFI_PASS CONFIG_WIFI_PASSWORD

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;

#define MQTT_BROKER  "iot.eclipse.org"  /* MQTT Broker Address*/
#define MQTT_PORT    1883             /* MQTT Port*/

#define MQTT_CLIENT_THREAD_NAME         "mqtt_client_thread"
#define MQTT_CLIENT_THREAD_STACK_WORDS  8192
#define MQTT_CLIENT_THREAD_PRIO         8

static const char *TAG = "example";

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_WIFI_SSID,
            .password = EXAMPLE_WIFI_PASS,
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

static void messageArrived(MessageData* data)
{
    printf("Message arrived: %s\n", (char*)data->message->payload);
}

static void mqtt_client_thread(void* pvParameters)
{
    MQTTClient client;
    Network network;
    unsigned char sendbuf[80], readbuf[80] = {0};
    int rc = 0, count = 0;
    MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;

    printf("mqtt client thread starts\n");

    /* Wait for the callback to set the CONNECTED_BIT in the
       event group.
    */
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                        false, true, portMAX_DELAY);
    ESP_LOGI(TAG, "Connected to AP");

    NetworkInit(&network);
    MQTTClientInit(&client, &network, 30000, sendbuf, sizeof(sendbuf), readbuf, sizeof(readbuf));

    char* address = MQTT_BROKER;

    if ((rc = NetworkConnect(&network, address, MQTT_PORT)) != 0) {
        printf("Return code from network connect is %d\n", rc);
    }

#if defined(MQTT_TASK)

    if ((rc = MQTTStartTask(&client)) != pdPASS) {
        printf("Return code from start tasks is %d\n", rc);
    } else {
        printf("Use MQTTStartTask\n");
    }

#endif

    connectData.MQTTVersion = 3;
    connectData.clientID.cstring = "ESP8266_sample";

    if ((rc = MQTTConnect(&client, &connectData)) != 0) {
        printf("Return code from MQTT connect is %d\n", rc);
    } else {
        printf("MQTT Connected\n");
    }

    if ((rc = MQTTSubscribe(&client, "ESP8266/sample/sub", 2, messageArrived)) != 0) {
        printf("Return code from MQTT subscribe is %d\n", rc);
    } else {
        printf("MQTT subscribe to topic \"ESP8266/sample/sub\"\n");
    }

    while (++count) {
        MQTTMessage message;
        char payload[30];

        message.qos = QOS2;
        message.retained = 0;
        message.payload = payload;
        sprintf(payload, "message number %d", count);
        message.payloadlen = strlen(payload);

        if ((rc = MQTTPublish(&client, "ESP8266/sample/pub", &message)) != 0) {
            printf("Return code from MQTT publish is %d\n", rc);
        } else {
            printf("MQTT publish topic \"ESP8266/sample/pub\", message number is %d\n", count);
        }

        vTaskDelay(1000 / portTICK_RATE_MS);  //send every 1 seconds
    }

    printf("mqtt_client_thread going to be deleted\n");
    vTaskDelete(NULL);
    return;
}

void app_main(void)
{
    ESP_ERROR_CHECK( nvs_flash_init() );
    initialise_wifi();
    xTaskCreate(&mqtt_client_thread,
                MQTT_CLIENT_THREAD_NAME,
                MQTT_CLIENT_THREAD_STACK_WORDS,
                NULL,
                MQTT_CLIENT_THREAD_PRIO,
                NULL);
}

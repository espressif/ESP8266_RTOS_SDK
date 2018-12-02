#ifndef CSRO_CONFIG_H_
#define CSRO_CONFIG_H_

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/uart.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_smartconfig.h"
#include "portmacro.h"
#include "FreeRTOSConfig.h"
#include "MQTTClient.h"
#include "FreeRTOS/MQTTFreeRTOS.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "cJSON.h"


#define MQTT_BUFFER_LENGTH  1000
#define	DEBUG


#ifdef 	   DEBUG
   #define debug(format, ...) printf(format, ## __VA_ARGS__)
#else
   #define debug(format, ...)
#endif

typedef enum
{
    IDLE = 0,
    SMARTCONFIG = 1,
    SMARTCONFIG_TIMEOUT = 2,
    NORMAL_START_NOWIFI = 3,
    NORMAL_START_NOSERVER = 4,
    NORMAL_START_OK = 5,
    RESET_PENDING = 6
} csro_system_status;


typedef struct
{
    char ssid[50];
    char pass[50];
    int8_t flag;
} csro_wifi_param;

typedef struct
{
    uint8_t device_type[20];
    uint8_t mac[6];
    uint8_t mac_string[20];
    uint8_t host_name[20];
    uint8_t ip[4];
    uint8_t ip_string[20];
    uint8_t power_on_time[30];
    uint8_t serv_conn_time[30];
    uint8_t time_stamp[30];
    int32_t power_on_count;
    uint32_t wifi_conn_count;
    uint32_t serv_conn_count;
    uint32_t publish_count;
    uint32_t run_minutes;
    csro_system_status status;
} csro_system_info;

typedef struct
{
    uint8_t id[50];
    uint8_t name[50];
    uint8_t password[50];
    uint8_t sub_topic[100];
    uint8_t pub_topic[100];
    uint8_t sendbuf[MQTT_BUFFER_LENGTH];
    uint8_t recvbuf[MQTT_BUFFER_LENGTH];
    uint8_t content[MQTT_BUFFER_LENGTH];
    uint8_t broker[50];
    uint8_t prefix[50];
    struct Network network;
    MQTTClient client;
    MQTTMessage message;
} csro_mqtt;

typedef struct
{
    time_t time_now;
    time_t time_run;
    struct tm timeinfo;
    char strtime[64];
} csro_date_time;


#endif
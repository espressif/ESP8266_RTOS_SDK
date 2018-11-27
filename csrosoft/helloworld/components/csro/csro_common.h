#ifndef CSRO_COMMON_H_
#define CSRO_COMMON_H_

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
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

void csro_system_init(void);

#endif
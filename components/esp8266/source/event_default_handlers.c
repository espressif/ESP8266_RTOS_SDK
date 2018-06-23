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

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_system.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "tcpip_adapter.h"
#include "esp_log.h"

static const char* TAG = "event";

typedef esp_err_t (*system_event_handler_t)(system_event_t *e);

static esp_err_t system_event_ap_start_handle_default(system_event_t *event);
static esp_err_t system_event_ap_stop_handle_default(system_event_t *event);
static esp_err_t system_event_sta_start_handle_default(system_event_t *event);
static esp_err_t system_event_sta_stop_handle_default(system_event_t *event);
static esp_err_t system_event_sta_connected_handle_default(system_event_t *event);
static esp_err_t system_event_sta_disconnected_handle_default(system_event_t *event);
static esp_err_t system_event_sta_got_ip_default(system_event_t *event);
static esp_err_t system_event_sta_lost_ip_default(system_event_t *event);

/* Default event handler functions

   Any entry in this table which is disabled by config will have a NULL handler.
*/
static system_event_handler_t default_event_handlers[SYSTEM_EVENT_MAX] = { 0 };

static esp_err_t system_event_sta_got_ip_default(system_event_t *event)
{
    return ESP_OK;
}

static esp_err_t system_event_sta_lost_ip_default(system_event_t *event)
{
    return ESP_OK;
}

esp_err_t system_event_ap_start_handle_default(system_event_t *event)
{
    return ESP_OK;
}

esp_err_t system_event_ap_stop_handle_default(system_event_t *event)
{
    return ESP_OK;
}

esp_err_t system_event_sta_start_handle_default(system_event_t *event)
{
    return ESP_OK;
}

esp_err_t system_event_sta_stop_handle_default(system_event_t *event)
{

    return ESP_OK;
}

esp_err_t system_event_sta_connected_handle_default(system_event_t *event)
{
    return ESP_OK;
}

esp_err_t system_event_sta_disconnected_handle_default(system_event_t *event)
{
    return ESP_OK;
}



esp_err_t esp_event_process_default(system_event_t *event)
{
    if (event == NULL) {
        return ESP_FAIL;
    }

    if ((event->event_id < SYSTEM_EVENT_MAX)) {
        if (default_event_handlers[event->event_id] != NULL) {
            default_event_handlers[event->event_id](event);
        }
    } else {
        return ESP_FAIL;
    }
    return ESP_OK;
}

void esp_event_set_default_wifi_handlers()
{
     default_event_handlers[SYSTEM_EVENT_STA_START]        = system_event_sta_start_handle_default;
     default_event_handlers[SYSTEM_EVENT_STA_STOP]         = system_event_sta_stop_handle_default;
     default_event_handlers[SYSTEM_EVENT_STA_CONNECTED]    = system_event_sta_connected_handle_default;
     default_event_handlers[SYSTEM_EVENT_STA_DISCONNECTED] = system_event_sta_disconnected_handle_default;
     default_event_handlers[SYSTEM_EVENT_STA_GOT_IP]       = system_event_sta_got_ip_default;
     default_event_handlers[SYSTEM_EVENT_STA_LOST_IP]      = system_event_sta_lost_ip_default;
     default_event_handlers[SYSTEM_EVENT_AP_START]         = system_event_ap_start_handle_default;
     default_event_handlers[SYSTEM_EVENT_AP_STOP]          = system_event_ap_stop_handle_default;
}
#include "csro_common.h"

static EventGroupHandle_t wifi_event_group;
static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;

static void sc_callback(smartconfig_status_t status, void *pdata)
{
    switch (status) {
        case SC_STATUS_WAIT:
            debug("SC_STATUS_WAIT\r\n");
            break;
        case SC_STATUS_FIND_CHANNEL:
            debug("SC_STATUS_FINDING_CHANNEL\r\n");
            break;
        case SC_STATUS_GETTING_SSID_PSWD:
            debug("SC_STATUS_GETTING_SSID_PSWD\r\n");
            break;
        case SC_STATUS_LINK:
            debug("SC_STATUS_LINK\r\n");
            wifi_config_t *wifi_config = pdata;
            //strcpy(wifi_param.pass, wifi_config->sta.ssid);
            //strcpy(wifi_param.pass, wifi_config->sta.password);
            strcpy(wifi_param.ssid, "Jupiter");
            strcpy(wifi_param.pass, "150933205");
            debug("SSID:%s.\r\n", wifi_config->sta.ssid);
            debug("PASSWORD:%s.\r\n", wifi_config->sta.password);
            esp_wifi_disconnect();
            esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_config);
            esp_wifi_connect();
            break;
        case SC_STATUS_LINK_OVER:
            debug("SC_STATUS_LINK_OVER\r\n");
            xEventGroupSetBits(wifi_event_group, ESPTOUCH_DONE_BIT);
            break;
        default:
            break;
    }
}

void smartconfig_task(void * parm)
{
    EventBits_t uxBits;
    esp_smartconfig_set_type(SC_TYPE_ESPTOUCH);
    esp_smartconfig_start(sc_callback);
    while (1) 
    {
        uxBits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY); 
        if(uxBits & CONNECTED_BIT) 
        {
            debug("WiFi Connected to ap\r\n");
        }
        if(uxBits & ESPTOUCH_DONE_BIT) 
        {
            debug("smartconfig over\r\n");
            esp_smartconfig_stop();
            nvs_handle handle;
            nvs_open("system_info", NVS_READWRITE, &handle);
            nvs_set_str(handle, "ssid", wifi_param.ssid);
            nvs_set_str(handle, "pass", wifi_param.pass);
            nvs_set_i8(handle, "wifi_flag", 1);
            nvs_commit(handle);
            nvs_close(handle);
            esp_restart();
            vTaskDelete(NULL);
        }
    }
}


static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        debug("SYSTEM_EVENT_STA_START\r\n");
        xTaskCreate(smartconfig_task, "smartconfig_task", 4096, NULL, 3, NULL);
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        debug("SYSTEM_EVENT_STA_GOT_IP\r\n");
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        debug("SYSTEM_EVENT_STA_DISCONNECTED\r\n");
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

void csro_smartconfig_task(void *pvParameters)
{
    csro_system_set_status(SMARTCONFIG);
    wifi_event_group = xEventGroupCreate();
    tcpip_adapter_init();
    esp_event_loop_init(event_handler, NULL);
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&config);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();

    while(1)
    {
        debug("running smartconfig task. free heap %d. \r\n", esp_get_free_heap_size());
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}
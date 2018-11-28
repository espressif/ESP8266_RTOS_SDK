#include "csro_common.h"

csro_system_info    sys_info;
csro_wifi_param     wifi_param;


void idle_task(void *pvParameters)
{
    while(1)
    {
        debug("running idle task.\r\n");
        debug("system has powered on for %d times. ", sys_info.power_on_count);
        debug("system free heap %d. ", esp_get_free_heap_size());
        vTaskDelay(10 / portTICK_RATE_MS);
    }
}


void csro_system_init()
{
    nvs_handle handle;

    debug("==========CSRO 2018.11.28 Derek Li==========\r\n");

    nvs_flash_init();
    nvs_open("system_info", NVS_READWRITE, &handle);
    nvs_get_i32(handle, "power_on_count", &sys_info.power_on_count);
    sys_info.power_on_count = sys_info.power_on_count + 1;
    nvs_set_i32(handle, "power_on_count", sys_info.power_on_count);
    nvs_commit(handle);

    nvs_get_i8(handle, "wifi_flag", &wifi_param.flag);
    
    if (wifi_param.flag == 1)
    {
        csro_system_set_status(NORMAL_START_NOWIFI);
        nvs_get_str(handle, "ssid", wifi_param.ssid, 40);
        nvs_get_str(handle, "pass", wifi_param.pass, 40);
        xTaskCreate(csro_mqtt_task, "csro_mqtt_task", 1024, NULL, 6, NULL);
    }
    else
    {
        csro_system_set_status(SMARTCONFIG);
        
    }
    





    xTaskCreate(idle_task, "idle_task", 1024, NULL, 1, NULL);
}


void csro_system_set_status(csro_system_status status)
{
    if((sys_info.status != status) && (sys_info.status != RESET_PENDING))
    {
        sys_info.status = status;
        debug("system status is %d", sys_info.status);
    }
}
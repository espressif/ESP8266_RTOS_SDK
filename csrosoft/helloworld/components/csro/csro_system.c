#include "csro_common.h"

csro_system_info    sys_info;
csro_wifi_param     wifi_param;

void csro_system_init()
{
    nvs_handle handle;
    debug("==========CSRO 2018.11.28 Derek Li==========\r\n");

    nvs_flash_init();
    nvs_open("system_info", NVS_READWRITE, &handle);
    nvs_get_i32(handle, "power_on_count", &sys_info.power_on_count);
    sys_info.power_on_count = sys_info.power_on_count + 1;
    nvs_set_i32(handle, "power_on_count", sys_info.power_on_count);
    
    nvs_get_i8(handle, "wifi_flag", &wifi_param.flag);
    if (wifi_param.flag)
    {
        size_t ssid_len = 0;
        size_t pass_len = 0;
        bzero(wifi_param.ssid, 40);
        bzero(wifi_param.pass, 40);
        nvs_get_str(handle, "ssid", &wifi_param.ssid[0], &ssid_len);
        nvs_get_str(handle, "pass", &wifi_param.pass[0], &pass_len);
        debug("ssid: %d, pass: %d.\r\n", ssid_len, pass_len);
        debug("ssid: %d, pass: %d.\r\n", strlen(wifi_param.ssid), strlen(wifi_param.pass));
        xTaskCreate(csro_mqtt_task, "csro_mqtt_task", 1024, NULL, 6, NULL);
    }
    else
    {
        xTaskCreate(csro_smartconfig_task, "csro_smartconfig_task", 1024, NULL, 6, NULL);
    }
    nvs_commit(handle);
    nvs_close(handle);
}


void csro_system_set_status(csro_system_status status)
{
    if((sys_info.status != status) && (sys_info.status != RESET_PENDING))
    {
        sys_info.status = status;
        debug("system status is %d.\r\n", sys_info.status);
    }
}
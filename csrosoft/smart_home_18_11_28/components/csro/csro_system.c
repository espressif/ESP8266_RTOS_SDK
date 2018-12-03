#include "csro_common.h"
#include "../device/csro_device.h"

csro_system_info    sys_info;
csro_wifi_param     wifi_param;
csro_date_time      date_time;
csro_mqtt           mqtt;

void test_clear_wifi_param(void)
{
    nvs_handle handle;
    nvs_open("system_info", NVS_READWRITE, &handle);
    nvs_erase_key(handle, "wifi_flag");
    nvs_erase_key(handle, "ssid");
    nvs_erase_key(handle, "pass");
    nvs_commit(handle);
    nvs_close(handle);
}

void test_fake_wifi_param(void)
{
    nvs_handle handle;
    nvs_open("system_info", NVS_READWRITE, &handle);
    nvs_set_str(handle, "ssid", "Jupiter");
    nvs_set_str(handle, "pass", "150933205");
    nvs_set_i8(handle, "wifi_flag", 1);
    nvs_commit(handle);
    nvs_close(handle);
}

void csro_system_init(void)
{
    nvs_handle handle;
    debug("==========CSRO 2018.11.28 Derek Li==========\r\n");

    nvs_flash_init();
    // test_clear_wifi_param();
    // test_fake_wifi_param();
    nvs_open("system_info", NVS_READWRITE, &handle);
    nvs_get_i32(handle, "power_on_count", &sys_info.power_on_count);
    sys_info.power_on_count = sys_info.power_on_count + 1;
    nvs_set_i32(handle, "power_on_count", sys_info.power_on_count);
    debug("power count %d.\r\n", sys_info.power_on_count);

    csro_datetime_init();
    csro_device_init();
    
    nvs_get_i8(handle, "wifi_flag", &wifi_param.flag);
    if (wifi_param.flag)
    {
        size_t ssid_len = 0;
        size_t pass_len = 0;
        nvs_get_str(handle, "ssid", NULL, &ssid_len);
        nvs_get_str(handle, "ssid", wifi_param.ssid, &ssid_len);
        nvs_get_str(handle, "pass", NULL, &pass_len);
        nvs_get_str(handle, "pass", wifi_param.pass, &pass_len);
        xTaskCreate(csro_mqtt_task, "csro_mqtt_task", 2048, NULL, 6, NULL);
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

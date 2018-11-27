#include "csro_common.h"



void csro_power_count(void)
{
    nvs_handle handle;
    int32_t count = 0;
    nvs_flash_init();
    nvs_open("powercount", NVS_READWRITE, &handle);
    nvs_get_i32(handle, "count", &count);

    printf("Power Count = %d\r\n", count);

    count = count + 1;
    nvs_set_i32(handle, "count", count);
    nvs_commit(handle);
    nvs_close(handle);
}


void csro_system_init(void)
{
    csro_power_count();
}
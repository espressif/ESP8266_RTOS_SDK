#include "csro_device.h"
#include "../modbus/modbus_master.h"

uint8_t result[10];
uint16_t holidng[10];

static void modbus_master_task(void *pvParameters)
{
	for(;;)
	{
        modbus_master_read_coils(&Master, 1, 0, 8, result);
        for(size_t i = 0; i < 10; i++)
        {
            debug("%d ", result[i]);
        }
        debug("\r\n");

        modbus_master_read_holding_regs(&Master, 1, 0, 8, holidng);
        for(size_t i = 0; i < 10; i++)
        {
            debug("%d ", holidng[i]);
        }
        debug("\r\n");
        vTaskDelay(500 / portTICK_RATE_MS);
    }
	vTaskDelete(NULL);
}




void csro_air_system_prepare_basic_message(void)
{

}

void csro_air_system_handle_message(MessageData* data)
{

}

void csro_air_system_init(void)
{
    modbus_master_init();
    xTaskCreate(modbus_master_task, "modbus_master_task", 1024, NULL, 5, NULL);
}
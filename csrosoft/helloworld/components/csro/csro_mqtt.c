#include "csro_common.h"

void csro_mqtt_task(void *pvParameters)
{
    while(1)
    {
        debug("running mqtt task. free heap %d. \r\n", esp_get_free_heap_size());
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}
#include <stdio.h>
#include <stdint.h>

#include "esp_system.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "unity.h"
#include "unity_config.h"

void unityTask(void *pvParameters)
{
    vTaskDelay(2); /* Delay a bit to let the main task be deleted */
    unity_run_menu(); /* Doesn't return */
}

void app_main(void)
{
    xTaskCreate(unityTask, "unityTask", 8192, NULL, UNITY_FREERTOS_PRIORITY, NULL);
}

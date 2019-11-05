/* IR TX Example
  This is an ir tx demo of ir function( NEC CODE ,32 BIT LENGTH)
  This example code is in the Public Domain (or CC0 licensed, at your option.)
  Unless required by applicable law or agreed to in writing, this
  software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
  CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "driver/ir_tx.h"

static const char *TAG = "main";

#define IR_TX_IO_NUM 14

void ir_tx_task(void *arg)
{
    ir_tx_config_t ir_tx_config = {
        .io_num = IR_TX_IO_NUM,
        .freq = 38000,
        .timer = IR_TX_WDEV_TIMER // WDEV timer will be more accurate, but PWM will not work
    };

    ir_tx_init(&ir_tx_config);

    ir_tx_nec_data_t ir_data[5];
    /*
        The standard NEC ir code is:
        addr + ~addr + cmd + ~cmd
    */
    ir_data[0].addr1 = 0x55;
    ir_data[0].addr2 = ~0x55;
    ir_data[0].cmd1 = 0x00;
    ir_data[0].cmd2 = ~0x00;

    while (1) {
        for (int x = 1; x < 5; x++) { // repeat 4 times
            ir_data[x] = ir_data[0];
        }

        ir_tx_send_data(ir_data, 5, portMAX_DELAY);
        ESP_LOGI(TAG, "ir tx nec: addr:%02xh;cmd:%02xh;repeat:%d", ir_data[0].addr1, ir_data[0].cmd1, 4);
        ir_data[0].cmd1++;
        ir_data[0].cmd2 = ~ir_data[0].cmd1;
        vTaskDelay(1000 / portTICK_RATE_MS);
    }

    vTaskDelete(NULL);
}

void app_main()
{
    xTaskCreate(ir_tx_task, "ir_tx_task", 2048, NULL, 5, NULL);
}
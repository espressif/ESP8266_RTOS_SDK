/* spi_slave example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "driver/spi.h"
#include "driver/hspi_logic_layer.h"

static const char *TAG = "spi_slave_example";

#define SPI_SLAVE_HANDSHARK_GPIO                4

#define SPI_WRITE_BUFFER_MAX_SIZE               2048
#define SPI_READ_BUFFER_MAX_SIZE                1024

//#define SPI_SLAVE_TEST_SEND    // Define the macro is slave send mode, delete will be master send mode

#ifdef SPI_SLAVE_TEST_SEND
static void IRAM_ATTR spi_slave_write_master_task(void *arg)
{
    static uint8_t write_data[SPI_WRITE_BUFFER_MAX_SIZE];
    uint32_t total_tx_count = 0;

    for (int32_t loop = 0; loop < SPI_WRITE_BUFFER_MAX_SIZE; loop++) {
        write_data[loop] = 0xa3;
    }

    vTaskDelay(5000 / portTICK_RATE_MS);
    ESP_LOGI(TAG, "SPI slave start send data");
    for (;;) {
        total_tx_count += hspi_slave_logic_write_data(write_data, SPI_READ_BUFFER_MAX_SIZE, portMAX_DELAY);

        if (total_tx_count >= 10*1024*1024) {
            ESP_LOGI(TAG, "send done; %d bytes\r\n", total_tx_count);
            break;
        }
    }
    vTaskDelete(NULL);
}
#else
uint32_t read_count = 0;

static void spi_slave_receive_count_task(void* arg)
{
    uint32_t tmp_count = 0;
    while(1){
        ESP_LOGI(TAG,"recv_count:  %d , speed: %dB/s\n", read_count, ((read_count - tmp_count)/2));
        tmp_count = read_count;
        vTaskDelay(2000 / portTICK_RATE_MS);
    }
}

static void IRAM_ATTR spi_slave_read_master_task(void *arg)
{
    static uint8_t read_data[SPI_READ_BUFFER_MAX_SIZE];
    uint32_t read_len = 0;
    for (;;) {
        read_len = hspi_slave_logic_read_data(read_data, SPI_READ_BUFFER_MAX_SIZE, 1000);
        for(int i=0;i< read_len;i++){
            
            if(read_data[i] != 0x34){
                ESP_LOGE(TAG,"Receive error data: %x\n", read_data[i]);
            }
            
        }
        read_count += read_len;
        memset(read_data, 0x0, SPI_READ_BUFFER_MAX_SIZE);
    }
}
#endif

void app_main(void)
{
    spi_config_t spi_config;
    // Load default interface parameters
    // CS_EN:1, MISO_EN:1, MOSI_EN:1, BYTE_TX_ORDER:1, BYTE_TX_ORDER:1, BIT_RX_ORDER:0, BIT_TX_ORDER:0, CPHA:0, CPOL:0
    spi_config.interface.val = SPI_DEFAULT_INTERFACE;

    // Load default interrupt enable
    // TRANS_DONE: false, WRITE_STATUS: true, READ_STATUS: true, WRITE_BUFFER: true, READ_BUFFER: ture
    spi_config.intr_enable.val = SPI_SLAVE_DEFAULT_INTR_ENABLE;
    // Set SPI to slave mode
    spi_config.mode = SPI_SLAVE_MODE;
    // Register SPI event callback function
    spi_config.event_cb = NULL;
    spi_init(HSPI_HOST, &spi_config);

    hspi_slave_logic_device_create(SPI_SLAVE_HANDSHARK_GPIO, 1, SPI_WRITE_BUFFER_MAX_SIZE, SPI_READ_BUFFER_MAX_SIZE);
#ifdef SPI_SLAVE_TEST_SEND
    xTaskCreate(spi_slave_write_master_task, "spi_slave_write_master_task", 2048, NULL, 2, NULL);
#else
    // create spi_slave_read_master_task
    xTaskCreate(spi_slave_read_master_task, "spi_slave_read_master_task", 2048, NULL, 5, NULL);
    xTaskCreate(spi_slave_receive_count_task, "spi_slave_receive_count_task", 2048, NULL, 4, NULL);
#endif
}



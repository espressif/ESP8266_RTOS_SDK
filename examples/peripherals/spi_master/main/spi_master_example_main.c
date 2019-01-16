/* spi_master example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp8266/gpio_struct.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "driver/spi.h"

static const char *TAG = "spi_master_example";

#define SPI_SLAVE_HANDSHARK_GPIO     4
#define SPI_SLAVE_HANDSHARK_SEL      (1ULL<<SPI_SLAVE_HANDSHARK_GPIO)

static xQueueHandle gpio_evt_queue = NULL;

static void gpio_isr_handler(void *arg)
{
    BaseType_t xHigherPriorityTaskWoken;
    uint32_t gpio_num = (uint32_t) arg;

    xQueueSendFromISR(gpio_evt_queue, &gpio_num, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken == pdTRUE) {
        taskYIELD();
    }
}

void IRAM_ATTR spi_event_callback(int event, void *arg)
{
    switch (event) {
        case SPI_INIT_EVENT: {

        }
        break;

        case SPI_TRANS_START_EVENT: {

        }
        break;

        case SPI_TRANS_DONE_EVENT: {

        }
        break;

        case SPI_DEINIT_EVENT: {

        }
        break;
    }

}

static void spi_master_write_slave_task(void *arg)
{
    uint16_t cmd;
    uint32_t addr;
    uint32_t write_data[8];
    spi_trans_t trans;
    uint32_t status;

    trans.cmd = &cmd;
    trans.addr = &addr;
    addr = 0x0;
    write_data[0] = 0;
    write_data[1] = 0x11111111;
    write_data[2] = 0x22222222;
    write_data[3] = 0x33333333;
    write_data[4] = 0x44444444;
    write_data[5] = 0x55555555;
    write_data[6] = 0x66666666;
    write_data[7] = 0x77777777;

    while (1) {
        if (addr % 50 == 0) {
            vTaskDelay(1000 / portTICK_RATE_MS);
        }

        trans.miso = &status;
        trans.bits.val = 0;
        trans.bits.cmd = 8 * 1;
        trans.bits.miso = 32 * 1;
        // Read status from the ESP8266 Slave use "SPI_MASTER_READ_STATUS_FROM_SLAVE_CMD" cmd
        cmd = SPI_MASTER_READ_STATUS_FROM_SLAVE_CMD;
        // Get the slave status and send data when the slave is idle
        while (1) {
            spi_trans(HSPI_HOST, trans);
            if (status == true) {
                break;
            }
            vTaskDelay(10 / portTICK_RATE_MS);
        }

        trans.mosi = write_data;
        trans.bits.val = 0;
        trans.bits.cmd = 8 * 1;
        trans.bits.addr = 32 * 1;
        trans.bits.mosi = 32 * 8;
        // Write data to the ESP8266 Slave use "SPI_MASTER_WRITE_DATA_TO_SLAVE_CMD" cmd
        cmd = SPI_MASTER_WRITE_DATA_TO_SLAVE_CMD;
        spi_trans(HSPI_HOST, trans);

        addr++;
        write_data[0]++;
        vTaskDelay(10 / portTICK_RATE_MS);
    }
}

static void spi_master_read_slave_task(void *arg)
{
    int x;
    uint32_t io_num;
    uint16_t cmd;
    uint32_t addr;
    uint32_t read_data[8];
    spi_trans_t trans;
    uint32_t status = true;

    trans.cmd = &cmd;
    // Write status to the ESP8266 Slave use "SPI_MASTER_WRITE_STATUS_TO_SLAVE_CMD" cmd
    cmd = SPI_MASTER_WRITE_STATUS_TO_SLAVE_CMD;
    trans.mosi = &status;
    trans.bits.val = 0;
    trans.bits.cmd = 8 * 1;
    trans.bits.mosi = 32 * 1;
    spi_trans(HSPI_HOST, trans);

    trans.addr = &addr;
    trans.miso = read_data;
    trans.bits.val = 0;
    trans.bits.cmd = 8 * 1;
    trans.bits.addr = 32 * 1;
    trans.bits.miso = 32 * 8;
    // Read data from the ESP8266 Slave use "SPI_MASTER_READ_DATA_FROM_SLAVE_CMD" cmd
    cmd = SPI_MASTER_READ_DATA_FROM_SLAVE_CMD;
    addr = 0x0;

    while (1) {
        xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY);

        if (SPI_SLAVE_HANDSHARK_GPIO != io_num) {
            break;
        }

        spi_trans(HSPI_HOST, trans);
        ESP_LOGI(TAG, "------Master read------\n");
        ESP_LOGI(TAG, "addr: 0x%x\n", addr);

        for (x = 0; x < 8; x++) {
            ESP_LOGI(TAG, "read_data[%d]: 0x%x\n", x, read_data[x]);
        }

        addr++;
    }
}

void app_main(void)
{
    gpio_evt_queue = xQueueCreate(1, sizeof(uint32_t));

    ESP_LOGI(TAG, "init gpio");
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = SPI_SLAVE_HANDSHARK_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(SPI_SLAVE_HANDSHARK_GPIO, gpio_isr_handler, (void *) SPI_SLAVE_HANDSHARK_GPIO);

    ESP_LOGI(TAG, "init spi");
    spi_config_t spi_config;
    // Load default interface parameters
    // CS_EN:1, MISO_EN:1, MOSI_EN:1, BYTE_TX_ORDER:1, BYTE_TX_ORDER:1, BIT_RX_ORDER:0, BIT_TX_ORDER:0, CPHA:0, CPOL:0
    spi_config.interface.val = SPI_DEFAULT_INTERFACE;
    // Load default interrupt enable
    // TRANS_DONE: true, WRITE_STATUS: false, READ_STATUS: false, WRITE_BUFFER: false, READ_BUFFER: false
    spi_config.intr_enable.val = SPI_MASTER_DEFAULT_INTR_ENABLE;
    // Set SPI to master mode
    // ESP8266 Only support half-duplex
    spi_config.mode = SPI_MASTER_MODE;
    // Set the SPI clock frequency division factor
    spi_config.clk_div = SPI_10MHz_DIV;
    // Register SPI event callback function
    spi_config.event_cb = spi_event_callback;
    spi_init(HSPI_HOST, &spi_config);

    // create spi_master_write_slave_task
    xTaskCreate(spi_master_write_slave_task, "spi_master_write_slave_task", 2048, NULL, 10, NULL);

    // create spi_master_read_slave_task
    xTaskCreate(spi_master_read_slave_task, "spi_master_read_slave_task", 2048, NULL, 10, NULL);
}



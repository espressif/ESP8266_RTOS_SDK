/* spi_slave example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "esp8266/gpio_struct.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "driver/spi.h"

static const char *TAG = "spi_slave_example";

#define SPI_SLAVE_HANDSHARK_GPIO     4
#define SPI_SLAVE_HANDSHARK_SEL      (1ULL<<SPI_SLAVE_HANDSHARK_GPIO)

static SemaphoreHandle_t spi_wr_sta_done_sem = NULL;
static SemaphoreHandle_t spi_rd_buf_done_sem = NULL;
static SemaphoreHandle_t spi_wr_buf_done_sem = NULL;

void IRAM_ATTR spi_event_callback(int event, void *arg)
{
    BaseType_t xHigherPriorityTaskWoken;
    uint32_t status;
    uint32_t trans_done;

    switch (event) {
        case SPI_INIT_EVENT: {
            status = false;
            spi_slave_set_status(HSPI_HOST, &status);
        }
        break;

        case SPI_TRANS_START_EVENT: {

        }
        break;

        case SPI_TRANS_DONE_EVENT: {
            trans_done = *(uint32_t *)arg;

            if (trans_done & SPI_SLV_RD_BUF_DONE) {
                xSemaphoreGiveFromISR(spi_rd_buf_done_sem, &xHigherPriorityTaskWoken);
            }
            if (trans_done & SPI_SLV_WR_BUF_DONE) {
                status = false;
                spi_slave_set_status(HSPI_HOST, &status);
                xSemaphoreGiveFromISR(spi_wr_buf_done_sem, &xHigherPriorityTaskWoken);
            }
            if (trans_done & SPI_SLV_RD_STA_DONE) {
                status = false;
                spi_slave_set_status(HSPI_HOST, &status);
            }
            if (trans_done & SPI_SLV_WR_STA_DONE) {
                spi_slave_get_status(HSPI_HOST, &status);
                if (status == true) {
                    xSemaphoreGiveFromISR(spi_wr_sta_done_sem, &xHigherPriorityTaskWoken);
                }
            }

            if (xHigherPriorityTaskWoken == pdTRUE) {
                taskYIELD();
            }
        }
        break;

        case SPI_DEINIT_EVENT: {

        }
        break;
    }

}

static void spi_slave_write_master_task(void *arg)
{
    uint16_t cmd;
    uint32_t addr;
    uint32_t write_data[8];
    spi_trans_t trans;

    trans.cmd = &cmd;
    trans.addr = &addr;
    trans.miso = write_data;
    trans.bits.val = 0;
    // In Slave mode, spi cmd must be longer than 3 bits and shorter than 16 bits
    trans.bits.cmd = 8 * 1;
    // In Slave mode, spi addr must be longer than 1 bits and shorter than 32 bits
    trans.bits.addr = 32 * 1;
    trans.bits.mosi = 0;
    trans.bits.miso = 32 * 8;

    write_data[0] = 0xAAAAAAAA;
    write_data[1] = 0xBBBBBBBB;
    write_data[2] = 0xCCCCCCCC;
    write_data[3] = 0xDDDDDDDD;
    write_data[4] = 0xEEEEEEEE;
    write_data[5] = 0xFFFFFFFF;
    write_data[6] = 0xAAAABBBB;
    write_data[7] = 0;

    // Waiting for master idle
    xSemaphoreTake(spi_wr_sta_done_sem, portMAX_DELAY);

    while (1) {
        gpio_set_level(SPI_SLAVE_HANDSHARK_GPIO, 0);

        if (write_data[7] % 50 == 0) {
            vTaskDelay(1000 / portTICK_RATE_MS);
        }
        
        // load new data
        spi_trans(HSPI_HOST, trans);
        write_data[7]++;
        // The rising edge informs the master to get the data
        gpio_set_level(SPI_SLAVE_HANDSHARK_GPIO, 1);
        xSemaphoreTake(spi_rd_buf_done_sem, portMAX_DELAY);
        vTaskDelay(10 / portTICK_RATE_MS);
    }
}

static void spi_slave_read_master_task(void *arg)
{
    int x;
    uint16_t cmd;
    uint32_t addr;
    uint32_t read_data[8];
    spi_trans_t trans;
    uint32_t status = true;

    trans.cmd = &cmd;
    trans.addr = &addr;
    trans.mosi = read_data;
    trans.bits.val = 0;
    // In Slave mode, spi cmd must be longer than 3 bits and shorter than 16 bits
    trans.bits.cmd = 8 * 1;
    // In Slave mode, spi addr must be longer than 1 bits and shorter than 32 bits
    trans.bits.addr = 32 * 1;
    trans.bits.mosi = 32 * 8;

    while (1) {
        spi_slave_set_status(HSPI_HOST, &status);
        xSemaphoreTake(spi_wr_buf_done_sem, portMAX_DELAY);

        spi_trans(HSPI_HOST, trans);

        if (cmd == SPI_MASTER_WRITE_DATA_TO_SLAVE_CMD) {
            ESP_LOGI(TAG, "------Slave read------\n");
            ESP_LOGI(TAG, "addr: 0x%x\n", addr);

            for (x = 0; x < 8; x++) {
                ESP_LOGI(TAG, "read_data[%d]: 0x%x\n", x, read_data[x]);
            }
        }
    }
}

void app_main(void)
{
    spi_wr_sta_done_sem = xSemaphoreCreateBinary();
    spi_rd_buf_done_sem = xSemaphoreCreateBinary();
    spi_wr_buf_done_sem = xSemaphoreCreateBinary();

    ESP_LOGI(TAG, "init gpio");
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = SPI_SLAVE_HANDSHARK_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_level(SPI_SLAVE_HANDSHARK_GPIO, 1);

    ESP_LOGI(TAG, "init spi");

    spi_config_t spi_config;
    // Load default interface parameters
    // CS_EN:1, MISO_EN:1, MOSI_EN:1, BYTE_TX_ORDER:1, BYTE_TX_ORDER:1, BIT_RX_ORDER:0, BIT_TX_ORDER:0, CPHA:0, CPOL:0
    spi_config.interface.val = SPI_DEFAULT_INTERFACE;
    // Load default interrupt enable
    // TRANS_DONE: false, WRITE_STATUS: true, READ_STATUS: true, WRITE_BUFFER: true, READ_BUFFER: ture
    spi_config.intr_enable.val = SPI_SLAVE_DEFAULT_INTR_ENABLE;
    // Set SPI to slave mode
    spi_config.mode = SPI_SLAVE_MODE;
    // Set the SPI clock frequency division factor
    spi_config.clk_div = SPI_10MHz_DIV;
    // Register SPI event callback function
    spi_config.event_cb = spi_event_callback;
    spi_init(HSPI_HOST, &spi_config);

    // create spi_slave_write_master_task
    xTaskCreate(spi_slave_write_master_task, "spi_slave_write_master_task", 2048, NULL, 10, NULL);

    // create spi_slave_read_master_task
    xTaskCreate(spi_slave_read_master_task, "spi_slave_read_master_task", 2048, NULL, 10, NULL);
}



/* spi_slave example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "ringbuf.h"

#include "esp8266/spi_struct.h"
#include "esp8266/gpio_struct.h"
#include "esp_system.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "driver/spi.h"

static const char *TAG = "spi_slave_example";

#define SPI_SLAVE_HANDSHARK_GPIO     4
#define SPI_SLAVE_HANDSHARK_SEL      (1ULL<<SPI_SLAVE_HANDSHARK_GPIO)

RingbufHandle_t spi_slave_tx_ring_buf;
RingbufHandle_t spi_slave_rx_ring_buf;
static SemaphoreHandle_t spi_slave_tx_done_sem = NULL;
static SemaphoreHandle_t spi_slave_wr_sta_done_sem = NULL;
static struct timeval now;

void IRAM_ATTR spi_event_callback(int event, void *arg)
{
    int x;
    BaseType_t xHigherPriorityTaskWoken;
    uint32_t status;
    uint32_t trans_done;
    uint32_t *write_data = NULL;
    uint32_t read_data[8];
    uint32_t size;

    switch (event) {
        case SPI_INIT_EVENT: {

        }
        break;

        case SPI_TRANS_START_EVENT: {

        }
        break;

        case SPI_TRANS_DONE_EVENT: {
            trans_done = *(uint32_t *)arg;

            if (trans_done & SPI_SLV_RD_BUF_DONE) {
                gpio_set_level(SPI_SLAVE_HANDSHARK_GPIO, 0);
                write_data = (uint32_t *) xRingbufferReceiveFromISR(spi_slave_tx_ring_buf, &size);
                if (write_data) {
                    for (x = 0; x < 8; x++) {
                        SPI1.data_buf[x + 8] = write_data[x];
                    }
                    vRingbufferReturnItemFromISR(spi_slave_tx_ring_buf, write_data, &xHigherPriorityTaskWoken);
                    gpio_set_level(SPI_SLAVE_HANDSHARK_GPIO, 1);
                } else {
                    xSemaphoreGiveFromISR(spi_slave_tx_done_sem, &xHigherPriorityTaskWoken);
                }
            }
            if (trans_done & SPI_SLV_WR_BUF_DONE) {
                for (x = 0; x < 8; x++) {
                    read_data[x] = SPI1.data_buf[x];
                }
                xRingbufferSendFromISR(spi_slave_rx_ring_buf, (void *) read_data, sizeof(uint32_t) * 8, &xHigherPriorityTaskWoken);
            }
            if (trans_done & SPI_SLV_RD_STA_DONE) {

            }
            if (trans_done & SPI_SLV_WR_STA_DONE) {
                spi_slave_get_status(HSPI_HOST, &status);
                if (status == true) {
                    xSemaphoreGiveFromISR(spi_slave_wr_sta_done_sem, &xHigherPriorityTaskWoken);
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
    int x;
    uint16_t cmd;
    uint32_t addr;
    uint32_t write_data[8];
    spi_trans_t trans;
    uint32_t size;
    uint64_t time_start, time_end;

    trans.cmd = &cmd;
    trans.addr = &addr;
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
    write_data[7] = 1;

    // Waiting for master idle
    xSemaphoreTake(spi_slave_wr_sta_done_sem, portMAX_DELAY);

    while (1) {
        for (x = 0;x < 100;x++) {
            xRingbufferSend(spi_slave_tx_ring_buf, (void *) write_data, sizeof(uint32_t) * 8, portMAX_DELAY);
            write_data[7]++;
        }

        trans.miso = (uint32_t *) xRingbufferReceive(spi_slave_tx_ring_buf, &size, portMAX_DELAY);
        gettimeofday(&now, NULL); 
        time_start = now.tv_usec;
        gpio_set_level(SPI_SLAVE_HANDSHARK_GPIO, 0);
        spi_trans(HSPI_HOST, trans);
        gpio_set_level(SPI_SLAVE_HANDSHARK_GPIO, 1);
        vRingbufferReturnItem(spi_slave_tx_ring_buf, trans.miso);
        xSemaphoreTake(spi_slave_tx_done_sem, portMAX_DELAY);
        gettimeofday(&now, NULL); 
        time_end = now.tv_usec;

        ESP_LOGI(TAG, "Slave wrote 3200 bytes in %d us", (int)(time_end - time_start));
        vTaskDelay(100 / portTICK_RATE_MS);
    }
}

static void spi_slave_read_master_task(void *arg)
{
    uint32_t *read_data = NULL;
    uint32_t size;

    while (1) {
        read_data = (uint32_t *) xRingbufferReceive(spi_slave_rx_ring_buf, &size, portMAX_DELAY);

        if (read_data) {
            vRingbufferReturnItem(spi_slave_rx_ring_buf, read_data);
            if (read_data[7] % 100 == 0) {
                vTaskDelay(100 / portTICK_RATE_MS);
            }
        }
    }
}

void app_main(void)
{
    spi_trans_t trans = {0};
    spi_slave_tx_ring_buf = xRingbufferCreate(4096, RINGBUF_TYPE_NOSPLIT);
    spi_slave_rx_ring_buf = xRingbufferCreate(4096, RINGBUF_TYPE_NOSPLIT);
    spi_slave_wr_sta_done_sem = xSemaphoreCreateBinary();
    spi_slave_tx_done_sem = xSemaphoreCreateBinary();

    ESP_LOGI(TAG, "init gpio");
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = SPI_SLAVE_HANDSHARK_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_level(SPI_SLAVE_HANDSHARK_GPIO, 0);

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
    // Register SPI event callback function
    spi_config.event_cb = spi_event_callback;
    spi_init(HSPI_HOST, &spi_config);

    trans.bits.val = 0;
    // In Slave mode, spi cmd must be longer than 3 bits and shorter than 16 bits
    trans.bits.cmd = 8 * 1;
    // In Slave mode, spi addr must be longer than 1 bits and shorter than 32 bits
    trans.bits.addr = 32 * 1;
    trans.bits.mosi = 32 * 8;
    spi_trans(HSPI_HOST, trans); // init spi slave buf

    // create spi_slave_write_master_task
    xTaskCreate(spi_slave_write_master_task, "spi_slave_write_master_task", 2048, NULL, 10, NULL);

    // create spi_slave_read_master_task
    xTaskCreate(spi_slave_read_master_task, "spi_slave_read_master_task", 2048, NULL, 10, NULL);
}



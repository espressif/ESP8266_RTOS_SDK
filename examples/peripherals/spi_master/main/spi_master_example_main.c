/* spi_master example

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

static const char *TAG = "spi_master_example";

#define SPI_MASTER_HANDSHARK_GPIO     4
#define SPI_MASTER_HANDSHARK_SEL      (1ULL<<SPI_MASTER_HANDSHARK_GPIO)

RingbufHandle_t spi_master_rx_ring_buf;
struct timeval now;

static void gpio_isr_handler(void *arg)
{
    int x;
    BaseType_t xHigherPriorityTaskWoken;
    uint32_t read_data[8];

    if ((int)arg == SPI_MASTER_HANDSHARK_GPIO) {
        while (SPI1.cmd.usr);
        SPI1.user.usr_command = 1;
        SPI1.user.usr_addr = 1;
        SPI1.user.usr_mosi = 0;
        SPI1.user.usr_miso = 1;
        SPI1.user2.usr_command_bitlen = 8 - 1;
        SPI1.user1.usr_addr_bitlen = 32 - 1;
        SPI1.user1.usr_miso_bitlen = 32 * 8 - 1;
        SPI1.user2.usr_command_value = SPI_MASTER_READ_DATA_FROM_SLAVE_CMD;
        SPI1.addr = 0;
        SPI1.cmd.usr = 1;
        while (SPI1.cmd.usr);
        for (x = 0; x < 8; x++) {
            read_data[x] = SPI1.data_buf[x];
        }
        xRingbufferSendFromISR(spi_master_rx_ring_buf, (void *) read_data, sizeof(uint32_t) * 8, &xHigherPriorityTaskWoken);

        if (xHigherPriorityTaskWoken == pdTRUE) {
            taskYIELD();
        }
    }
}

static void spi_master_write_slave_task(void *arg)
{
    int x;
    uint32_t write_data[8];
    spi_trans_t trans;
    uint16_t cmd;
    uint32_t addr;
    uint64_t time_start, time_end;

    trans.bits.val = 0;
    trans.bits.cmd = 8 * 1;
    trans.bits.addr = 32 * 1;
    trans.bits.mosi = 32 * 8;
    // Write data to the ESP8266 Slave use "SPI_MASTER_WRITE_DATA_TO_SLAVE_CMD" cmd
    trans.cmd = &cmd;
    trans.addr = &addr;
    trans.mosi = write_data;
    cmd = SPI_MASTER_WRITE_DATA_TO_SLAVE_CMD;
    addr = 0;
    write_data[0] = 1;
    write_data[1] = 0x11111111;
    write_data[2] = 0x22222222;
    write_data[3] = 0x33333333;
    write_data[4] = 0x44444444;
    write_data[5] = 0x55555555;
    write_data[6] = 0x66666666;
    write_data[7] = 0x77777777;

    while (1) {
        gettimeofday(&now, NULL); 
        time_start = now.tv_usec;
        for (x = 0;x < 100;x++) {
            spi_trans(HSPI_HOST, trans);
            write_data[0]++;
        }
        gettimeofday(&now, NULL); 
        time_end = now.tv_usec;

        ESP_LOGI(TAG, "Master wrote 3200 bytes in %d us", (int)(time_end - time_start));
        vTaskDelay(100 / portTICK_RATE_MS);
    }
}

static void spi_master_read_slave_task(void *arg)
{
    uint32_t *read_data = NULL;
    uint32_t size;

    while (1) {
        read_data = (uint32_t *) xRingbufferReceive(spi_master_rx_ring_buf, &size, portMAX_DELAY);

        if (read_data) {
            vRingbufferReturnItem(spi_master_rx_ring_buf, read_data);
            if (read_data[7] % 100 == 0) {
                vTaskDelay(100 / portTICK_RATE_MS);
            }
        }
    }
}

void app_main(void)
{
    spi_trans_t trans = {0};
    uint16_t cmd;
    uint32_t status = true;
    spi_master_rx_ring_buf = xRingbufferCreate(4096, RINGBUF_TYPE_NOSPLIT);

    ESP_LOGI(TAG, "init gpio");
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = SPI_MASTER_HANDSHARK_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(SPI_MASTER_HANDSHARK_GPIO, gpio_isr_handler, (void *) SPI_MASTER_HANDSHARK_GPIO);

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
    spi_config.event_cb = NULL;
    spi_init(HSPI_HOST, &spi_config);

    // Write status to the ESP8266 Slave use "SPI_MASTER_WRITE_STATUS_TO_SLAVE_CMD" cmd
    cmd = SPI_MASTER_WRITE_STATUS_TO_SLAVE_CMD;
    trans.cmd = &cmd;
    trans.mosi = &status;
    trans.bits.val = 0;
    trans.bits.cmd = 8 * 1;
    trans.bits.mosi = 32 * 1;
    spi_trans(HSPI_HOST, trans);

    // create spi_master_write_slave_task
    xTaskCreate(spi_master_write_slave_task, "spi_master_write_slave_task", 2048, NULL, 10, NULL);

    // create spi_master_read_slave_task
    xTaskCreate(spi_master_read_slave_task, "spi_master_read_slave_task", 2048, NULL, 10, NULL);
}



/* spi_slave example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "freertos/stream_buffer.h"
#include "ringbuf.h"

#include "esp8266/spi_struct.h"
#include "esp8266/gpio_struct.h"
#include "esp_system.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "driver/spi.h"

static const char* TAG = "spi_slave_example";

#define SPI_SLAVE_HANDSHARK_GPIO     4
#define SPI_SLAVE_HANDSHARK_SEL      (1ULL<<SPI_SLAVE_HANDSHARK_GPIO)

#define SPI_WRITE_BUFFER_MAX_SIZE               2048
#define SPI_READ_BUFFER_MAX_SIZE                1024

#define ESP_SPI_SLAVE_RECV    // Define the macro is master send mode, delete will be slave send mode

static StreamBufferHandle_t spi_slave_tx_ring_buf;
static StreamBufferHandle_t spi_slave_rx_ring_buf;
static uint32_t total_recv_len = 0;
static bool wait_recv_data = false;
static bool sending_flag = false;
static volatile uint32_t total_tx_count = 0;
static volatile uint32_t total_send_len = 0;

static void IRAM_ATTR spi_event_callback(int event, void* arg)
{
    int x;
    BaseType_t xHigherPriorityTaskWoken;
    uint32_t status;
    uint32_t trans_done;
    uint32_t data[16];
    spi_trans_t trans = {0};
    uint16_t cmd = 0;
    bool trigger_flag = false;

    switch (event) {
        case SPI_TRANS_DONE_EVENT: {
            gpio_set_level(SPI_SLAVE_HANDSHARK_GPIO, 0);
            trans_done = *(uint32_t*)arg;

            if (trans_done & SPI_SLV_RD_BUF_DONE) {   // slave -> master data
                if (total_send_len == 0) {
                    sending_flag = false;
                    total_send_len = xStreamBufferBytesAvailable(spi_slave_tx_ring_buf);

                    if (total_send_len > 0) {    // Have some data send to MCU
                        spi_slave_set_status(HSPI_HOST, (uint32_t*)&total_send_len);
                        sending_flag = true;
                        trigger_flag = true;
                    }
                } else {     // Have some data send to MCU
                    memset(&trans, 0x0, sizeof(trans));
                    trans.cmd = &cmd;
                    trans.addr = NULL;
                    trans.bits.val = 0;
                    trans.bits.cmd = 8 * 1;
                    trans.bits.addr = 8 * 1;
                    trans.bits.mosi = 0;
                    trans.miso = data;
                    trans.bits.miso = xStreamBufferReceiveFromISR(spi_slave_tx_ring_buf, data, 64, &xHigherPriorityTaskWoken);    // send max 32bytes

                    if (trans.bits.miso != 0) {
                        total_send_len -= trans.bits.miso;
                        trans.bits.miso <<= 3;
                        spi_trans(HSPI_HOST, &trans); 
                        trigger_flag = true;;
                    }
                }
            }

            if (trans_done & SPI_SLV_WR_BUF_DONE) {      // master -> slave   data
                uint32_t len = total_recv_len;

                if (len > 64) {   // only send max 32bytes one time
                    len = 64;
                }

                if (len > 0) {
                    for (x = 0; x < 16; x++) {
                        data[x] = SPI1.data_buf[x];
                    }

                    xStreamBufferSendFromISR(spi_slave_rx_ring_buf, (void*) data, len, &xHigherPriorityTaskWoken);
                    total_recv_len -= len;
                }

                if (xStreamBufferSpacesAvailable(spi_slave_rx_ring_buf) >= 64) {    // Stream buffer not full, can be read agian
                    trigger_flag = true;
                } else {
                    wait_recv_data = true;
                }
            }

            if (trans_done & SPI_SLV_WR_STA_DONE) {        // master -> slave status len  
                spi_slave_get_status(HSPI_HOST, &status);
                total_recv_len = status;
                uint32_t tx_size = xStreamBufferBytesAvailable(spi_slave_tx_ring_buf);

                if (total_recv_len > 0) {
                    trigger_flag = true;
                } else if (tx_size > 0) {                // SPI send done and ESP8266 send buffer have data
                    if (sending_flag == false) {
                        spi_slave_set_status(HSPI_HOST, &tx_size);
                    }

                    trigger_flag = true;
                }
            }

            if (trans_done & SPI_SLV_RD_STA_DONE) {      // Slave -> Master status len
                memset(&trans, 0x0, sizeof(trans));
                trans.cmd = &cmd;
                trans.addr = NULL;
                trans.bits.val = 0;
                trans.bits.cmd = 8 * 1;
                trans.bits.addr = 8 * 1;
                trans.bits.mosi = 0;
                trans.miso = data;
                trans.bits.miso = xStreamBufferReceiveFromISR(spi_slave_tx_ring_buf, data, 64, &xHigherPriorityTaskWoken);

                if (trans.bits.miso != 0) {
                    total_send_len -= trans.bits.miso;
                    trans.bits.miso <<= 3;
                    spi_trans(HSPI_HOST, &trans);
                    trigger_flag = true;
                }
            }

            if (trigger_flag) {
                gpio_set_level(SPI_SLAVE_HANDSHARK_GPIO, 1);
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

#ifdef ESP_SPI_SLAVE_RECV
uint32_t read_count = 0;

static void spi_slave_count_task(void* arg)
{
    uint32_t tmp_count = 0;

    while (1) {
        printf("recv_count:  %d , speed: %dB/s\n", read_count, ((read_count - tmp_count) / 2));
        tmp_count = read_count;
        vTaskDelay(2000 / portTICK_RATE_MS);
    }
}

static void IRAM_ATTR spi_slave_read_master_task(void* arg)
{
    uint8_t read_data[SPI_READ_BUFFER_MAX_SIZE + 1];
    size_t xReceivedBytes;

    while (1) {
        xReceivedBytes = xStreamBufferReceive(spi_slave_rx_ring_buf, read_data, SPI_READ_BUFFER_MAX_SIZE, 2000 / portTICK_RATE_MS);

        if (xReceivedBytes != 0) {
            for (int i = 0; i < xReceivedBytes; i++) {
                if (read_data[i] != 0x33) {
                    printf("receive error data: %x\n", read_data[i]);
                }
            }

            read_count += xReceivedBytes;
            memset(read_data, 0x0, xReceivedBytes);

            // steam buffer full
            if (wait_recv_data) {
                if (xStreamBufferBytesAvailable(spi_slave_rx_ring_buf) > 64) {
                    gpio_set_level(SPI_SLAVE_HANDSHARK_GPIO, 1);
                    wait_recv_data = false;
                }
            }
        }
    }
}
#else
#include "time.h"
static void IRAM_ATTR spi_slave_write_master_task(void* arg)
{
    static uint8_t write_data[SPI_WRITE_BUFFER_MAX_SIZE];
    memset(write_data, 0x44, SPI_WRITE_BUFFER_MAX_SIZE);
    vTaskDelay(5000 / portTICK_RATE_MS);
    printf("Test send\r\n");
    time_t start = time(NULL);

    while (1) {
        total_tx_count += xStreamBufferSend(spi_slave_tx_ring_buf, write_data, SPI_WRITE_BUFFER_MAX_SIZE, portMAX_DELAY);
        portENTER_CRITICAL();

        if (sending_flag == false) {
            total_send_len = xStreamBufferBytesAvailable(spi_slave_tx_ring_buf);
            spi_slave_set_status(HSPI_HOST, (uint32_t*)&total_send_len);
            sending_flag = true;
            gpio_set_level(SPI_SLAVE_HANDSHARK_GPIO, 1);
        }

        portEXIT_CRITICAL();

        if (total_tx_count >= 20 * 1024 * 1024) {
            printf("tx done; %d bytes, time : %ld\r\n", total_tx_count, time(NULL) - start);

            for (;;) {
                vTaskDelay(100);
            }
        }

    }
}
#endif

void app_main(void)
{
    spi_slave_tx_ring_buf = xStreamBufferCreate(4096, 1);
    spi_slave_rx_ring_buf = xStreamBufferCreate(4096, 1024);

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

#ifdef ESP_SPI_SLAVE_RECV
    // create spi_slave_read_master_task
    xTaskCreate(spi_slave_read_master_task, "spi_slave_read_master_task", 2048, NULL, 6, NULL);
    xTaskCreate(spi_slave_count_task, "spi_slave_count_task", 2048, NULL, 3, NULL);
#else
    // create spi_slave_write_master_task
    xTaskCreate(spi_slave_write_master_task, "spi_slave_write_master_task", 2048, NULL, 2, NULL);
#endif
}

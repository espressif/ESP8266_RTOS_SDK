/* spi_master example

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

#define SPI_WRITE_BUFFER_MAX_SIZE               2048

#define ESP_SPI_MASTER_TEST_SEND              // Define the macro is master send mode, delete will be slave send mode

static SemaphoreHandle_t semphor = NULL;

typedef enum {
    SPI_SEND = 0,
    SPI_RECV
} spi_master_mode_t;

/* Master receive data or send data done both need wait isr */
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if ((int)arg == SPI_MASTER_HANDSHARK_GPIO) {

        xSemaphoreGiveFromISR(semphor, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken == pdTRUE) {
            taskYIELD();
        }
    }
}

/* SPI transmit data, format: 8bit command (read value: 3, write value: 4) + 8bit address(value: 0x0) + 64byte data */
static void IRAM_ATTR spi_master_transmit(spi_master_mode_t trans_mode, uint32_t* data, uint32_t len)
{
    spi_trans_t trans;
    uint16_t cmd;
    uint32_t addr = 0x0;

    if (len > 16) {
        ESP_LOGE(TAG, "ESP8266 only support transmit 64bytes(16 * sizeof(uint32_t)) one time");
        return;
    }

    memset(&trans, 0x0, sizeof(trans));
    trans.bits.val = 0;            // clear all bit

    if (trans_mode == SPI_SEND) {
        cmd = SPI_MASTER_WRITE_DATA_TO_SLAVE_CMD;  
        trans.bits.mosi = len * 32;             // One time transmit only support 64bytes
        trans.mosi = data;
    } else {
        cmd = SPI_MASTER_READ_DATA_FROM_SLAVE_CMD;
        trans.bits.miso = len * 32;
        trans.miso = data;
    }

    trans.bits.cmd = 8 * 1;
    trans.bits.addr = 8 * 1;     // transmit data will use 8bit address
    trans.cmd = &cmd;
    trans.addr = &addr;
    
    spi_trans(HSPI_HOST, &trans); 
}

#ifdef ESP_SPI_MASTER_TEST_SEND

/* SPI master send length, format: 8bit command(value:1) + 32bit status length */
static void IRAM_ATTR spi_master_send_length(uint32_t len)
{
    spi_trans_t trans;
    uint16_t cmd = SPI_MASTER_WRITE_STATUS_TO_SLAVE_CMD;
    memset(&trans, 0x0, sizeof(trans));
    trans.bits.val = 0;
    trans.bits.cmd = 8 * 1;  
    trans.bits.addr = 0;          // transmit status do not use address bit
    trans.bits.mosi = 8 * 4;      // status length is 32bit
    trans.cmd = &cmd;
    trans.addr = NULL;
    trans.mosi = &len;
    spi_trans(HSPI_HOST, &trans);    
}

static void IRAM_ATTR spi_master_write_slave_task(void *arg)
{
    /* In order to improve the transmission efficiency, it is recommended that the external 
        incoming data is (uint32_t *) type data, do not use other type data. */
    static uint32_t write_data[SPI_WRITE_BUFFER_MAX_SIZE/4];
    uint32_t total_len = 0;
    uint32_t send_len = 0;
    
    for (uint32_t loop = 0; loop < sizeof(write_data)/sizeof(write_data[0]);loop++) {
        write_data[loop] = 0x34343434;
    }

    xSemaphoreTake(semphor, 0);
    vTaskDelay(5000 / portTICK_RATE_MS);
    ESP_LOGI(TAG, "Start test send data");
    while (1) {

        send_len = sizeof(write_data);
        spi_master_send_length(send_len);
        
        // wait ESP8266 received the length
        xSemaphoreTake(semphor, portMAX_DELAY);

        for (uint32_t loop = 0; loop < (send_len + 63)/64; loop++) {
            // transmit data, ESP8266 only transmit 64bytes one time
            spi_master_transmit(SPI_SEND, write_data + (loop * 16), 64 /sizeof(uint32_t));
            xSemaphoreTake(semphor, portMAX_DELAY);
        }
        // send 0 to clear send length, and tell Slave send done
        spi_master_send_length(0);

        total_len += send_len;

        if (total_len >= (10*1024*1024)) {
            ESP_LOGI(TAG, "total_len=%d\r\n", total_len);
            for (;;) {
                vTaskDelay(1000);
            }
        }
    }
}
#else

/* SPI master revecive length, format: 8bit command(value:4) + 32bit status length */
static uint32_t IRAM_ATTR spi_master_get_length(void)
{
    spi_trans_t trans;
    uint32_t len = 0;
    uint16_t cmd = SPI_MASTER_READ_STATUS_FROM_SLAVE_CMD;
    memset(&trans, 0x0, sizeof(trans));
    trans.bits.val = 0;
    trans.cmd = &cmd;
    trans.miso = &len;
    trans.addr = NULL;
    trans.bits.cmd = 8 * 1;   
    trans.bits.miso = 8 * 4;
    spi_trans(HSPI_HOST, &trans);
    return len;
}

uint32_t read_count = 0;
static void spi_master_count_task(void* arg)
{
    uint32_t tmp_count = 0;
    while(1){
        printf("recv_count:  %d , speed: %dB/s\n", read_count, ((read_count - tmp_count)/2));
        tmp_count = read_count;
        vTaskDelay(2000 / portTICK_RATE_MS);
    }
}

static void IRAM_ATTR spi_master_read_slave_task(void *arg)
{
    uint32_t read_data[16];
    uint32_t read_len = 0;
    uint32_t read_time = 0;

    while (1) {
        xSemaphoreTake(semphor, portMAX_DELAY);

        read_len = spi_master_get_length();
        ESP_LOGD(TAG, "read len: %d\n", read_len);
        xSemaphoreTake(semphor, portMAX_DELAY);
        read_count += read_len;

        if (read_len > 0) {
            read_time = (read_len + 63) / 64;    // read_len is the total bytes, every time send 32bytes, so send time will be divided by 32
            while(read_time > 0) {
                spi_master_transmit(SPI_RECV, read_data, 64 /sizeof(uint32_t));
                for (int x = 0; x < 16; x++) {
                    if (read_data[x] != 0xa3a3a3a3) {
                        ESP_LOGE(TAG, "error 0x%02x,%d\r\n", read_data[x], x);
                    }
                }
                read_time--;
                if(read_time != 0) {
                    xSemaphoreTake(semphor, portMAX_DELAY);
                }
            }

        }
    }
}
#endif

void app_main(void)
{
    semphor = xSemaphoreCreateBinary();

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
    spi_config.event_cb = NULL;
    spi_init(HSPI_HOST, &spi_config);

#ifdef ESP_SPI_MASTER_TEST_SEND
    xTaskCreate(spi_master_write_slave_task, "spi_master_write_slave_task", 2048, NULL, 3, NULL);
#else
    // create spi_master_read_slave_task
    xTaskCreate(spi_master_read_slave_task, "spi_master_read_slave_task", 2048, NULL, 2, NULL);
    xTaskCreate(spi_master_count_task, "spi_master_count_task", 2048, NULL, 4, NULL);
#endif
}
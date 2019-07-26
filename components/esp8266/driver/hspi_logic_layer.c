
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
#include "driver/hspi_logic_layer.h"

static const char *TAG = "hspi_logic";
#define SPI_CHECK(a, str, ret_val) \
    do { \
        if (!(a)) { \
            ESP_LOGE(TAG,"%s(%d): %s", __FUNCTION__, __LINE__, str); \
            return (ret_val); \
        } \
    } while(0)

typedef struct {
    gpio_num_t trigger_pin;
    uint8_t trigger_level;
    bool is_sending;
    bool is_blocking_recv;
    uint32_t sending_len;
    uint32_t recving_len;
    StreamBufferHandle_t* tx_buffer;
    StreamBufferHandle_t* rx_buffer;

    SemaphoreHandle_t semphor;
    spi_event_callback_t event_cb;
} spi_logic_device_t;

static spi_logic_device_t * spi_logic_device;

static void IRAM_ATTR hspi_slave_event_callback(int event, void *arg)
{
    int x;
    BaseType_t xHigherPriorityTaskWoken;
    uint32_t status;
    uint32_t trans_done;
    uint32_t data[16];
    spi_trans_t trans;
    uint16_t cmd = 0;
    bool trigger_flag = false;

    switch (event) {
        case SPI_TRANS_DONE_EVENT: {
            gpio_set_level(spi_logic_device->trigger_pin, !spi_logic_device->trigger_level);
            trans_done = *(uint32_t *)arg;
            if (trans_done & SPI_SLV_RD_BUF_DONE) {
                if (spi_logic_device->sending_len == 0) {
                    spi_logic_device->is_sending = false;
                    spi_logic_device->sending_len = xStreamBufferBytesAvailable(spi_logic_device->tx_buffer);
                    if (spi_logic_device->sending_len > 0) {
                        spi_slave_set_status(HSPI_HOST, (uint32_t*)&spi_logic_device->sending_len);
                        spi_logic_device->is_sending = true;
                        trigger_flag = true;
                    }
                } else {
                    memset(&trans, 0x0, sizeof(trans));
                    trans.cmd = &cmd;
                    trans.addr = NULL;
                    trans.bits.val = 0;
                    // In Slave mode, spi cmd must be longer than 3 bits and shorter than 16 bits
                    trans.bits.cmd = 8 * 1;
                    // In Slave mode, spi addr must be longer than 1 bits and shorter than 32 bits
                    trans.bits.addr = 8 * 1;
                    trans.bits.mosi = 0;
                    trans.miso = data;
                    trans.bits.miso = xStreamBufferReceiveFromISR(spi_logic_device->tx_buffer, data, 64, &xHigherPriorityTaskWoken);
                    if (trans.bits.miso != 0) {
                        spi_logic_device->sending_len -= trans.bits.miso;
                        trans.bits.miso <<= 3;
                        spi_trans(HSPI_HOST, &trans);
                        trigger_flag = true;;
                    }
                }
            }

            if (trans_done & SPI_SLV_WR_BUF_DONE) {
                uint32_t len = spi_logic_device->recving_len;
                if (len > 64) {
                    len = 64;
                }

                if (len > 0) {
                    for (x = 0; x < 16; x++) {
                        data[x] = SPI1.data_buf[x];
                    }
                    xStreamBufferSendFromISR(spi_logic_device->rx_buffer, (void *) data, len, &xHigherPriorityTaskWoken);
                    spi_logic_device->recving_len -= len;
                } else {
                    ets_printf("remained %d\r\n", len);
                }

                if (xStreamBufferSpacesAvailable(spi_logic_device->rx_buffer) >= 64) {
                    trigger_flag = true;
                } else {
                    spi_logic_device->is_blocking_recv = true;
                }
            }

            if (trans_done & SPI_SLV_WR_STA_DONE) {
                spi_slave_get_status(HSPI_HOST, &status);
                spi_logic_device->recving_len = status;
                uint32_t tx_size = xStreamBufferBytesAvailable(spi_logic_device->tx_buffer);

                if (spi_logic_device->recving_len > 0) {
                    trigger_flag = true;
                } else if (tx_size > 0) {
                    if (spi_logic_device->is_sending == false) {
                        spi_slave_set_status(HSPI_HOST, &tx_size);
                    }
                    trigger_flag = true;
                }
            }

            if (trans_done & SPI_SLV_RD_STA_DONE) {
                memset(&trans, 0x0, sizeof(trans));
                trans.cmd = &cmd;
                trans.addr = NULL;
                trans.bits.val = 0;
                // In Slave mode, spi cmd must be longer than 3 bits and shorter than 16 bits
                trans.bits.cmd = 8 * 1;
                // In Slave mode, spi addr must be longer than 1 bits and shorter than 32 bits
                trans.bits.addr = 8 * 1;
                trans.bits.mosi = 0;
                trans.miso = data;
                trans.bits.miso = xStreamBufferReceiveFromISR(spi_logic_device->tx_buffer, data, 64, &xHigherPriorityTaskWoken);
                if (trans.bits.miso != 0) {
                    spi_logic_device->sending_len -= trans.bits.miso;
                    trans.bits.miso <<= 3;
                    spi_trans(HSPI_HOST, &trans);
                    trigger_flag = true;
                }
            }

            if (trigger_flag) {
                gpio_set_level(spi_logic_device->trigger_pin, spi_logic_device->trigger_level);
            }

            if (spi_logic_device->event_cb) {
                spi_logic_device->event_cb(event, arg);
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

uint32_t hspi_slave_logic_read_data(uint8_t*data, uint32_t len, TickType_t xTicksToWait)
{
    uint32_t ret = 0;

    ret = xStreamBufferReceive(spi_logic_device->rx_buffer, data, len, xTicksToWait);
    if (spi_logic_device->is_blocking_recv) {
        if (xStreamBufferBytesAvailable(spi_logic_device->rx_buffer) > 64) {
            gpio_set_level(spi_logic_device->trigger_pin, spi_logic_device->trigger_level);
            spi_logic_device->is_blocking_recv = false;
        }
    }

    return ret;
}

uint32_t hspi_slave_logic_write_data(uint8_t*data, uint32_t len, TickType_t xTicksToWait)
{
    uint32_t ret = 0;
    uint32_t avail_spaces = 0;

    if (!spi_logic_device->is_sending) {
        portENTER_CRITICAL();
        avail_spaces = xStreamBufferSpacesAvailable(spi_logic_device->tx_buffer);
        if (avail_spaces > len) {
            avail_spaces = len;
        }
        ret = xStreamBufferSend(spi_logic_device->tx_buffer, data, avail_spaces, xTicksToWait); // 
        spi_logic_device->sending_len = xStreamBufferBytesAvailable(spi_logic_device->tx_buffer);
        spi_slave_set_status(HSPI_HOST, (uint32_t*)&spi_logic_device->sending_len);
        spi_logic_device->is_sending = true;
        gpio_set_level(spi_logic_device->trigger_pin, spi_logic_device->trigger_level);
        portEXIT_CRITICAL();
    }

    if (ret < len) {
        ret += xStreamBufferSend(spi_logic_device->tx_buffer, data + ret, len - ret, xTicksToWait);
    }

    return ret;
}

esp_err_t hspi_slave_logic_device_create(gpio_num_t trigger_pin, uint32_t trigger_level,uint32_t tx_buffer_size, uint32_t rx_buffer_size)
{
    SPI_CHECK(GPIO_IS_VALID_GPIO(trigger_pin), "gpio num error", ESP_ERR_INVALID_ARG);
    SPI_CHECK(tx_buffer_size != 0, "tx buffer error", ESP_ERR_INVALID_ARG);
    SPI_CHECK(rx_buffer_size != 0, "rx buffer error", ESP_ERR_INVALID_ARG);

    gpio_config_t io_conf;
    
    if (spi_logic_device) {
        hspi_slave_logic_device_delete();
    }
    spi_logic_device = (spi_logic_device_t*)malloc(sizeof(spi_logic_device_t));
    assert(spi_logic_device);

    memset(spi_logic_device, 0x0, sizeof(spi_logic_device_t));
    spi_logic_device->tx_buffer = xStreamBufferCreate(tx_buffer_size,1);
    if (!spi_logic_device->tx_buffer) {
        free(spi_logic_device);
        spi_logic_device = NULL;
        return ESP_ERR_NO_MEM;
    }

    spi_logic_device->rx_buffer = xStreamBufferCreate(rx_buffer_size,1);
    if (!spi_logic_device->rx_buffer) {
        vStreamBufferDelete(spi_logic_device->tx_buffer);
        spi_logic_device->tx_buffer = NULL;

        free(spi_logic_device);
        spi_logic_device = NULL;
        return ESP_ERR_NO_MEM;
    }

    spi_logic_device->trigger_pin = trigger_pin;
    spi_logic_device->trigger_level = (trigger_level==1)?1:0;

    memset(&io_conf, 0x0, sizeof(io_conf));
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << trigger_pin);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_level(trigger_pin, !spi_logic_device->trigger_level);

    spi_get_event_callback(HSPI_HOST, &spi_logic_device->event_cb);

    spi_event_callback_t event_cb = hspi_slave_event_callback;
    spi_set_event_callback(HSPI_HOST, &event_cb);

    return ESP_OK;
}

esp_err_t hspi_slave_logic_device_delete(void)
{
    if (spi_logic_device == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    vStreamBufferDelete(spi_logic_device->tx_buffer);
    spi_logic_device->tx_buffer = NULL;

    vStreamBufferDelete(spi_logic_device->rx_buffer);
    spi_logic_device->rx_buffer = NULL;

    free(spi_logic_device);
    spi_logic_device = NULL;

    return ESP_OK;
}

// Copyright 2018-2025 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ir_rx.h"

static const char *TAG = "ir rx";

#define IR_RX_CHECK(a, str, ret_val) \
    if (!(a)) { \
        ESP_LOGE(TAG,"%s(%d): %s", __FUNCTION__, __LINE__, str); \
        return (ret_val); \
    }

typedef struct {
    uint32_t io_num;
    uint32_t buf_len;
    SemaphoreHandle_t recv_mux;
    RingbufHandle_t ring_buf;        /*!< rx ring buffer handler*/
} ir_rx_obj_t;

ir_rx_obj_t *ir_rx_obj = NULL;

typedef enum {
    IR_RX_IDLE,
    IR_RX_HEADER,
    IR_RX_DATA,
    IR_RX_REP,
} ir_rx_state_t;

/**
 * @brief ir rx state machine via gpio intr
 */
static void IRAM_ATTR ir_rx_intr_handler(void *arg)
{
    static int ir_state  =  IR_RX_IDLE;
    static int ir_repeat =  0;
    static ir_rx_nec_data_t ir_data = {0};
    static int cnt  =  0;
    static uint8_t rep_flg = 0;
    static uint32_t time_last = 0;

    BaseType_t xHigherPriorityTaskWoken;
    uint32_t time_escape, time_current;
    struct timeval now;

    gettimeofday(&now, NULL);
    time_current = now.tv_sec * 1000 * 1000 + now.tv_usec;
    time_escape = time_current - time_last;
    time_last = time_current;

    switch (ir_state) {
        case IR_RX_IDLE: {
            if (time_escape < IR_RX_NEC_HEADER_US + IR_RX_ERROR_US && time_escape > IR_RX_NEC_HEADER_US - IR_RX_ERROR_US) {
                ir_state = IR_RX_DATA;
            }
        }
        break;

        case IR_RX_DATA: {
            if (time_escape < IR_RX_NEC_DATA1_US + IR_RX_ERROR_US && time_escape > IR_RX_NEC_DATA1_US - IR_RX_ERROR_US) {
                ir_data.val = (ir_data.val >> 1) | (0x1 << (IR_RX_NEC_BIT_NUM * 4 - 1));
                cnt++;
            } else if (time_escape < IR_RX_NEC_DATA0_US + IR_RX_ERROR_US && time_escape > IR_RX_NEC_DATA0_US - IR_RX_ERROR_US) {
                ir_data.val = (ir_data.val >> 1) | (0x0 << (IR_RX_NEC_BIT_NUM * 4 - 1));
                cnt++;
            } else {
                goto reset_status;
            }

            if (cnt == IR_RX_NEC_BIT_NUM * 4) {
                // push rcv data to ringbuf
                xRingbufferSendFromISR(ir_rx_obj->ring_buf, (void *) &ir_data, sizeof(ir_rx_nec_data_t) * 1, &xHigherPriorityTaskWoken);
                ir_state = IR_RX_REP;
                rep_flg = 0;
            }
        }
        break;

        case IR_RX_REP: {
            if (rep_flg == 0) {
                if (time_escape > IR_RX_NEC_TM1_REP_US  &&  time_escape < IR_RX_NEC_TM1_REP_US * 8) {
                    rep_flg = 1;
                } else {
                    goto reset_status;
                }
            } else if (rep_flg == 1) {
                if (time_escape < IR_RX_NEC_TM1_REP_US + IR_RX_ERROR_US && IR_RX_NEC_TM2_REP_US - IR_RX_ERROR_US) {
                    // push rcv data to ringbuf
                    xRingbufferSendFromISR(ir_rx_obj->ring_buf, (void *) &ir_data, sizeof(ir_rx_nec_data_t) * 1, &xHigherPriorityTaskWoken);
                    ir_repeat++;
                    rep_flg = 0;
                } else {
                    goto reset_status;
                }
            }
        }
        break;
    }

    if (xHigherPriorityTaskWoken == pdTRUE) {
        taskYIELD();
    }

    return;

reset_status:
    ir_state = IR_RX_IDLE;
    cnt = 0;
    ir_data.val = 0;
    ir_repeat = 0;
    rep_flg = 0;
}

esp_err_t ir_rx_disable()
{
    IR_RX_CHECK(ir_rx_obj, "ir rx not been initialized yet", ESP_FAIL);
    gpio_isr_handler_remove(ir_rx_obj->io_num);

    return ESP_OK;
}

esp_err_t ir_rx_enable()
{
    IR_RX_CHECK(ir_rx_obj, "ir rx not been initialized yet", ESP_FAIL);
    gpio_isr_handler_add(ir_rx_obj->io_num, ir_rx_intr_handler, (void *) ir_rx_obj->io_num);

    return ESP_OK;
}

int ir_rx_recv_data(ir_rx_nec_data_t *data, size_t len, uint32_t timeout_ticks)
{
    IR_RX_CHECK(ir_rx_obj, "ir rx not been initialized yet", ESP_FAIL);
    int ret;
    ir_rx_nec_data_t *buf = NULL;
    size_t size = 0;
    uint32_t ticks_escape = 0, ticks_last = 0;
    struct timeval now;

    if (timeout_ticks != portMAX_DELAY) {
        gettimeofday(&now, NULL);
        ticks_last = (now.tv_sec * 1000 + now.tv_usec / 1000) / portTICK_RATE_MS;
    }

    ret = xSemaphoreTake(ir_rx_obj->recv_mux, timeout_ticks);

    if (ret != pdTRUE) {
        IR_RX_CHECK(false, "SemaphoreTake error", -1);
    }

    if (timeout_ticks != portMAX_DELAY) {
        gettimeofday(&now, NULL);
        ticks_escape = (now.tv_sec * 1000 + now.tv_usec / 1000) / portTICK_RATE_MS - ticks_last;

        if (timeout_ticks <= ticks_escape) {
            xSemaphoreGive(ir_rx_obj->recv_mux);
            IR_RX_CHECK(false, "timeout", -1);
        } else {
            timeout_ticks -= ticks_escape;
        }
    }

    for (int x = 0; x < len;) {
        buf = (ir_rx_nec_data_t *) xRingbufferReceive(ir_rx_obj->ring_buf, &size, timeout_ticks);

        if (buf == NULL) {
            xSemaphoreGive(ir_rx_obj->recv_mux);
            IR_RX_CHECK(false, "RingbufferReceive error", -1);
        }

        memcpy(&data[x], buf, size);
        vRingbufferReturnItem(ir_rx_obj->ring_buf, buf);
        x += size;

        if (timeout_ticks != portMAX_DELAY) {
            gettimeofday(&now, NULL);
            ticks_escape = (now.tv_sec * 1000 + now.tv_usec / 1000) / portTICK_RATE_MS - ticks_last;

            if (timeout_ticks <= ticks_escape) {
                xSemaphoreGive(ir_rx_obj->recv_mux);
                IR_RX_CHECK(false, "timeout, return the actual accepted length", x);
            } else {
                timeout_ticks -= ticks_escape;
            }
        }
    }

    xSemaphoreGive(ir_rx_obj->recv_mux);

    return len;
}

static esp_err_t ir_rx_gpio_init(uint32_t io_num)
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.pin_bit_mask = 1ULL << io_num;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
    gpio_install_isr_service(0);

    return ESP_OK;
}

esp_err_t ir_rx_deinit()
{
    IR_RX_CHECK(ir_rx_obj, "ir rx has not been initialized yet.", ESP_FAIL);

    ir_rx_disable();

    if (ir_rx_obj->ring_buf) {
        vRingbufferDelete(ir_rx_obj->ring_buf);
        ir_rx_obj->ring_buf = NULL;
    }

    if (ir_rx_obj->recv_mux) {
        vSemaphoreDelete(ir_rx_obj->recv_mux);
        ir_rx_obj->recv_mux = NULL;
    }

    heap_caps_free(ir_rx_obj);
    ir_rx_obj = NULL;
    return ESP_OK;
}

esp_err_t ir_rx_init(ir_rx_config_t *config)
{
    IR_RX_CHECK(config, "config error", ESP_ERR_INVALID_ARG);
    IR_RX_CHECK(NULL == ir_rx_obj, "ir rx has been initialized", ESP_FAIL);

    ir_rx_obj = heap_caps_malloc(sizeof(ir_rx_obj_t), MALLOC_CAP_8BIT);
    IR_RX_CHECK(ir_rx_obj, "ir rx object malloc error", ESP_ERR_NO_MEM);
    ir_rx_obj->io_num = config->io_num;
    ir_rx_obj->buf_len = config->buf_len;
    ir_rx_obj->ring_buf = xRingbufferCreate(sizeof(ir_rx_nec_data_t) * ir_rx_obj->buf_len, RINGBUF_TYPE_NOSPLIT);
    ir_rx_obj->recv_mux = xSemaphoreCreateMutex();

    if (NULL == ir_rx_obj->ring_buf || NULL == ir_rx_obj->recv_mux) {
        ir_rx_deinit();
        IR_RX_CHECK(false, "Ringbuffer or Mutex create fail", ESP_ERR_NO_MEM);
    }

    // gpio configure for IR rx pin
    ir_rx_gpio_init(ir_rx_obj->io_num);
    ir_rx_enable();

    return ESP_OK;
}


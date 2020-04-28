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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp8266/eagle_soc.h"
#include "esp8266/pin_mux_register.h"
#include "esp8266/i2s_register.h"
#include "esp8266/i2s_struct.h"
#include "esp8266/slc_register.h"
#include "esp8266/slc_struct.h"
#include "rom/ets_sys.h"
#include "esp_attr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_libc.h"
#include "esp_heap_caps.h"
#include "driver/i2s.h"

static const char *I2S_TAG = "i2s";

#define I2S_CHECK(a, str, ret_val) \
    if (!(a)) { \
        ESP_LOGE(I2S_TAG,"%s(%d): %s", __FUNCTION__, __LINE__, str); \
        return (ret_val); \
    }

#define portYIELD_FROM_ISR() taskYIELD()
#define dma_intr_enable() _xt_isr_unmask(1 << ETS_SLC_INUM)
#define dma_intr_disable() _xt_isr_mask(1 << ETS_SLC_INUM)
#define dma_intr_register(a, b) _xt_isr_attach(ETS_SLC_INUM, (a), (b))

#define I2S_MAX_BUFFER_SIZE               (4 * 1024 * 1024) // the maximum RAM can be allocated
#define I2S_BASE_CLK                      (2 * APB_CLK_FREQ)
#define I2S_ENTER_CRITICAL()              portENTER_CRITICAL()
#define I2S_EXIT_CRITICAL()               portEXIT_CRITICAL()
#define I2S_FULL_DUPLEX_SLAVE_MODE_MASK   (I2S_MODE_TX | I2S_MODE_RX | I2S_MODE_SLAVE)
#define I2S_FULL_DUPLEX_MASTER_MODE_MASK  (I2S_MODE_TX | I2S_MODE_RX | I2S_MODE_MASTER)

typedef struct lldesc {
    uint32_t                blocksize : 12;
    uint32_t                datalen   : 12;
    uint32_t                unused    :  5;
    uint32_t                sub_sof   :  1;
    uint32_t                eof       :  1;
    volatile uint32_t       owner     :  1; // DMA can change this value
    uint32_t               *buf_ptr;
    struct lldesc          *next_link_ptr;
} lldesc_t;

/**
 * @brief DMA buffer object
 */
typedef struct {
    char **buf;
    int buf_size;
    int rw_pos;
    void *curr_ptr;
    SemaphoreHandle_t mux;
    xQueueHandle queue;
    lldesc_t **desc;
} i2s_dma_t;

/**
 * @brief I2S object instance
 */
typedef struct {
    i2s_port_t i2s_num;         /*!< I2S port number*/
    int queue_size;             /*!< I2S event queue size*/
    QueueHandle_t i2s_queue;    /*!< I2S queue handler*/
    int dma_buf_count;          /*!< DMA buffer count, number of buffer*/
    int dma_buf_len;            /*!< DMA buffer length, length of each buffer*/
    i2s_dma_t *rx;              /*!< DMA Tx buffer*/
    i2s_dma_t *tx;              /*!< DMA Rx buffer*/
    int channel_num;            /*!< Number of channels*/
    int bytes_per_sample;       /*!< Bytes per sample*/
    int bits_per_sample;        /*!< Bits per sample*/
    i2s_mode_t mode;            /*!< I2S Working mode*/
    uint32_t sample_rate;       /*!< I2S sample rate */
    bool tx_desc_auto_clear;    /*!< I2S auto clear tx descriptor on underflow */
    slc_struct_t *dma;
} i2s_obj_t;

static i2s_obj_t *p_i2s_obj[I2S_NUM_MAX] = {0};
static i2s_struct_t *I2S[I2S_NUM_MAX] = {&I2S0};

static i2s_dma_t *i2s_create_dma_queue(i2s_port_t i2s_num, int dma_buf_count, int dma_buf_len);
static esp_err_t i2s_destroy_dma_queue(i2s_port_t i2s_num, i2s_dma_t *dma);

static esp_err_t i2s_reset_fifo(i2s_port_t i2s_num)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_ENTER_CRITICAL();
    I2S[i2s_num]->conf.rx_fifo_reset = 1;
    I2S[i2s_num]->conf.rx_fifo_reset = 0;
    I2S[i2s_num]->conf.tx_fifo_reset = 1;
    I2S[i2s_num]->conf.tx_fifo_reset = 0;
    I2S_EXIT_CRITICAL();
    return ESP_OK;
}

esp_err_t i2s_clear_intr_status(i2s_port_t i2s_num, uint32_t clr_mask)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK(p_i2s_obj[i2s_num], "i2s not installed yet", ESP_FAIL);
    p_i2s_obj[i2s_num]->dma->int_clr.val = clr_mask;
    return ESP_OK;
}

esp_err_t i2s_enable_rx_intr(i2s_port_t i2s_num)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK(p_i2s_obj[i2s_num], "i2s not installed yet", ESP_FAIL);
    I2S_ENTER_CRITICAL();
    p_i2s_obj[i2s_num]->dma->int_ena.tx_suc_eof = 1;
    p_i2s_obj[i2s_num]->dma->int_ena.tx_dscr_err = 1;
    I2S_EXIT_CRITICAL();
    return ESP_OK;
}

esp_err_t i2s_disable_rx_intr(i2s_port_t i2s_num)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK(p_i2s_obj[i2s_num], "i2s not installed yet", ESP_FAIL);
    I2S_ENTER_CRITICAL();
    p_i2s_obj[i2s_num]->dma->int_ena.tx_suc_eof = 0;
    p_i2s_obj[i2s_num]->dma->int_ena.tx_dscr_err = 0;
    I2S_EXIT_CRITICAL();
    return ESP_OK;
}

esp_err_t i2s_disable_tx_intr(i2s_port_t i2s_num)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK(p_i2s_obj[i2s_num], "i2s not installed yet", ESP_FAIL);
    I2S_ENTER_CRITICAL();
    p_i2s_obj[i2s_num]->dma->int_ena.rx_eof = 0;
    p_i2s_obj[i2s_num]->dma->int_ena.rx_dscr_err = 0;
    I2S_EXIT_CRITICAL();
    return ESP_OK;
}

esp_err_t i2s_enable_tx_intr(i2s_port_t i2s_num)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK(p_i2s_obj[i2s_num], "i2s not installed yet", ESP_FAIL);
    I2S_ENTER_CRITICAL();
    p_i2s_obj[i2s_num]->dma->int_ena.rx_eof = 1;
    p_i2s_obj[i2s_num]->dma->int_ena.rx_dscr_err = 1;
    I2S_EXIT_CRITICAL();
    return ESP_OK;
}

static void IRAM_ATTR i2s_intr_handler_default(void *arg)
{
    i2s_obj_t *p_i2s = (i2s_obj_t *) arg;
    slc_struct_t *dma_reg = p_i2s->dma;
    i2s_event_t i2s_event;
    int dummy;

    portBASE_TYPE high_priority_task_awoken = 0;

    lldesc_t *finish_desc;

    if (dma_reg->int_st.tx_dscr_err || dma_reg->int_st.rx_dscr_err) {
        ESP_EARLY_LOGE(I2S_TAG, "dma error, interrupt status: 0x%08x", dma_reg->int_st.val);

        if (p_i2s->i2s_queue) {
            i2s_event.type = I2S_EVENT_DMA_ERROR;

            if (xQueueIsQueueFullFromISR(p_i2s->i2s_queue)) {
                xQueueReceiveFromISR(p_i2s->i2s_queue, &dummy, &high_priority_task_awoken);
            }

            xQueueSendFromISR(p_i2s->i2s_queue, (void *)&i2s_event, &high_priority_task_awoken);
        }
    }

    if (dma_reg->int_st.rx_eof && p_i2s->tx) {
        finish_desc = (lldesc_t *) dma_reg->rx_eof_des_addr;

        // All buffers are empty. This means we have an underflow on our hands.
        if (xQueueIsQueueFullFromISR(p_i2s->tx->queue)) {
            xQueueReceiveFromISR(p_i2s->tx->queue, &dummy, &high_priority_task_awoken);

            // See if tx descriptor needs to be auto cleared:
            // This will avoid any kind of noise that may get introduced due to transmission
            // of previous data from tx descriptor on I2S line.
            if (p_i2s->tx_desc_auto_clear == true) {
                memset((void *) dummy, 0, p_i2s->tx->buf_size);
            }
        }

        xQueueSendFromISR(p_i2s->tx->queue, (void *)(&finish_desc->buf_ptr), &high_priority_task_awoken);

        if (p_i2s->i2s_queue) {
            i2s_event.type = I2S_EVENT_TX_DONE;

            if (xQueueIsQueueFullFromISR(p_i2s->i2s_queue)) {
                xQueueReceiveFromISR(p_i2s->i2s_queue, &dummy, &high_priority_task_awoken);
            }

            xQueueSendFromISR(p_i2s->i2s_queue, (void *)&i2s_event, &high_priority_task_awoken);
        }
    }

    if (dma_reg->int_st.tx_suc_eof && p_i2s->rx) {
        // All buffers are full. This means we have an overflow.
        finish_desc = (lldesc_t *) dma_reg->tx_eof_des_addr;
        finish_desc->owner = 1;

        if (xQueueIsQueueFullFromISR(p_i2s->rx->queue)) {
            xQueueReceiveFromISR(p_i2s->rx->queue, &dummy, &high_priority_task_awoken);
        }

        xQueueSendFromISR(p_i2s->rx->queue, (void *)(&finish_desc->buf_ptr), &high_priority_task_awoken);

        if (p_i2s->i2s_queue) {
            i2s_event.type = I2S_EVENT_RX_DONE;

            if (p_i2s->i2s_queue && xQueueIsQueueFullFromISR(p_i2s->i2s_queue)) {
                xQueueReceiveFromISR(p_i2s->i2s_queue, &dummy, &high_priority_task_awoken);
            }

            xQueueSendFromISR(p_i2s->i2s_queue, (void *)&i2s_event, &high_priority_task_awoken);
        }
    }

    if (high_priority_task_awoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }

    dma_reg->int_clr.val = dma_reg->int_st.val;
}

static esp_err_t i2s_destroy_dma_queue(i2s_port_t i2s_num, i2s_dma_t *dma)
{
    int bux_idx;

    if (p_i2s_obj[i2s_num] == NULL) {
        ESP_LOGE(I2S_TAG, "Not initialized yet");
        return ESP_ERR_INVALID_ARG;
    }

    if (dma == NULL) {
        ESP_LOGE(I2S_TAG, "dma is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    for (bux_idx = 0; bux_idx < p_i2s_obj[i2s_num]->dma_buf_count; bux_idx++) {
        if (dma->desc && dma->desc[bux_idx]) {
            heap_caps_free(dma->desc[bux_idx]);
        }

        if (dma->buf && dma->buf[bux_idx]) {
            heap_caps_free(dma->buf[bux_idx]);
        }
    }

    if (dma->buf) {
        heap_caps_free(dma->buf);
    }

    if (dma->desc) {
        heap_caps_free(dma->desc);
    }

    vQueueDelete(dma->queue);
    vSemaphoreDelete(dma->mux);
    heap_caps_free(dma);
    return ESP_OK;
}

static i2s_dma_t *i2s_create_dma_queue(i2s_port_t i2s_num, int dma_buf_count, int dma_buf_len)
{
    int bux_idx;
    int sample_size = p_i2s_obj[i2s_num]->bytes_per_sample * p_i2s_obj[i2s_num]->channel_num;
    i2s_dma_t *dma = (i2s_dma_t *)heap_caps_zalloc(sizeof(i2s_dma_t), MALLOC_CAP_8BIT);

    if (dma == NULL) {
        ESP_LOGE(I2S_TAG, "Error malloc i2s_dma_t");
        return NULL;
    }

    dma->buf = (char **)heap_caps_zalloc(sizeof(char *) * dma_buf_count, MALLOC_CAP_8BIT);

    if (dma->buf == NULL) {
        ESP_LOGE(I2S_TAG, "Error malloc dma buffer pointer");
        heap_caps_free(dma);
        return NULL;
    }

    for (bux_idx = 0; bux_idx < dma_buf_count; bux_idx++) {
        dma->buf[bux_idx] = (char *) heap_caps_calloc(1, dma_buf_len * sample_size, MALLOC_CAP_8BIT);

        if (dma->buf[bux_idx] == NULL) {
            ESP_LOGE(I2S_TAG, "Error malloc dma buffer");
            i2s_destroy_dma_queue(i2s_num, dma);
            return NULL;
        }

        ESP_LOGD(I2S_TAG, "Addr[%d] = %d", bux_idx, (int)dma->buf[bux_idx]);
    }

    dma->desc = (lldesc_t **)heap_caps_malloc(sizeof(lldesc_t *) * dma_buf_count, MALLOC_CAP_8BIT);

    if (dma->desc == NULL) {
        ESP_LOGE(I2S_TAG, "Error malloc dma description");
        i2s_destroy_dma_queue(i2s_num, dma);
        return NULL;
    }

    for (bux_idx = 0; bux_idx < dma_buf_count; bux_idx++) {
        dma->desc[bux_idx] = (lldesc_t *) heap_caps_malloc(sizeof(lldesc_t), MALLOC_CAP_8BIT);

        if (dma->desc[bux_idx] == NULL) {
            ESP_LOGE(I2S_TAG, "Error malloc dma description entry");
            i2s_destroy_dma_queue(i2s_num, dma);
            return NULL;
        }
    }

    for (bux_idx = 0; bux_idx < dma_buf_count; bux_idx++) {
        // Configuring the DMA queue
        dma->desc[bux_idx]->owner = 1;
        dma->desc[bux_idx]->eof = 1;    // Each linked list produces an EOF interrupt that notifies the task of filling the data more quickly
        dma->desc[bux_idx]->sub_sof = 0;
        dma->desc[bux_idx]->datalen = dma_buf_len * sample_size;  // Actual number of bytes of data
        dma->desc[bux_idx]->blocksize = dma_buf_len * sample_size;  // Total number of bytes of data
        dma->desc[bux_idx]->buf_ptr = (uint32_t *) dma->buf[bux_idx];
        dma->desc[bux_idx]->unused = 0;
        dma->desc[bux_idx]->next_link_ptr = (lldesc_t *)((bux_idx < (dma_buf_count - 1)) ? (dma->desc[bux_idx + 1]) : dma->desc[0]);
    }

    dma->queue = xQueueCreate(dma_buf_count - 1, sizeof(char *));
    dma->mux = xSemaphoreCreateMutex();
    dma->rw_pos = 0;
    dma->buf_size = dma_buf_len * sample_size;
    dma->curr_ptr = NULL;
    ESP_LOGI(I2S_TAG, "DMA Malloc info, datalen=blocksize=%d, dma_buf_count=%d", dma_buf_len * sample_size, dma_buf_count);
    return dma;
}

esp_err_t i2s_start(i2s_port_t i2s_num)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK(p_i2s_obj[i2s_num], "i2s not installed yet", ESP_FAIL);
    // start DMA link
    I2S_ENTER_CRITICAL();
    i2s_reset_fifo(i2s_num);
    // reset dma
    p_i2s_obj[i2s_num]->dma->conf0.rx_rst = 1;
    p_i2s_obj[i2s_num]->dma->conf0.rx_rst = 0;
    p_i2s_obj[i2s_num]->dma->conf0.tx_rst = 1;
    p_i2s_obj[i2s_num]->dma->conf0.tx_rst = 0;

    I2S[i2s_num]->conf.tx_reset = 1;
    I2S[i2s_num]->conf.tx_reset = 0;
    I2S[i2s_num]->conf.rx_reset = 1;
    I2S[i2s_num]->conf.rx_reset = 0;

    dma_intr_disable();

    if (p_i2s_obj[i2s_num]->mode & I2S_MODE_TX) {
        i2s_enable_tx_intr(i2s_num);
        p_i2s_obj[i2s_num]->dma->rx_link.start = 1;
    }

    if (p_i2s_obj[i2s_num]->mode & I2S_MODE_RX) {
        i2s_enable_rx_intr(i2s_num);
        p_i2s_obj[i2s_num]->dma->tx_link.start = 1;
    }

    // Both TX and RX are started to ensure clock generation
    I2S[i2s_num]->conf.val |= I2S_I2S_TX_START | I2S_I2S_RX_START;
    // Simultaneously reset to ensure the same phase.
    I2S[i2s_num]->conf.val |= I2S_I2S_RESET_MASK;
    I2S[i2s_num]->conf.val &= ~I2S_I2S_RESET_MASK;
    dma_intr_enable();
    I2S_EXIT_CRITICAL();
    return ESP_OK;
}

esp_err_t i2s_stop(i2s_port_t i2s_num)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK(p_i2s_obj[i2s_num], "i2s not installed yet", ESP_FAIL);
    I2S_ENTER_CRITICAL();
    dma_intr_disable();

    if (p_i2s_obj[i2s_num]->mode & I2S_MODE_TX) {
        p_i2s_obj[i2s_num]->dma->rx_link.stop = 1;
        i2s_disable_tx_intr(i2s_num);
    }

    if (p_i2s_obj[i2s_num]->mode & I2S_MODE_RX) {
        p_i2s_obj[i2s_num]->dma->tx_link.stop = 1;
        i2s_disable_rx_intr(i2s_num);
    }

    I2S[i2s_num]->conf.val &= ~(I2S_I2S_TX_START | I2S_I2S_RX_START);
    p_i2s_obj[i2s_num]->dma->int_clr.val = p_i2s_obj[i2s_num]->dma->int_st.val; // clear pending interrupt
    I2S_EXIT_CRITICAL();
    return ESP_OK;
}

esp_err_t i2s_set_pin(i2s_port_t i2s_num, const i2s_pin_config_t *pin)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK(pin, "param null", ESP_ERR_INVALID_ARG);

    if (pin->bck_o_en == true) {
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_I2SO_BCK);
    }

    if (pin->ws_o_en == true) {
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_I2SO_WS);
    }

    if (pin->data_out_en == true) {
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_I2SO_DATA);
    }

    if (pin->bck_i_en == true) {
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_I2SI_BCK);
    }

    if (pin->ws_i_en == true) {
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_I2SI_WS);
    }

    if (pin->data_in_en == true) {
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_I2SI_DATA);
    }

    return ESP_OK;
}

static esp_err_t i2s_set_rate(i2s_port_t i2s_num, uint32_t rate)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK(p_i2s_obj[i2s_num], "i2s not installed yet", ESP_FAIL);
    uint8_t bck_div = 1;
    uint8_t mclk_div = 1;

    // Calculate the frequency division corresponding to the bit rate
    uint32_t scaled_base_freq = I2S_BASE_CLK / 32;
    float delta_best = scaled_base_freq;

    for (uint8_t i = 1; i < 64; i++) {
        for (uint8_t j = i; j < 64; j++) {
            float new_delta = abs(((float)scaled_base_freq / i / j) - rate);

            if (new_delta < delta_best) {
                delta_best = new_delta;
                bck_div = i;
                mclk_div = j;
            }
        }
    }

    // Configure the frequency division of I2S
    I2S_ENTER_CRITICAL();
    I2S[i2s_num]->conf.bck_div_num = bck_div & 0x3F;
    I2S[i2s_num]->conf.clkm_div_num = mclk_div & 0x3F;
    I2S_EXIT_CRITICAL();
    p_i2s_obj[i2s_num]->sample_rate = rate;
    return ESP_OK;
}

esp_err_t i2s_set_clk(i2s_port_t i2s_num, uint32_t rate, i2s_bits_per_sample_t bits, i2s_channel_t ch)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK(p_i2s_obj[i2s_num], "i2s not installed yet", ESP_FAIL);

    i2s_dma_t *save_tx = NULL, *save_rx = NULL;

    if (bits % 8 != 0 || bits > I2S_BITS_PER_SAMPLE_24BIT || bits < I2S_BITS_PER_SAMPLE_16BIT) {
        ESP_LOGE(I2S_TAG, "Invalid bits per sample");
        return ESP_ERR_INVALID_ARG;
    }

    // wait all on-going writing finish
    if ((p_i2s_obj[i2s_num]->mode & I2S_MODE_TX) && p_i2s_obj[i2s_num]->tx) {
        xSemaphoreTake(p_i2s_obj[i2s_num]->tx->mux, (portTickType)portMAX_DELAY);
    }

    if ((p_i2s_obj[i2s_num]->mode & I2S_MODE_RX) && p_i2s_obj[i2s_num]->rx) {
        xSemaphoreTake(p_i2s_obj[i2s_num]->rx->mux, (portTickType)portMAX_DELAY);
    }

    i2s_stop(i2s_num);

    i2s_set_rate(i2s_num, rate);

    uint32_t cur_mode = 0;

    if (p_i2s_obj[i2s_num]->channel_num != ch) {
        p_i2s_obj[i2s_num]->channel_num = (ch == 2) ? 2 : 1;
        cur_mode = I2S[i2s_num]->fifo_conf.tx_fifo_mod;
        I2S[i2s_num]->fifo_conf.tx_fifo_mod = (ch == 2) ? cur_mode - 1 : cur_mode + 1;
        cur_mode = I2S[i2s_num]->fifo_conf.rx_fifo_mod;
        I2S[i2s_num]->fifo_conf.rx_fifo_mod = (ch == 2) ? cur_mode - 1  : cur_mode + 1;
        I2S[i2s_num]->conf_chan.tx_chan_mod = (ch == 2) ? 0 : 1;
        I2S[i2s_num]->conf_chan.rx_chan_mod = (ch == 2) ? 0 : 1;
    }

    if (bits != p_i2s_obj[i2s_num]->bits_per_sample) {
        //change fifo mode
        if (p_i2s_obj[i2s_num]->bits_per_sample <= 16 && bits > 16) {
            I2S[i2s_num]->fifo_conf.tx_fifo_mod += 2;
            I2S[i2s_num]->fifo_conf.rx_fifo_mod += 2;
        } else if (p_i2s_obj[i2s_num]->bits_per_sample > 16 && bits <= 16) {
            I2S[i2s_num]->fifo_conf.tx_fifo_mod -= 2;
            I2S[i2s_num]->fifo_conf.rx_fifo_mod -= 2;
        }

        p_i2s_obj[i2s_num]->bits_per_sample = bits;
        p_i2s_obj[i2s_num]->bytes_per_sample = p_i2s_obj[i2s_num]->bits_per_sample / 8;

        // Round bytes_per_sample up to next multiple of 16 bits
        int halfwords_per_sample = (p_i2s_obj[i2s_num]->bits_per_sample + 15) / 16;
        p_i2s_obj[i2s_num]->bytes_per_sample = halfwords_per_sample * 2;

        // Because limited of DMA buffer is 4092 bytes
        if (p_i2s_obj[i2s_num]->dma_buf_len * p_i2s_obj[i2s_num]->bytes_per_sample * p_i2s_obj[i2s_num]->channel_num > 4092) {
            p_i2s_obj[i2s_num]->dma_buf_len = 4092 / p_i2s_obj[i2s_num]->bytes_per_sample / p_i2s_obj[i2s_num]->channel_num;
        }

        // Re-create TX DMA buffer
        if (p_i2s_obj[i2s_num]->mode & I2S_MODE_TX) {

            save_tx = p_i2s_obj[i2s_num]->tx;

            p_i2s_obj[i2s_num]->tx = i2s_create_dma_queue(i2s_num, p_i2s_obj[i2s_num]->dma_buf_count, p_i2s_obj[i2s_num]->dma_buf_len);

            if (p_i2s_obj[i2s_num]->tx == NULL) {
                ESP_LOGE(I2S_TAG, "Failed to create tx dma buffer");
                i2s_driver_uninstall(i2s_num);
                return ESP_ERR_NO_MEM;
            }

            p_i2s_obj[i2s_num]->dma->rx_link.addr = (uint32_t) p_i2s_obj[i2s_num]->tx->desc[0];

            // destroy old tx dma if exist
            if (save_tx) {
                i2s_destroy_dma_queue(i2s_num, save_tx);
            }
        }

        // Re-create RX DMA buffer
        if (p_i2s_obj[i2s_num]->mode & I2S_MODE_RX) {

            save_rx = p_i2s_obj[i2s_num]->rx;

            p_i2s_obj[i2s_num]->rx = i2s_create_dma_queue(i2s_num, p_i2s_obj[i2s_num]->dma_buf_count, p_i2s_obj[i2s_num]->dma_buf_len);

            if (p_i2s_obj[i2s_num]->rx == NULL) {
                ESP_LOGE(I2S_TAG, "Failed to create rx dma buffer");
                i2s_driver_uninstall(i2s_num);
                return ESP_ERR_NO_MEM;
            }

            I2S[i2s_num]->rx_eof_num = (p_i2s_obj[i2s_num]->dma_buf_len * p_i2s_obj[i2s_num]->channel_num * p_i2s_obj[i2s_num]->bytes_per_sample) / 4;
            p_i2s_obj[i2s_num]->dma->tx_link.addr = (uint32_t) p_i2s_obj[i2s_num]->rx->desc[0];

            // destroy old rx dma if exist
            if (save_rx) {
                i2s_destroy_dma_queue(i2s_num, save_rx);
            }
        }

    }

    I2S[i2s_num]->conf.bits_mod = bits;

    // wait all writing on-going finish
    if ((p_i2s_obj[i2s_num]->mode & I2S_MODE_TX) && p_i2s_obj[i2s_num]->tx) {
        xSemaphoreGive(p_i2s_obj[i2s_num]->tx->mux);
    }

    if ((p_i2s_obj[i2s_num]->mode & I2S_MODE_RX) && p_i2s_obj[i2s_num]->rx) {
        xSemaphoreGive(p_i2s_obj[i2s_num]->rx->mux);
    }

    i2s_start(i2s_num);
    return ESP_OK;
}

esp_err_t i2s_set_sample_rates(i2s_port_t i2s_num, uint32_t rate)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK((p_i2s_obj[i2s_num]->bytes_per_sample > 0), "bits_per_sample not set", ESP_ERR_INVALID_ARG);
    return i2s_set_clk(i2s_num, rate, p_i2s_obj[i2s_num]->bits_per_sample, p_i2s_obj[i2s_num]->channel_num);
}

static esp_err_t i2s_param_config(i2s_port_t i2s_num, const i2s_config_t *i2s_config)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK(p_i2s_obj[i2s_num], "i2s not installed yet", ESP_FAIL);
    I2S_CHECK((i2s_config), "param null", ESP_ERR_INVALID_ARG);

    // configure I2S data port interface.
    i2s_reset_fifo(i2s_num);
    //reset i2s
    I2S[i2s_num]->conf.tx_reset = 1;
    I2S[i2s_num]->conf.tx_reset = 0;
    I2S[i2s_num]->conf.rx_reset = 1;
    I2S[i2s_num]->conf.rx_reset = 0;

    // disable all i2s interrupt
    I2S[i2s_num]->int_ena.val = 0;

    //reset dma
    p_i2s_obj[i2s_num]->dma->conf0.rx_rst = 1;
    p_i2s_obj[i2s_num]->dma->conf0.rx_rst = 0;
    p_i2s_obj[i2s_num]->dma->conf0.tx_rst = 1;
    p_i2s_obj[i2s_num]->dma->conf0.tx_rst = 0;

    //Enable and configure DMA
    p_i2s_obj[i2s_num]->dma->conf0.txdata_burst_en = 0;
    p_i2s_obj[i2s_num]->dma->conf0.txdscr_burst_en = 1;
    p_i2s_obj[i2s_num]->dma->rx_dscr_conf.rx_fill_mode = 0;
    p_i2s_obj[i2s_num]->dma->rx_dscr_conf.rx_eof_mode = 0;
    p_i2s_obj[i2s_num]->dma->rx_dscr_conf.rx_fill_en = 0;
    p_i2s_obj[i2s_num]->dma->rx_dscr_conf.token_no_replace = 1;
    p_i2s_obj[i2s_num]->dma->rx_dscr_conf.infor_no_replace = 1;


    I2S[i2s_num]->fifo_conf.dscr_en = 0;

    I2S[i2s_num]->conf_chan.tx_chan_mod = i2s_config->channel_format < I2S_CHANNEL_FMT_ONLY_RIGHT ? i2s_config->channel_format : (i2s_config->channel_format >> 1); // 0-two channel;1-right;2-left;3-righ;4-left
    I2S[i2s_num]->fifo_conf.tx_fifo_mod = i2s_config->channel_format < I2S_CHANNEL_FMT_ONLY_RIGHT ? 0 : 1; // 0-right&left channel;1-one channel

    I2S[i2s_num]->conf_chan.rx_chan_mod = i2s_config->channel_format < I2S_CHANNEL_FMT_ONLY_RIGHT ? i2s_config->channel_format : (i2s_config->channel_format >> 1); // 0-two channel;1-right;2-left;3-righ;4-left
    I2S[i2s_num]->fifo_conf.rx_fifo_mod = i2s_config->channel_format < I2S_CHANNEL_FMT_ONLY_RIGHT ? 0 : 1; // 0-right&left channel;1-one channel

    I2S[i2s_num]->fifo_conf.dscr_en = 1;// connect dma to fifo

    I2S[i2s_num]->conf.tx_start = 0;
    I2S[i2s_num]->conf.rx_start = 0;

    I2S[i2s_num]->conf.msb_right = 1;
    I2S[i2s_num]->conf.right_first = 1;

    if (i2s_config->mode & I2S_MODE_TX) {
        I2S[i2s_num]->conf.tx_slave_mod = 0; // Master

        if (i2s_config->mode & I2S_MODE_SLAVE) {
            I2S[i2s_num]->conf.tx_slave_mod = 1;// TX Slave
        }
    }

    if (i2s_config->mode & I2S_MODE_RX) {
        I2S[i2s_num]->conf.rx_slave_mod = 0; // Master

        if (i2s_config->mode & I2S_MODE_SLAVE) {
            I2S[i2s_num]->conf.rx_slave_mod = 1;// RX Slave
        }
    }

    if (i2s_config->communication_format & I2S_COMM_FORMAT_I2S) {
        I2S[i2s_num]->conf.tx_msb_shift = 1;
        I2S[i2s_num]->conf.rx_msb_shift = 1;

        if (i2s_config->communication_format & I2S_COMM_FORMAT_I2S_LSB) {
            if (i2s_config->mode & I2S_MODE_TX) {
                I2S[i2s_num]->conf.tx_msb_shift = 0;
            }

            if (i2s_config->mode & I2S_MODE_RX) {
                I2S[i2s_num]->conf.rx_msb_shift = 0;
            }
        }
    }

    if ((p_i2s_obj[i2s_num]->mode & I2S_MODE_RX) && (p_i2s_obj[i2s_num]->mode & I2S_MODE_TX)) {
        if (p_i2s_obj[i2s_num]->mode & I2S_MODE_MASTER) {
            I2S[i2s_num]->conf.tx_slave_mod = 0;    // TX Master
            I2S[i2s_num]->conf.rx_slave_mod = 0;    // RX Master
        } else {
            I2S[i2s_num]->conf.tx_slave_mod = 1;    // TX Slave
            I2S[i2s_num]->conf.rx_slave_mod = 1;    // RX Slave
        }
    }

    p_i2s_obj[i2s_num]->tx_desc_auto_clear = i2s_config->tx_desc_auto_clear;
    return ESP_OK;
}

esp_err_t i2s_zero_dma_buffer(i2s_port_t i2s_num)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK(p_i2s_obj[i2s_num], "i2s not installed yet", ESP_FAIL);

    if (p_i2s_obj[i2s_num]->rx && p_i2s_obj[i2s_num]->rx->buf != NULL && p_i2s_obj[i2s_num]->rx->buf_size != 0) {
        for (int i = 0; i < p_i2s_obj[i2s_num]->dma_buf_count; i++) {
            memset(p_i2s_obj[i2s_num]->rx->buf[i], 0, p_i2s_obj[i2s_num]->rx->buf_size);
        }
    }

    if (p_i2s_obj[i2s_num]->tx && p_i2s_obj[i2s_num]->tx->buf != NULL && p_i2s_obj[i2s_num]->tx->buf_size != 0) {
        int bytes_left = 0;
        bytes_left = (p_i2s_obj[i2s_num]->tx->buf_size - p_i2s_obj[i2s_num]->tx->rw_pos) % 4;

        if (bytes_left) {
            size_t zero_bytes = 0, bytes_written;
            i2s_write(i2s_num, (void *)&zero_bytes, bytes_left, &bytes_written, portMAX_DELAY);
        }

        for (int i = 0; i < p_i2s_obj[i2s_num]->dma_buf_count; i++) {
            memset(p_i2s_obj[i2s_num]->tx->buf[i], 0, p_i2s_obj[i2s_num]->tx->buf_size);
        }
    }

    return ESP_OK;
}

esp_err_t i2s_write(i2s_port_t i2s_num, const void *src, size_t size, size_t *bytes_written, TickType_t ticks_to_wait)
{
    char *data_ptr, *src_byte;
    int bytes_can_write;
    *bytes_written = 0;
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK((size < I2S_MAX_BUFFER_SIZE), "size is too large", ESP_ERR_INVALID_ARG);
    I2S_CHECK((p_i2s_obj[i2s_num]->tx), "tx NULL", ESP_ERR_INVALID_ARG);
    xSemaphoreTake(p_i2s_obj[i2s_num]->tx->mux, (portTickType)portMAX_DELAY);

    src_byte = (char *)src;

    while (size > 0) {
        if (p_i2s_obj[i2s_num]->tx->rw_pos == p_i2s_obj[i2s_num]->tx->buf_size || p_i2s_obj[i2s_num]->tx->curr_ptr == NULL) {
            if (xQueueReceive(p_i2s_obj[i2s_num]->tx->queue, &p_i2s_obj[i2s_num]->tx->curr_ptr, ticks_to_wait) == pdFALSE) {
                break;
            }

            p_i2s_obj[i2s_num]->tx->rw_pos = 0;
        }

        ESP_LOGD(I2S_TAG, "size: %d, rw_pos: %d, buf_size: %d, curr_ptr: %d", size, p_i2s_obj[i2s_num]->tx->rw_pos, p_i2s_obj[i2s_num]->tx->buf_size, (int)p_i2s_obj[i2s_num]->tx->curr_ptr);
        data_ptr = (char *)p_i2s_obj[i2s_num]->tx->curr_ptr;
        data_ptr += p_i2s_obj[i2s_num]->tx->rw_pos;
        bytes_can_write = p_i2s_obj[i2s_num]->tx->buf_size - p_i2s_obj[i2s_num]->tx->rw_pos;

        if (bytes_can_write > size) {
            bytes_can_write = size;
        }

        memcpy(data_ptr, src_byte, bytes_can_write);
        size -= bytes_can_write;
        src_byte += bytes_can_write;
        p_i2s_obj[i2s_num]->tx->rw_pos += bytes_can_write;
        (*bytes_written) += bytes_can_write;
    }

    xSemaphoreGive(p_i2s_obj[i2s_num]->tx->mux);
    return ESP_OK;
}

esp_err_t i2s_read(i2s_port_t i2s_num, void *dest, size_t size, size_t *bytes_read, TickType_t ticks_to_wait)
{
    char *data_ptr, *dest_byte;
    int bytes_can_read;
    *bytes_read = 0;
    dest_byte = (char *)dest;
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK((size < I2S_MAX_BUFFER_SIZE), "size is too large", ESP_ERR_INVALID_ARG);
    I2S_CHECK((p_i2s_obj[i2s_num]->rx), "rx NULL", ESP_ERR_INVALID_ARG);
    xSemaphoreTake(p_i2s_obj[i2s_num]->rx->mux, (portTickType)portMAX_DELAY);

    while (size > 0) {
        if (p_i2s_obj[i2s_num]->rx->rw_pos == p_i2s_obj[i2s_num]->rx->buf_size || p_i2s_obj[i2s_num]->rx->curr_ptr == NULL) {
            if (xQueueReceive(p_i2s_obj[i2s_num]->rx->queue, &p_i2s_obj[i2s_num]->rx->curr_ptr, ticks_to_wait) == pdFALSE) {
                break;
            }

            p_i2s_obj[i2s_num]->rx->rw_pos = 0;
        }

        data_ptr = (char *)p_i2s_obj[i2s_num]->rx->curr_ptr;
        data_ptr += p_i2s_obj[i2s_num]->rx->rw_pos;
        bytes_can_read = p_i2s_obj[i2s_num]->rx->buf_size - p_i2s_obj[i2s_num]->rx->rw_pos;

        if (bytes_can_read > size) {
            bytes_can_read = size;
        }

        memcpy(dest_byte, data_ptr, bytes_can_read);
        size -= bytes_can_read;
        dest_byte += bytes_can_read;
        p_i2s_obj[i2s_num]->rx->rw_pos += bytes_can_read;
        (*bytes_read) += bytes_can_read;
    }

    xSemaphoreGive(p_i2s_obj[i2s_num]->rx->mux);
    return ESP_OK;
}

esp_err_t i2s_driver_uninstall(i2s_port_t i2s_num)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK(p_i2s_obj[i2s_num], "already uninstalled", ESP_FAIL);

    i2s_stop(i2s_num);
    dma_intr_register(NULL, NULL);

    if (p_i2s_obj[i2s_num]->tx != NULL && p_i2s_obj[i2s_num]->mode & I2S_MODE_TX) {
        i2s_destroy_dma_queue(i2s_num, p_i2s_obj[i2s_num]->tx);
        p_i2s_obj[i2s_num]->tx = NULL;
    }

    if (p_i2s_obj[i2s_num]->rx != NULL && p_i2s_obj[i2s_num]->mode & I2S_MODE_RX) {
        i2s_destroy_dma_queue(i2s_num, p_i2s_obj[i2s_num]->rx);
        p_i2s_obj[i2s_num]->rx = NULL;
    }

    if (p_i2s_obj[i2s_num]->i2s_queue) {
        vQueueDelete(p_i2s_obj[i2s_num]->i2s_queue);
        p_i2s_obj[i2s_num]->i2s_queue = NULL;
    }

    heap_caps_free(p_i2s_obj[i2s_num]);
    p_i2s_obj[i2s_num] = NULL;

    return ESP_OK;
}

esp_err_t i2s_driver_install(i2s_port_t i2s_num, const i2s_config_t *i2s_config, int queue_size, void *i2s_queue)
{
    esp_err_t err;
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK((i2s_config != NULL), "I2S configuration must not NULL", ESP_ERR_INVALID_ARG);
    I2S_CHECK((i2s_config->dma_buf_count >= 2 && i2s_config->dma_buf_count <= 128), "I2S buffer count less than 128 and more than 2", ESP_ERR_INVALID_ARG);
    I2S_CHECK((i2s_config->dma_buf_len >= 8 && i2s_config->dma_buf_len <= 1024), "I2S buffer length at most 1024 and more than 8", ESP_ERR_INVALID_ARG);

    if (p_i2s_obj[i2s_num] == NULL) {
        p_i2s_obj[i2s_num] = (i2s_obj_t *)heap_caps_zalloc(sizeof(i2s_obj_t), MALLOC_CAP_8BIT);
        I2S_CHECK(p_i2s_obj[i2s_num], "Malloc I2S driver error", ESP_ERR_NO_MEM);

        p_i2s_obj[i2s_num]->i2s_num = i2s_num;
        p_i2s_obj[i2s_num]->dma = (slc_struct_t *)&SLC0;
        p_i2s_obj[i2s_num]->dma_buf_count = i2s_config->dma_buf_count;
        p_i2s_obj[i2s_num]->dma_buf_len = i2s_config->dma_buf_len;
        p_i2s_obj[i2s_num]->i2s_queue = i2s_queue;
        p_i2s_obj[i2s_num]->mode = i2s_config->mode;

        p_i2s_obj[i2s_num]->bits_per_sample = 0;
        p_i2s_obj[i2s_num]->bytes_per_sample = 0; // Not initialized yet
        p_i2s_obj[i2s_num]->channel_num = i2s_config->channel_format < I2S_CHANNEL_FMT_ONLY_RIGHT ? 2 : 1;

        //initial interrupt
        dma_intr_register(i2s_intr_handler_default, p_i2s_obj[i2s_num]);
        i2s_stop(i2s_num);
        err = i2s_param_config(i2s_num, i2s_config);

        if (err != ESP_OK) {
            i2s_driver_uninstall(i2s_num);
            ESP_LOGE(I2S_TAG, "I2S param configure error");
            return err;
        }

        if (i2s_queue) {
            p_i2s_obj[i2s_num]->i2s_queue = xQueueCreate(queue_size, sizeof(i2s_event_t));
            *((QueueHandle_t *) i2s_queue) = p_i2s_obj[i2s_num]->i2s_queue;
            ESP_LOGI(I2S_TAG, "queue heap_caps_free spaces: %d", (int)uxQueueSpacesAvailable(p_i2s_obj[i2s_num]->i2s_queue));
        } else {
            p_i2s_obj[i2s_num]->i2s_queue = NULL;
        }

        // set clock and start
        return i2s_set_clk(i2s_num, i2s_config->sample_rate, i2s_config->bits_per_sample, p_i2s_obj[i2s_num]->channel_num);
    }

    ESP_LOGW(I2S_TAG, "I2S driver already installed");
    return ESP_OK;
}
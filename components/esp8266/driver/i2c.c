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
//
// =============================================================================
// REFACTORED DRIVER — full concurrency-safe architecture
// =============================================================================
//
// Key model:
//   * i2s_drv_state_t: UNINIT / INSTALLING / STOPPED / RUNNING /
//                      RECONFIGURING / STOPPING / UNINSTALLING
//   * api_lock:        serialises control operations
//   * tx_users/rx_users: reference count of tasks inside write/read
//   * tx_waiters/rx_waiters: count of tasks blocked on DMA queue
//   * life_refs:        lifetime reference count for public API calls
//   * idle_evt:        event group used to wait for users to drain
//   * NULL sentinel:   pushed into DMA queue to wake blocked waiters
//
// Fixed issues:
//   * No recursive api_lock deadlock during waiter wake-up.
//   * set_clk() allocates new queues before stopping and never uninstalls.
//   * set_clk() preserves RUNNING/STOPPED state correctly.
//   * Task-context drain checks actual EOF status flags.
//   * Event queue is not deleted by uninstall.
//   * ISR does not process EOF descriptors on DMA error.
//   * ISR uses IRAM zero loop instead of memset().
// =============================================================================

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

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
#include "esp_wifi_types.h"
#include "esp_wifi.h"

static const char *I2S_TAG = "i2s";

extern void rom_i2c_writeReg_Mask(uint8_t block,
                                  uint8_t host_id,
                                  uint8_t reg_add,
                                  uint8_t msb,
                                  uint8_t lsb,
                                  uint8_t indata);

#define I2S_MEMW() __asm__ __volatile__("memw" ::: "memory")

#define I2S_CHECK(a, str, ret_val)                                        \
    do                                                                    \
    {                                                                     \
        if (!(a))                                                         \
        {                                                                 \
            ESP_LOGE(I2S_TAG, "%s(%d): %s", __FUNCTION__, __LINE__, str); \
            return (ret_val);                                             \
        }                                                                 \
    } while (0)

#define dma_intr_enable() _xt_isr_unmask(1 << ETS_SLC_INUM)
#define dma_intr_disable() _xt_isr_mask(1 << ETS_SLC_INUM)
#define dma_intr_register(a, b) _xt_isr_attach(ETS_SLC_INUM, (a), (b))

#define I2S_MAX_BUFFER_SIZE (4 * 1024 * 1024)
#define I2S_BASE_CLK (2 * APB_CLK_FREQ)

#define I2S_ENTER_CRITICAL() portENTER_CRITICAL()
#define I2S_EXIT_CRITICAL() portEXIT_CRITICAL()

/* Timeout for acquiring api_lock and for waiting active read/write to drain. */
#define I2S_API_LOCK_TIMEOUT_MS 2000
#define I2S_DRAIN_TIMEOUT_MS 2000

/* Event group bits set when tx_users / rx_users reach zero. */
#define I2S_TX_IDLE_BIT (1 << 0)
#define I2S_RX_IDLE_BIT (1 << 1)

typedef struct lldesc
{
    uint32_t blocksize : 12;
    uint32_t datalen : 12;
    uint32_t unused : 5;
    uint32_t sub_sof : 1;
    uint32_t eof : 1;
    volatile uint32_t owner : 1;
    uint32_t *buf_ptr;
    struct lldesc *next_link_ptr;
} lldesc_t;

typedef struct
{
    char **buf;
    int buf_size;
    int buf_count;         /* number of buffers in this queue */
    int rw_pos;
    void *curr_ptr;
    SemaphoreHandle_t mux; /* protects curr_ptr / rw_pos only */
    QueueHandle_t queue;   /* holds buf pointers */
    lldesc_t **desc;
} i2s_dma_t;

typedef struct
{
    i2s_port_t i2s_num;

    volatile i2s_drv_state_t drv_state;

    SemaphoreHandle_t api_lock;  /* serialises control operations */
    EventGroupHandle_t idle_evt; /* TX/RX idle signalling */

    volatile int tx_users; /* tasks inside i2s_write() */
    volatile int rx_users; /* tasks inside i2s_read() */

    volatile int tx_waiters; /* tasks blocked on tx->queue */
    volatile int rx_waiters; /* tasks blocked on rx->queue */

    volatile int life_refs; /* public API calls holding this object alive */

    int queue_size;
    QueueHandle_t i2s_queue; /* event queue — app-owned after uninstall */

    int dma_buf_count;
    int dma_buf_len;
    int dma_buf_len_orig;

    i2s_dma_t *rx;
    i2s_dma_t *tx;

    int channel_num;
    int bytes_per_sample;
    int bits_per_sample;
    i2s_mode_t mode;
    uint32_t sample_rate;
    bool tx_desc_auto_clear;
    i2s_channel_fmt_t channel_format;

    slc_struct_t *dma;
} i2s_obj_t;

static i2s_obj_t *p_i2s_obj[I2S_NUM_MAX] = {0};
static i2s_struct_t *I2S[I2S_NUM_MAX] = {&I2S0};

/* Forward declarations. */
static i2s_dma_t *i2s_dma_queue_create(int dma_buf_count,
                                       int dma_buf_len,
                                       int sample_size);

static esp_err_t i2s_destroy_dma_queue(i2s_dma_t *dma);

static esp_err_t i2s_start_internal(i2s_port_t i2s_num);
static esp_err_t i2s_stop_internal(i2s_port_t i2s_num);

static void i2s_drain_pending_descriptors_task(i2s_port_t i2s_num,
                                               bool tx_done,
                                               bool rx_done);

static esp_err_t i2s_param_config(i2s_port_t i2s_num,
                                  const i2s_config_t *i2s_config);

static esp_err_t i2s_set_rate(i2s_port_t i2s_num,
                              uint32_t rate,
                              int bits_per_sample);

/* ===========================================================================
 *  Lifetime / user accounting helpers.
 * ======================================================================== */

static i2s_obj_t *i2s_obj_acquire(i2s_port_t i2s_num)
{
    if (i2s_num >= I2S_NUM_MAX)
        return NULL;

    i2s_obj_t *obj = NULL;

    I2S_ENTER_CRITICAL();
    if (p_i2s_obj[i2s_num] != NULL &&
        p_i2s_obj[i2s_num]->drv_state != I2S_DRV_UNINSTALLING)
    {
        obj = p_i2s_obj[i2s_num];
        obj->life_refs++;
    }
    I2S_EXIT_CRITICAL();

    return obj;
}

static void i2s_obj_release(i2s_obj_t *obj)
{
    if (obj == NULL)
        return;

    I2S_ENTER_CRITICAL();
    if (obj->life_refs > 0)
        obj->life_refs--;
    I2S_EXIT_CRITICAL();
}

static bool i2s_user_begin(i2s_obj_t *obj, bool is_tx)
{
    if (xSemaphoreTake(obj->api_lock, pdMS_TO_TICKS(I2S_API_LOCK_TIMEOUT_MS)) != pdTRUE)
        return false;

    if (obj->drv_state != I2S_DRV_RUNNING)
    {
        xSemaphoreGive(obj->api_lock);
        return false;
    }

    if (is_tx)
    {
        if (obj->tx_users == 0)
            xEventGroupClearBits(obj->idle_evt, I2S_TX_IDLE_BIT);
        obj->tx_users++;
    }
    else
    {
        if (obj->rx_users == 0)
            xEventGroupClearBits(obj->idle_evt, I2S_RX_IDLE_BIT);
        obj->rx_users++;
    }

    xSemaphoreGive(obj->api_lock);
    return true;
}

static void i2s_user_end(i2s_obj_t *obj, bool is_tx)
{
    xSemaphoreTake(obj->api_lock, portMAX_DELAY);

    if (is_tx)
    {
        if (obj->tx_users > 0)
            obj->tx_users--;
        if (obj->tx_users == 0)
            xEventGroupSetBits(obj->idle_evt, I2S_TX_IDLE_BIT);
    }
    else
    {
        if (obj->rx_users > 0)
            obj->rx_users--;
        if (obj->rx_users == 0)
            xEventGroupSetBits(obj->idle_evt, I2S_RX_IDLE_BIT);
    }

    xSemaphoreGive(obj->api_lock);
}

static esp_err_t i2s_wait_dir_idle(i2s_obj_t *obj, bool is_tx)
{
    EventBits_t bit = is_tx ? I2S_TX_IDLE_BIT : I2S_RX_IDLE_BIT;

    xSemaphoreTake(obj->api_lock, portMAX_DELAY);
    int users = is_tx ? obj->tx_users : obj->rx_users;
    if (users == 0)
        xEventGroupSetBits(obj->idle_evt, bit);
    else
        xEventGroupClearBits(obj->idle_evt, bit);
    xSemaphoreGive(obj->api_lock);

    if (users == 0)
        return ESP_OK;

    EventBits_t bits = xEventGroupWaitBits(obj->idle_evt, bit, pdTRUE, pdTRUE,
                                           pdMS_TO_TICKS(I2S_DRAIN_TIMEOUT_MS));
    if ((bits & bit) == 0)
    {
        ESP_LOGE(I2S_TAG, "timeout waiting for %s users to drain",
                 is_tx ? "tx" : "rx");
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

static esp_err_t i2s_wait_tx_rx_idle(i2s_obj_t *obj)
{
    esp_err_t e;
    if (obj->mode & I2S_MODE_TX)
    {
        e = i2s_wait_dir_idle(obj, true);
        if (e != ESP_OK) return e;
    }
    if (obj->mode & I2S_MODE_RX)
    {
        e = i2s_wait_dir_idle(obj, false);
        if (e != ESP_OK) return e;
    }
    return ESP_OK;
}

/* Cyclically push NULL sentinels until the waiter count drops to zero.
 * More robust than one-shot snapshot-based sending: handles the case where
 * a task increments tx_waiters/rx_waiters but hasn't yet blocked on
 * xQueueReceive(). Returns ESP_ERR_TIMEOUT if waiters don't drain in time. */
static esp_err_t i2s_wake_dir_waiters(i2s_obj_t *obj, bool is_tx, QueueHandle_t q)
{
    volatile int *waiters = is_tx ? &obj->tx_waiters : &obj->rx_waiters;

    if (q == NULL)
    {
        return ESP_OK;
    }

    for (int t = 0; t < I2S_DRAIN_TIMEOUT_MS; t++)
    {
        int w;

        xSemaphoreTake(obj->api_lock, portMAX_DELAY);
        w = *waiters;
        xSemaphoreGive(obj->api_lock);

        if (w == 0)
        {
            return ESP_OK;
        }

        for (int i = 0; i < w; i++)
        {
            void *sentinel = NULL;
            /* FIX: Do NOT evict real DMA buffers. If the queue is full,
             * the sentinel is simply dropped. Any blocked task waking up 
             * on a real buffer will check drv_state, see it is no longer 
             * RUNNING, and safely abort. This prevents DMA pool corruption. */
            xQueueSend(q, &sentinel, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }

    return ESP_ERR_TIMEOUT;
}

/* ===========================================================================
 *  Register-level helpers
 * ======================================================================== */

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

static esp_err_t i2s_enable_rx_intr(i2s_port_t i2s_num)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK(p_i2s_obj[i2s_num], "i2s not installed yet", ESP_FAIL);

    I2S_ENTER_CRITICAL();
    p_i2s_obj[i2s_num]->dma->int_ena.tx_suc_eof = 1;
    p_i2s_obj[i2s_num]->dma->int_ena.tx_dscr_err = 1;
    I2S_EXIT_CRITICAL();

    return ESP_OK;
}

static esp_err_t i2s_disable_rx_intr(i2s_port_t i2s_num)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK(p_i2s_obj[i2s_num], "i2s not installed yet", ESP_FAIL);

    I2S_ENTER_CRITICAL();
    p_i2s_obj[i2s_num]->dma->int_ena.tx_suc_eof = 0;
    p_i2s_obj[i2s_num]->dma->int_ena.tx_dscr_err = 0;
    I2S_EXIT_CRITICAL();

    return ESP_OK;
}

static esp_err_t i2s_disable_tx_intr(i2s_port_t i2s_num)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK(p_i2s_obj[i2s_num], "i2s not installed yet", ESP_FAIL);

    I2S_ENTER_CRITICAL();
    p_i2s_obj[i2s_num]->dma->int_ena.rx_eof = 0;
    p_i2s_obj[i2s_num]->dma->int_ena.rx_dscr_err = 0;
    I2S_EXIT_CRITICAL();

    return ESP_OK;
}

static esp_err_t i2s_enable_tx_intr(i2s_port_t i2s_num)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK(p_i2s_obj[i2s_num], "i2s not installed yet", ESP_FAIL);

    I2S_ENTER_CRITICAL();
    p_i2s_obj[i2s_num]->dma->int_ena.rx_eof = 1;
    p_i2s_obj[i2s_num]->dma->int_ena.rx_dscr_err = 1;
    I2S_EXIT_CRITICAL();

    return ESP_OK;
}

/* ===========================================================================
 *  ISR
 * ======================================================================== */

static void IRAM_ATTR i2s_zero_buffer_iram(void *buf, size_t len)
{
    uint32_t *p = (uint32_t *)buf;
    size_t words = len / 4;

    for (size_t i = 0; i < words; i++)
    {
        p[i] = 0;
    }
}

static void IRAM_ATTR i2s_intr_handler_default(void *arg)
{
    i2s_obj_t *p_i2s = (i2s_obj_t *)arg;
    slc_struct_t *dma_reg = p_i2s->dma;

    i2s_event_t i2s_event = {0};
    void *discarded_buf = NULL;
    i2s_event_t discarded_event;

    BaseType_t high_priority_task_awoken = pdFALSE;
    lldesc_t *finish_desc;

    typeof(dma_reg->int_st) int_st_snap;
    int_st_snap.val = dma_reg->int_st.val;

    uint32_t rx_eof_des_addr_snap = dma_reg->rx_eof_des_addr;
    uint32_t tx_eof_des_addr_snap = dma_reg->tx_eof_des_addr;

    /* On DMA descriptor error: report event and do NOT parse EOF descriptors. */
    if (int_st_snap.tx_dscr_err || int_st_snap.rx_dscr_err)
    {
        if (p_i2s->i2s_queue)
        {
            i2s_event.type = I2S_EVENT_DMA_ERROR;
            i2s_event.size = 0;

            if (xQueueIsQueueFullFromISR(p_i2s->i2s_queue))
            {
                xQueueReceiveFromISR(p_i2s->i2s_queue, &discarded_event, &high_priority_task_awoken);
            }

            xQueueSendFromISR(p_i2s->i2s_queue, (void *)&i2s_event, &high_priority_task_awoken);
        }

        if (high_priority_task_awoken == pdTRUE)
        {
            portYIELD_FROM_ISR();
        }

        dma_reg->int_clr.val = int_st_snap.val;
        return;
    }

    /* TX done (DMA rx_eof) — return the finished buffer to the free queue. */
    if (int_st_snap.rx_eof && p_i2s->tx)
    {
        finish_desc = (lldesc_t *)rx_eof_des_addr_snap;

        if (finish_desc != NULL && finish_desc->buf_ptr != NULL)
        {
            if (xQueueIsQueueFullFromISR(p_i2s->tx->queue))
            {
                BaseType_t recv_ok = xQueueReceiveFromISR(p_i2s->tx->queue,
                                                          &discarded_buf,
                                                          &high_priority_task_awoken);

                if (recv_ok == pdPASS &&
                    p_i2s->tx_desc_auto_clear == true &&
                    discarded_buf != NULL)
                {
                    i2s_zero_buffer_iram(discarded_buf, (size_t)p_i2s->tx->buf_size);
                }
            }

            (void)xQueueSendFromISR(p_i2s->tx->queue,
                                    (void *)(&finish_desc->buf_ptr),
                                    &high_priority_task_awoken);

            if (p_i2s->i2s_queue)
            {
                i2s_event.type = I2S_EVENT_TX_DONE;
                i2s_event.size = (size_t)p_i2s->tx->buf_size;

                if (xQueueIsQueueFullFromISR(p_i2s->i2s_queue))
                {
                    (void)xQueueReceiveFromISR(p_i2s->i2s_queue,
                                               &discarded_event,
                                               &high_priority_task_awoken);
                }

                (void)xQueueSendFromISR(p_i2s->i2s_queue,
                                        (void *)&i2s_event,
                                        &high_priority_task_awoken);
            }
        }
    }

    /* RX done (DMA tx_suc_eof) — hand the filled buffer to the reader. */
    if (int_st_snap.tx_suc_eof && p_i2s->rx)
    {
        finish_desc = (lldesc_t *)tx_eof_des_addr_snap;

        if (finish_desc != NULL && finish_desc->buf_ptr != NULL)
        {
            finish_desc->owner = 1;

            if (xQueueIsQueueFullFromISR(p_i2s->rx->queue))
            {
                (void)xQueueReceiveFromISR(p_i2s->rx->queue,
                                           &discarded_buf,
                                           &high_priority_task_awoken);
            }

            (void)xQueueSendFromISR(p_i2s->rx->queue,
                                    (void *)(&finish_desc->buf_ptr),
                                    &high_priority_task_awoken);

            if (p_i2s->i2s_queue)
            {
                i2s_event.type = I2S_EVENT_RX_DONE;
                i2s_event.size = (size_t)p_i2s->rx->buf_size;

                if (xQueueIsQueueFullFromISR(p_i2s->i2s_queue))
                {
                    (void)xQueueReceiveFromISR(p_i2s->i2s_queue,
                                               &discarded_event,
                                               &high_priority_task_awoken);
                }

                (void)xQueueSendFromISR(p_i2s->i2s_queue,
                                        (void *)&i2s_event,
                                        &high_priority_task_awoken);
            }
        }
    }

    if (high_priority_task_awoken == pdTRUE)
    {
        portYIELD_FROM_ISR();
    }

    dma_reg->int_clr.val = int_st_snap.val;
}

/* ===========================================================================
 *  Task-context drain
 * ======================================================================== */

static void i2s_drain_pending_descriptors_task(i2s_port_t i2s_num, bool tx_done, bool rx_done)
{
    i2s_obj_t *obj = p_i2s_obj[i2s_num];

    if (obj == NULL)
        return;

    /* TX side: DMA rx_eof means a TX buffer finished transmitting. */
    if (tx_done && (obj->mode & I2S_MODE_TX) && obj->tx)
    {
        lldesc_t *finish_desc = (lldesc_t *)obj->dma->rx_eof_des_addr;

        if (finish_desc != NULL && finish_desc->buf_ptr != NULL)
        {
            QueueHandle_t queue = obj->tx->queue;

            if (uxQueueSpacesAvailable(queue) == 0)
            {
                void *discarded_buf = NULL;
                BaseType_t recv_ok = xQueueReceive(queue, &discarded_buf, 0);

                if (recv_ok == pdPASS &&
                    obj->tx_desc_auto_clear &&
                    discarded_buf != NULL)
                {
                    memset(discarded_buf, 0, obj->tx->buf_size);
                }
            }

            xQueueSend(queue, &finish_desc->buf_ptr, 0);
        }
    }

    /* RX side: DMA tx_suc_eof means an RX buffer finished receiving. */
    if (rx_done && (obj->mode & I2S_MODE_RX) && obj->rx)
    {
        lldesc_t *finish_desc = (lldesc_t *)obj->dma->tx_eof_des_addr;

        if (finish_desc != NULL && finish_desc->buf_ptr != NULL)
        {
            finish_desc->owner = 1;

            QueueHandle_t queue = obj->rx->queue;

            if (uxQueueSpacesAvailable(queue) == 0)
            {
                void *discarded_buf = NULL;
                (void)xQueueReceive(queue, &discarded_buf, 0);
            }

            xQueueSend(queue, &finish_desc->buf_ptr, 0);
        }
    }
}

/* ===========================================================================
 *  DMA queue create / destroy
 * ======================================================================== */

static esp_err_t i2s_destroy_dma_queue(i2s_dma_t *dma)
{
    int bux_idx;

    if (dma == NULL)
    {
        ESP_LOGE(I2S_TAG, "dma is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    for (bux_idx = 0; bux_idx < dma->buf_count; bux_idx++)
    {
        if (dma->desc && dma->desc[bux_idx])
        {
            heap_caps_free(dma->desc[bux_idx]);
        }

        if (dma->buf && dma->buf[bux_idx])
        {
            heap_caps_free(dma->buf[bux_idx]);
        }
    }

    if (dma->buf)
    {
        heap_caps_free(dma->buf);
    }

    if (dma->desc)
    {
        heap_caps_free(dma->desc);
    }

    if (dma->queue)
    {
        vQueueDelete(dma->queue);
    }

    if (dma->mux)
    {
        vSemaphoreDelete(dma->mux);
    }

    heap_caps_free(dma);

    return ESP_OK;
}

/* Decoupled creator: builds a complete DMA queue given explicit geometry.
 * Does not touch p_i2s_obj, so it is safe to call before switching state.
 */
static i2s_dma_t *i2s_dma_queue_create(int dma_buf_count,
                                       int dma_buf_len,
                                       int sample_size)
{
    int bux_idx;
    int buf_bytes = dma_buf_len * sample_size;

    if (sample_size <= 0 ||
        dma_buf_count < 2 ||
        dma_buf_len <= 0 ||
        buf_bytes <= 0 ||
        buf_bytes > 4095)
    {
        ESP_LOGE(I2S_TAG,
                 "Invalid DMA queue geometry: sample_size=%d, dma_buf_count=%d, dma_buf_len=%d, buf_bytes=%d",
                 sample_size,
                 dma_buf_count,
                 dma_buf_len,
                 buf_bytes);
        return NULL;
    }

    i2s_dma_t *dma = (i2s_dma_t *)heap_caps_zalloc(sizeof(i2s_dma_t), MALLOC_CAP_8BIT);
    if (dma == NULL)
    {
        ESP_LOGE(I2S_TAG, "Error malloc i2s_dma_t");
        return NULL;
    }

    dma->buf = (char **)heap_caps_zalloc(sizeof(char *) * dma_buf_count, MALLOC_CAP_8BIT);
    if (dma->buf == NULL)
    {
        ESP_LOGE(I2S_TAG, "Error malloc dma buffer pointer");
        heap_caps_free(dma);
        return NULL;
    }

    for (bux_idx = 0; bux_idx < dma_buf_count; bux_idx++)
    {
        dma->buf[bux_idx] = (char *)heap_caps_calloc(1, buf_bytes, MALLOC_CAP_8BIT);

        if (dma->buf[bux_idx] == NULL)
        {
            ESP_LOGE(I2S_TAG, "Error malloc dma buffer");

            for (int j = 0; j < bux_idx; j++)
            {
                if (dma->buf[j])
                    heap_caps_free(dma->buf[j]);
            }

            heap_caps_free(dma->buf);
            heap_caps_free(dma);
            return NULL;
        }

        if (((uintptr_t)dma->buf[bux_idx] & 3) != 0)
        {
            ESP_LOGE(I2S_TAG, "DMA buffer %d is not 4-byte aligned", bux_idx);

            for (int j = 0; j <= bux_idx; j++)
            {
                if (dma->buf[j])
                    heap_caps_free(dma->buf[j]);
            }

            heap_caps_free(dma->buf);
            heap_caps_free(dma);
            return NULL;
        }

        ESP_LOGD(I2S_TAG, "Addr[%d] = %p", bux_idx, dma->buf[bux_idx]);
    }

    dma->desc = (lldesc_t **)heap_caps_zalloc(sizeof(lldesc_t *) * dma_buf_count, MALLOC_CAP_8BIT);
    if (dma->desc == NULL)
    {
        ESP_LOGE(I2S_TAG, "Error malloc dma description");

        for (int j = 0; j < dma_buf_count; j++)
        {
            if (dma->buf[j])
                heap_caps_free(dma->buf[j]);
        }

        heap_caps_free(dma->buf);
        heap_caps_free(dma);
        return NULL;
    }

    for (bux_idx = 0; bux_idx < dma_buf_count; bux_idx++)
    {
        dma->desc[bux_idx] = (lldesc_t *)heap_caps_malloc(sizeof(lldesc_t), MALLOC_CAP_8BIT);

        if (dma->desc[bux_idx] == NULL)
        {
            ESP_LOGE(I2S_TAG, "Error malloc dma description entry");

            for (int j = 0; j < bux_idx; j++)
            {
                if (dma->desc[j])
                    heap_caps_free(dma->desc[j]);
            }

            heap_caps_free(dma->desc);

            for (int j = 0; j < dma_buf_count; j++)
            {
                if (dma->buf[j])
                    heap_caps_free(dma->buf[j]);
            }

            heap_caps_free(dma->buf);
            heap_caps_free(dma);
            return NULL;
        }

        if (((uintptr_t)dma->desc[bux_idx] & 3) != 0)
        {
            ESP_LOGE(I2S_TAG, "DMA descriptor %d is not 4-byte aligned", bux_idx);

            for (int j = 0; j <= bux_idx; j++)
            {
                if (dma->desc[j])
                    heap_caps_free(dma->desc[j]);
            }

            heap_caps_free(dma->desc);

            for (int j = 0; j < dma_buf_count; j++)
            {
                if (dma->buf[j])
                    heap_caps_free(dma->buf[j]);
            }

            heap_caps_free(dma->buf);
            heap_caps_free(dma);
            return NULL;
        }
    }

    for (bux_idx = 0; bux_idx < dma_buf_count; bux_idx++)
    {
        dma->desc[bux_idx]->owner = 1;
        dma->desc[bux_idx]->eof = 1;
        dma->desc[bux_idx]->sub_sof = 0;
        dma->desc[bux_idx]->datalen = buf_bytes;
        dma->desc[bux_idx]->blocksize = buf_bytes;
        dma->desc[bux_idx]->buf_ptr = (uint32_t *)dma->buf[bux_idx];
        dma->desc[bux_idx]->unused = 0;
        dma->desc[bux_idx]->next_link_ptr =
            (lldesc_t *)((bux_idx < (dma_buf_count - 1)) ? (dma->desc[bux_idx + 1]) : dma->desc[0]);
    }

    dma->queue = xQueueCreate(dma_buf_count - 1, sizeof(char *));
    dma->mux = xSemaphoreCreateMutex();

    if (dma->queue == NULL || dma->mux == NULL)
    {
        ESP_LOGE(I2S_TAG, "Error creating dma queue/mutex");

        if (dma->queue)
            vQueueDelete(dma->queue);

        if (dma->mux)
            vSemaphoreDelete(dma->mux);

        for (int j = 0; j < dma_buf_count; j++)
        {
            if (dma->desc[j])
                heap_caps_free(dma->desc[j]);

            if (dma->buf[j])
                heap_caps_free(dma->buf[j]);
        }

        heap_caps_free(dma->desc);
        heap_caps_free(dma->buf);
        heap_caps_free(dma);

        return NULL;
    }

    dma->rw_pos = 0;
    dma->buf_size = buf_bytes;
    dma->buf_count = dma_buf_count;
    dma->curr_ptr = NULL;

    ESP_LOGI(I2S_TAG,
             "DMA Malloc info, datalen=blocksize=%d, dma_buf_count=%d",
             buf_bytes,
             dma_buf_count);

    return dma;
}

/* ===========================================================================
 *  start / stop internal
 * ======================================================================== */

static esp_err_t i2s_start_internal(i2s_port_t i2s_num)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK(p_i2s_obj[i2s_num], "i2s not installed yet", ESP_FAIL);

    i2s_obj_t *obj = p_i2s_obj[i2s_num];

    /* Cold-start: reset queues + read/write cursors under data mutex. */
    if (obj->tx)
    {
        xSemaphoreTake(obj->tx->mux, portMAX_DELAY);

        if (obj->tx->queue)
            xQueueReset(obj->tx->queue);

        obj->tx->curr_ptr = NULL;
        obj->tx->rw_pos = 0;

        xSemaphoreGive(obj->tx->mux);
    }

    if (obj->rx)
    {
        xSemaphoreTake(obj->rx->mux, portMAX_DELAY);

        if (obj->rx->queue)
            xQueueReset(obj->rx->queue);

        obj->rx->curr_ptr = NULL;
        obj->rx->rw_pos = 0;

        xSemaphoreGive(obj->rx->mux);
    }

    I2S_ENTER_CRITICAL();

    i2s_reset_fifo(i2s_num);

    obj->dma->conf0.rx_rst = 1;
    obj->dma->conf0.rx_rst = 0;
    obj->dma->conf0.tx_rst = 1;
    obj->dma->conf0.tx_rst = 0;
    I2S_MEMW();

    I2S[i2s_num]->conf.tx_reset = 1;
    I2S[i2s_num]->conf.tx_reset = 0;
    I2S[i2s_num]->conf.rx_reset = 1;
    I2S[i2s_num]->conf.rx_reset = 0;
    I2S_MEMW();

    dma_intr_disable();

    obj->dma->int_clr.val = 0xFFFFFFFF;
    I2S_MEMW();

    if (obj->tx)
    {
        for (int i = 0; i < obj->dma_buf_count; i++)
        {
            if (obj->tx->desc && obj->tx->desc[i])
                obj->tx->desc[i]->owner = 1;
        }
    }

    if (obj->rx)
    {
        for (int i = 0; i < obj->dma_buf_count; i++)
        {
            if (obj->rx->desc && obj->rx->desc[i])
                obj->rx->desc[i]->owner = 1;
        }
    }

    if ((obj->mode & I2S_MODE_TX) && obj->tx)
    {
        i2s_enable_tx_intr(i2s_num);
        obj->dma->rx_link.start = 1;
    }

    if ((obj->mode & I2S_MODE_RX) && obj->rx)
    {
        i2s_enable_rx_intr(i2s_num);
        obj->dma->tx_link.start = 1;
    }

    I2S_MEMW();

    /* ESP8266 I2S uses the same clock domain for TX and RX.
     * Keep both I2S units started together; DMA direction is controlled
     * independently through the SLC TX/RX links above. */
    I2S[i2s_num]->conf.val |= I2S_I2S_TX_START | I2S_I2S_RX_START;
    I2S_MEMW();

    I2S[i2s_num]->conf.val |= I2S_I2S_RESET_MASK;
    I2S_MEMW();

    I2S[i2s_num]->conf.val &= ~I2S_I2S_RESET_MASK;
    I2S_MEMW();

    /* Clear any stale DMA interrupt flags before enabling the interrupt. */
    obj->dma->int_clr.val = 0xFFFFFFFF;
    I2S_MEMW();

    dma_intr_enable();

    I2S_EXIT_CRITICAL();

    return ESP_OK;
}

static esp_err_t i2s_stop_internal(i2s_port_t i2s_num)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK(p_i2s_obj[i2s_num], "i2s not installed yet", ESP_FAIL);

    i2s_obj_t *obj = p_i2s_obj[i2s_num];

    uint32_t int_st_snap_val;
    bool tx_done = false; /* DMA rx_eof: TX buffer finished */
    bool rx_done = false; /* DMA tx_suc_eof: RX buffer finished */

    I2S_ENTER_CRITICAL();

    dma_intr_disable();

    if (obj->mode & I2S_MODE_TX)
    {
        obj->dma->rx_link.stop = 1;
    }

    if (obj->mode & I2S_MODE_RX)
    {
        obj->dma->tx_link.stop = 1;
    }

    I2S[i2s_num]->conf.val &= ~(I2S_I2S_TX_START | I2S_I2S_RX_START);
    I2S_MEMW();

    /* Snapshot interrupt status BEFORE disabling int_ena.
     * Use the snapshot for ALL subsequent checks — do NOT re-read the register.
     * Per slc_struct.h: rx_eof = bit 17, tx_suc_eof = bit 15. */
    int_st_snap_val = obj->dma->int_st.val;
    typeof(obj->dma->int_st) int_st_snap;
    int_st_snap.val = int_st_snap_val;
    tx_done = int_st_snap.rx_eof ? true : false;
    rx_done = int_st_snap.tx_suc_eof ? true : false;

    /* Disable interrupt sources. */
    if (obj->mode & I2S_MODE_TX)
    {
        i2s_disable_tx_intr(i2s_num);
    }

    if (obj->mode & I2S_MODE_RX)
    {
        i2s_disable_rx_intr(i2s_num);
    }

    I2S_EXIT_CRITICAL();

    /* Task-context drain — regular queue API, not FromISR. */
    i2s_drain_pending_descriptors_task(i2s_num, tx_done, rx_done);

    I2S_ENTER_CRITICAL();
    obj->dma->int_clr.val = int_st_snap_val;
    I2S_MEMW();
    I2S_EXIT_CRITICAL();

    /* Reset read/write cursors under data mutex. */
    if (obj->tx)
    {
        xSemaphoreTake(obj->tx->mux, portMAX_DELAY);
        obj->tx->curr_ptr = NULL;
        obj->tx->rw_pos = 0;
        xSemaphoreGive(obj->tx->mux);
    }

    if (obj->rx)
    {
        xSemaphoreTake(obj->rx->mux, portMAX_DELAY);
        obj->rx->curr_ptr = NULL;
        obj->rx->rw_pos = 0;
        xSemaphoreGive(obj->rx->mux);
    }

    return ESP_OK;
}

/* ===========================================================================
 *  Public start / stop wrappers
 * ======================================================================== */

esp_err_t i2s_start(i2s_port_t i2s_num)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    i2s_obj_t *obj = i2s_obj_acquire(i2s_num);
    I2S_CHECK(obj, "i2s not installed or being uninstalled", ESP_ERR_INVALID_STATE);

    if (xSemaphoreTake(obj->api_lock, pdMS_TO_TICKS(I2S_API_LOCK_TIMEOUT_MS)) != pdTRUE)
    {
        ESP_LOGE(I2S_TAG, "i2s_start: api_lock busy");
        i2s_obj_release(obj);
        return ESP_ERR_TIMEOUT;
    }

    if (obj->drv_state != I2S_DRV_STOPPED)
    {
        xSemaphoreGive(obj->api_lock);
        i2s_obj_release(obj);
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = i2s_start_internal(i2s_num);

    if (ret == ESP_OK)
        obj->drv_state = I2S_DRV_RUNNING;
    else
        obj->drv_state = I2S_DRV_STOPPED;

    xSemaphoreGive(obj->api_lock);
    i2s_obj_release(obj);

    return ret;
}

esp_err_t i2s_stop(i2s_port_t i2s_num)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    i2s_obj_t *obj = i2s_obj_acquire(i2s_num);
    I2S_CHECK(obj, "i2s not installed or being uninstalled", ESP_ERR_INVALID_STATE);

    if (xSemaphoreTake(obj->api_lock, pdMS_TO_TICKS(I2S_API_LOCK_TIMEOUT_MS)) != pdTRUE)
    {
        ESP_LOGE(I2S_TAG, "i2s_stop: api_lock busy");
        i2s_obj_release(obj);
        return ESP_ERR_TIMEOUT;
    }

    if (obj->drv_state != I2S_DRV_RUNNING)
    {
        xSemaphoreGive(obj->api_lock);
        /* Already stopped / not running — not an error. */
        i2s_obj_release(obj);
        return ESP_OK;
    }

    obj->drv_state = I2S_DRV_STOPPING;

    QueueHandle_t tq = obj->tx ? obj->tx->queue : NULL;
    QueueHandle_t rq = obj->rx ? obj->rx->queue : NULL;

    xSemaphoreGive(obj->api_lock);

    /* Wake blocked waiters OUTSIDE api_lock. */
    if (obj->mode & I2S_MODE_TX)
    {
        i2s_wake_dir_waiters(obj, true, tq);
    }

    if (obj->mode & I2S_MODE_RX)
    {
        i2s_wake_dir_waiters(obj, false, rq);
    }

    esp_err_t drain_err = i2s_wait_tx_rx_idle(obj);
    if (drain_err != ESP_OK)
    {
        ESP_LOGE(I2S_TAG, "i2s_stop: drain timeout, aborting stop to prevent corruption");
        xSemaphoreTake(obj->api_lock, portMAX_DELAY);
        obj->drv_state = I2S_DRV_RUNNING;
        xSemaphoreGive(obj->api_lock);
        i2s_obj_release(obj);
        return drain_err;
    }
    xSemaphoreTake(obj->api_lock, portMAX_DELAY);
    esp_err_t ret = i2s_stop_internal(i2s_num);
    obj->drv_state = I2S_DRV_STOPPED;
    xSemaphoreGive(obj->api_lock);
    i2s_obj_release(obj);
    return ret;
}

/* ===========================================================================
 *  Pin config
 * ======================================================================== */

esp_err_t i2s_set_pin(i2s_port_t i2s_num, const i2s_pin_config_t *pin)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK(pin, "param null", ESP_ERR_INVALID_ARG);

    if (pin->bck_o_en > 0)
    {
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_I2SO_BCK);
    }

    if (pin->ws_o_en > 0)
    {
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_I2SO_WS);
    }

    if (pin->data_out_en > 0)
    {
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_I2SO_DATA);
    }

    if (pin->bck_i_en > 0)
    {
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_I2SI_BCK);
    }

    if (pin->ws_i_en > 0)
    {
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_I2SI_WS);
    }

    if (pin->data_in_en > 0)
    {
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_I2SI_DATA);
    }

    return ESP_OK;
}

/* ===========================================================================
 *  Clock divider calculation
 * ======================================================================== */

static esp_err_t i2s_set_rate(i2s_port_t i2s_num, uint32_t rate, int bits_per_sample)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK(p_i2s_obj[i2s_num], "i2s not installed yet", ESP_FAIL);
    I2S_CHECK((rate > 0), "rate must be > 0", ESP_ERR_INVALID_ARG);

    uint8_t bck_div = 1;
    uint8_t mclk_div = 1;

    float scaled_base_freq =
        (float)I2S_BASE_CLK / (bits_per_sample == I2S_BITS_PER_SAMPLE_16BIT ? 32.0f : 48.0f);

    float delta_best = FLT_MAX;

    for (uint8_t i = 1; i < 64; i++)
    {
        for (uint8_t j = i; j < 64; j++)
        {
            float new_delta = fabsf((scaled_base_freq / i / j) - (float)rate);

            if (new_delta < delta_best)
            {
                delta_best = new_delta;
                bck_div = i;
                mclk_div = j;

                if (new_delta == 0.0f)
                    goto done;
            }
        }
    }

done:
    I2S_ENTER_CRITICAL();
    I2S[i2s_num]->conf.bck_div_num = bck_div & 0x3F;
    I2S[i2s_num]->conf.clkm_div_num = mclk_div & 0x3F;
    I2S_MEMW();
    I2S_EXIT_CRITICAL();

    p_i2s_obj[i2s_num]->sample_rate = rate;

    return ESP_OK;
}

/* ===========================================================================
 *  i2s_set_clk
 * ======================================================================== */

esp_err_t i2s_set_clk(i2s_port_t i2s_num,
                      uint32_t rate,
                      i2s_bits_per_sample_t bits,
                      i2s_channel_t ch)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK((ch == 1 || ch == 2), "channel must be 1 (mono) or 2 (stereo)", ESP_ERR_INVALID_ARG);

    if (bits % 8 != 0 ||
        bits > I2S_BITS_PER_SAMPLE_24BIT ||
        bits < I2S_BITS_PER_SAMPLE_16BIT)
    {
        ESP_LOGE(I2S_TAG, "Invalid bits per sample");
        return ESP_ERR_INVALID_ARG;
    }

    I2S_CHECK((rate > 0), "rate must be > 0", ESP_ERR_INVALID_ARG);

    i2s_obj_t *obj = i2s_obj_acquire(i2s_num);
    I2S_CHECK(obj, "i2s not installed or being uninstalled", ESP_ERR_INVALID_STATE);

    if (xSemaphoreTake(obj->api_lock, pdMS_TO_TICKS(I2S_API_LOCK_TIMEOUT_MS)) != pdTRUE)
    {
        ESP_LOGE(I2S_TAG, "i2s_set_clk: api_lock busy");
        i2s_obj_release(obj);
        return ESP_ERR_TIMEOUT;
    }

    if (obj->drv_state != I2S_DRV_RUNNING && obj->drv_state != I2S_DRV_STOPPED)
    {
        xSemaphoreGive(obj->api_lock);
        i2s_obj_release(obj);
        return ESP_ERR_INVALID_STATE;
    }

    i2s_drv_state_t prev_state = obj->drv_state;

    /* Compute new geometry without side effects. */
    int new_bytes_per_sample = (bits == I2S_BITS_PER_SAMPLE_16BIT) ? 2 : 4;
    int new_channel_num = (ch == 2) ? 2 : 1;
    int new_sample_size = new_bytes_per_sample * new_channel_num;

    int new_dma_buf_len = obj->dma_buf_len_orig;

    if (new_dma_buf_len * new_bytes_per_sample * new_channel_num > 4092)
    {
        new_dma_buf_len = 4092 / new_bytes_per_sample / new_channel_num;
    }

    int buf_size_chk = new_dma_buf_len * new_sample_size;

    while ((buf_size_chk % 4) != 0 && new_dma_buf_len > 1)
    {
        new_dma_buf_len--;
        buf_size_chk -= new_sample_size;
    }

    if ((buf_size_chk % 4) != 0 || buf_size_chk <= 0)
    {
        ESP_LOGE(I2S_TAG,
                 "DMA buffer size (%d bytes) must be a positive multiple of 4",
                 buf_size_chk);
        xSemaphoreGive(obj->api_lock);
        i2s_obj_release(obj);
        return ESP_ERR_INVALID_ARG;
    }

    bool need_rebuild = (bits != obj->bits_per_sample) || (obj->channel_num != ch);

    /* Allocate new DMA queues BEFORE stopping. OOM leaves old state intact. */
    i2s_dma_t *new_tx = NULL;
    i2s_dma_t *new_rx = NULL;

    if (need_rebuild)
    {
        if (obj->mode & I2S_MODE_TX)
        {
            new_tx = i2s_dma_queue_create(obj->dma_buf_count,
                                          new_dma_buf_len,
                                          new_sample_size);

            if (new_tx == NULL)
            {
                ESP_LOGE(I2S_TAG,
                         "Failed to allocate new tx dma queue (OOM) — old state preserved");
                xSemaphoreGive(obj->api_lock);
                i2s_obj_release(obj);
                return ESP_ERR_NO_MEM;
            }
        }

        if (obj->mode & I2S_MODE_RX)
        {
            new_rx = i2s_dma_queue_create(obj->dma_buf_count,
                                          new_dma_buf_len,
                                          new_sample_size);

            if (new_rx == NULL)
            {
                ESP_LOGE(I2S_TAG,
                         "Failed to allocate new rx dma queue (OOM) — old state preserved");

                if (new_tx)
                    i2s_destroy_dma_queue(new_tx);

                xSemaphoreGive(obj->api_lock);
                i2s_obj_release(obj);
                return ESP_ERR_NO_MEM;
            }
        }
    }

    /* Enter RECONFIGURING: block new write/read, wake blocked waiters. */
    obj->drv_state = I2S_DRV_RECONFIGURING;

    QueueHandle_t tq = obj->tx ? obj->tx->queue : NULL;
    QueueHandle_t rq = obj->rx ? obj->rx->queue : NULL;

    xSemaphoreGive(obj->api_lock);

    if (obj->mode & I2S_MODE_TX)
    {
        i2s_wake_dir_waiters(obj, true, tq);
    }

    if (obj->mode & I2S_MODE_RX)
    {
        i2s_wake_dir_waiters(obj, false, rq);
    }

    esp_err_t drain_err = i2s_wait_tx_rx_idle(obj);
    if (drain_err != ESP_OK)
    {
        ESP_LOGE(I2S_TAG, "i2s_set_clk: drain timeout, aborting reconfiguration");
        if (new_tx)
            i2s_destroy_dma_queue(new_tx);
        if (new_rx)
            i2s_destroy_dma_queue(new_rx);
        xSemaphoreTake(obj->api_lock, portMAX_DELAY);
        obj->drv_state = prev_state;
        xSemaphoreGive(obj->api_lock);
        i2s_obj_release(obj);
        return drain_err;
    }
    /* Stop engine. */
    i2s_stop_internal(i2s_num);

    /* Reset old queues before changing registers. */
    if (obj->tx)
    {
        xSemaphoreTake(obj->tx->mux, portMAX_DELAY);
        if (obj->tx->queue)
            xQueueReset(obj->tx->queue);
        obj->tx->curr_ptr = NULL;
        obj->tx->rw_pos = 0;
        xSemaphoreGive(obj->tx->mux);
    }

    if (obj->rx)
    {
        xSemaphoreTake(obj->rx->mux, portMAX_DELAY);
        if (obj->rx->queue)
            xQueueReset(obj->rx->queue);
        obj->rx->curr_ptr = NULL;
        obj->rx->rw_pos = 0;
        xSemaphoreGive(obj->rx->mux);
    }

    /* Set clock divider. */
    esp_err_t rate_err = i2s_set_rate(i2s_num, rate, bits);
    if (rate_err != ESP_OK)
    {
        if (new_tx)
            i2s_destroy_dma_queue(new_tx);
        if (new_rx)
            i2s_destroy_dma_queue(new_rx);

        if (prev_state == I2S_DRV_RUNNING)
            i2s_start_internal(i2s_num);

        xSemaphoreTake(obj->api_lock, portMAX_DELAY);
        obj->drv_state = prev_state;
        xSemaphoreGive(obj->api_lock);

        i2s_obj_release(obj);
        return rate_err;
    }

    /* Apply channel register changes. */
    if (obj->channel_num != ch)
    {
        obj->channel_num = new_channel_num;

        i2s_channel_fmt_t cf = obj->channel_format;

        if (ch == 2 && cf >= I2S_CHANNEL_FMT_ONLY_RIGHT)
        {
            cf = (cf == I2S_CHANNEL_FMT_ONLY_LEFT) ? I2S_CHANNEL_FMT_ALL_LEFT
                                                   : I2S_CHANNEL_FMT_ALL_RIGHT;
            obj->channel_format = cf;
        }
        else if (ch == 1 && cf < I2S_CHANNEL_FMT_ONLY_RIGHT)
        {
            cf = (cf == I2S_CHANNEL_FMT_ALL_LEFT) ? I2S_CHANNEL_FMT_ONLY_LEFT
                                                  : I2S_CHANNEL_FMT_ONLY_RIGHT;
            obj->channel_format = cf;
        }

        uint32_t tx_fifo_base = I2S[i2s_num]->fifo_conf.tx_fifo_mod;
        uint32_t rx_fifo_base = I2S[i2s_num]->fifo_conf.rx_fifo_mod;

        uint32_t fifo_single_bit;

        if (cf < I2S_CHANNEL_FMT_ONLY_RIGHT)
        {
            fifo_single_bit = (ch == 2) ? 0u : 1u;
        }
        else
        {
            fifo_single_bit = 1u;
        }

        tx_fifo_base = (tx_fifo_base & ~(uint32_t)1) | fifo_single_bit;
        rx_fifo_base = (rx_fifo_base & ~(uint32_t)1) | fifo_single_bit;

        if (tx_fifo_base > 5)
            tx_fifo_base = 5;

        if (rx_fifo_base > 5)
            rx_fifo_base = 5;

        I2S[i2s_num]->fifo_conf.tx_fifo_mod = tx_fifo_base;
        I2S[i2s_num]->fifo_conf.rx_fifo_mod = rx_fifo_base;

        uint32_t tx_chan_mod;
        uint32_t rx_chan_mod;

        if (ch == 2)
        {
            tx_chan_mod = (uint32_t)cf;
            rx_chan_mod = (cf < I2S_CHANNEL_FMT_ONLY_RIGHT)
                              ? (uint32_t)cf
                              : (uint32_t)(cf >> 1);
        }
        else
        {
            uint32_t mono_mod =
                (cf == I2S_CHANNEL_FMT_ALL_LEFT || cf == I2S_CHANNEL_FMT_ONLY_LEFT) ? 2u : 1u;

            tx_chan_mod = mono_mod;
            rx_chan_mod = mono_mod;
        }

        I2S[i2s_num]->conf_chan.tx_chan_mod = tx_chan_mod;
        I2S[i2s_num]->conf_chan.rx_chan_mod = rx_chan_mod;
        I2S_MEMW();
    }

    /* Swap DMA queues if rebuilt. */
    if (need_rebuild)
    {
        i2s_dma_t *old_tx = NULL;
        i2s_dma_t *old_rx = NULL;

        if (bits != obj->bits_per_sample)
        {
            bool is_24 = (bits > 16);
            bool was_24 = (obj->bits_per_sample > 16);

            uint32_t tx_cur = I2S[i2s_num]->fifo_conf.tx_fifo_mod;
            uint32_t rx_cur = I2S[i2s_num]->fifo_conf.rx_fifo_mod;

            uint32_t tx_new, rx_new;

            if (is_24 && !was_24)
            {
                tx_new = tx_cur + 2;
                rx_new = rx_cur + 2;
            }
            else if (!is_24 && was_24)
            {
                tx_new = tx_cur - 2;
                rx_new = rx_cur - 2;
            }
            else
            {
                tx_new = tx_cur;
                rx_new = rx_cur;
            }

            if (tx_new > 5)
                tx_new = 5;

            if (rx_new > 5)
                rx_new = 5;

            I2S[i2s_num]->fifo_conf.tx_fifo_mod = tx_new;
            I2S[i2s_num]->fifo_conf.rx_fifo_mod = rx_new;
            I2S_MEMW();

            obj->bits_per_sample = bits;
            obj->bytes_per_sample = new_bytes_per_sample;
        }

        obj->dma_buf_len = new_dma_buf_len;

        if (new_tx)
        {
            old_tx = obj->tx;
            obj->tx = new_tx;
            obj->dma->rx_link.addr = (uint32_t)obj->tx->desc[0];
        }

        if (new_rx)
        {
            old_rx = obj->rx;
            obj->rx = new_rx;

            I2S[i2s_num]->rx_eof_num =
                (obj->dma_buf_len * obj->channel_num * obj->bytes_per_sample) / 4;

            obj->dma->tx_link.addr = (uint32_t)obj->rx->desc[0];
        }

        if (old_tx)
            i2s_destroy_dma_queue(old_tx);
        if (old_rx)
            i2s_destroy_dma_queue(old_rx);
    }

    I2S[i2s_num]->conf.bits_mod = (bits == I2S_BITS_PER_SAMPLE_16BIT) ? 0 : 8;

    /* Restart only if we were running before. */
    esp_err_t start_err = ESP_OK;

    if (prev_state == I2S_DRV_RUNNING)
    {
        start_err = i2s_start_internal(i2s_num);
    }

    xSemaphoreTake(obj->api_lock, portMAX_DELAY);

    if (prev_state == I2S_DRV_RUNNING && start_err == ESP_OK)
        obj->drv_state = I2S_DRV_RUNNING;
    else
        obj->drv_state = I2S_DRV_STOPPED;

    xSemaphoreGive(obj->api_lock);
    i2s_obj_release(obj);

    return start_err;
}

esp_err_t i2s_set_sample_rates(i2s_port_t i2s_num, uint32_t rate)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);

    i2s_obj_t *obj = i2s_obj_acquire(i2s_num);
    I2S_CHECK(obj, "i2s not installed or being uninstalled", ESP_ERR_INVALID_STATE);
    if (obj->bytes_per_sample <= 0)
    {
        i2s_obj_release(obj);
        ESP_LOGE(I2S_TAG, "bits_per_sample not set");
        return ESP_ERR_INVALID_ARG;
    }

    i2s_bits_per_sample_t bits = (i2s_bits_per_sample_t)obj->bits_per_sample;
    i2s_channel_t ch = (i2s_channel_t)obj->channel_num;
    i2s_obj_release(obj);

    return i2s_set_clk(i2s_num, rate, bits, ch);
}

/* ===========================================================================
 *  param_config
 * ======================================================================== */

static esp_err_t i2s_param_config(i2s_port_t i2s_num, const i2s_config_t *i2s_config)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK(p_i2s_obj[i2s_num], "i2s not installed yet", ESP_FAIL);
    I2S_CHECK((i2s_config), "param null", ESP_ERR_INVALID_ARG);

    i2s_reset_fifo(i2s_num);
    I2S_MEMW();

    I2S[i2s_num]->conf.tx_reset = 1;
    I2S[i2s_num]->conf.tx_reset = 0;
    I2S[i2s_num]->conf.rx_reset = 1;
    I2S[i2s_num]->conf.rx_reset = 0;
    I2S_MEMW();

    I2S[i2s_num]->int_ena.val = 0;

    p_i2s_obj[i2s_num]->dma->conf0.rx_rst = 1;
    p_i2s_obj[i2s_num]->dma->conf0.rx_rst = 0;
    p_i2s_obj[i2s_num]->dma->conf0.tx_rst = 1;
    p_i2s_obj[i2s_num]->dma->conf0.tx_rst = 0;
    I2S_MEMW();

    p_i2s_obj[i2s_num]->dma->conf0.txdata_burst_en = 0;
    p_i2s_obj[i2s_num]->dma->conf0.txdscr_burst_en = 1;

    p_i2s_obj[i2s_num]->dma->rx_dscr_conf.rx_fill_mode = 0;
    p_i2s_obj[i2s_num]->dma->rx_dscr_conf.rx_eof_mode = 0;
    p_i2s_obj[i2s_num]->dma->rx_dscr_conf.rx_fill_en = 0;
    p_i2s_obj[i2s_num]->dma->rx_dscr_conf.token_no_replace = 1;
    p_i2s_obj[i2s_num]->dma->rx_dscr_conf.infor_no_replace = 1;
    I2S_MEMW();

    I2S[i2s_num]->fifo_conf.dscr_en = 0;
    I2S_MEMW();

    uint32_t tx_chan_mod_init =
        (i2s_config->channel_format < I2S_CHANNEL_FMT_ONLY_RIGHT)
            ? (uint32_t)i2s_config->channel_format
            : (uint32_t)(i2s_config->channel_format >> 1);

    uint32_t rx_chan_mod_init =
        (i2s_config->channel_format < I2S_CHANNEL_FMT_ONLY_RIGHT)
            ? (uint32_t)i2s_config->channel_format
            : (uint32_t)(i2s_config->channel_format >> 1);

    I2S[i2s_num]->conf_chan.tx_chan_mod = tx_chan_mod_init;
    I2S[i2s_num]->fifo_conf.tx_fifo_mod =
        (i2s_config->channel_format < I2S_CHANNEL_FMT_ONLY_RIGHT) ? 0 : 1;

    I2S[i2s_num]->conf_chan.rx_chan_mod = rx_chan_mod_init;
    I2S[i2s_num]->fifo_conf.rx_fifo_mod =
        (i2s_config->channel_format < I2S_CHANNEL_FMT_ONLY_RIGHT) ? 0 : 1;

    I2S_MEMW();

    I2S[i2s_num]->fifo_conf.dscr_en = 1;
    I2S_MEMW();

    I2S[i2s_num]->conf.tx_start = 0;
    I2S[i2s_num]->conf.rx_start = 0;
    I2S[i2s_num]->conf.msb_right = 1;
    I2S[i2s_num]->conf.right_first = 1;

    if (i2s_config->mode & I2S_MODE_TX)
    {
        I2S[i2s_num]->conf.tx_slave_mod = 0;

        if (i2s_config->mode & I2S_MODE_SLAVE)
        {
            I2S[i2s_num]->conf.tx_slave_mod = 1;
        }
    }

    if (i2s_config->mode & I2S_MODE_RX)
    {
        I2S[i2s_num]->conf.rx_slave_mod = 0;

        if (i2s_config->mode & I2S_MODE_SLAVE)
        {
            I2S[i2s_num]->conf.rx_slave_mod = 1;
        }
    }

    if (i2s_config->communication_format & I2S_COMM_FORMAT_I2S)
    {
        I2S[i2s_num]->conf.tx_msb_shift = 1;
        I2S[i2s_num]->conf.rx_msb_shift = 1;

        if (i2s_config->communication_format & I2S_COMM_FORMAT_I2S_LSB)
        {
            if (i2s_config->mode & I2S_MODE_TX)
            {
                I2S[i2s_num]->conf.tx_msb_shift = 0;
            }

            if (i2s_config->mode & I2S_MODE_RX)
            {
                I2S[i2s_num]->conf.rx_msb_shift = 0;
            }
        }
    }

    if ((p_i2s_obj[i2s_num]->mode & I2S_MODE_RX) &&
        (p_i2s_obj[i2s_num]->mode & I2S_MODE_TX))
    {
        if (p_i2s_obj[i2s_num]->mode & I2S_MODE_MASTER)
        {
            I2S[i2s_num]->conf.tx_slave_mod = 0;
            I2S[i2s_num]->conf.rx_slave_mod = 0;
        }
        else if (p_i2s_obj[i2s_num]->mode & I2S_MODE_SLAVE)
        {
            I2S[i2s_num]->conf.tx_slave_mod = 1;
            I2S[i2s_num]->conf.rx_slave_mod = 1;
        }
    }

    p_i2s_obj[i2s_num]->tx_desc_auto_clear = i2s_config->tx_desc_auto_clear;

    return ESP_OK;
}

/* ===========================================================================
 *  zero_dma_buffer
 * ======================================================================== */

esp_err_t i2s_zero_dma_buffer(i2s_port_t i2s_num)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    i2s_obj_t *obj = i2s_obj_acquire(i2s_num);
    I2S_CHECK(obj, "i2s not installed or being uninstalled", ESP_ERR_INVALID_STATE);

    if (xSemaphoreTake(obj->api_lock, pdMS_TO_TICKS(I2S_API_LOCK_TIMEOUT_MS)) != pdTRUE)
    {
        ESP_LOGE(I2S_TAG, "zero_dma_buffer: api_lock busy");
        i2s_obj_release(obj);
        return ESP_ERR_TIMEOUT;
    }

    if (obj->drv_state != I2S_DRV_RUNNING && obj->drv_state != I2S_DRV_STOPPED)
    {
        xSemaphoreGive(obj->api_lock);
        i2s_obj_release(obj);
        return ESP_ERR_INVALID_STATE;
    }

    i2s_drv_state_t prev_state = obj->drv_state;

    obj->drv_state = I2S_DRV_RECONFIGURING;

    QueueHandle_t tq = obj->tx ? obj->tx->queue : NULL;
    QueueHandle_t rq = obj->rx ? obj->rx->queue : NULL;

    xSemaphoreGive(obj->api_lock);

    if (obj->mode & I2S_MODE_TX)
    {
        i2s_wake_dir_waiters(obj, true, tq);
    }

    if (obj->mode & I2S_MODE_RX)
    {
        i2s_wake_dir_waiters(obj, false, rq);
    }

    esp_err_t drain_err = i2s_wait_tx_rx_idle(obj);
    if (drain_err != ESP_OK)
    {
        ESP_LOGE(I2S_TAG, "zero_dma_buffer: drain timeout, aborting zero");
        xSemaphoreTake(obj->api_lock, portMAX_DELAY);
        obj->drv_state = prev_state;
        xSemaphoreGive(obj->api_lock);
        i2s_obj_release(obj);
        return drain_err;
    }
    i2s_stop_internal(i2s_num);

    if (obj->rx && obj->rx->buf != NULL && obj->rx->buf_size != 0)
    {
        xSemaphoreTake(obj->rx->mux, portMAX_DELAY);

        for (int i = 0; i < obj->dma_buf_count; i++)
        {
            if (obj->rx->buf[i])
                memset(obj->rx->buf[i], 0, obj->rx->buf_size);
        }

        obj->rx->rw_pos = 0;
        obj->rx->curr_ptr = NULL;

        if (obj->rx->queue)
            xQueueReset(obj->rx->queue);

        xSemaphoreGive(obj->rx->mux);
    }

    if (obj->tx && obj->tx->buf != NULL && obj->tx->buf_size != 0)
    {
        xSemaphoreTake(obj->tx->mux, portMAX_DELAY);

        for (int i = 0; i < obj->dma_buf_count; i++)
        {
            if (obj->tx->buf[i])
                memset(obj->tx->buf[i], 0, obj->tx->buf_size);
        }

        obj->tx->rw_pos = 0;
        obj->tx->curr_ptr = NULL;

        if (obj->tx->queue)
            xQueueReset(obj->tx->queue);

        xSemaphoreGive(obj->tx->mux);
    }

    esp_err_t start_err = ESP_OK;

    if (prev_state == I2S_DRV_RUNNING)
    {
        start_err = i2s_start_internal(i2s_num);
    }

    xSemaphoreTake(obj->api_lock, portMAX_DELAY);

    if (prev_state == I2S_DRV_RUNNING && start_err == ESP_OK)
        obj->drv_state = I2S_DRV_RUNNING;
    else
        obj->drv_state = I2S_DRV_STOPPED;

    xSemaphoreGive(obj->api_lock);
    i2s_obj_release(obj);

    return start_err;
}

/* ===========================================================================
 *  i2s_write / i2s_read
 * ======================================================================== */

esp_err_t i2s_write(i2s_port_t i2s_num,
                    const void *src,
                    size_t size,
                    size_t *bytes_written,
                    TickType_t ticks_to_wait)
{
    char *data_ptr, *src_byte;
    size_t bytes_can_write;

    I2S_CHECK(bytes_written, "bytes_written is NULL", ESP_ERR_INVALID_ARG);
    *bytes_written = 0;

    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK((size < I2S_MAX_BUFFER_SIZE), "size is too large", ESP_ERR_INVALID_ARG);
    I2S_CHECK((src != NULL || size == 0), "src is NULL", ESP_ERR_INVALID_ARG);

    if (size == 0)
        return ESP_OK;

    i2s_obj_t *obj = i2s_obj_acquire(i2s_num);
    I2S_CHECK(obj, "i2s not installed or being uninstalled", ESP_ERR_INVALID_STATE);
    if (obj->tx == NULL)
    {
        i2s_obj_release(obj);
        ESP_LOGE(I2S_TAG, "tx NULL");
        return ESP_ERR_INVALID_ARG;
    }

    if (!i2s_user_begin(obj, true))
    {
        i2s_obj_release(obj);
        return ESP_ERR_INVALID_STATE;
    }

    src_byte = (char *)src;

    while (size > 0)
    {
        /* Short critical section: check whether we already have a buffer. */
        xSemaphoreTake(obj->tx->mux, portMAX_DELAY);

        bool have_buffer =
            (obj->tx->curr_ptr != NULL) &&
            (obj->tx->rw_pos < obj->tx->buf_size);

        xSemaphoreGive(obj->tx->mux);

        if (!have_buffer)
        {
            void *buf = NULL;

            /* Atomically check state and mark ourselves as waiter. */
            xSemaphoreTake(obj->api_lock, portMAX_DELAY);

            if (obj->drv_state != I2S_DRV_RUNNING)
            {
                xSemaphoreGive(obj->api_lock);
                break;
            }

            obj->tx_waiters++;

            xSemaphoreGive(obj->api_lock);

            BaseType_t ok = xQueueReceive(obj->tx->queue, &buf, ticks_to_wait);

            xSemaphoreTake(obj->api_lock, portMAX_DELAY);
            obj->tx_waiters--;
            xSemaphoreGive(obj->api_lock);

            if (ok != pdTRUE)
            {
                break;
            }

            if (buf == NULL)
            {
                /* Received a sentinel. Check whether the control operation that sent it
                 * actually completed (state != RUNNING) or was aborted (state == RUNNING).
                 * If aborted, the sentinel is stale — ignore it and keep waiting. */
                bool still_running;

                xSemaphoreTake(obj->api_lock, portMAX_DELAY);
                still_running = (obj->drv_state == I2S_DRV_RUNNING);
                xSemaphoreGive(obj->api_lock);

                if (!still_running)
                {
                    break;
                }

                continue;
            }

            /* drv_state is protected by api_lock — read it there instead of
             * under the data mutex, keeping the locking discipline consistent
             * with the rest of the driver. */
            bool still_running;

            xSemaphoreTake(obj->api_lock, portMAX_DELAY);
            still_running = (obj->drv_state == I2S_DRV_RUNNING);
            xSemaphoreGive(obj->api_lock);

            if (!still_running)
            {
                /* A real buffer raced with stop/reconfiguration. Do not drop it.
                 * Return it to the free queue using the same bounded-queue policy
                 * as the ISR. */
                if (uxQueueSpacesAvailable(obj->tx->queue) == 0)
                {
                    void *discarded_buf = NULL;
                    if (xQueueReceive(obj->tx->queue, &discarded_buf, 0) == pdPASS &&
                        obj->tx_desc_auto_clear &&
                        discarded_buf != NULL)
                    {
                        memset(discarded_buf, 0, obj->tx->buf_size);
                    }
                }

                if (xQueueSend(obj->tx->queue, &buf, 0) != pdPASS)
                {
                    ESP_LOGE(I2S_TAG, "failed to return TX DMA buffer after state transition");
                }
                break;
            }

            xSemaphoreTake(obj->tx->mux, portMAX_DELAY);
            obj->tx->curr_ptr = buf;
            obj->tx->rw_pos = 0;
            xSemaphoreGive(obj->tx->mux);

            continue;
        }

        /* Short critical section: copy data into curr_ptr. */
        xSemaphoreTake(obj->tx->mux, portMAX_DELAY);

        ESP_LOGD(I2S_TAG,
                 "size: %u, rw_pos: %d, buf_size: %d, curr_ptr: %d",
                 (unsigned)size,
                 obj->tx->rw_pos,
                 obj->tx->buf_size,
                 (int)obj->tx->curr_ptr);

        data_ptr = (char *)obj->tx->curr_ptr;
        data_ptr += obj->tx->rw_pos;

        bytes_can_write = (size_t)(obj->tx->buf_size - obj->tx->rw_pos);

        if (bytes_can_write > size)
        {
            bytes_can_write = size;
        }

        memcpy(data_ptr, src_byte, bytes_can_write);

        size -= bytes_can_write;
        src_byte += bytes_can_write;

        obj->tx->rw_pos += (int)bytes_can_write;
        (*bytes_written) += bytes_can_write;

        xSemaphoreGive(obj->tx->mux);
    }

    i2s_user_end(obj, true);
    i2s_obj_release(obj);

    return ESP_OK;
}

esp_err_t i2s_write_expand(i2s_port_t i2s_num,
                           const void *src,
                           size_t size,
                           size_t src_bits,
                           size_t aim_bits,
                           size_t *bytes_written,
                           TickType_t ticks_to_wait)
{
    I2S_CHECK(bytes_written, "bytes_written is NULL", ESP_ERR_INVALID_ARG);
    *bytes_written = 0;
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK((size < I2S_MAX_BUFFER_SIZE), "size is too large", ESP_ERR_INVALID_ARG);
    I2S_CHECK((src != NULL || size == 0), "src is NULL", ESP_ERR_INVALID_ARG);
    I2S_CHECK((src_bits == 8 || src_bits == 16 || src_bits == 24),
              "src_bits must be 8, 16 or 24", ESP_ERR_INVALID_ARG);
    I2S_CHECK((aim_bits == 16 || aim_bits == 32),
              "aim_bits must be 16 or 32 (24-bit DMA words are 32-bit aligned on ESP8266)",
              ESP_ERR_INVALID_ARG);
    I2S_CHECK((src_bits < aim_bits), "src_bits must be < aim_bits", ESP_ERR_INVALID_ARG);

    if (size == 0)
        return ESP_OK;

    i2s_obj_t *obj = i2s_obj_acquire(i2s_num);
    I2S_CHECK(obj, "i2s not installed or being uninstalled", ESP_ERR_INVALID_STATE);
    i2s_obj_release(obj);

    size_t src_bytes = src_bits / 8;
    size_t aim_bytes = aim_bits / 8;

    I2S_CHECK((size % src_bytes) == 0,
              "size must be a multiple of source sample size",
              ESP_ERR_INVALID_ARG);

    size_t n_samples = size / src_bytes;
    I2S_CHECK((n_samples <= SIZE_MAX / aim_bytes),
              "expanded size overflow",
              ESP_ERR_INVALID_ARG);

    size_t expanded_size = n_samples * aim_bytes;
    I2S_CHECK((expanded_size < I2S_MAX_BUFFER_SIZE),
              "expanded size is too large",
              ESP_ERR_INVALID_ARG);

    uint8_t *expanded = (uint8_t *)heap_caps_malloc(expanded_size, MALLOC_CAP_8BIT);
    if (expanded == NULL)
    {
        ESP_LOGE(I2S_TAG, "i2s_write_expand: OOM for %u bytes", (unsigned)expanded_size);
        return ESP_ERR_NO_MEM;
    }

    const uint8_t *src_byte = (const uint8_t *)src;
    uint8_t *dst_byte = expanded;
    int shift = (int)(aim_bits - src_bits);

    for (size_t i = 0; i < n_samples; i++)
    {
        /* Read source sample (little-endian). */
        int32_t sample = 0;
        for (size_t b = 0; b < src_bytes; b++)
        {
            sample |= (int32_t)src_byte[b] << (b * 8);
        }

        /* Sign-extend the source sample. */
        if (src_bits < 32)
        {
            int32_t sign_bit = 1 << (src_bits - 1);
            if (sample & sign_bit)
            {
                sample |= ~((1 << src_bits) - 1);
            }
        }

        /* Expand: shift left into the high bits.
         * Use uint32_t to avoid undefined behaviour on negative left-shift. */
        uint32_t expanded_sample = (uint32_t)sample << shift;

        /* Write expanded sample (little-endian). */
        for (size_t b = 0; b < aim_bytes; b++)
        {
            dst_byte[b] = (uint8_t)((expanded_sample >> (b * 8)) & 0xFF);
        }

        src_byte += src_bytes;
        dst_byte += aim_bytes;
    }

    /* Write expanded data through the normal i2s_write(). */
    size_t written = 0;
    esp_err_t err = i2s_write(i2s_num, expanded, expanded_size, &written, ticks_to_wait);

    heap_caps_free(expanded);

    /* Convert written bytes back to source bytes. */
    if (err == ESP_OK)
    {
        *bytes_written = (written / aim_bytes) * src_bytes;
    }
    return err;
}

esp_err_t i2s_read(i2s_port_t i2s_num,
                   void *dest,
                   size_t size,
                   size_t *bytes_read,
                   TickType_t ticks_to_wait)
{
    char *data_ptr, *dest_byte;
    size_t bytes_can_read;

    I2S_CHECK(bytes_read, "bytes_read is NULL", ESP_ERR_INVALID_ARG);
    *bytes_read = 0;

    dest_byte = (char *)dest;

    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK((size < I2S_MAX_BUFFER_SIZE), "size is too large", ESP_ERR_INVALID_ARG);
    I2S_CHECK((dest != NULL || size == 0), "dest is NULL", ESP_ERR_INVALID_ARG);

    if (size == 0)
        return ESP_OK;

    i2s_obj_t *obj = i2s_obj_acquire(i2s_num);
    I2S_CHECK(obj, "i2s not installed or being uninstalled", ESP_ERR_INVALID_STATE);
    if (obj->rx == NULL)
    {
        i2s_obj_release(obj);
        ESP_LOGE(I2S_TAG, "rx NULL");
        return ESP_ERR_INVALID_ARG;
    }

    if (!i2s_user_begin(obj, false))
    {
        i2s_obj_release(obj);
        return ESP_ERR_INVALID_STATE;
    }

    while (size > 0)
    {
        xSemaphoreTake(obj->rx->mux, portMAX_DELAY);

        bool have_buffer =
            (obj->rx->curr_ptr != NULL) &&
            (obj->rx->rw_pos < obj->rx->buf_size);

        xSemaphoreGive(obj->rx->mux);

        if (!have_buffer)
        {
            void *buf = NULL;

            xSemaphoreTake(obj->api_lock, portMAX_DELAY);

            if (obj->drv_state != I2S_DRV_RUNNING)
            {
                xSemaphoreGive(obj->api_lock);
                break;
            }

            obj->rx_waiters++;

            xSemaphoreGive(obj->api_lock);

            BaseType_t ok = xQueueReceive(obj->rx->queue, &buf, ticks_to_wait);

            xSemaphoreTake(obj->api_lock, portMAX_DELAY);
            obj->rx_waiters--;
            xSemaphoreGive(obj->api_lock);

            if (ok != pdTRUE)
            {
                break;
            }

            if (buf == NULL)
            {
                bool still_running;

                xSemaphoreTake(obj->api_lock, portMAX_DELAY);
                still_running = (obj->drv_state == I2S_DRV_RUNNING);
                xSemaphoreGive(obj->api_lock);

                if (!still_running)
                {
                    break;
                }

                continue;
            }

            /* drv_state is protected by api_lock — read it there instead of
             * under the data mutex, keeping the locking discipline consistent
             * with the rest of the driver. */
            bool still_running;

            xSemaphoreTake(obj->api_lock, portMAX_DELAY);
            still_running = (obj->drv_state == I2S_DRV_RUNNING);
            xSemaphoreGive(obj->api_lock);

            if (!still_running)
            {
                /* A real buffer raced with stop/reconfiguration. Return it to the
                 * RX free queue rather than losing it. */
                if (uxQueueSpacesAvailable(obj->rx->queue) == 0)
                {
                    void *discarded_buf = NULL;
                    (void)xQueueReceive(obj->rx->queue, &discarded_buf, 0);
                }

                if (xQueueSend(obj->rx->queue, &buf, 0) != pdPASS)
                {
                    ESP_LOGE(I2S_TAG, "failed to return RX DMA buffer after state transition");
                }
                break;
            }

            xSemaphoreTake(obj->rx->mux, portMAX_DELAY);
            obj->rx->curr_ptr = buf;
            obj->rx->rw_pos = 0;
            xSemaphoreGive(obj->rx->mux);

            continue;
        }

        xSemaphoreTake(obj->rx->mux, portMAX_DELAY);

        data_ptr = (char *)obj->rx->curr_ptr;
        data_ptr += obj->rx->rw_pos;

        bytes_can_read = (size_t)(obj->rx->buf_size - obj->rx->rw_pos);

        if (bytes_can_read > size)
        {
            bytes_can_read = size;
        }

        memcpy(dest_byte, data_ptr, bytes_can_read);

        size -= bytes_can_read;
        dest_byte += bytes_can_read;

        obj->rx->rw_pos += (int)bytes_can_read;
        (*bytes_read) += bytes_can_read;

        xSemaphoreGive(obj->rx->mux);
    }

    i2s_user_end(obj, false);
    i2s_obj_release(obj);

    return ESP_OK;
}

/* ===========================================================================
 *  uninstall
 * ======================================================================== */

/* ===========================================================================
 *  Internal cleanup for failed installation
 * ======================================================================== */
static void i2s_cleanup_on_install_fail(i2s_port_t i2s_num, i2s_obj_t *obj)
{
    /* Publish the transition the same way i2s_driver_uninstall() does.
     * i2s_obj_acquire() rejects UNINSTALLING, so no new references to
     * the object can be acquired from this point on. */
    I2S_ENTER_CRITICAL();
    obj->drv_state = I2S_DRV_UNINSTALLING;
    I2S_EXIT_CRITICAL();

    QueueHandle_t tq = obj->tx ? obj->tx->queue : NULL;
    QueueHandle_t rq = obj->rx ? obj->rx->queue : NULL;

    /* Defensive: wake and drain any writer/reader that slipped in during
     * the brief RUNNING window before set_clk, exactly like uninstall. */
    if (obj->mode & I2S_MODE_TX)
    {
        i2s_wake_dir_waiters(obj, true, tq);
    }
    if (obj->mode & I2S_MODE_RX)
    {
        i2s_wake_dir_waiters(obj, false, rq);
    }

    if (i2s_wait_tx_rx_idle(obj) != ESP_OK)
    {
        /* Practically unreachable during install (no users can exist yet).
         * Leave the object alive in STOPPED state so the application can
         * recover via i2s_driver_uninstall(). */
        ESP_LOGE(I2S_TAG, "install_cleanup: user drain timeout, aborting cleanup");
        I2S_ENTER_CRITICAL();
        obj->drv_state = I2S_DRV_STOPPED;
        I2S_EXIT_CRITICAL();
        return;
    }

    /* Wait for in-flight public API references to drain. The install call
     * itself holds no reference, so the counter must reach zero. */
    TickType_t start_tick = xTaskGetTickCount();
    for (;;)
    {
        int refs;

        I2S_ENTER_CRITICAL();
        refs = obj->life_refs;
        I2S_EXIT_CRITICAL();

        if (refs <= 0)
            break;

        if ((xTaskGetTickCount() - start_tick) >= pdMS_TO_TICKS(I2S_DRAIN_TIMEOUT_MS))
        {
            /* Practically unreachable during install. Leave the object alive
             * in STOPPED state instead of freeing it out from under an
             * in-flight caller. */
            ESP_LOGE(I2S_TAG, "install_cleanup: API reference drain timeout, aborting cleanup");
            I2S_ENTER_CRITICAL();
            obj->drv_state = I2S_DRV_STOPPED;
            I2S_EXIT_CRITICAL();
            return;
        }
        vTaskDelay(1);
    }

    /* Stop hardware while p_i2s_obj[i2s_num] is still published:
     * i2s_stop_internal() requires the object to be reachable. */
    xSemaphoreTake(obj->api_lock, portMAX_DELAY);
    (void)i2s_stop_internal(i2s_num);
    xSemaphoreGive(obj->api_lock);

    dma_intr_register(NULL, NULL);

    /* Reset hardware to safe state */
    I2S[i2s_num]->conf.tx_reset = 1;
    I2S[i2s_num]->conf.tx_reset = 0;
    I2S[i2s_num]->conf.rx_reset = 1;
    I2S[i2s_num]->conf.rx_reset = 0;
    I2S_MEMW();
    SLC0.conf0.tx_rst = 1;
    SLC0.conf0.tx_rst = 0;
    SLC0.conf0.rx_rst = 1;
    SLC0.conf0.rx_rst = 0;
    I2S_MEMW();

    /* Free allocated resources */
    if (obj->tx != NULL) i2s_destroy_dma_queue(obj->tx);
    if (obj->rx != NULL) i2s_destroy_dma_queue(obj->rx);
    if (obj->i2s_queue != NULL) vQueueDelete(obj->i2s_queue);

    SemaphoreHandle_t api_lock = obj->api_lock;
    EventGroupHandle_t idle_evt = obj->idle_evt;

    /* Unpublish before freeing the object. */
    I2S_ENTER_CRITICAL();
    p_i2s_obj[i2s_num] = NULL;
    I2S_EXIT_CRITICAL();
    I2S_MEMW();

    heap_caps_free(obj);

    if (idle_evt)
        vEventGroupDelete(idle_evt);
    if (api_lock)
        vSemaphoreDelete(api_lock);
}

esp_err_t i2s_driver_uninstall(i2s_port_t i2s_num)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);

    i2s_obj_t *obj = NULL;

    /* Take a stable reference before touching the object. */
    I2S_ENTER_CRITICAL();
    if (p_i2s_obj[i2s_num] == NULL)
    {
        I2S_EXIT_CRITICAL();
        return ESP_ERR_INVALID_STATE;
    }
    obj = p_i2s_obj[i2s_num];
    obj->life_refs++;
    I2S_EXIT_CRITICAL();

    if (xSemaphoreTake(obj->api_lock, pdMS_TO_TICKS(I2S_API_LOCK_TIMEOUT_MS)) != pdTRUE)
    {
        i2s_obj_release(obj);
        ESP_LOGE(I2S_TAG, "uninstall: api_lock busy");
        return ESP_ERR_TIMEOUT;
    }

    if (obj->drv_state == I2S_DRV_UNINSTALLING ||
        obj->drv_state == I2S_DRV_STOPPING ||
        obj->drv_state == I2S_DRV_RECONFIGURING ||
        obj->drv_state == I2S_DRV_INSTALLING)
    {
        ESP_LOGE(I2S_TAG, "uninstall: cannot uninstall during transition (state=%d)", obj->drv_state);
        xSemaphoreGive(obj->api_lock);
        i2s_obj_release(obj);
        return ESP_ERR_INVALID_STATE;
    }

    i2s_drv_state_t prev_state = obj->drv_state;

    /* Publish the transition under the same critical section used by
     * i2s_obj_acquire(), so a concurrent API entry cannot observe a stale
     * RUNNING state after uninstall has begun. */
    I2S_ENTER_CRITICAL();
    obj->drv_state = I2S_DRV_UNINSTALLING;
    I2S_EXIT_CRITICAL();

    QueueHandle_t tq = obj->tx ? obj->tx->queue : NULL;
    QueueHandle_t rq = obj->rx ? obj->rx->queue : NULL;

    xSemaphoreGive(obj->api_lock);

    if (obj->mode & I2S_MODE_TX)
    {
        i2s_wake_dir_waiters(obj, true, tq);
    }

    if (obj->mode & I2S_MODE_RX)
    {
        i2s_wake_dir_waiters(obj, false, rq);
    }

    esp_err_t drain_err = i2s_wait_tx_rx_idle(obj);
    if (drain_err != ESP_OK)
    {
        ESP_LOGE(I2S_TAG, "uninstall: drain timeout, aborting uninstall");
        xSemaphoreTake(obj->api_lock, portMAX_DELAY);
        obj->drv_state = prev_state;
        xSemaphoreGive(obj->api_lock);
        i2s_obj_release(obj);
        return drain_err;
    }

    /* Any public operation which was already in flight holds one reference. */
    TickType_t start_tick = xTaskGetTickCount();
    for (;;)
    {
        int refs;
        I2S_ENTER_CRITICAL();
        refs = obj->life_refs;
        I2S_EXIT_CRITICAL();

        if (refs <= 1)
            break;

        if ((xTaskGetTickCount() - start_tick) >= pdMS_TO_TICKS(I2S_DRAIN_TIMEOUT_MS))
        {
            ESP_LOGE(I2S_TAG, "uninstall: timeout waiting for API references to drain");
            xSemaphoreTake(obj->api_lock, portMAX_DELAY);
            obj->drv_state = prev_state;
            xSemaphoreGive(obj->api_lock);
            i2s_obj_release(obj);
            return ESP_ERR_TIMEOUT;
        }
        vTaskDelay(1);
    }

    xSemaphoreTake(obj->api_lock, portMAX_DELAY);
    (void)i2s_stop_internal(i2s_num);
    xSemaphoreGive(obj->api_lock);

    dma_intr_register(NULL, NULL);

    I2S[i2s_num]->conf.tx_reset = 1;
    I2S[i2s_num]->conf.tx_reset = 0;
    I2S[i2s_num]->conf.rx_reset = 1;
    I2S[i2s_num]->conf.rx_reset = 0;
    I2S_MEMW();

    SLC0.conf0.tx_rst = 1;
    SLC0.conf0.tx_rst = 0;
    SLC0.conf0.rx_rst = 1;
    SLC0.conf0.rx_rst = 0;
    I2S_MEMW();

    if (obj->tx != NULL && (obj->mode & I2S_MODE_TX))
    {
        i2s_destroy_dma_queue(obj->tx);
        obj->tx = NULL;
    }
    if (obj->rx != NULL && (obj->mode & I2S_MODE_RX))
    {
        i2s_destroy_dma_queue(obj->rx);
        obj->rx = NULL;
    }

    obj->i2s_queue = NULL;
    SemaphoreHandle_t api_lock = obj->api_lock;
    EventGroupHandle_t idle_evt = obj->idle_evt;

    /* Remove publication while the uninstall call itself still owns a ref. */
    I2S_ENTER_CRITICAL();
    p_i2s_obj[i2s_num] = NULL;
    I2S_EXIT_CRITICAL();

    i2s_obj_release(obj);

    heap_caps_free(obj);

    if (idle_evt)
        vEventGroupDelete(idle_evt);
    if (api_lock)
        vSemaphoreDelete(api_lock);

    return ESP_OK;
}

/* ===========================================================================
 *  install
 * ======================================================================== */

esp_err_t i2s_driver_install(i2s_port_t i2s_num,
                             const i2s_config_t *i2s_config,
                             int queue_size,
                             void *i2s_queue)
{
    esp_err_t err;

    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK((i2s_config != NULL), "I2S configuration must not NULL", ESP_ERR_INVALID_ARG);

    I2S_CHECK((i2s_config->dma_buf_count >= 2 && i2s_config->dma_buf_count <= 128),
              "dma_buf_count must be 2..128",
              ESP_ERR_INVALID_ARG);

    I2S_CHECK((i2s_config->dma_buf_len >= 8 && i2s_config->dma_buf_len <= 1024),
              "dma_buf_len must be 8..1024",
              ESP_ERR_INVALID_ARG);

    I2S_CHECK((i2s_config->sample_rate > 0),
              "sample_rate must be > 0",
              ESP_ERR_INVALID_ARG);

    I2S_CHECK((i2s_config->mode & (I2S_MODE_TX | I2S_MODE_RX)),
              "mode must include TX and/or RX",
              ESP_ERR_INVALID_ARG);

    I2S_CHECK((i2s_config->channel_format >= I2S_CHANNEL_FMT_RIGHT_LEFT &&
               i2s_config->channel_format <= I2S_CHANNEL_FMT_ONLY_LEFT),
              "channel_format invalid",
              ESP_ERR_INVALID_ARG);

    I2S_CHECK((i2s_config->bits_per_sample == I2S_BITS_PER_SAMPLE_16BIT ||
               i2s_config->bits_per_sample == I2S_BITS_PER_SAMPLE_24BIT),
              "bits_per_sample must be 16 or 24",
              ESP_ERR_INVALID_ARG);

    I2S_CHECK(((i2s_config->mode & I2S_MODE_MASTER) ||
               (i2s_config->mode & I2S_MODE_SLAVE)),
              "mode must include MASTER or SLAVE",
              ESP_ERR_INVALID_ARG);

    I2S_CHECK(!((i2s_config->mode & I2S_MODE_MASTER) &&
                (i2s_config->mode & I2S_MODE_SLAVE)),
              "mode must not include both MASTER and SLAVE",
              ESP_ERR_INVALID_ARG);

    I2S_CHECK((i2s_config->communication_format &
               (I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB | I2S_COMM_FORMAT_I2S_LSB)) != 0,
              "communication_format invalid",
              ESP_ERR_INVALID_ARG);

    I2S_CHECK((i2s_config->communication_format &
               ~(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB | I2S_COMM_FORMAT_I2S_LSB)) == 0,
              "communication_format has unknown bits",
              ESP_ERR_INVALID_ARG);

    I2S_CHECK((i2s_config->communication_format & I2S_COMM_FORMAT_I2S),
              "communication_format must include I2S_COMM_FORMAT_I2S",
              ESP_ERR_INVALID_ARG);

    if (i2s_queue != NULL)
    {
        I2S_CHECK((queue_size > 0),
                  "queue_size must be > 0 when i2s_queue is provided",
                  ESP_ERR_INVALID_ARG);
    }

    I2S_ENTER_CRITICAL();
    bool already_installed = (p_i2s_obj[i2s_num] != NULL);
    I2S_EXIT_CRITICAL();
    if (already_installed)
    {
        ESP_LOGW(I2S_TAG, "I2S driver already installed");
        return ESP_ERR_INVALID_STATE;
    }

    i2s_obj_t *obj = (i2s_obj_t *)heap_caps_zalloc(sizeof(i2s_obj_t), MALLOC_CAP_8BIT);
    I2S_CHECK(obj, "Malloc I2S driver error", ESP_ERR_NO_MEM);

    obj->i2s_num = i2s_num;
    obj->drv_state = I2S_DRV_INSTALLING;
    obj->dma = (slc_struct_t *)&SLC0;

    obj->api_lock = xSemaphoreCreateMutex();
    obj->idle_evt = xEventGroupCreate();

    if (obj->api_lock == NULL || obj->idle_evt == NULL)
    {
        ESP_LOGE(I2S_TAG, "Failed to create api_lock / idle_evt");

        if (obj->api_lock)
            vSemaphoreDelete(obj->api_lock);

        if (obj->idle_evt)
            vEventGroupDelete(obj->idle_evt);

        heap_caps_free(obj);
        p_i2s_obj[i2s_num] = NULL;

        return ESP_ERR_NO_MEM;
    }

    /* Seed idle bits — no users yet. */
    xEventGroupSetBits(obj->idle_evt, I2S_TX_IDLE_BIT | I2S_RX_IDLE_BIT);

    obj->queue_size = queue_size;
    obj->dma_buf_count = i2s_config->dma_buf_count;
    obj->dma_buf_len = i2s_config->dma_buf_len;
    obj->dma_buf_len_orig = i2s_config->dma_buf_len;
    obj->mode = i2s_config->mode;

    obj->bits_per_sample = 0;
    obj->bytes_per_sample = 0;

    obj->channel_num =
        (i2s_config->channel_format == I2S_CHANNEL_FMT_ONLY_RIGHT ||
         i2s_config->channel_format == I2S_CHANNEL_FMT_ONLY_LEFT)
            ? 1
            : 2;

    obj->channel_format = i2s_config->channel_format;

    /* Publish only after the object and synchronization primitives are fully initialized. */
    I2S_ENTER_CRITICAL();
    if (p_i2s_obj[i2s_num] != NULL)
    {
        I2S_EXIT_CRITICAL();
        if (obj->api_lock)
            vSemaphoreDelete(obj->api_lock);
        if (obj->idle_evt)
            vEventGroupDelete(obj->idle_evt);
        heap_caps_free(obj);
        return ESP_ERR_INVALID_STATE;
    }
    p_i2s_obj[i2s_num] = obj;
    I2S_EXIT_CRITICAL();

    dma_intr_register(i2s_intr_handler_default, obj);

    if (esp_wifi_get_state() == WIFI_STATE_DEINIT)
    {
        rom_i2c_writeReg_Mask(0x67, 4, 4, 7, 7, 1);
        I2S_MEMW();
    }

    i2s_stop_internal(i2s_num);

    err = i2s_param_config(i2s_num, i2s_config);
    if (err != ESP_OK)
    {
        i2s_cleanup_on_install_fail(i2s_num, obj);
        ESP_LOGE(I2S_TAG, "I2S param configure error");
        return err;
    }

    bool event_queue_created = false;

    if (i2s_queue)
    {
        obj->i2s_queue = xQueueCreate(queue_size, sizeof(i2s_event_t));

        if (obj->i2s_queue == NULL)
        {
            ESP_LOGE(I2S_TAG, "Failed to create i2s event queue");
            i2s_cleanup_on_install_fail(i2s_num, obj);
            return ESP_ERR_NO_MEM;
        }

        event_queue_created = true;
        *((QueueHandle_t *)i2s_queue) = obj->i2s_queue;

        ESP_LOGI(I2S_TAG,
                 "queue free spaces: %d",
                 (int)uxQueueSpacesAvailable(obj->i2s_queue));
    }
    else
    {
        obj->i2s_queue = NULL;
    }

    /* set_clk will perform full build + start and finish in RUNNING. */
    obj->drv_state = I2S_DRV_RUNNING;

    esp_err_t set_clk_err = i2s_set_clk(i2s_num,
                                        i2s_config->sample_rate,
                                        i2s_config->bits_per_sample,
                                        obj->channel_num);

    if (set_clk_err != ESP_OK)
    {
        ESP_LOGE(I2S_TAG,
                 "I2S set_clk failed during install: 0x%x",
                 (int)set_clk_err);

        if (event_queue_created && obj->i2s_queue)
        {
            vQueueDelete(obj->i2s_queue);
            obj->i2s_queue = NULL;

            if (i2s_queue)
                *((QueueHandle_t *)i2s_queue) = NULL;
        }

        if (p_i2s_obj[i2s_num] != NULL)
        {
            i2s_cleanup_on_install_fail(i2s_num, obj);
        }

        return set_clk_err;
    }

    return ESP_OK;
}

/* ===========================================================================
 *  State introspection
 * ======================================================================== */

i2s_drv_state_t i2s_get_driver_state(i2s_port_t i2s_num)
{
    i2s_obj_t *obj;
    i2s_drv_state_t state;

    if (i2s_num >= I2S_NUM_MAX)
        return I2S_DRV_UNINIT;

    obj = i2s_obj_acquire(i2s_num);
    if (obj == NULL)
        return I2S_DRV_UNINIT;

    state = obj->drv_state;
    i2s_obj_release(obj);
    return state;
}

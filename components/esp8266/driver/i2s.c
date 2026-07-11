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
#include <stdbool.h>
/* FIX (GROK-1.4): assert.h for the defensive DMA alignment asserts in
 * i2s_create_dma_queue. */
#include <assert.h>
#include <math.h>
#include <float.h>
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
#include "esp_wifi_types.h"
#include "esp_wifi.h"

static const char *I2S_TAG = "i2s";

/* BBPLL audio clock enable — required by TRM §10.2.1.2 before I2S can operate.
 * The SDK's i2s_driver_install() was missing this, causing BCK/WS to stay LOW. */
extern void rom_i2c_writeReg_Mask(uint8_t block, uint8_t host_id, uint8_t reg_add, uint8_t msb, uint8_t lsb, uint8_t indata);

/* Memory barrier for peripheral register access.
 * Xtensa LX106 'memw' instruction guarantees all prior memory writes complete
 * before any subsequent memory access. Without this, -O2 optimization reorders
 * register writes, breaking I2S peripheral initialization sequence.
 * The peripheral hardware requires strict ordering: reset → configure → enable.
 * At -Og these writes happen in separate functions (no reordering possible).
 * At -Os/-Ox the compiler inlines everything and freely reorders, breaking I2S. */
#define I2S_MEMW() __asm__ __volatile__("memw" ::: "memory")

#define I2S_CHECK(a, str, ret_val)                                    \
    if (!(a))                                                         \
    {                                                                 \
        ESP_LOGE(I2S_TAG, "%s(%d): %s", __FUNCTION__, __LINE__, str); \
        return (ret_val);                                             \
    }

#define dma_intr_enable() _xt_isr_unmask(1 << ETS_SLC_INUM)
#define dma_intr_disable() _xt_isr_mask(1 << ETS_SLC_INUM)
#define dma_intr_register(a, b) _xt_isr_attach(ETS_SLC_INUM, (a), (b))

#define I2S_MAX_BUFFER_SIZE (4 * 1024 * 1024) // the maximum RAM can be allocated
#define I2S_BASE_CLK (2 * APB_CLK_FREQ)
#define I2S_ENTER_CRITICAL() portENTER_CRITICAL()
#define I2S_EXIT_CRITICAL() portEXIT_CRITICAL()
// I2S_FULL_DUPLEX_*_MODE_MASK macros removed (dead code, never used)

typedef struct lldesc
{
    uint32_t blocksize : 12;
    uint32_t datalen : 12;
    uint32_t unused : 5;
    uint32_t sub_sof : 1;
    uint32_t eof : 1;
    volatile uint32_t owner : 1; // DMA can change this value
    uint32_t *buf_ptr;
    struct lldesc *next_link_ptr;
} lldesc_t;

/**
 * @brief DMA buffer object
 */
typedef struct
{
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
typedef struct
{
    i2s_port_t i2s_num;      /*!< I2S port number*/
    int queue_size;          /*!< I2S event queue size*/
    QueueHandle_t i2s_queue; /*!< I2S queue handler*/
    int dma_buf_count;       /*!< DMA buffer count, number of buffer*/
    int dma_buf_len;         /*!< DMA buffer length, length of each buffer (may be clamped to 4092)*/
    int dma_buf_len_orig;    /*!< FIX N2: original dma_buf_len from config, not clamped — used so we can grow back when bytes/ch shrink */

    i2s_dma_t *rx;                    /*!< DMA Rx buffer*/
    i2s_dma_t *tx;                    /*!< DMA Tx buffer*/
    int channel_num;                  /*!< Number of channels*/
    int bytes_per_sample;             /*!< Bytes per sample*/
    int bits_per_sample;              /*!< Bits per sample*/
    i2s_mode_t mode;                  /*!< I2S Working mode*/
    uint32_t sample_rate;             /*!< I2S sample rate */
    bool tx_desc_auto_clear;          /*!< I2S auto clear tx descriptor on underflow */
    i2s_channel_fmt_t channel_format; /*!< Stored channel_format for proper tx/rx_chan_mod restoration */
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

// FIX N4: removed dead `i2s_clear_intr_status` function. After fix #29 made
// the intr-helper functions `static`, this one was never called from anywhere
// in the driver. Userspace should not be calling it either (it is not in the
// public i2s.h API).

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

static void IRAM_ATTR i2s_intr_handler_default(void *arg)
{
    i2s_obj_t *p_i2s = (i2s_obj_t *)arg;
    slc_struct_t *dma_reg = p_i2s->dma;
    i2s_event_t i2s_event;
    void *discarded_buf = NULL;
    /* FIX (GROK-1.6-review): use a dedicated i2s_event_t variable for the
     * event-queue drop path. The TX/RX queue item size is sizeof(char*) = 4,
     * and sizeof(i2s_event_t) currently also happens to be 4 (one enum
     * field), so the old `void *discarded_buf` worked by accident. If
     * i2s_event_t is ever extended (size, timestamp, ...),
     * xQueueReceiveFromISR would write past the 4-byte void* and smash the
     * ISR stack. Keep discarded_buf for the TX/RX-queue drop path (where
     * the item really is a char*). */
    i2s_event_t discarded_event;

    portBASE_TYPE high_priority_task_awoken = 0;

    lldesc_t *finish_desc;

    // FIX #8 (full): snapshot the entire int_st register into a LOCAL non-volatile
    // copy via the union's .val field. All subsequent field accesses on the local
    // copy do NOT re-read the register, eliminating two hazards:
    //   (1) "spurious ISR" — a bit set after the snapshot would be processed by
    //       field-access-on-live-register but NOT cleared by int_clr=snapshot,
    //       causing duplicate processing in the next ISR invocation.
    //   (2) "lost interrupt" — a bit set just before the snapshot but not
    //       processed by any condition would still be cleared by int_clr=snapshot.
    // FIX N3: `typeof` is a GCC extension (standardised only in C23). ESP-IDF
    // uses GCC, so this compiles, but we annotate it for portability awareness.
    // Hardcoding the struct name (e.g. slc_int_st_t) would require knowing the
    // exact SDK type name, which varies across SDK versions.
    typeof(dma_reg->int_st) int_st_snap;
    int_st_snap.val = dma_reg->int_st.val;

    // FIX U1: snapshot the address registers too. Without this, the following
    // race exists:
    //   1. Descriptor X completes -> rx_eof=1, rx_eof_des_addr=X
    //   2. ISR runs, snapshots int_st.val (rx_eof bit captured)
    //   3. BEFORE we read rx_eof_des_addr, descriptor X+1 completes ->
    //      rx_eof_des_addr=X+1 (rx_eof bit stays 1)
    //   4. ISR reads rx_eof_des_addr, gets X+1
    //   5. Descriptor X's buf_ptr is never returned to tx->queue -> queue
    //      eventually empties -> underflow -> audio glitches.
    // Snapshotting the address registers atomically with the status snapshot
    // guarantees we process the descriptor that triggered the interrupt.
    uint32_t rx_eof_des_addr_snap = dma_reg->rx_eof_des_addr;
    uint32_t tx_eof_des_addr_snap = dma_reg->tx_eof_des_addr;

    if (int_st_snap.tx_dscr_err || int_st_snap.rx_dscr_err)
    {
        ESP_EARLY_LOGE(I2S_TAG, "dma error, interrupt status: 0x%08x", int_st_snap.val);

        if (p_i2s->i2s_queue)
        {
            i2s_event.type = I2S_EVENT_DMA_ERROR;

            if (xQueueIsQueueFullFromISR(p_i2s->i2s_queue))
            {
                xQueueReceiveFromISR(p_i2s->i2s_queue, &discarded_event, &high_priority_task_awoken);
            }

            xQueueSendFromISR(p_i2s->i2s_queue, (void *)&i2s_event, &high_priority_task_awoken);
        }
    }

    if (int_st_snap.rx_eof && p_i2s->tx)
    {
        finish_desc = (lldesc_t *)rx_eof_des_addr_snap;

        // All buffers are empty. This means we have an underflow on our hands.
        if (xQueueIsQueueFullFromISR(p_i2s->tx->queue))
        {
            // FIX D2 (v4.8): check the return value of xQueueReceiveFromISR
            // before using discarded_buf. The original code assumed the receive
            // would always succeed (since the queue was just reported as full),
            // but on FreeRTOS the queue state could change between the
            // IsQueueFull check and the Receive call if a higher-priority ISR
            // ran in between (unlikely on single-core ESP8266 but possible in
            // theory). If the receive fails, discarded_buf stays NULL and
            // memset would crash.
            BaseType_t recv_ok = xQueueReceiveFromISR(p_i2s->tx->queue, &discarded_buf, &high_priority_task_awoken);

            // See if tx descriptor needs to be auto cleared:
            // This will avoid any kind of noise that may get introduced due to transmission
            // of previous data from tx descriptor on I2S line.
            if (recv_ok == pdPASS && p_i2s->tx_desc_auto_clear == true)
            {
                memset(discarded_buf, 0, p_i2s->tx->buf_size);
            }
        }

        xQueueSendFromISR(p_i2s->tx->queue, (void *)(&finish_desc->buf_ptr), &high_priority_task_awoken);

        if (p_i2s->i2s_queue)
        {
            i2s_event.type = I2S_EVENT_TX_DONE;

            if (xQueueIsQueueFullFromISR(p_i2s->i2s_queue))
            {
                // FIX D2: same defensive check for the event queue receive.
                (void)xQueueReceiveFromISR(p_i2s->i2s_queue, &discarded_event, &high_priority_task_awoken);
            }

            xQueueSendFromISR(p_i2s->i2s_queue, (void *)&i2s_event, &high_priority_task_awoken);
        }
    }

    if (int_st_snap.tx_suc_eof && p_i2s->rx)
    {
        // All buffers are full. This means we have an overflow.
        finish_desc = (lldesc_t *)tx_eof_des_addr_snap;
        finish_desc->owner = 1;

        if (xQueueIsQueueFullFromISR(p_i2s->rx->queue))
        {
            // FIX D2: defensive check (same reasoning as TX branch above).
            (void)xQueueReceiveFromISR(p_i2s->rx->queue, &discarded_buf, &high_priority_task_awoken);
        }

        xQueueSendFromISR(p_i2s->rx->queue, (void *)(&finish_desc->buf_ptr), &high_priority_task_awoken);

        if (p_i2s->i2s_queue)
        {
            i2s_event.type = I2S_EVENT_RX_DONE;

            // FIX #27: redundant `p_i2s->i2s_queue &&` removed — we already are inside the if.
            if (xQueueIsQueueFullFromISR(p_i2s->i2s_queue))
            {
                // FIX D2: defensive check.
                (void)xQueueReceiveFromISR(p_i2s->i2s_queue, &discarded_event, &high_priority_task_awoken);
            }

            xQueueSendFromISR(p_i2s->i2s_queue, (void *)&i2s_event, &high_priority_task_awoken);
        }
    }

    if (high_priority_task_awoken == pdTRUE)
    {
        portYIELD_FROM_ISR();
    }

    // FIX #8: clear only the bits we actually snapshotted at entry.
    dma_reg->int_clr.val = int_st_snap.val;
}

// FIX U2: drain any pending DMA descriptors in i2s_stop.
//
// Problem (per user-supplied analysis):
//   dma_intr_disable() in i2s_stop prevents the ISR from firing. If a DMA
//   descriptor was just completed (int_st.rx_eof or int_st.tx_suc_eof set)
//   but the ISR hasn't run yet, the corresponding buf_ptr would be lost when
//   we clear int_st. The application might be blocked in
//   i2s_write(portMAX_DELAY) waiting for a buf_ptr to become available, and
//   would never unblock.
//
// Fix: before clearing int_st, check the status register and manually add
// the pending buf_ptrs to the application queues — same as the ISR would do.
//
// IMPORTANT (corrections to user's original proposal):
//   (1) Use xQueueSendFromISR / xQueueReceiveFromISR (NOT xQueueSend / xQueueReceive).
//       The regular forms are not safe inside portENTER_CRITICAL even with 0
//       ticks: they may call vTaskSwitchContext if xYieldPending is set, and
//       in debug builds assert on xTaskGetSchedulerState(). The FromISR forms
//       explicitly disable the scheduler-yield path.
//   (2) Set finish_desc->owner = 1 BEFORE enqueuing the buf_ptr (not after,
//       as in the user's draft). Otherwise there is a tiny window where the
//       app receives the buf_ptr via i2s_read but DMA may still be writing to
//       it. The ISR does set-owner-first; we mirror that order.
//   (3) No IRAM_ATTR — i2s_stop is only called from task context (never from
//       ISR or flash-cache-disabled code), so this helper doesn't need to
//       live in IRAM.
//
// Caveat: a small race window still exists between reading int_st and reading
// rx_eof_des_addr (same caveat as fix U1). On ESP8266 the SLC hardware does
// not provide atomic multi-register reads, so this is the best we can do.
static void i2s_drain_pending_descriptors(i2s_port_t i2s_num)
{
    i2s_obj_t *obj = p_i2s_obj[i2s_num];
    if (obj == NULL)
    {
        return;
    }
    slc_struct_t *dma_reg = obj->dma;

    // Process pending TX (app -> I2S): descriptor with rx_eof set
    if ((obj->mode & I2S_MODE_TX) && obj->tx && dma_reg->int_st.rx_eof)
    {
        lldesc_t *finish_desc = (lldesc_t *)dma_reg->rx_eof_des_addr;
        if (finish_desc != NULL)
        {
            QueueHandle_t queue = obj->tx->queue;
            // If queue is full, drop the oldest entry (same as ISR behavior).
            if (xQueueIsQueueFullFromISR(queue))
            {
                // FIX D2: defensive check — see i2s_intr_handler_default for
                // the reasoning. discarded_buf stays NULL if receive fails.
                void *discarded_buf = NULL;
                (void)xQueueReceiveFromISR(queue, &discarded_buf, NULL);
            }
            // Enqueue the descriptor's buf_ptr so i2s_write can pick it up.
            xQueueSendFromISR(queue, &finish_desc->buf_ptr, NULL);
        }
    }

    // Process pending RX (I2S -> app): descriptor with tx_suc_eof set
    if ((obj->mode & I2S_MODE_RX) && obj->rx && dma_reg->int_st.tx_suc_eof)
    {
        lldesc_t *finish_desc = (lldesc_t *)dma_reg->tx_eof_des_addr;
        if (finish_desc != NULL)
        {
            // Set owner BEFORE enqueue (see correction #2 above).
            finish_desc->owner = 1;

            QueueHandle_t queue = obj->rx->queue;
            if (xQueueIsQueueFullFromISR(queue))
            {
                // FIX D2: defensive check.
                void *discarded_buf = NULL;
                (void)xQueueReceiveFromISR(queue, &discarded_buf, NULL);
            }
            xQueueSendFromISR(queue, &finish_desc->buf_ptr, NULL);
        }
    }
}

static esp_err_t i2s_destroy_dma_queue(i2s_port_t i2s_num, i2s_dma_t *dma)
{
    int bux_idx;

    if (p_i2s_obj[i2s_num] == NULL)
    {
        ESP_LOGE(I2S_TAG, "Not initialized yet");
        return ESP_ERR_INVALID_ARG;
    }

    if (dma == NULL)
    {
        ESP_LOGE(I2S_TAG, "dma is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    for (bux_idx = 0; bux_idx < p_i2s_obj[i2s_num]->dma_buf_count; bux_idx++)
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

    // FIX #7: queue and mux may not have been created yet if allocation failed
    // before xQueueCreate()/xSemaphoreCreateMutex() in i2s_create_dma_queue().
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

static i2s_dma_t *i2s_create_dma_queue(i2s_port_t i2s_num, int dma_buf_count, int dma_buf_len)
{
    int bux_idx;
    int sample_size = p_i2s_obj[i2s_num]->bytes_per_sample * p_i2s_obj[i2s_num]->channel_num;
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
        dma->buf[bux_idx] = (char *)heap_caps_calloc(1, dma_buf_len * sample_size, MALLOC_CAP_8BIT);

        if (dma->buf[bux_idx] == NULL)
        {
            ESP_LOGE(I2S_TAG, "Error malloc dma buffer");
            i2s_destroy_dma_queue(i2s_num, dma);
            return NULL;
        }

        ESP_LOGD(I2S_TAG, "Addr[%d] = %d", bux_idx, (int)dma->buf[bux_idx]);
    }

    /* FIX (GROK-1.6-review): heap_caps_zalloc, not heap_caps_malloc. The
     * destroy path (i2s_destroy_dma_queue) iterates desc[0..count-1] and
     * frees non-NULL entries. With malloc, on OOM mid-loop
     * desc[K+1..N-1] contain garbage, the destroy loop's `if (desc[i])`
     * passes on garbage, and we heap_caps_free(random) -> heap corruption.
     * buf[] above is zalloc'd already; desc must match. */
    dma->desc = (lldesc_t **)heap_caps_zalloc(sizeof(lldesc_t *) * dma_buf_count, MALLOC_CAP_8BIT);

    if (dma->desc == NULL)
    {
        ESP_LOGE(I2S_TAG, "Error malloc dma description");
        i2s_destroy_dma_queue(i2s_num, dma);
        return NULL;
    }

    for (bux_idx = 0; bux_idx < dma_buf_count; bux_idx++)
    {
        dma->desc[bux_idx] = (lldesc_t *)heap_caps_malloc(sizeof(lldesc_t), MALLOC_CAP_8BIT);

        if (dma->desc[bux_idx] == NULL)
        {
            ESP_LOGE(I2S_TAG, "Error malloc dma description entry");
            i2s_destroy_dma_queue(i2s_num, dma);
            return NULL;
        }
        /* FIX (GROK-1.4): defensive alignment assert. ESP8266 SLC requires
         * word-aligned DMA descriptors. The heap allocator always returns
         * 8-byte-aligned blocks, but assert here so a future custom
         * allocator or memory shrink can't silently break SLC. MALLOC_CAP_DMA
         * is an ESP32 concept and doesn't exist on ESP8266, so we use
         * MALLOC_CAP_8BIT + runtime assert instead. */
        assert(((uintptr_t)dma->desc[bux_idx] & 3) == 0);
    }

    for (bux_idx = 0; bux_idx < dma_buf_count; bux_idx++)
    {
        // Configuring the DMA queue
        dma->desc[bux_idx]->owner = 1;
        dma->desc[bux_idx]->eof = 1; // Each linked list produces an EOF interrupt that notifies the task of filling the data more quickly
        dma->desc[bux_idx]->sub_sof = 0;
        dma->desc[bux_idx]->datalen = dma_buf_len * sample_size;   // Actual number of bytes of data
        dma->desc[bux_idx]->blocksize = dma_buf_len * sample_size; // Total number of bytes of data
        dma->desc[bux_idx]->buf_ptr = (uint32_t *)dma->buf[bux_idx];
        dma->desc[bux_idx]->unused = 0;
        dma->desc[bux_idx]->next_link_ptr = (lldesc_t *)((bux_idx < (dma_buf_count - 1)) ? (dma->desc[bux_idx + 1]) : dma->desc[0]);
        /* FIX (GROK-1.4): defensive alignment assert for DMA data buffer. */
        assert(((uintptr_t)dma->buf[bux_idx] & 3) == 0);
    }

    dma->queue = xQueueCreate(dma_buf_count - 1, sizeof(char *));
    dma->mux = xSemaphoreCreateMutex();
    // FIX N1: xQueueCreate / xSemaphoreCreateMutex can return NULL on OOM.
    // Without this check, i2s_set_clk would store a dma object with NULL
    // queue/mux into p_i2s_obj[->tx], and the ISR's xQueueSendFromISR would
    // crash on the NULL queue. Fix #7 made i2s_destroy_dma_queue safe for
    // NULL queue/mux, so we can simply call it here to clean up.
    if (dma->queue == NULL || dma->mux == NULL)
    {
        ESP_LOGE(I2S_TAG, "Error creating dma queue/mutex");
        i2s_destroy_dma_queue(i2s_num, dma);
        return NULL;
    }
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
    I2S_MEMW(); // ensure DMA reset completes before I2S reset

    I2S[i2s_num]->conf.tx_reset = 1;
    I2S[i2s_num]->conf.tx_reset = 0;
    I2S[i2s_num]->conf.rx_reset = 1;
    I2S[i2s_num]->conf.rx_reset = 0;
    I2S_MEMW(); // ensure I2S reset completes before enabling interrupts/DMA

    dma_intr_disable();

    /* FIX (GROK-1.6-review): defensively re-arm owner=1 on all TX/RX
     * descriptors. The ISR TX branch does not set finish_desc->owner = 1
     * (only the RX branch does), and i2s_write only enqueues buf_ptr
     * without touching lldesc.owner. This works today because
     * token_no_replace=1 + infor_no_replace=1 mean the SLC hardware
     * doesn't strictly require software re-arm. But after a rate-only
     * set_clk (need_rebuild=false), i2s_stop/i2s_start run without
     * reinitializing descriptors — if a future chip revision enforces
     * owner strictly, the stream would stall after one buffer cycle.
     * Cheap insurance. */
    if (p_i2s_obj[i2s_num]->tx)
    {
        for (int i = 0; i < p_i2s_obj[i2s_num]->dma_buf_count; i++)
        {
            if (p_i2s_obj[i2s_num]->tx->desc && p_i2s_obj[i2s_num]->tx->desc[i])
                p_i2s_obj[i2s_num]->tx->desc[i]->owner = 1;
        }
    }
    if (p_i2s_obj[i2s_num]->rx)
    {
        for (int i = 0; i < p_i2s_obj[i2s_num]->dma_buf_count; i++)
        {
            if (p_i2s_obj[i2s_num]->rx->desc && p_i2s_obj[i2s_num]->rx->desc[i])
                p_i2s_obj[i2s_num]->rx->desc[i]->owner = 1;
        }
    }

    if (p_i2s_obj[i2s_num]->mode & I2S_MODE_TX)
    {
        i2s_enable_tx_intr(i2s_num);
        p_i2s_obj[i2s_num]->dma->rx_link.start = 1;
    }

    if (p_i2s_obj[i2s_num]->mode & I2S_MODE_RX)
    {
        i2s_enable_rx_intr(i2s_num);
        p_i2s_obj[i2s_num]->dma->tx_link.start = 1;
    }
    I2S_MEMW(); // ensure DMA links started before I2S start

    // Both TX and RX are started to ensure clock generation
    I2S[i2s_num]->conf.val |= I2S_I2S_TX_START | I2S_I2S_RX_START;
    I2S_MEMW(); // ensure TX/RX start bits set before reset
    // Simultaneously reset to ensure the same phase.
    I2S[i2s_num]->conf.val |= I2S_I2S_RESET_MASK;
    I2S_MEMW(); // ensure reset asserted before clearing
    I2S[i2s_num]->conf.val &= ~I2S_I2S_RESET_MASK;
    I2S_MEMW(); // ensure reset cleared before enabling interrupts
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

    if (p_i2s_obj[i2s_num]->mode & I2S_MODE_TX)
    {
        p_i2s_obj[i2s_num]->dma->rx_link.stop = 1;
        I2S_MEMW();
        i2s_disable_tx_intr(i2s_num);
    }

    if (p_i2s_obj[i2s_num]->mode & I2S_MODE_RX)
    {
        p_i2s_obj[i2s_num]->dma->tx_link.stop = 1;
        I2S_MEMW();
        i2s_disable_rx_intr(i2s_num);
    }

    I2S[i2s_num]->conf.val &= ~(I2S_I2S_TX_START | I2S_I2S_RX_START);
    I2S_MEMW();
    // FIX U2: drain any pending descriptors before clearing int_st.
    // After dma_intr_disable(), the ISR can no longer fire for any descriptor
    // that has just completed. Without this drain, those descriptors' buf_ptrs
    // would be lost when we clear int_st below — leaving any task blocked in
    // i2s_write(portMAX_DELAY) / i2s_read(portMAX_DELAY) stuck forever.
    // i2s_drain_pending_descriptors enqueues the buf_ptrs in task context
    // using the FromISR queue API (safe inside a critical section).
    i2s_drain_pending_descriptors(i2s_num);

    p_i2s_obj[i2s_num]->dma->int_clr.val = p_i2s_obj[i2s_num]->dma->int_st.val; // clear pending interrupt
    I2S_MEMW();
    I2S_EXIT_CRITICAL();
    return ESP_OK;
}

esp_err_t i2s_set_pin(i2s_port_t i2s_num, const i2s_pin_config_t *pin)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK(pin, "param null", ESP_ERR_INVALID_ARG);

    // FIX #15: fields of i2s_pin_config_t are `int` (kept as `int` to support
    // the `1/-1` sentinel pattern used by other SDK drivers like ir_tx.c).
    // The old `== true` comparison only accepted the literal value 1, silently
    // rejecting any other nonzero value (e.g. -1 sentinel, flag macros). Use a
    // truthy test instead — any nonzero value enables the pin.
    // (M1 attempted to change fields to `bool` for type safety, but that broke
    // the ir_tx.c `1/-1` pattern in the SDK itself, so M1 was reverted.)
    if (pin->bck_o_en)
    {
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_I2SO_BCK);
    }

    if (pin->ws_o_en)
    {
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_I2SO_WS);
    }

    if (pin->data_out_en)
    {
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_I2SO_DATA);
    }

    if (pin->bck_i_en)
    {
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_I2SI_BCK);
    }

    if (pin->ws_i_en)
    {
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_I2SI_WS);
    }

    if (pin->data_in_en)
    {
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_I2SI_DATA);
    }

    return ESP_OK;
}

static esp_err_t i2s_set_rate(i2s_port_t i2s_num, uint32_t rate, int bits_per_sample)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK(p_i2s_obj[i2s_num], "i2s not installed yet", ESP_FAIL);
    // FIX #32: rate == 0 is invalid. Without this check, the divider search loop
    // will pick the largest divider pair (≈1.2 kHz) and silently set a wrong rate.
    I2S_CHECK((rate > 0), "rate must be > 0", ESP_ERR_INVALID_ARG);
    uint8_t bck_div = 1;
    uint8_t mclk_div = 1;

    uint32_t scaled_base_freq = I2S_BASE_CLK / (bits_per_sample == I2S_BITS_PER_SAMPLE_16BIT ? 32 : 48);

    // FIX N7: delta_best must be larger than any possible delta. Using
    // scaled_base_freq as the initial value fails for very small rates
    // (e.g. rate=1 Hz): the first iteration's new_delta is
    // |scaled_base_freq - 1|, which is NOT strictly less than
    // scaled_base_freq, so bck_div/mclk_div never get updated from 1 and
    // the function silently sets the clock to scaled_base_freq instead of
    // the requested rate. Use FLT_MAX instead.
    float delta_best = FLT_MAX;

    for (uint8_t i = 1; i < 64; i++)
    {
        for (uint8_t j = i; j < 64; j++)
        {
            float new_delta = fabsf(((float)scaled_base_freq / i / j) - rate);

            if (new_delta < delta_best)
            {
                delta_best = new_delta;
                bck_div = i;
                mclk_div = j;
                if (new_delta == 0.0f)
                    goto done; // Точное попадание — выходим
            }
        }
    }
done:

    I2S_ENTER_CRITICAL();
    I2S[i2s_num]->conf.bck_div_num = bck_div & 0x3F;
    I2S[i2s_num]->conf.clkm_div_num = mclk_div & 0x3F;
    I2S_MEMW(); // ensure clock dividers are written before exiting critical
    I2S_EXIT_CRITICAL();
    p_i2s_obj[i2s_num]->sample_rate = rate;
    return ESP_OK;
}

esp_err_t i2s_set_clk(i2s_port_t i2s_num, uint32_t rate, i2s_bits_per_sample_t bits, i2s_channel_t ch)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK(p_i2s_obj[i2s_num], "i2s not installed yet", ESP_FAIL);
    // FIX #21: validate channel count. Without this, ch=0 or ch=3 silently
    // collapses to mono via the `(ch == 2) ? 2 : 1` ternary below.
    I2S_CHECK((ch == 1 || ch == 2), "channel must be 1 (mono) or 2 (stereo)", ESP_ERR_INVALID_ARG);

    if (bits % 8 != 0 || bits > I2S_BITS_PER_SAMPLE_24BIT || bits < I2S_BITS_PER_SAMPLE_16BIT)
    {
        ESP_LOGE(I2S_TAG, "Invalid bits per sample");
        return ESP_ERR_INVALID_ARG;
    }

    /* FIX (GROK-1.6-review): validate rate BEFORE i2s_stop. i2s_set_rate
     * checks `rate > 0` internally, but that runs AFTER i2s_stop. On
     * error we returned ESP_ERR_INVALID_ARG with the peripheral left
     * stopped — any subsequent i2s_write(portMAX_DELAY) hung forever
     * (soft-brick). Hoist the check to the top, alongside ch/bits, so
     * we fail fast without touching hardware. */
    I2S_CHECK((rate > 0), "rate must be > 0", ESP_ERR_INVALID_ARG);

// FIX #4/#5: take the *current* tx/rx mutexes once at entry. We must Give
// these exact same handles before destroying them; the new DMA queues (if
// created) start with their own fresh unlocked mutexes, and we never Give
// a mutex we did not Take.
//
// FIX D1 (v4.7): use a FINITE timeout instead of portMAX_DELAY.
//
// Problem: i2s_write/i2s_read take tx->mux/rx->mux and then block on
// xQueueReceive(portMAX_DELAY) while STILL HOLDING the mux. If the DMA is
// stalled (e.g. no underflow/EOF trigger, or hardware not started), the
// writer task blocks forever holding the mux. A subsequent i2s_set_clk
// call from another task would then block forever on xSemaphoreTake,
// hanging the whole system.
//
// Fix: i2s_set_clk uses a 1-second timeout on mux-take. If a writer is
// stuck, i2s_set_clk fails fast with ESP_ERR_TIMEOUT instead of hanging.
// The user can then call i2s_driver_uninstall (which doesn't need the
// mux) to recover, or investigate the stuck writer.
//
// This is a WORKAROUND, not a root-cause fix. The proper fix (Variant 2
// or 3) requires dropping the mux before the blocking queue wait, which
// is an architectural change to i2s_write/i2s_read. See CHANGES.md
// "Known limitations" for details.
#define I2S_SET_CLK_MUX_TIMEOUT_MS 1000
    SemaphoreHandle_t tx_mux_taken = NULL, rx_mux_taken = NULL;
    i2s_dma_t *save_tx = NULL, *save_rx = NULL;

    if ((p_i2s_obj[i2s_num]->mode & I2S_MODE_TX) && p_i2s_obj[i2s_num]->tx)
    {
        tx_mux_taken = p_i2s_obj[i2s_num]->tx->mux;
        if (xSemaphoreTake(tx_mux_taken, pdMS_TO_TICKS(I2S_SET_CLK_MUX_TIMEOUT_MS)) != pdTRUE)
        {
            ESP_LOGE(I2S_TAG, "Failed to acquire tx mux within %d ms — i2s_write may be blocked",
                     I2S_SET_CLK_MUX_TIMEOUT_MS);
            return ESP_ERR_TIMEOUT;
        }
    }

    if ((p_i2s_obj[i2s_num]->mode & I2S_MODE_RX) && p_i2s_obj[i2s_num]->rx)
    {
        rx_mux_taken = p_i2s_obj[i2s_num]->rx->mux;
        if (xSemaphoreTake(rx_mux_taken, pdMS_TO_TICKS(I2S_SET_CLK_MUX_TIMEOUT_MS)) != pdTRUE)
        {
            ESP_LOGE(I2S_TAG, "Failed to acquire rx mux within %d ms — i2s_read may be blocked",
                     I2S_SET_CLK_MUX_TIMEOUT_MS);
            if (tx_mux_taken)
                xSemaphoreGive(tx_mux_taken);
            return ESP_ERR_TIMEOUT;
        }
    }

    i2s_stop(i2s_num);

    // FIX R3: i2s_set_rate now validates `rate > 0` (fix #32) and returns
    // ESP_ERR_INVALID_ARG before touching the divider registers on failure.
    // We must propagate that error instead of continuing with stale dividers.
    esp_err_t rate_err = i2s_set_rate(i2s_num, rate, bits);
    if (rate_err != ESP_OK)
    {
        // Release any mutexes we took before returning.
        if (tx_mux_taken)
            xSemaphoreGive(tx_mux_taken);
        if (rx_mux_taken)
            xSemaphoreGive(rx_mux_taken);
        return rate_err;
    }

    // FIX #33: removed dead `uint32_t cur_mode = 0;` initialization.

    // ----- Capture pre-mutation state for need_rebuild decision -----
    // FIX R1: previously `channel_num` was updated INSIDE the channel-change
    // block, and then `need_rebuild` checked `channel_num != ch` — which was
    // always false because we had just set them equal. That silently disabled
    // fixes #9 (DMA rebuild on channel-only change) and #10 (4092-byte limit
    // on channel-only change). Snapshot the originals BEFORE any mutation.
    int orig_channel_num = p_i2s_obj[i2s_num]->channel_num;
    int orig_bits_per_sample = p_i2s_obj[i2s_num]->bits_per_sample;

    // ----- Handle channel change -----
    if (orig_channel_num != ch)
    {
        p_i2s_obj[i2s_num]->channel_num = (ch == 2) ? 2 : 1;

        // FIX M4 (v4.6): keep channel_format consistent with the new ch.
        //
        // Problem: if the user installed with cf=ONLY_LEFT (channel_num=1) and
        // later calls i2s_set_clk(ch=2), the channel_num becomes 2 but cf stays
        // ONLY_LEFT. This creates an inconsistent state:
        //   - DMA buffer is sized for 2 samples/frame (channel_num=2)
        //   - But fifo_mod=1 (single) and rx_chan_mod=2 (Left) — I2S produces
        //     only 1 sample/frame
        //   - DMA fills at half the expected rate
        //   - Application perceives half the sample rate
        //
        // Fix: auto-upgrade/downgrade channel_format to match ch:
        //   ch=2 + ONLY_RIGHT (3) -> ALL_RIGHT  (1)  [stereo with both slots = R]
        //   ch=2 + ONLY_LEFT  (4) -> ALL_LEFT   (2)  [stereo with both slots = L]
        //   ch=1 + RIGHT_LEFT (0) -> ONLY_RIGHT (3)  [mono, default to right]
        //   ch=1 + ALL_RIGHT  (1) -> ONLY_RIGHT (3)  [mono right]
        //   ch=1 + ALL_LEFT   (2) -> ONLY_LEFT  (4)  [mono left]
        //   ch matches cf category -> no change
        i2s_channel_fmt_t cf = p_i2s_obj[i2s_num]->channel_format;
        if (ch == 2 && cf >= I2S_CHANNEL_FMT_ONLY_RIGHT)
        {
            // Mono -> Stereo: ONLY_RIGHT -> ALL_RIGHT, ONLY_LEFT -> ALL_LEFT
            cf = (cf == I2S_CHANNEL_FMT_ONLY_LEFT) ? I2S_CHANNEL_FMT_ALL_LEFT
                                                   : I2S_CHANNEL_FMT_ALL_RIGHT;
            p_i2s_obj[i2s_num]->channel_format = cf;
        }
        else if (ch == 1 && cf < I2S_CHANNEL_FMT_ONLY_RIGHT)
        {
            // Stereo -> Mono: RIGHT_LEFT/ALL_RIGHT -> ONLY_RIGHT, ALL_LEFT -> ONLY_LEFT
            cf = (cf == I2S_CHANNEL_FMT_ALL_LEFT) ? I2S_CHANNEL_FMT_ONLY_LEFT
                                                  : I2S_CHANNEL_FMT_ONLY_RIGHT;
            p_i2s_obj[i2s_num]->channel_format = cf;
        }

        // FIX #14 + M2 (v4.5 corrected): recompute fifo_mod from a known-good
        // base, instead of blindly doing +/-1 on the current register value.
        //
        // fifo_mod layout (TRM §10.2.1.3):
        //   0: 16-bit dual      1: 16-bit single
        //   2: 24-bit dual disc 3: 24-bit single disc
        //   4: 24-bit dual cont 5: 24-bit single cont
        // Driver uses only {0,1,2,3}. The single/dual bit is bit 0.
        //
        // FIX M2 (v4.5 corrected): the single/dual bit must respect cf AND ch.
        // Per TRM §10.2.1.4 and user clarification:
        //   - RIGHT_LEFT (cf=0): 2 different samples per frame -> dual (0) for stereo
        //   - ALL_RIGHT  (cf=1): 2 slots, both with right-channel data -> dual (0) for stereo
        //   - ALL_LEFT   (cf=2): 2 slots, both with left-channel data  -> dual (0) for stereo
        //   - ONLY_RIGHT (cf=3): 1 real slot + 1 regfile constant -> always single (1)
        //   - ONLY_LEFT  (cf=4): 1 real slot + 1 regfile constant -> always single (1)
        //
        // My original M2 (v4.1) wrongly set fifo_mod=1 (single) for ALL_* modes
        // even in stereo. This was based on a misunderstanding — I thought the
        // I2S peripheral would duplicate a single DMA sample into both slots,
        // but actually the peripheral takes 2 samples from the FIFO and the
        // user's code must supply 2 identical samples for ALL_* modes.
        //
        // Correct logic: dual (0) for cf < ONLY_RIGHT in stereo, single (1)
        // for cf >= ONLY_RIGHT or for mono (ch=1).
        uint32_t tx_fifo_base = I2S[i2s_num]->fifo_conf.tx_fifo_mod;
        uint32_t rx_fifo_base = I2S[i2s_num]->fifo_conf.rx_fifo_mod;
        // Clear the single/dual bit, then set it based on cf AND ch.
        uint32_t fifo_single_bit;
        if (cf < I2S_CHANNEL_FMT_ONLY_RIGHT)
        {
            // RIGHT_LEFT/ALL_RIGHT/ALL_LEFT: dual (0) for stereo, single (1) for mono.
            // In stereo, DMA supplies 2 samples per frame (both real, even if same data
            // for ALL_*). In mono, DMA supplies 1 sample, second slot is unused.
            fifo_single_bit = (ch == 2) ? 0u : 1u;
        }
        else
        {
            // ONLY_RIGHT/ONLY_LEFT: always single (1), because the second slot
            // is filled from regfile constant by the I2S peripheral, not by DMA.
            fifo_single_bit = 1u;
        }
        tx_fifo_base = (tx_fifo_base & ~(uint32_t)1) | fifo_single_bit;
        rx_fifo_base = (rx_fifo_base & ~(uint32_t)1) | fifo_single_bit;
        // FIX #20: clamp to valid range [0..5] (modes 6,7 are invalid).
        if (tx_fifo_base > 5)
            tx_fifo_base = 5;
        if (rx_fifo_base > 5)
            rx_fifo_base = 5;
        I2S[i2s_num]->fifo_conf.tx_fifo_mod = tx_fifo_base;
        I2S[i2s_num]->fifo_conf.rx_fifo_mod = rx_fifo_base;

        // FIX #14 (corrected) + M3 + N5b-REGRESS: compute chan_mod from cf and ch.
        //
        // TRM §10.2.1.4 chan_mod values:
        //   TX (3 bits, 0-4 documented):
        //     0: dual (real L + real R)
        //     1: right both slots
        //     2: left both slots
        //     3: right + regfile constant in left slot
        //     4: left + regfile constant in right slot
        //   RX (2 bits, 0-2 documented; 3 reserved):
        //     0: dual
        //     1: right (sample only R slot)
        //     2: left  (sample only L slot)
        //
        // For ch=2 (stereo):
        //   TX: chan_mod = cf directly (TX supports 0-4)
        //   RX: chan_mod = (cf < ONLY_RIGHT) ? cf : (cf >> 1)
        //       (RX only supports 0-2; ONLY_RIGHT(3)→1, ONLY_LEFT(4)→2)
        //
        // FIX N5b-REGRESS (v4.4): my original N5b set BOTH tx_chan_mod and
        // rx_chan_mod to the same value. For cf=ONLY_LEFT (4), rx_chan_mod=4
        // got truncated to 0 (dual mode) by the 2-bit field mask, causing RX
        // to sample both slots. With fifo_mod=1 (single), only the R-slot
        // data was kept — which is zeros for an INMP441 mic tied for LEFT.
        // Symptom: i2s_read returns all zeros in ONLY_LEFT mode.
        //
        // Fix: split TX and RX chan_mod computations. RX uses the original
        // (cf < ONLY_RIGHT) ? cf : (cf >> 1) formula.
        //
        // For ch=1 (mono), both TX and RX use 1 or 2 (both valid in both
        // registers), so we can share the formula. See M3 comment for the
        // reasoning about why mono collapses ONLY_* to 1/2.
        uint32_t tx_chan_mod;
        uint32_t rx_chan_mod;
        if (ch == 2)
        {
            // Stereo
            tx_chan_mod = (uint32_t)cf;
            rx_chan_mod = (cf < I2S_CHANNEL_FMT_ONLY_RIGHT) ? (uint32_t)cf : (uint32_t)(cf >> 1);
        }
        else
        {
            // Mono: collapse each cf to its single-channel equivalent (1 or 2).
            //   cf=0 RIGHT_LEFT  -> 1 (right, default — no side preference)
            //   cf=1 ALL_RIGHT   -> 1 (right)
            //   cf=2 ALL_LEFT    -> 2 (left)
            //   cf=3 ONLY_RIGHT  -> 1 (right)
            //   cf=4 ONLY_LEFT   -> 2 (left)
            // 1 and 2 are valid for both tx_chan_mod and rx_chan_mod.
            uint32_t mono_mod = (cf == I2S_CHANNEL_FMT_ALL_LEFT || cf == I2S_CHANNEL_FMT_ONLY_LEFT) ? 2u : 1u;
            tx_chan_mod = mono_mod;
            rx_chan_mod = mono_mod;
        }
        I2S[i2s_num]->conf_chan.tx_chan_mod = tx_chan_mod;
        I2S[i2s_num]->conf_chan.rx_chan_mod = rx_chan_mod;
        I2S_MEMW(); // FIX: ensure channel/fifo config flushes before continuing
    }

    // ----- Determine whether DMA buffers must be rebuilt -----
    // FIX #9 (now actually working): previously DMA was rebuilt only when
    // `bits` changed. But `sample_size = bytes_per_sample * channel_num` also
    // changes when the channel count changes — so the old buffers end up the
    // wrong size, causing memory corruption on the next i2s_write.
    // FIX #10: the 4092-byte SLC descriptor limit must be enforced for
    // channel-only changes too, not only for bit-depth changes.
    // FIX R1: use `orig_*` (pre-mutation) values here, not the just-updated
    // p_i2s_obj[->channel_num].
    bool need_rebuild = (bits != orig_bits_per_sample) || (orig_channel_num != ch);

    if (need_rebuild)
    {
        // Update bits state first, so i2s_create_dma_queue sees the new sample size.
        if (bits != p_i2s_obj[i2s_num]->bits_per_sample)
        {
            // change fifo mode between 16-bit and 24-bit family (the high bit
            // of fifo_mod distinguishes these). Recompute from scratch to keep
            // the single/dual bit intact and avoid walking out of range.
            bool is_24 = (bits > 16);
            bool was_24 = (p_i2s_obj[i2s_num]->bits_per_sample > 16);
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
            // FIX #20: clamp.
            if (tx_new > 5)
                tx_new = 5;
            if (rx_new > 5)
                rx_new = 5;
            I2S[i2s_num]->fifo_conf.tx_fifo_mod = tx_new;
            I2S[i2s_num]->fifo_conf.rx_fifo_mod = rx_new;

            I2S_MEMW(); // FIX: ensure fifo mode change completes before rebuilding DMA

            p_i2s_obj[i2s_num]->bits_per_sample = bits;
            p_i2s_obj[i2s_num]->bytes_per_sample = (bits == I2S_BITS_PER_SAMPLE_16BIT) ? 2 : 4;
        }

        // FIX N2: re-evaluate the 4092-byte SLC descriptor limit from the
        // ORIGINAL user-requested dma_buf_len, not from the (possibly already
        // clamped) dma_buf_len. This lets the buffer grow back when bytes or
        // channel count decrease. Without this, one clamp (e.g. 24-bit stereo)
        // permanently shrinks the buffer even after switching to 16-bit mono.
        p_i2s_obj[i2s_num]->dma_buf_len = p_i2s_obj[i2s_num]->dma_buf_len_orig;
        if (p_i2s_obj[i2s_num]->dma_buf_len * p_i2s_obj[i2s_num]->bytes_per_sample * p_i2s_obj[i2s_num]->channel_num > 4092)
        {
            p_i2s_obj[i2s_num]->dma_buf_len = 4092 / p_i2s_obj[i2s_num]->bytes_per_sample / p_i2s_obj[i2s_num]->channel_num;
        }

        // Re-create TX DMA buffer
        if (p_i2s_obj[i2s_num]->mode & I2S_MODE_TX)
        {
            save_tx = p_i2s_obj[i2s_num]->tx;

            p_i2s_obj[i2s_num]->tx = i2s_create_dma_queue(i2s_num, p_i2s_obj[i2s_num]->dma_buf_count, p_i2s_obj[i2s_num]->dma_buf_len);

            if (p_i2s_obj[i2s_num]->tx == NULL)
            {
                ESP_LOGE(I2S_TAG, "Failed to create tx dma buffer");
                // FIX R2: Give the muxes BEFORE destroying save_tx/save_rx.
                // tx_mux_taken points to the SAME mux as save_tx->mux (we took
                // it from the old p_i2s_obj[->tx]->mux at function entry), and
                // rx_mux_taken points to save_rx->mux. Calling vSemaphoreDelete
                // inside i2s_destroy_dma_queue frees the mux; Calling Give after
                // that is use-after-free. Give first, NULL the handles, then destroy.
                if (tx_mux_taken)
                {
                    xSemaphoreGive(tx_mux_taken);
                    tx_mux_taken = NULL;
                }
                if (rx_mux_taken)
                {
                    xSemaphoreGive(rx_mux_taken);
                    rx_mux_taken = NULL;
                }
                // FIX #6: destroy save_tx explicitly so it is not leaked;
                // i2s_driver_uninstall only knows about p_i2s_obj[->tx] (now NULL).
                if (save_tx)
                    i2s_destroy_dma_queue(i2s_num, save_tx);
                if (save_rx)
                    i2s_destroy_dma_queue(i2s_num, save_rx);
                i2s_driver_uninstall(i2s_num);
                return ESP_ERR_NO_MEM;
            }

            p_i2s_obj[i2s_num]->dma->rx_link.addr = (uint32_t)p_i2s_obj[i2s_num]->tx->desc[0];

            // destroy old tx dma if exist. FIX #4: we held the OLD mux; give it
            // BEFORE destroying so vSemaphoreDelete does not race with our hold.
            if (save_tx)
            {
                if (tx_mux_taken)
                {
                    xSemaphoreGive(tx_mux_taken);
                    tx_mux_taken = NULL;
                }
                i2s_destroy_dma_queue(i2s_num, save_tx);
                save_tx = NULL;
            }
        }

        // Re-create RX DMA buffer
        if (p_i2s_obj[i2s_num]->mode & I2S_MODE_RX)
        {
            save_rx = p_i2s_obj[i2s_num]->rx;

            p_i2s_obj[i2s_num]->rx = i2s_create_dma_queue(i2s_num, p_i2s_obj[i2s_num]->dma_buf_count, p_i2s_obj[i2s_num]->dma_buf_len);

            if (p_i2s_obj[i2s_num]->rx == NULL)
            {
                ESP_LOGE(I2S_TAG, "Failed to create rx dma buffer");
                // FIX R2: Give the muxes BEFORE destroying save_rx/save_tx (UAF).
                if (tx_mux_taken)
                {
                    xSemaphoreGive(tx_mux_taken);
                    tx_mux_taken = NULL;
                }
                if (rx_mux_taken)
                {
                    xSemaphoreGive(rx_mux_taken);
                    rx_mux_taken = NULL;
                }
                // FIX #6: destroy save_rx explicitly so it is not leaked.
                if (save_rx)
                    i2s_destroy_dma_queue(i2s_num, save_rx);
                if (save_tx)
                    i2s_destroy_dma_queue(i2s_num, save_tx); // safety; normally NULL here
                i2s_driver_uninstall(i2s_num);
                return ESP_ERR_NO_MEM;
            }

            I2S[i2s_num]->rx_eof_num = (p_i2s_obj[i2s_num]->dma_buf_len * p_i2s_obj[i2s_num]->channel_num * p_i2s_obj[i2s_num]->bytes_per_sample) / 4;
            p_i2s_obj[i2s_num]->dma->tx_link.addr = (uint32_t)p_i2s_obj[i2s_num]->rx->desc[0];

            // destroy old rx dma if exist. FIX #4: give the OLD mux before destroy.
            if (save_rx)
            {
                if (rx_mux_taken)
                {
                    xSemaphoreGive(rx_mux_taken);
                    rx_mux_taken = NULL;
                }
                i2s_destroy_dma_queue(i2s_num, save_rx);
                save_rx = NULL;
            }
        }
    }

    /* FIX (GROK-1.2): bits_mod = "extra bits per slot beyond 16". The stock
     * SDK sets `bits_mod = bits` (16 or 24), but on ESP8266 this produces
     * right-justified samples in the 32-bit DMA word (low bits used). The
     * streamer's i2s_capture.c expects LEFT-justified 24-bit samples (high
     * bits used, low 8 bits = 0x00 padding) and extracts them via
     * `buf[i] >>= 8`. The custom `bits_mod = 0/8` setting produces exactly
     * this left-justified layout:
     *   - 16-bit: bits_mod=0 -> 16-bit slot, no padding
     *   - 24-bit: bits_mod=8 -> 24-bit slot LEFT-justified in 32-bit DMA word
     * Verified by AT+DUMP hex output: low 8 bits are always 0x00 (padding),
     * confirming the left-justified layout. Switching to the stock
     * `bits_mod = bits` would BREAK the streamer's `>>8` extraction.
     * See i2s_capture.c:17-20 and i2s_capture.h:24-29 for the verification. */
    I2S[i2s_num]->conf.bits_mod = bits == I2S_BITS_PER_SAMPLE_16BIT ? 0 : 8;

    // FIX #5: Give the mutexes we actually Took. If a queue was rebuilt, the
    // corresponding `*_mux_taken` was already Given (and set to NULL) inside
    // the rebuild block above, so we don't double-Give.
    if (tx_mux_taken)
    {
        xSemaphoreGive(tx_mux_taken);
    }
    if (rx_mux_taken)
    {
        xSemaphoreGive(rx_mux_taken);
    }

    i2s_start(i2s_num);
    return ESP_OK;
}

esp_err_t i2s_set_sample_rates(i2s_port_t i2s_num, uint32_t rate)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    // FIX #1: if driver was not installed yet, p_i2s_obj[i2s_num] is NULL and the
    // next I2S_CHECK would deref it. Other functions check this; this one didn't.
    I2S_CHECK(p_i2s_obj[i2s_num], "i2s not installed yet", ESP_FAIL);
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
    I2S_MEMW(); // ensure FIFO reset completes before I2S reset
    // reset i2s
    I2S[i2s_num]->conf.tx_reset = 1;
    I2S[i2s_num]->conf.tx_reset = 0;
    I2S[i2s_num]->conf.rx_reset = 1;
    I2S[i2s_num]->conf.rx_reset = 0;
    I2S_MEMW(); // ensure I2S reset completes before config

    // disable all i2s interrupt
    I2S[i2s_num]->int_ena.val = 0;

    // reset dma
    p_i2s_obj[i2s_num]->dma->conf0.rx_rst = 1;
    p_i2s_obj[i2s_num]->dma->conf0.rx_rst = 0;
    p_i2s_obj[i2s_num]->dma->conf0.tx_rst = 1;
    p_i2s_obj[i2s_num]->dma->conf0.tx_rst = 0;
    I2S_MEMW(); // ensure DMA reset completes before config

    // Enable and configure DMA
    p_i2s_obj[i2s_num]->dma->conf0.txdata_burst_en = 0;
    p_i2s_obj[i2s_num]->dma->conf0.txdscr_burst_en = 1;
    p_i2s_obj[i2s_num]->dma->rx_dscr_conf.rx_fill_mode = 0;
    p_i2s_obj[i2s_num]->dma->rx_dscr_conf.rx_eof_mode = 0;
    p_i2s_obj[i2s_num]->dma->rx_dscr_conf.rx_fill_en = 0;
    p_i2s_obj[i2s_num]->dma->rx_dscr_conf.token_no_replace = 1;
    p_i2s_obj[i2s_num]->dma->rx_dscr_conf.infor_no_replace = 1;
    I2S_MEMW(); // FIX: ensure DMA config completes before disconnecting
    I2S[i2s_num]->fifo_conf.dscr_en = 0;
    I2S_MEMW(); // FIX: ensure DMA is disconnected before changing FIFO/chan config
    // FIX N5: cf -> chan_mod mapping per TRM §10.2.1.4. The previous formula
    // `cf < ONLY_RIGHT ? cf : (cf >> 1)` collapsed cf=3 (ONLY_RIGHT) to
    // chan_mod=1 (ALL_RIGHT) and cf=4 (ONLY_LEFT) to chan_mod=2 (ALL_LEFT),
    // which is incorrect for TX. Per TRM:
    //   cf=0 RIGHT_LEFT  -> chan_mod=0 (dual: real L + real R)
    //   cf=1 ALL_RIGHT   -> chan_mod=1 (R in both slots)
    //   cf=2 ALL_LEFT    -> chan_mod=2 (L in both slots)
    //   cf=3 ONLY_RIGHT  -> chan_mod=3 (R + constant from regfile in L slot)
    //   cf=4 ONLY_LEFT   -> chan_mod=4 (L + constant from regfile in R slot)
    //
    // FIX N5-REGRESS (v4.4): IMPORTANT — TX and RX use DIFFERENT bit widths!
    //   tx_chan_mod: bits 0-2 (3 bits, values 0-4 documented)
    //   rx_chan_mod: bits 3-4 (2 bits, values 0-2 documented; 3 reserved)
    //
    // My original N5 fix set BOTH registers to `cf`. For cf=ONLY_LEFT (4),
    // rx_chan_mod=4 gets truncated to 0 (dual mode) by the 2-bit field mask.
    // This caused RX to sample BOTH slots while fifo_mod was single, ending up
    // storing the R-slot data — which is zeros for an INMP441 mic tied for
    // LEFT-only capture. Symptom: i2s_read returns all zeros in ONLY_LEFT mode.
    //
    // Fix: TX keeps chan_mod=cf (supports 0-4). RX uses the original
    // (cf < ONLY_RIGHT) ? cf : (cf >> 1) formula (RX only supports 0-2, and
    // ONLY_RIGHT/ONLY_LEFT correctly map to 1/2 = Right/Left sample-only).
    uint32_t tx_chan_mod_init = (uint32_t)i2s_config->channel_format;
    uint32_t rx_chan_mod_init = (i2s_config->channel_format < I2S_CHANNEL_FMT_ONLY_RIGHT)
                                    ? (uint32_t)i2s_config->channel_format
                                    : (uint32_t)(i2s_config->channel_format >> 1);
    I2S[i2s_num]->conf_chan.tx_chan_mod = tx_chan_mod_init;
    // FIX M2 (v4.5 corrected): fifo_mod controls how many samples per FIFO
    // entry the DMA supplies:
    //   0 (dual): 2 samples per frame (L + R slots both supplied by DMA)
    //   1 (single): 1 sample per frame (second slot filled by I2S peripheral
    //               from regfile constant)
    //
    // Per TRM §10.2.1.4 and user clarification:
    //   - RIGHT_LEFT (cf=0): stereo, 2 different samples per frame -> dual (0)
    //   - ALL_RIGHT  (cf=1): 2 slots, both with right-channel data -> dual (0)
    //     (DMA supplies 2 samples, both are right-channel data)
    //   - ALL_LEFT   (cf=2): 2 slots, both with left-channel data  -> dual (0)
    //   - ONLY_RIGHT (cf=3): 1 real slot + 1 regfile constant      -> single (1)
    //   - ONLY_LEFT  (cf=4): 1 real slot + 1 regfile constant      -> single (1)
    //
    // My original M2 (v4.1) wrongly set fifo_mod=1 (single) for ALL_* modes,
    // thinking "if both slots have same data, DMA can supply 1 sample and I2S
    // duplicates it". This is incorrect — the I2S peripheral does NOT duplicate;
    // it takes 2 samples from the FIFO and sends them to both slots. The user's
    // code must supply 2 identical samples for ALL_* modes.
    //
    // The correct formula matches the original SDK behaviour:
    //   fifo_mod = (cf < ONLY_RIGHT) ? 0 (dual) : 1 (single)
    I2S[i2s_num]->fifo_conf.tx_fifo_mod = (i2s_config->channel_format < I2S_CHANNEL_FMT_ONLY_RIGHT) ? 0 : 1;

    I2S[i2s_num]->conf_chan.rx_chan_mod = rx_chan_mod_init;
    I2S[i2s_num]->fifo_conf.rx_fifo_mod = (i2s_config->channel_format < I2S_CHANNEL_FMT_ONLY_RIGHT) ? 0 : 1;
    I2S_MEMW(); // ensure channel/fifo config completes before enabling DMA

    I2S[i2s_num]->fifo_conf.dscr_en = 1; // connect dma to fifo
    I2S_MEMW();                          // ensure DMA connection is established before continuing

    I2S[i2s_num]->conf.tx_start = 0;
    I2S[i2s_num]->conf.rx_start = 0;

    I2S[i2s_num]->conf.msb_right = 1;
    I2S[i2s_num]->conf.right_first = 1;

    if (i2s_config->mode & I2S_MODE_TX)
    {
        I2S[i2s_num]->conf.tx_slave_mod = 0; // Master

        if (i2s_config->mode & I2S_MODE_SLAVE)
        {
            I2S[i2s_num]->conf.tx_slave_mod = 1; // TX Slave
        }
    }

    if (i2s_config->mode & I2S_MODE_RX)
    {
        I2S[i2s_num]->conf.rx_slave_mod = 0; // Master

        if (i2s_config->mode & I2S_MODE_SLAVE)
        {
            I2S[i2s_num]->conf.rx_slave_mod = 1; // RX Slave
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

    if ((p_i2s_obj[i2s_num]->mode & I2S_MODE_RX) && (p_i2s_obj[i2s_num]->mode & I2S_MODE_TX))
    {
        if (p_i2s_obj[i2s_num]->mode & I2S_MODE_MASTER)
        {
            I2S[i2s_num]->conf.tx_slave_mod = 0; // TX Master
            I2S[i2s_num]->conf.rx_slave_mod = 0; // RX Master
        }
        else
        {
            I2S[i2s_num]->conf.tx_slave_mod = 1; // TX Slave
            I2S[i2s_num]->conf.rx_slave_mod = 1; // RX Slave
        }
    }

    p_i2s_obj[i2s_num]->tx_desc_auto_clear = i2s_config->tx_desc_auto_clear;
    return ESP_OK;
}

esp_err_t i2s_zero_dma_buffer(i2s_port_t i2s_num)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK(p_i2s_obj[i2s_num], "i2s not installed yet", ESP_FAIL);

    /* FIX (GROK-1.6-review): the mux only guards against parallel app
     * access (i2s_read/i2s_write). It does NOT stop the SLC DMA from
     * walking buf[i] in the background — memset'ing a buffer the DMA is
     * currently reading/writing produces clicks on TX and corrupted
     * samples on RX. Stock SDK has the same hole.
     *
     * Fix: stop the DMA hardware first (i2s_stop drains pending
     * descriptors via i2s_drain_pending_descriptors), zero under mux,
     * then restart. The brief audio gap is acceptable — callers use this
     * either before the first write (no real data yet) or to recover
     * from underflow (silence is the goal anyway). */
    i2s_stop(i2s_num);

    // FIX #17: take rx->mux while memset'ing RX buffers, otherwise a parallel
    //          i2s_read() can be reading these very buffers via curr_ptr.
    if (p_i2s_obj[i2s_num]->rx && p_i2s_obj[i2s_num]->rx->buf != NULL && p_i2s_obj[i2s_num]->rx->buf_size != 0)
    {
        xSemaphoreTake(p_i2s_obj[i2s_num]->rx->mux, (portTickType)portMAX_DELAY);
        for (int i = 0; i < p_i2s_obj[i2s_num]->dma_buf_count; i++)
        {
            memset(p_i2s_obj[i2s_num]->rx->buf[i], 0, p_i2s_obj[i2s_num]->rx->buf_size);
        }
        p_i2s_obj[i2s_num]->rx->rw_pos = 0;
        p_i2s_obj[i2s_num]->rx->curr_ptr = NULL;
        xSemaphoreGive(p_i2s_obj[i2s_num]->rx->mux);
    }

    if (p_i2s_obj[i2s_num]->tx && p_i2s_obj[i2s_num]->tx->buf != NULL && p_i2s_obj[i2s_num]->tx->buf_size != 0)
    {
        // FIX #17: take tx->mux for the same reason.
        xSemaphoreTake(p_i2s_obj[i2s_num]->tx->mux, (portTickType)portMAX_DELAY);

        // FIX R4: removed the dead `align_bytes` computation — it was a leftover
        // from the deleted i2s_write() alignment call (fixes #18/#19) and served
        // no purpose. The memset below zeroes ALL buffers; rw_pos is reset to 0.
        for (int i = 0; i < p_i2s_obj[i2s_num]->dma_buf_count; i++)
        {
            memset(p_i2s_obj[i2s_num]->tx->buf[i], 0, p_i2s_obj[i2s_num]->tx->buf_size);
        }
        p_i2s_obj[i2s_num]->tx->rw_pos = 0;
        p_i2s_obj[i2s_num]->tx->curr_ptr = NULL;
        xSemaphoreGive(p_i2s_obj[i2s_num]->tx->mux);
    }

    i2s_start(i2s_num);
    return ESP_OK;
}

esp_err_t i2s_write(i2s_port_t i2s_num, const void *src, size_t size, size_t *bytes_written, TickType_t ticks_to_wait)
{
    char *data_ptr, *src_byte;
    // FIX N8: use size_t (matches the type of `size` and of buf_size) to avoid
    // signed/unsigned comparison warnings with -Wsign-compare.
    size_t bytes_can_write;
    // FIX #22: bytes_written may be NULL; guard against the deref below.
    I2S_CHECK(bytes_written, "bytes_written is NULL", ESP_ERR_INVALID_ARG);
    *bytes_written = 0;
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    // FIX #2: p_i2s_obj[i2s_num] may be NULL if driver not installed yet.
    I2S_CHECK(p_i2s_obj[i2s_num], "i2s not installed yet", ESP_FAIL);
    I2S_CHECK((size < I2S_MAX_BUFFER_SIZE), "size is too large", ESP_ERR_INVALID_ARG);
    I2S_CHECK((p_i2s_obj[i2s_num]->tx), "tx NULL", ESP_ERR_INVALID_ARG);
    /* FIX (GROK-1.6-review): NULL src with size > 0 reaches memcpy below
     * and throws LoadProhibited. size == 0 with NULL src is safe (loop
     * skipped). */
    I2S_CHECK((src != NULL || size == 0), "src is NULL", ESP_ERR_INVALID_ARG);
    xSemaphoreTake(p_i2s_obj[i2s_num]->tx->mux, (portTickType)portMAX_DELAY);

    src_byte = (char *)src;

    while (size > 0)
    {
        if (p_i2s_obj[i2s_num]->tx->rw_pos == p_i2s_obj[i2s_num]->tx->buf_size || p_i2s_obj[i2s_num]->tx->curr_ptr == NULL)
        {
            if (xQueueReceive(p_i2s_obj[i2s_num]->tx->queue, &p_i2s_obj[i2s_num]->tx->curr_ptr, ticks_to_wait) == pdFALSE)
            {
                break;
            }

            p_i2s_obj[i2s_num]->tx->rw_pos = 0;
        }

        // FIX R9: %u + (unsigned) cast for `size` (size_t) to avoid -Wformat
        // warning that was introduced/remaining after N8 changed bytes_can_write
        // to size_t. Using %zu would be C99-correct but is not universally
        // supported by embedded toolchains, so we use the portable %u+cast form.
        ESP_LOGD(I2S_TAG, "size: %u, rw_pos: %d, buf_size: %d, curr_ptr: %d",
                 (unsigned)size,
                 p_i2s_obj[i2s_num]->tx->rw_pos,
                 p_i2s_obj[i2s_num]->tx->buf_size,
                 (int)p_i2s_obj[i2s_num]->tx->curr_ptr);
        data_ptr = (char *)p_i2s_obj[i2s_num]->tx->curr_ptr;
        data_ptr += p_i2s_obj[i2s_num]->tx->rw_pos;
        bytes_can_write = (size_t)(p_i2s_obj[i2s_num]->tx->buf_size - p_i2s_obj[i2s_num]->tx->rw_pos);

        if (bytes_can_write > size)
        {
            bytes_can_write = size;
        }

        memcpy(data_ptr, src_byte, bytes_can_write);
        size -= bytes_can_write;
        src_byte += bytes_can_write;
        p_i2s_obj[i2s_num]->tx->rw_pos += (int)bytes_can_write;
        (*bytes_written) += bytes_can_write;
    }

    xSemaphoreGive(p_i2s_obj[i2s_num]->tx->mux);
    return ESP_OK;
}

esp_err_t i2s_read(i2s_port_t i2s_num, void *dest, size_t size, size_t *bytes_read, TickType_t ticks_to_wait)
{
    char *data_ptr, *dest_byte;
    // FIX N8: use size_t to avoid signed/unsigned comparison warnings.
    size_t bytes_can_read;
    // FIX #22: bytes_read may be NULL; guard against the deref below.
    I2S_CHECK(bytes_read, "bytes_read is NULL", ESP_ERR_INVALID_ARG);
    *bytes_read = 0;
    dest_byte = (char *)dest;
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    // FIX #3: p_i2s_obj[i2s_num] may be NULL if driver not installed yet.
    I2S_CHECK(p_i2s_obj[i2s_num], "i2s not installed yet", ESP_FAIL);
    I2S_CHECK((size < I2S_MAX_BUFFER_SIZE), "size is too large", ESP_ERR_INVALID_ARG);
    I2S_CHECK((p_i2s_obj[i2s_num]->rx), "rx NULL", ESP_ERR_INVALID_ARG);
    /* FIX (GROK-1.6-review): NULL dest with size > 0 reaches memcpy below
     * and throws StoreProhibited. size == 0 with NULL dest is safe. */
    I2S_CHECK((dest != NULL || size == 0), "dest is NULL", ESP_ERR_INVALID_ARG);
    xSemaphoreTake(p_i2s_obj[i2s_num]->rx->mux, (portTickType)portMAX_DELAY);

    while (size > 0)
    {
        if (p_i2s_obj[i2s_num]->rx->rw_pos == p_i2s_obj[i2s_num]->rx->buf_size || p_i2s_obj[i2s_num]->rx->curr_ptr == NULL)
        {
            if (xQueueReceive(p_i2s_obj[i2s_num]->rx->queue, &p_i2s_obj[i2s_num]->rx->curr_ptr, ticks_to_wait) == pdFALSE)
            {
                break;
            }

            p_i2s_obj[i2s_num]->rx->rw_pos = 0;
        }

        data_ptr = (char *)p_i2s_obj[i2s_num]->rx->curr_ptr;
        data_ptr += p_i2s_obj[i2s_num]->rx->rw_pos;
        bytes_can_read = (size_t)(p_i2s_obj[i2s_num]->rx->buf_size - p_i2s_obj[i2s_num]->rx->rw_pos);

        if (bytes_can_read > size)
        {
            bytes_can_read = size;
        }

        memcpy(dest_byte, data_ptr, bytes_can_read);
        size -= bytes_can_read;
        dest_byte += bytes_can_read;
        p_i2s_obj[i2s_num]->rx->rw_pos += (int)bytes_can_read;
        (*bytes_read) += bytes_can_read;
    }

    xSemaphoreGive(p_i2s_obj[i2s_num]->rx->mux);
    return ESP_OK;
}

esp_err_t i2s_driver_uninstall(i2s_port_t i2s_num)
{
    I2S_CHECK((i2s_num < I2S_NUM_MAX), "i2s_num error", ESP_ERR_INVALID_ARG);
    I2S_CHECK(p_i2s_obj[i2s_num], "already uninstalled", ESP_FAIL);

    /* FIX (GROK-1.6): take the TX/RX muxes with a timeout BEFORE destroying
     * the queues. Previously uninstall did NOT take the muxes at all — if a
     * reader was blocked in i2s_read() holding rx->mux (inside the
     * xQueueReceive(ticks_to_wait) call), the subsequent i2s_destroy_dma_queue
     * would vQueueDelete(rx->queue) + vSemaphoreDelete(rx->mux) out from under
     * the blocked reader → FreeRTOS undefined behavior (use-after-free of queue
     * memory, mutex corruption). The streamer mitigates this via
     * wait_for_task_exit before calling i2s_capture_deinit → uninstall, but
     * a force-delete of the I2S task mid-read (HOTRESTART edge case) could
     * still trigger the UB window.
     *
     * Now: try to take each mux with a 2s timeout. If the mux is held by a
     * blocked reader, we refuse uninstall (return ESP_ERR_TIMEOUT) so the
     * caller can handle the stuck reader explicitly (e.g. force-delete the
     * task, then retry). If the mux is free (normal case), we proceed. */
    SemaphoreHandle_t tx_m = (p_i2s_obj[i2s_num]->tx) ? p_i2s_obj[i2s_num]->tx->mux : NULL;
    SemaphoreHandle_t rx_m = (p_i2s_obj[i2s_num]->rx) ? p_i2s_obj[i2s_num]->rx->mux : NULL;
    BaseType_t tx_taken = pdFALSE, rx_taken = pdFALSE;
    if (tx_m)
    {
        if (xSemaphoreTake(tx_m, pdMS_TO_TICKS(2000)) != pdTRUE)
        {
            ESP_LOGE(I2S_TAG, "uninstall: tx mux busy (writer stuck?) — refusing uninstall");
            return ESP_ERR_TIMEOUT;
        }
        tx_taken = pdTRUE;
    }
    if (rx_m)
    {
        if (xSemaphoreTake(rx_m, pdMS_TO_TICKS(2000)) != pdTRUE)
        {
            ESP_LOGE(I2S_TAG, "uninstall: rx mux busy (reader stuck?) — refusing uninstall");
            if (tx_taken) xSemaphoreGive(tx_m);
            return ESP_ERR_TIMEOUT;
        }
        rx_taken = pdTRUE;
    }

    i2s_stop(i2s_num);

    /* FIX (GROK-1.6-review): Give the mutexes BEFORE i2s_destroy_dma_queue
     * calls vSemaphoreDelete on them. FreeRTOS rule: "Do not delete a
     * mutex type semaphore if the mutex is held by a task." We took
     * tx_m/rx_m above and never Gave them — vSemaphoreDelete on a held
     * mutex corrupts priority-inheritance state and the holder TCB.
     * This also fixes the same bug on the i2s_set_clk error-path that
     * calls i2s_driver_uninstall (when i2s_create_dma_queue fails, the
     * uninstall path takes rx_m and destroys it without Give).
     *
     * Note: there is a pre-existing race if i2s_write/i2s_read is running
     * concurrently (they could take the mux after Give and operate on
     * the queue before destroy frees it). The streamer mitigates this
     * via wait_for_task_exit before calling uninstall. A full fix
     * requires the architectural state-lock change (see CHANGES.md
     * "Known limitations"). */
    if (tx_taken) { xSemaphoreGive(tx_m); tx_taken = pdFALSE; }
    if (rx_taken) { xSemaphoreGive(rx_m); rx_taken = pdFALSE; }

    dma_intr_register(NULL, NULL);

    // 1. Stop I2S peripheral (stop sending DMA requests)
    I2S[i2s_num]->conf.tx_reset = 1;
    I2S[i2s_num]->conf.tx_reset = 0;
    I2S[i2s_num]->conf.rx_reset = 1;
    I2S[i2s_num]->conf.rx_reset = 0;

    I2S_MEMW();

    // 2. Reset SLC DMA (force-abort current descriptor)
    SLC0.conf0.tx_rst = 1;
    SLC0.conf0.tx_rst = 0;
    SLC0.conf0.rx_rst = 1;
    SLC0.conf0.rx_rst = 0;

    I2S_MEMW();

    /* Disable BBPLL audio clock output
   rom_i2c_writeReg_Mask(0x67, 4, 4, 7, 7, 0);
   you can't do this here, it can break other peripherals when uninstalling*/

    if (p_i2s_obj[i2s_num]->tx != NULL && p_i2s_obj[i2s_num]->mode & I2S_MODE_TX)
    {
        i2s_destroy_dma_queue(i2s_num, p_i2s_obj[i2s_num]->tx);
        p_i2s_obj[i2s_num]->tx = NULL;
    }

    if (p_i2s_obj[i2s_num]->rx != NULL && p_i2s_obj[i2s_num]->mode & I2S_MODE_RX)
    {
        i2s_destroy_dma_queue(i2s_num, p_i2s_obj[i2s_num]->rx);
        p_i2s_obj[i2s_num]->rx = NULL;
    }

    if (p_i2s_obj[i2s_num]->i2s_queue)
    {
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
    // FIX #23: if user wants an event queue, queue_size must be positive.
    if (i2s_queue != NULL)
    {
        I2S_CHECK((queue_size > 0), "queue_size must be > 0 when i2s_queue is provided", ESP_ERR_INVALID_ARG);
    }

    if (p_i2s_obj[i2s_num] == NULL)
    {
        p_i2s_obj[i2s_num] = (i2s_obj_t *)heap_caps_zalloc(sizeof(i2s_obj_t), MALLOC_CAP_8BIT);
        I2S_CHECK(p_i2s_obj[i2s_num], "Malloc I2S driver error", ESP_ERR_NO_MEM);

        p_i2s_obj[i2s_num]->i2s_num = i2s_num;
        p_i2s_obj[i2s_num]->dma = (slc_struct_t *)&SLC0;
        p_i2s_obj[i2s_num]->dma_buf_count = i2s_config->dma_buf_count;
        p_i2s_obj[i2s_num]->dma_buf_len = i2s_config->dma_buf_len;
        // FIX N2: remember the user-requested dma_buf_len so that the 4092-byte
        // clamp in i2s_set_clk can be re-evaluated (and grown back) on every
        // bits/ch change, instead of permanently shrinking.
        p_i2s_obj[i2s_num]->dma_buf_len_orig = i2s_config->dma_buf_len;
        // FIX #24: removed dead `p_i2s_obj[i2s_num]->i2s_queue = i2s_queue;`
        // assignment. It wrote a void* into a QueueHandle_t field (type mismatch)
        // and was unconditionally overwritten below (real handle or NULL).
        p_i2s_obj[i2s_num]->mode = i2s_config->mode;

        p_i2s_obj[i2s_num]->bits_per_sample = 0;
        p_i2s_obj[i2s_num]->bytes_per_sample = 0; // Not initialized yet
        // FIX N6: I2S channel count. For RIGHT_LEFT/ALL_RIGHT/ALL_LEFT the I2S
        // peripheral still produces 2 slots per frame (one is duplicated or
        // filled from regfile), so the DMA must supply 2 samples per frame
        // -> channel_num = 2. For ONLY_RIGHT/ONLY_LEFT the second slot is
        // filled from regfile (constant), so the DMA only needs 1 sample per
        // frame -> channel_num = 1. The old formula
        // `cf < ONLY_RIGHT ? 2 : 1` incorrectly set channel_num=1 for
        // ALL_RIGHT/ALL_LEFT, causing DMA buffers to be half the required size.
        p_i2s_obj[i2s_num]->channel_num =
            (i2s_config->channel_format == I2S_CHANNEL_FMT_ONLY_RIGHT ||
             i2s_config->channel_format == I2S_CHANNEL_FMT_ONLY_LEFT)
                ? 1
                : 2;
        // FIX #14: remember the original channel_format so i2s_set_clk can
        // restore conf_chan correctly on later channel/bit reconfigurations.
        p_i2s_obj[i2s_num]->channel_format = i2s_config->channel_format;

        // initial interrupt
        dma_intr_register(i2s_intr_handler_default, p_i2s_obj[i2s_num]);
        /* Enable BBPLL audio clock output — TRM §10.2.1.2:
         * "To start the I2S module, firstly you need to provide a running clock" */

        if (esp_wifi_get_state() == WIFI_STATE_DEINIT)
        {
            rom_i2c_writeReg_Mask(0x67, 4, 4, 7, 7, 1);
            I2S_MEMW(); // ensure BBPLL enable completes before I2S config
        }
        
        i2s_stop(i2s_num);

        err = i2s_param_config(i2s_num, i2s_config);

        if (err != ESP_OK)
        {
            i2s_driver_uninstall(i2s_num);
            ESP_LOGE(I2S_TAG, "I2S param configure error");
            return err;
        }

        if (i2s_queue)
        {
            p_i2s_obj[i2s_num]->i2s_queue = xQueueCreate(queue_size, sizeof(i2s_event_t));
            // FIX #23: xQueueCreate can return NULL on OOM. Without this check,
            // we would write NULL into the user's QueueHandle_t and the ISR
            // would silently fail to enqueue events (no crash, but data loss).
            if (p_i2s_obj[i2s_num]->i2s_queue == NULL)
            {
                ESP_LOGE(I2S_TAG, "Failed to create i2s event queue");
                i2s_driver_uninstall(i2s_num);
                return ESP_ERR_NO_MEM;
            }
            *((QueueHandle_t *)i2s_queue) = p_i2s_obj[i2s_num]->i2s_queue;
            ESP_LOGI(I2S_TAG, "queue heap_caps_free spaces: %d", (int)uxQueueSpacesAvailable(p_i2s_obj[i2s_num]->i2s_queue));
        }
        else
        {
            p_i2s_obj[i2s_num]->i2s_queue = NULL;
        }

        // set clock and start
        // FIX N10: if i2s_set_clk fails (e.g. bits=12 fails the bits % 8 check,
        // or rate=0 fails the rate > 0 check), we must clean up the partially-
        // installed driver. Otherwise p_i2s_obj leaks and subsequent install
        // calls hit the "already installed" path with no way to recover.
        // FIX R8: i2s_set_clk's own error-paths already call i2s_driver_uninstall
        // (so p_i2s_obj[i2s_num] becomes NULL). We must NOT call it again
        // unconditionally — that triggers an "already uninstalled" ERROR log.
        // Guard the second call with a NULL check.
        esp_err_t set_clk_err = i2s_set_clk(i2s_num, i2s_config->sample_rate, i2s_config->bits_per_sample, p_i2s_obj[i2s_num]->channel_num);
        if (set_clk_err != ESP_OK)
        {
            ESP_LOGE(I2S_TAG, "I2S set_clk failed during install: 0x%x", (int)set_clk_err);
            if (p_i2s_obj[i2s_num] != NULL)
            {
                i2s_driver_uninstall(i2s_num);
            }
            return set_clk_err;
        }
        return ESP_OK;
    }

    ESP_LOGW(I2S_TAG, "I2S driver already installed");
    return ESP_OK;
}

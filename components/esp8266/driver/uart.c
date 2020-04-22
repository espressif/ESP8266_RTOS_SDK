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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/ringbuf.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_attr.h"

#include "esp8266/uart_struct.h"
#include "esp8266/uart_register.h"
#include "esp8266/pin_mux_register.h"
#include "esp8266/eagle_soc.h"
#include "esp8266/rom_functions.h"

#include "rom/ets_sys.h"

#include "driver/uart.h"
#include "driver/uart_select.h"

#define portYIELD_FROM_ISR() taskYIELD()

#define UART_ENTER_CRITICAL()    portENTER_CRITICAL()
#define UART_EXIT_CRITICAL()     portEXIT_CRITICAL()

static const char *UART_TAG = "uart";
#define UART_CHECK(a, str, ret_val) \
    if (!(a)) { \
        ESP_LOGE(UART_TAG,"%s(%d): %s", __FUNCTION__, __LINE__, str); \
        return (ret_val); \
    }

#define UART_EMPTY_THRESH_DEFAULT  (10)
#define UART_FULL_THRESH_DEFAULT  (120)
#define UART_TOUT_THRESH_DEFAULT   (10)

typedef struct {
    uart_event_type_t type;        /*!< UART TX data type */
    struct {
        size_t size;
        uint8_t data[0];
    } tx_data;
} uart_tx_data_t;

typedef struct {
    uart_port_t uart_num;               /*!< UART port number*/
    int queue_size;                     /*!< UART event queue size*/
    QueueHandle_t xQueueUart;           /*!< UART queue handler*/
    uart_mode_t uart_mode;              /*!< UART controller actual mode set by uart_set_mode() */

    // rx parameters
    int rx_buffered_len;                  /*!< UART cached data length */
    SemaphoreHandle_t rx_mux;           /*!< UART RX data mutex*/
    int rx_buf_size;                    /*!< RX ring buffer size */
    RingbufHandle_t rx_ring_buf;        /*!< RX ring buffer handler*/
    bool rx_buffer_full_flg;            /*!< RX ring buffer full flag. */
    int rx_cur_remain;                  /*!< Data number that waiting to be read out in ring buffer item*/
    uint8_t *rx_ptr;                    /*!< pointer to the current data in ring buffer*/
    uint8_t *rx_head_ptr;               /*!< pointer to the head of RX item*/
    uint8_t rx_data_buf[UART_FIFO_LEN]; /*!< Data buffer to stash FIFO data*/
    uint8_t rx_stash_len;               /*!< stashed data length.(When using flow control, after reading out FIFO data, if we fail to push to buffer, we can just stash them.) */

    // tx parameters
    SemaphoreHandle_t tx_fifo_sem;      /*!< UART TX FIFO semaphore*/
    SemaphoreHandle_t tx_done_sem;      /*!< UART TX done semaphore*/
    SemaphoreHandle_t tx_mux;           /*!< UART TX mutex*/
    int tx_buf_size;                    /*!< TX ring buffer size */
    RingbufHandle_t tx_ring_buf;        /*!< TX ring buffer handler*/
    bool tx_waiting_fifo;               /*!< this flag indicates that some task is waiting for FIFO empty interrupt, used to send all data without any data buffer*/
    uint8_t *tx_ptr;                    /*!< TX data pointer to push to FIFO in TX buffer mode*/
    uart_tx_data_t *tx_head;            /*!< TX data pointer to head of the current buffer in TX ring buffer*/
    uint32_t tx_len_tot;                /*!< Total length of current item in ring buffer*/
    uint32_t tx_len_cur;
    bool wait_tx_done_flg;
    uart_select_notif_callback_t uart_select_notif_callback; /*!< Notification about select() events */
} uart_obj_t;

static uart_obj_t *p_uart_obj[UART_NUM_MAX] = {0};
// DRAM_ATTR is required to avoid UART array placed in flash, due to accessed from ISR
static DRAM_ATTR uart_dev_t *const UART[UART_NUM_MAX] = {&uart0, &uart1};

typedef void (*uart_isr_t)(void *);
typedef struct {
    uart_isr_t fn;   /*!< isr function */
    void *args;      /*!< isr function args */
} uart_isr_func_t;

static uart_isr_func_t uart_isr_func[UART_NUM_MAX];


esp_err_t uart_set_word_length(uart_port_t uart_num, uart_word_length_t data_bit)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_ERR_INVALID_ARG);
    UART_CHECK((data_bit < UART_DATA_BITS_MAX), "data bit error", ESP_ERR_INVALID_ARG);

    UART_ENTER_CRITICAL();
    UART[uart_num]->conf0.bit_num = data_bit;
    UART_EXIT_CRITICAL();
    return ESP_OK;
}

esp_err_t uart_get_word_length(uart_port_t uart_num, uart_word_length_t *data_bit)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_ERR_INVALID_ARG);
    UART_CHECK((data_bit), "empty pointer", ESP_FAIL);

    *(data_bit) = UART[uart_num]->conf0.bit_num;
    return ESP_OK;
}

esp_err_t uart_set_stop_bits(uart_port_t uart_num, uart_stop_bits_t stop_bit)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_ERR_INVALID_ARG);
    UART_CHECK((stop_bit < UART_STOP_BITS_MAX), "stop bit error", ESP_ERR_INVALID_ARG);

    UART_ENTER_CRITICAL();
    UART[uart_num]->conf0.stop_bit_num = stop_bit;
    UART_EXIT_CRITICAL();
    return ESP_OK;
}

esp_err_t uart_get_stop_bits(uart_port_t uart_num, uart_stop_bits_t *stop_bit)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_ERR_INVALID_ARG);
    UART_CHECK((stop_bit), "empty pointer", ESP_FAIL);

    (*stop_bit) = UART[uart_num]->conf0.stop_bit_num;
    return ESP_OK;
}

esp_err_t uart_set_parity(uart_port_t uart_num, uart_parity_t parity_mode)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_ERR_INVALID_ARG);
    UART_CHECK(((parity_mode == UART_PARITY_DISABLE) || (parity_mode == UART_PARITY_EVEN) || (parity_mode == UART_PARITY_ODD)),
               "parity_mode error", ESP_ERR_INVALID_ARG);

    UART_ENTER_CRITICAL();
    UART[uart_num]->conf0.parity = (parity_mode & 0x1);
    UART[uart_num]->conf0.parity_en = ((parity_mode >> 1) & 0x1);
    UART_EXIT_CRITICAL();
    return ESP_OK;
}

esp_err_t uart_get_parity(uart_port_t uart_num, uart_parity_t *parity_mode)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_ERR_INVALID_ARG);
    UART_CHECK((parity_mode), "empty pointer", ESP_ERR_INVALID_ARG);

    UART_ENTER_CRITICAL();

    if (UART[uart_num]->conf0.parity_en) {
        if (UART[uart_num]->conf0.parity) {
            (*parity_mode) = UART_PARITY_ODD;
        } else {
            (*parity_mode) = UART_PARITY_EVEN;
        }
    } else {
        (*parity_mode) = UART_PARITY_DISABLE;
    }

    UART_EXIT_CRITICAL();
    return ESP_OK;
}

esp_err_t uart_set_baudrate(uart_port_t uart_num, uint32_t baud_rate)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_ERR_INVALID_ARG);

    UART_ENTER_CRITICAL();
    UART[uart_num]->clk_div.val = (uint32_t)(UART_CLK_FREQ / baud_rate) & 0xFFFFF;
    UART_EXIT_CRITICAL();
    return ESP_OK;
}

esp_err_t uart_get_baudrate(uart_port_t uart_num, uint32_t *baudrate)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_ERR_INVALID_ARG);
    UART_CHECK((baudrate), "empty pointer", ESP_ERR_INVALID_ARG);

    (*baudrate) = (UART_CLK_FREQ / (UART[uart_num]->clk_div.val & 0xFFFFF));
    return ESP_OK;
}

esp_err_t uart_set_line_inverse(uart_port_t uart_num, uint32_t inverse_mask)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_ERR_INVALID_ARG);
    UART_CHECK((((inverse_mask & ~UART_LINE_INV_MASK) == 0) || (inverse_mask == 0)), "inverse_mask error", ESP_ERR_INVALID_ARG);

    UART_ENTER_CRITICAL();
    UART[uart_num]->conf0.val &= ~UART_LINE_INV_MASK;
    UART[uart_num]->conf0.val |= inverse_mask;
    UART_EXIT_CRITICAL();
    return ESP_OK;
}

esp_err_t uart_set_hw_flow_ctrl(uart_port_t uart_num, uart_hw_flowcontrol_t flow_ctrl, uint8_t rx_thresh)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_ERR_INVALID_ARG);
    UART_CHECK((flow_ctrl < UART_HW_FLOWCTRL_MAX), "uart_flow ctrl error", ESP_ERR_INVALID_ARG);

    UART_ENTER_CRITICAL();

    if (flow_ctrl & UART_HW_FLOWCTRL_RTS) {
        UART[uart_num]->conf1.rx_flow_thrhd = rx_thresh;
        UART[uart_num]->conf1.rx_flow_en = 1;
    } else {
        UART[uart_num]->conf1.rx_flow_en = 0;
    }

    if (flow_ctrl & UART_HW_FLOWCTRL_CTS) {
        UART[uart_num]->conf0.tx_flow_en = 1;
    } else {
        UART[uart_num]->conf0.tx_flow_en = 0;
    }

    UART_EXIT_CRITICAL();
    return ESP_OK;
}

esp_err_t uart_get_hw_flow_ctrl(uart_port_t uart_num, uart_hw_flowcontrol_t *flow_ctrl)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_ERR_INVALID_ARG);

    uart_hw_flowcontrol_t val = UART_HW_FLOWCTRL_DISABLE;

    if (UART[uart_num]->conf1.rx_flow_en) {
        val |= UART_HW_FLOWCTRL_RTS;
    }

    if (UART[uart_num]->conf0.tx_flow_en) {
        val |= UART_HW_FLOWCTRL_CTS;
    }

    (*flow_ctrl) = val;
    return ESP_OK;
}

esp_err_t uart_wait_tx_done(uart_port_t uart_num, TickType_t ticks_to_wait)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_ERR_INVALID_ARG);
    UART_CHECK((p_uart_obj[uart_num]), "uart driver error", ESP_ERR_INVALID_ARG);
    uint32_t baudrate;
    uint32_t byte_delay_us = 0;
    BaseType_t res;
    portTickType ticks_end = xTaskGetTickCount() + ticks_to_wait;

    // Take tx_mux
    res = xSemaphoreTake(p_uart_obj[uart_num]->tx_mux, (portTickType)ticks_to_wait);
    if(res == pdFALSE) {
        return ESP_ERR_TIMEOUT;
    }

    if(false == p_uart_obj[uart_num]->wait_tx_done_flg) {
        xSemaphoreGive(p_uart_obj[uart_num]->tx_mux);
        return ESP_OK;
    }

    uart_get_baudrate(uart_num, &baudrate);
    byte_delay_us = (uint32_t)(10000000 / baudrate); // (1/baudrate)*10*1000_000 us

    ticks_to_wait = ticks_end - xTaskGetTickCount();
    // wait for tx done sem.
    if (pdTRUE == xSemaphoreTake(p_uart_obj[uart_num]->tx_done_sem, ticks_to_wait)) {
        while (1) {
            if (UART[uart_num]->status.txfifo_cnt == 0) {
                ets_delay_us(byte_delay_us); // Delay one byte time to guarantee transmission completion 
                break;
            }
        }
        p_uart_obj[uart_num]->wait_tx_done_flg = false;
    } else {
        xSemaphoreGive(p_uart_obj[uart_num]->tx_mux);
        return ESP_ERR_TIMEOUT;
    }
    xSemaphoreGive(p_uart_obj[uart_num]->tx_mux);
    return ESP_OK;
}

esp_err_t uart_enable_swap(void)
{
    // wait for tx done.
    uart_wait_tx_done(UART_NUM_0, portMAX_DELAY);

    UART_ENTER_CRITICAL();
    // MTCK -> UART0_CTS -> U0RXD
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_UART0_CTS);
    // MTD0 -> UART0_RTS -> U0TXD
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_UART0_RTS);
    // enable swap U0TXD <-> UART0_RTS and U0RXD <-> UART0_CTS
    SET_PERI_REG_MASK(UART_SWAP_REG, 0x4);
    UART_EXIT_CRITICAL();

    return ESP_OK;
}

esp_err_t uart_disable_swap(void)
{
    // wait for tx done.
    uart_wait_tx_done(UART_NUM_0, portMAX_DELAY);

    UART_ENTER_CRITICAL();
    // disable swap U0TXD <-> UART0_RTS and U0RXD <-> UART0_CTS
    CLEAR_PERI_REG_MASK(UART_SWAP_REG, 0x4);
    UART_EXIT_CRITICAL();

    return ESP_OK;
}

static esp_err_t uart_reset_rx_fifo(uart_port_t uart_num)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_ERR_INVALID_ARG);

    UART_ENTER_CRITICAL();
    UART[uart_num]->conf0.rxfifo_rst = 0x1;
    UART[uart_num]->conf0.rxfifo_rst = 0x0;
    UART_EXIT_CRITICAL();
    return ESP_OK;
}

esp_err_t uart_clear_intr_status(uart_port_t uart_num, uint32_t mask)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_ERR_INVALID_ARG);

    UART_ENTER_CRITICAL();
    UART[uart_num]->int_clr.val |= mask;
    UART_EXIT_CRITICAL();
    return ESP_OK;
}


esp_err_t uart_enable_intr_mask(uart_port_t uart_num, uint32_t enable_mask)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_ERR_INVALID_ARG);

    UART_ENTER_CRITICAL();
    UART[uart_num]->int_ena.val |= enable_mask;
    UART_EXIT_CRITICAL();
    return ESP_OK;
}

esp_err_t uart_disable_intr_mask(uart_port_t uart_num, uint32_t disable_mask)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_ERR_INVALID_ARG);

    UART_ENTER_CRITICAL();
    UART[uart_num]->int_ena.val &= ~disable_mask;
    UART_EXIT_CRITICAL();
    return ESP_OK;
}

esp_err_t uart_enable_rx_intr(uart_port_t uart_num)
{
    return uart_enable_intr_mask(uart_num, UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_TOUT_INT_ENA);
}

esp_err_t uart_disable_rx_intr(uart_port_t uart_num)
{
    return uart_disable_intr_mask(uart_num, UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_TOUT_INT_ENA);
}

esp_err_t uart_disable_tx_intr(uart_port_t uart_num)
{
    return uart_disable_intr_mask(uart_num, UART_TXFIFO_EMPTY_INT_ENA);
}

esp_err_t uart_enable_tx_intr(uart_port_t uart_num, int enable, int thresh)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_ERR_INVALID_ARG);
    UART_CHECK((thresh < UART_FIFO_LEN), "empty intr threshold error", ESP_ERR_INVALID_ARG);

    UART_ENTER_CRITICAL();
    UART[uart_num]->int_clr.txfifo_empty = 1;
    UART[uart_num]->conf1.txfifo_empty_thrhd = thresh & 0x7f;
    UART[uart_num]->int_ena.txfifo_empty = enable & 0x1;
    UART_EXIT_CRITICAL();
    return ESP_OK;
}

static void uart_intr_service(void *arg)
{
    // UART intr process
    uint32_t uart_num = 0;
    // read status to get interrupt status for UART0-1
    uint32_t uart_intr_status = UART[uart_num]->int_st.val;

    if (uart_isr_func == NULL) {
        return;
    }

    do {
        uart_intr_status = UART[uart_num]->int_st.val;
        if (uart_intr_status != 0) {
            if (uart_isr_func[uart_num].fn != NULL) {
                uart_isr_func[uart_num].fn(uart_isr_func[uart_num].args);
            }
        }
    } while (++uart_num < UART_NUM_MAX);
}


esp_err_t uart_isr_register(uart_port_t uart_num, void (*fn)(void *), void *arg)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_ERR_INVALID_ARG);

    for (int num = 0; num < UART_NUM_MAX; num++) {
        if (p_uart_obj[num] == NULL) {
            uart_disable_intr_mask(num, UART_INTR_MASK);
        }
    }
    UART_ENTER_CRITICAL();
    _xt_isr_mask(1 << ETS_UART_INUM);
    _xt_isr_attach(ETS_UART_INUM, uart_intr_service, NULL);
    uart_isr_func[uart_num].fn = fn;
    uart_isr_func[uart_num].args = arg;
    _xt_isr_unmask(1 << ETS_UART_INUM);
    UART_EXIT_CRITICAL();
    return ESP_OK;
}

esp_err_t uart_param_config(uart_port_t uart_num,  uart_config_t *uart_conf)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_ERR_INVALID_ARG);

    if (uart_num == UART_NUM_1) {
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_U1TXD_BK);
    } else {
        PIN_PULLUP_DIS(PERIPHS_IO_MUX_U0TXD_U);
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_U0RXD);
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD);

        if (uart_conf->flow_ctrl & UART_HW_FLOWCTRL_RTS) {
            PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_U0RTS);
        }

        if (uart_conf->flow_ctrl & UART_HW_FLOWCTRL_CTS) {
            PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_UART0_CTS);
        }

        uart_set_hw_flow_ctrl(uart_num, uart_conf->flow_ctrl, uart_conf->rx_flow_ctrl_thresh);
    }

    uart_set_baudrate(uart_num, uart_conf->baud_rate);
    uart_set_word_length(uart_num, uart_conf->data_bits);
    uart_set_stop_bits(uart_num, uart_conf->stop_bits);
    uart_set_parity(uart_num, uart_conf->parity);
    uart_reset_rx_fifo(uart_num);

    return ESP_OK;
}

esp_err_t uart_intr_config(uart_port_t uart_num,  uart_intr_config_t *intr_conf)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_ERR_INVALID_ARG);

    uart_clear_intr_status(uart_num, UART_INTR_MASK);
    UART_ENTER_CRITICAL();
    UART[uart_num]->int_clr.val = UART_INTR_MASK;

    if (intr_conf->intr_enable_mask & UART_RXFIFO_TOUT_INT_ENA_M) {
        UART[uart_num]->conf1.rx_tout_thrhd = ((intr_conf->rx_timeout_thresh) & 0x7f);
        UART[uart_num]->conf1.rx_tout_en = 1;
    } else {
        UART[uart_num]->conf1.rx_tout_en = 0;
    }

    if (intr_conf->intr_enable_mask & UART_RXFIFO_FULL_INT_ENA_M) {
        UART[uart_num]->conf1.rxfifo_full_thrhd = intr_conf->rxfifo_full_thresh;
    }

    if (intr_conf->intr_enable_mask & UART_TXFIFO_EMPTY_INT_ENA_M) {
        UART[uart_num]->conf1.txfifo_empty_thrhd = intr_conf->txfifo_empty_intr_thresh;
    }

    // uart_clear_intr_status(UART[uart_num], mask);
    UART[uart_num]->int_ena.val = intr_conf->intr_enable_mask;
    _xt_isr_unmask(0x1 << ETS_UART_INUM);
    UART_EXIT_CRITICAL();

    return ESP_OK;
}

// internal isr handler for default driver code.
static void uart_rx_intr_handler_default(void *param)
{
    uart_obj_t *p_uart = (uart_obj_t *) param;
    uint8_t uart_num = p_uart->uart_num;
    uart_dev_t *uart_reg = UART[uart_num];
    int rx_fifo_len = uart_reg->status.rxfifo_cnt;
    uint8_t buf_idx = 0;
    uint32_t uart_intr_status = UART[uart_num]->int_st.val;
    uart_event_t uart_event;
    BaseType_t task_woken = 0;

    while (uart_intr_status != 0x0) {
        uart_select_notif_t notify = UART_SELECT_ERROR_NOTIF;

        buf_idx = 0;
        uart_event.type = UART_EVENT_MAX;

        if (uart_intr_status & UART_TXFIFO_EMPTY_INT_ST_M) {
            uart_clear_intr_status(uart_num, UART_TXFIFO_EMPTY_INT_CLR_M);
            uart_disable_intr_mask(uart_num, UART_TXFIFO_EMPTY_INT_ENA_M);

            // TX semaphore will only be used when tx_buf_size is zero.
            if (p_uart->tx_waiting_fifo == true && p_uart->tx_buf_size == 0) {
                p_uart->tx_waiting_fifo = false;
                xSemaphoreGiveFromISR(p_uart->tx_fifo_sem, &task_woken);

                if (task_woken == pdTRUE) {
                    portYIELD_FROM_ISR();
                }
            } else {
                // We don't use TX ring buffer, because the size is zero.
                if (p_uart->tx_buf_size == 0) {
                    continue;
                }

                int tx_fifo_rem = UART_FIFO_LEN - UART[uart_num]->status.txfifo_cnt;
                bool en_tx_flg = false;

                // We need to put a loop here, in case all the buffer items are very short.
                // That would cause a watch_dog reset because empty interrupt happens so often.
                // Although this is a loop in ISR, this loop will execute at most 128 turns.
                while (tx_fifo_rem) {
                    if (p_uart->tx_len_tot == 0 || p_uart->tx_ptr == NULL || p_uart->tx_len_cur == 0) {
                        size_t size;
                        p_uart->tx_head = (uart_tx_data_t *) xRingbufferReceiveFromISR(p_uart->tx_ring_buf, &size);

                        if (p_uart->tx_head) {
                            // The first item is the data description
                            // Get the first item to get the data information
                            if (p_uart->tx_len_tot == 0) {
                                p_uart->tx_ptr = NULL;
                                p_uart->tx_len_tot = p_uart->tx_head->tx_data.size;
                                // We have saved the data description from the 1st item, return buffer.
                                vRingbufferReturnItemFromISR(p_uart->tx_ring_buf, p_uart->tx_head, &task_woken);

                                if (task_woken == pdTRUE) {
                                    portYIELD_FROM_ISR();
                                }
                            } else if (p_uart->tx_ptr == NULL) {
                                // Update the TX item pointer, we will need this to return item to buffer.
                                p_uart->tx_ptr = (uint8_t *) p_uart->tx_head;
                                en_tx_flg = true;
                                p_uart->tx_len_cur = size;
                            }
                        } else {
                            // Can not get data from ring buffer, return;
                            break;
                        }
                    }

                    if (p_uart->tx_len_tot > 0 && p_uart->tx_ptr && p_uart->tx_len_cur > 0) {
                        // To fill the TX FIFO.
                        int send_len = p_uart->tx_len_cur > tx_fifo_rem ? tx_fifo_rem : p_uart->tx_len_cur;

                        for (buf_idx = 0; buf_idx < send_len; buf_idx++) {
                            UART[uart_num]->fifo.rw_byte = *(p_uart->tx_ptr++) & 0xff;
                        }

                        p_uart->tx_len_tot -= send_len;
                        p_uart->tx_len_cur -= send_len;
                        tx_fifo_rem -= send_len;

                        if (p_uart->tx_len_cur == 0) {
                            // Return item to ring buffer.
                            vRingbufferReturnItemFromISR(p_uart->tx_ring_buf, p_uart->tx_head, &task_woken);

                            if (task_woken == pdTRUE) {
                                portYIELD_FROM_ISR();
                            }

                            p_uart->tx_head = NULL;
                            p_uart->tx_ptr = NULL;
                        }

                        if (p_uart->tx_len_tot == 0) {
                            if (tx_fifo_rem == 0) {
                                en_tx_flg = true;
                            } else{
                                en_tx_flg = false;
                            }
                            xSemaphoreGiveFromISR(p_uart->tx_done_sem, &task_woken);
                            if (task_woken == pdTRUE) {
                                portYIELD_FROM_ISR();
                            }
                        } else {
                            en_tx_flg = true;
                        }
                    }
                }

                if (en_tx_flg) {
                    uart_clear_intr_status(uart_num, UART_TXFIFO_EMPTY_INT_CLR_M);
                    uart_enable_intr_mask(uart_num, UART_TXFIFO_EMPTY_INT_ENA_M);
                }
            }
        } else if ((uart_intr_status & UART_RXFIFO_TOUT_INT_ST_M)
                   || (uart_intr_status & UART_RXFIFO_FULL_INT_ST_M)
                  ) {
            rx_fifo_len = uart_reg->status.rxfifo_cnt;

            if (p_uart->rx_buffer_full_flg == false) {
                // We have to read out all data in RX FIFO to clear the interrupt signal
                while (buf_idx < rx_fifo_len) {
                    p_uart->rx_data_buf[buf_idx++] = uart_reg->fifo.rw_byte;
                }

                // Get the buffer from the FIFO
                // After Copying the Data From FIFO ,Clear intr_status
                uart_clear_intr_status(uart_num, UART_RXFIFO_TOUT_INT_CLR_M | UART_RXFIFO_FULL_INT_CLR_M);
                uart_event.type = UART_DATA;
                uart_event.size = rx_fifo_len;
                p_uart->rx_stash_len = rx_fifo_len;

                // If we fail to push data to ring buffer, we will have to stash the data, and send next time.
                // Mainly for applications that uses flow control or small ring buffer.
                if (pdFALSE == xRingbufferSendFromISR(p_uart->rx_ring_buf, p_uart->rx_data_buf, p_uart->rx_stash_len, &task_woken)) {
                    uart_disable_intr_mask(uart_num, UART_RXFIFO_TOUT_INT_ENA_M | UART_RXFIFO_FULL_INT_ENA_M);
                    uart_event.type = UART_BUFFER_FULL;
                    p_uart->rx_buffer_full_flg = true;
                } else {
                    p_uart->rx_buffered_len += p_uart->rx_stash_len;
                }

                notify = UART_SELECT_READ_NOTIF;

                if (task_woken == pdTRUE) {
                    portYIELD_FROM_ISR();
                }
            } else {
                uart_disable_intr_mask(uart_num, UART_RXFIFO_FULL_INT_ENA_M | UART_RXFIFO_TOUT_INT_ENA_M);
                uart_clear_intr_status(uart_num, UART_RXFIFO_FULL_INT_CLR_M | UART_RXFIFO_TOUT_INT_CLR_M);
            }
        } else if (uart_intr_status & UART_RXFIFO_OVF_INT_ST_M) {
            // When fifo overflows, we reset the fifo.
            uart_reset_rx_fifo(uart_num);
            uart_reg->int_clr.rxfifo_ovf = 1;
            uart_event.type = UART_FIFO_OVF;
            notify = UART_SELECT_ERROR_NOTIF;
        } else if (uart_intr_status & UART_FRM_ERR_INT_ST_M) {
            uart_reg->int_clr.frm_err = 1;
            uart_event.type = UART_FRAME_ERR;
            notify = UART_SELECT_ERROR_NOTIF;
        } else if (uart_intr_status & UART_PARITY_ERR_INT_ST_M) {
            uart_reg->int_clr.parity_err = 1;
            uart_event.type = UART_PARITY_ERR;
            notify = UART_SELECT_ERROR_NOTIF;
        } else {
            uart_reg->int_clr.val = uart_intr_status; // simply clear all other intr status
            uart_event.type = UART_EVENT_MAX;
            notify = UART_SELECT_ERROR_NOTIF;
        }

#ifdef CONFIG_USING_ESP_VFS
        if (uart_event.type != UART_EVENT_MAX && p_uart->uart_select_notif_callback) {
            p_uart->uart_select_notif_callback(uart_num, notify, &task_woken);
            if (task_woken == pdTRUE) {
                portYIELD_FROM_ISR();
            }
        }
#else
        (void)notify;
#endif

        if (uart_event.type != UART_EVENT_MAX && p_uart->xQueueUart) {
            if (pdFALSE == xQueueSendFromISR(p_uart->xQueueUart, (void *)&uart_event, &task_woken)) {
                ESP_EARLY_LOGV(UART_TAG, "UART event queue full");
            }

            if (task_woken == pdTRUE) {
                portYIELD_FROM_ISR();
            }
        }

        uart_intr_status = uart_reg->int_st.val;
    }
}

// Fill UART tx_fifo and return a number,
// This function by itself is not thread-safe, always call from within a muxed section.
static int uart_fill_fifo(uart_port_t uart_num, const char *buffer, uint32_t len)
{
    uint8_t i = 0;
    uint8_t tx_fifo_cnt = UART[uart_num]->status.txfifo_cnt;
    uint8_t tx_remain_fifo_cnt = (UART_FIFO_LEN - tx_fifo_cnt);
    uint8_t copy_cnt = (len >= tx_remain_fifo_cnt ? tx_remain_fifo_cnt : len);

    for (i = 0; i < copy_cnt; i++) {
        UART[uart_num]->fifo.rw_byte = buffer[i];
    }

    return copy_cnt;
}

int uart_tx_chars(uart_port_t uart_num, const char *buffer, uint32_t len)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", (-1));
    UART_CHECK((p_uart_obj[uart_num]), "uart driver error", (-1));
    UART_CHECK(buffer, "buffer null", (-1));

    if (len == 0) {
        return 0;
    }

    xSemaphoreTake(p_uart_obj[uart_num]->tx_mux, (portTickType)portMAX_DELAY);
    int tx_len = uart_fill_fifo(uart_num, (const char *) buffer, len);
    xSemaphoreGive(p_uart_obj[uart_num]->tx_mux);
    return tx_len;
}

static int uart_tx_all(uart_port_t uart_num, const char *src, size_t size)
{
    if (size == 0) {
        return 0;
    }

    size_t original_size = size;

    // lock for uart_tx
    xSemaphoreTake(p_uart_obj[uart_num]->tx_mux, (portTickType)portMAX_DELAY);
    p_uart_obj[uart_num]->wait_tx_done_flg = true;
    if (p_uart_obj[uart_num]->tx_buf_size > 0) {
        int max_size = xRingbufferGetMaxItemSize(p_uart_obj[uart_num]->tx_ring_buf);
        int offset = 0;
        uart_tx_data_t evt;
        evt.tx_data.size = size;
        evt.type = UART_DATA;
        xRingbufferSend(p_uart_obj[uart_num]->tx_ring_buf, (void *) &evt, sizeof(uart_tx_data_t), portMAX_DELAY);

        while (size > 0) {
            int send_size = size > max_size / 2 ? max_size / 2 : size;
            xRingbufferSend(p_uart_obj[uart_num]->tx_ring_buf, (void *)(src + offset), send_size, portMAX_DELAY);
            size -= send_size;
            offset += send_size;
            uart_enable_tx_intr(uart_num, 1, UART_EMPTY_THRESH_DEFAULT);
        }
    } else {
        while (size) {
            // semaphore for tx_fifo available
            if (pdTRUE == xSemaphoreTake(p_uart_obj[uart_num]->tx_fifo_sem, (portTickType)portMAX_DELAY)) {
                size_t sent = uart_fill_fifo(uart_num, (char *) src, size);

                if (sent < size) {
                    p_uart_obj[uart_num]->tx_waiting_fifo = true;
                    uart_enable_tx_intr(uart_num, 1, UART_EMPTY_THRESH_DEFAULT);
                }

                size -= sent;
                src += sent;
            }
        }

        xSemaphoreGive(p_uart_obj[uart_num]->tx_fifo_sem);
        xSemaphoreGive(p_uart_obj[uart_num]->tx_done_sem);
    }
    xSemaphoreGive(p_uart_obj[uart_num]->tx_mux);
    return original_size;
}

int uart_write_bytes(uart_port_t uart_num, const char *src, size_t size)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", (-1));
    UART_CHECK((p_uart_obj[uart_num]), "uart driver error", (-1));
    UART_CHECK(src, "buffer null", (-1));

    return uart_tx_all(uart_num, src, size);
}

int uart_read_bytes(uart_port_t uart_num, uint8_t *buf, uint32_t length, TickType_t ticks_to_wait)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", (-1));
    UART_CHECK((buf), "uart data null", (-1));
    UART_CHECK((p_uart_obj[uart_num]), "uart driver error", (-1));

    uint8_t *data = NULL;
    size_t size;
    size_t copy_len = 0;
    int len_tmp;

    if (xSemaphoreTake(p_uart_obj[uart_num]->rx_mux, (portTickType)ticks_to_wait) != pdTRUE) {
        return -1;
    }

    while (length) {
        if (p_uart_obj[uart_num]->rx_cur_remain == 0) {
            data = (uint8_t *) xRingbufferReceive(p_uart_obj[uart_num]->rx_ring_buf, &size, (portTickType) ticks_to_wait);

            if (data) {
                p_uart_obj[uart_num]->rx_head_ptr = data;
                p_uart_obj[uart_num]->rx_ptr = data;
                p_uart_obj[uart_num]->rx_cur_remain = size;
            } else {
                xSemaphoreGive(p_uart_obj[uart_num]->rx_mux);
                return copy_len;
            }
        }

        if (p_uart_obj[uart_num]->rx_cur_remain > length) {
            len_tmp = length;
        } else {
            len_tmp = p_uart_obj[uart_num]->rx_cur_remain;
        }

        memcpy(buf + copy_len, p_uart_obj[uart_num]->rx_ptr, len_tmp);
        UART_ENTER_CRITICAL();
        p_uart_obj[uart_num]->rx_buffered_len -= len_tmp;
        p_uart_obj[uart_num]->rx_ptr += len_tmp;
        UART_EXIT_CRITICAL();
        p_uart_obj[uart_num]->rx_cur_remain -= len_tmp;
        copy_len += len_tmp;
        length -= len_tmp;

        if (p_uart_obj[uart_num]->rx_cur_remain == 0) {
            vRingbufferReturnItem(p_uart_obj[uart_num]->rx_ring_buf, p_uart_obj[uart_num]->rx_head_ptr);
            p_uart_obj[uart_num]->rx_head_ptr = NULL;
            p_uart_obj[uart_num]->rx_ptr = NULL;

            if (p_uart_obj[uart_num]->rx_buffer_full_flg) {
                BaseType_t res = xRingbufferSend(p_uart_obj[uart_num]->rx_ring_buf, p_uart_obj[uart_num]->rx_data_buf, p_uart_obj[uart_num]->rx_stash_len, 1);

                if (res == pdTRUE) {
                    UART_ENTER_CRITICAL();
                    p_uart_obj[uart_num]->rx_buffered_len += p_uart_obj[uart_num]->rx_stash_len;
                    p_uart_obj[uart_num]->rx_buffer_full_flg = false;
                    UART_EXIT_CRITICAL();
                    uart_enable_rx_intr(p_uart_obj[uart_num]->uart_num);
                }
            }
        }
    }

    xSemaphoreGive(p_uart_obj[uart_num]->rx_mux);
    return copy_len;
}

esp_err_t uart_get_buffered_data_len(uart_port_t uart_num, size_t *size)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_ERR_INVALID_ARG);
    UART_CHECK((p_uart_obj[uart_num]), "uart driver error", ESP_ERR_INVALID_ARG);

    *size = p_uart_obj[uart_num]->rx_buffered_len;
    return ESP_OK;
}

esp_err_t uart_flush(uart_port_t uart_num) __attribute__((alias("uart_flush_input")));

esp_err_t uart_flush_input(uart_port_t uart_num)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_ERR_INVALID_ARG);
    UART_CHECK((p_uart_obj[uart_num]), "uart driver error", ESP_ERR_INVALID_ARG);
    uart_obj_t *p_uart = p_uart_obj[uart_num];
    uint8_t *data;
    size_t size;

    // rx sem protect the ring buffer read related functions
    xSemaphoreTake(p_uart->rx_mux, (portTickType)portMAX_DELAY);
    uart_disable_rx_intr(p_uart_obj[uart_num]->uart_num);

    while (true) {
        if (p_uart->rx_head_ptr) {
            vRingbufferReturnItem(p_uart->rx_ring_buf, p_uart->rx_head_ptr);
            UART_ENTER_CRITICAL();
            p_uart_obj[uart_num]->rx_buffered_len -= p_uart->rx_cur_remain;
            UART_EXIT_CRITICAL();
            p_uart->rx_ptr = NULL;
            p_uart->rx_cur_remain = 0;
            p_uart->rx_head_ptr = NULL;
        }

        data = (uint8_t *) xRingbufferReceive(p_uart->rx_ring_buf, &size, (portTickType) 0);

        if (data == NULL) {
            if (p_uart_obj[uart_num]->rx_buffered_len != 0) {
                ESP_LOGE(UART_TAG, "rx_buffered_len error");
                p_uart_obj[uart_num]->rx_buffered_len = 0;
            }

            // We also need to clear the `rx_buffer_full_flg` here.
            UART_ENTER_CRITICAL();
            p_uart_obj[uart_num]->rx_buffer_full_flg = false;
            UART_EXIT_CRITICAL();
            break;
        }

        UART_ENTER_CRITICAL();
        p_uart_obj[uart_num]->rx_buffered_len -= size;
        UART_EXIT_CRITICAL();
        vRingbufferReturnItem(p_uart->rx_ring_buf, data);

        if (p_uart_obj[uart_num]->rx_buffer_full_flg) {
            BaseType_t res = xRingbufferSend(p_uart_obj[uart_num]->rx_ring_buf, p_uart_obj[uart_num]->rx_data_buf, p_uart_obj[uart_num]->rx_stash_len, 1);

            if (res == pdTRUE) {
                UART_ENTER_CRITICAL();
                p_uart_obj[uart_num]->rx_buffered_len += p_uart_obj[uart_num]->rx_stash_len;
                p_uart_obj[uart_num]->rx_buffer_full_flg = false;
                UART_EXIT_CRITICAL();
            }
        }
    }

    p_uart->rx_ptr = NULL;
    p_uart->rx_cur_remain = 0;
    p_uart->rx_head_ptr = NULL;
    uart_reset_rx_fifo(uart_num);
    uart_enable_rx_intr(p_uart_obj[uart_num]->uart_num);
    xSemaphoreGive(p_uart->rx_mux);
    return ESP_OK;
}

esp_err_t uart_driver_install(uart_port_t uart_num, int rx_buffer_size, int tx_buffer_size, int queue_size, QueueHandle_t *uart_queue, int no_use)
{
    esp_err_t r;
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_ERR_INVALID_ARG);
    UART_CHECK((rx_buffer_size > UART_FIFO_LEN) || ((uart_num == UART_NUM_1) && (rx_buffer_size == 0)), "uart rx buffer length error(>128)", ESP_ERR_INVALID_ARG);
    UART_CHECK((tx_buffer_size > UART_FIFO_LEN) || (tx_buffer_size == 0), "uart tx buffer length error(>128 or 0)", ESP_ERR_INVALID_ARG);
    UART_CHECK((queue_size >= 0), "queue_size error(>=0)", ESP_ERR_INVALID_ARG);

    if (p_uart_obj[uart_num] == NULL) {
        p_uart_obj[uart_num] = (uart_obj_t *) calloc(1, sizeof(uart_obj_t));

        if (p_uart_obj[uart_num] == NULL) {
            ESP_LOGE(UART_TAG, "UART driver malloc error");
            return ESP_FAIL;
        }

        p_uart_obj[uart_num]->uart_num = uart_num;
        p_uart_obj[uart_num]->uart_mode = UART_MODE_UART;
        p_uart_obj[uart_num]->tx_fifo_sem = xSemaphoreCreateBinary();
        p_uart_obj[uart_num]->tx_done_sem = xSemaphoreCreateBinary();
        xSemaphoreGive(p_uart_obj[uart_num]->tx_fifo_sem);
        p_uart_obj[uart_num]->tx_mux = xSemaphoreCreateMutex();
        p_uart_obj[uart_num]->rx_mux = xSemaphoreCreateMutex();
        p_uart_obj[uart_num]->queue_size = queue_size;
        p_uart_obj[uart_num]->tx_ptr = NULL;
        p_uart_obj[uart_num]->tx_head = NULL;
        p_uart_obj[uart_num]->tx_len_tot = 0;
        p_uart_obj[uart_num]->rx_buffered_len = 0;
        p_uart_obj[uart_num]->wait_tx_done_flg = false;

        if (uart_queue) {
            p_uart_obj[uart_num]->xQueueUart = xQueueCreate(queue_size, sizeof(uart_event_t));
            *uart_queue = p_uart_obj[uart_num]->xQueueUart;
            ESP_LOGI(UART_TAG, "queue free spaces: %d", (int)uxQueueSpacesAvailable(p_uart_obj[uart_num]->xQueueUart));
        } else {
            p_uart_obj[uart_num]->xQueueUart = NULL;
        }

        p_uart_obj[uart_num]->rx_buffer_full_flg = false;
        p_uart_obj[uart_num]->tx_waiting_fifo = false;
        p_uart_obj[uart_num]->rx_ptr = NULL;
        p_uart_obj[uart_num]->rx_cur_remain = 0;
        p_uart_obj[uart_num]->rx_head_ptr = NULL;
        p_uart_obj[uart_num]->rx_ring_buf = xRingbufferCreate(rx_buffer_size, RINGBUF_TYPE_BYTEBUF);
        p_uart_obj[uart_num]->rx_buf_size = rx_buffer_size;

        if (tx_buffer_size > 0) {
            p_uart_obj[uart_num]->tx_ring_buf = xRingbufferCreate(tx_buffer_size, RINGBUF_TYPE_NOSPLIT);
            p_uart_obj[uart_num]->tx_buf_size = tx_buffer_size;
        } else {
            p_uart_obj[uart_num]->tx_ring_buf = NULL;
            p_uart_obj[uart_num]->tx_buf_size = 0;
        }

        p_uart_obj[uart_num]->uart_select_notif_callback = NULL;
    } else {
        ESP_LOGE(UART_TAG, "UART driver already installed");
        return ESP_FAIL;
    }

    r = uart_isr_register(uart_num, uart_rx_intr_handler_default, p_uart_obj[uart_num]);

    if (r != ESP_OK) {
        goto err;
    }

    uart_intr_config_t uart_intr = {
        .intr_enable_mask = UART_RXFIFO_FULL_INT_ENA_M
        | UART_RXFIFO_TOUT_INT_ENA_M
        | UART_FRM_ERR_INT_ENA_M
        | UART_RXFIFO_OVF_INT_ENA_M,
        .rxfifo_full_thresh = UART_FULL_THRESH_DEFAULT,
        .rx_timeout_thresh = UART_TOUT_THRESH_DEFAULT,
        .txfifo_empty_intr_thresh = UART_EMPTY_THRESH_DEFAULT
    };
    r = uart_intr_config(uart_num, &uart_intr);

    if (r != ESP_OK) {
        goto err;
    }

    return r;

err:
    ESP_LOGE(UART_TAG, "driver install error");
    uart_driver_delete(uart_num);
    return r;
}

// Make sure no other tasks are still using UART before you call this function
esp_err_t uart_driver_delete(uart_port_t uart_num)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_ERR_INVALID_ARG);

    if (p_uart_obj[uart_num] == NULL) {
        ESP_LOGI(UART_TAG, "ALREADY NULL");
        return ESP_OK;
    }

    uart_disable_rx_intr(uart_num);
    uart_disable_tx_intr(uart_num);
    _xt_isr_mask(0x1 << ETS_UART_INUM);

    if (p_uart_obj[uart_num]->tx_fifo_sem) {
        vSemaphoreDelete(p_uart_obj[uart_num]->tx_fifo_sem);
        p_uart_obj[uart_num]->tx_fifo_sem = NULL;
    }

    if (p_uart_obj[uart_num]->tx_done_sem) {
        vSemaphoreDelete(p_uart_obj[uart_num]->tx_done_sem);
        p_uart_obj[uart_num]->tx_done_sem = NULL;
    }

    if (p_uart_obj[uart_num]->tx_mux) {
        vSemaphoreDelete(p_uart_obj[uart_num]->tx_mux);
        p_uart_obj[uart_num]->tx_mux = NULL;
    }

    if (p_uart_obj[uart_num]->rx_mux) {
        vSemaphoreDelete(p_uart_obj[uart_num]->rx_mux);
        p_uart_obj[uart_num]->rx_mux = NULL;
    }

    if (p_uart_obj[uart_num]->xQueueUart) {
        vQueueDelete(p_uart_obj[uart_num]->xQueueUart);
        p_uart_obj[uart_num]->xQueueUart = NULL;
    }

    if (p_uart_obj[uart_num]->rx_ring_buf) {
        vRingbufferDelete(p_uart_obj[uart_num]->rx_ring_buf);
        p_uart_obj[uart_num]->rx_ring_buf = NULL;
    }

    if (p_uart_obj[uart_num]->tx_ring_buf) {
        vRingbufferDelete(p_uart_obj[uart_num]->tx_ring_buf);
        p_uart_obj[uart_num]->tx_ring_buf = NULL;
    }

    free(p_uart_obj[uart_num]);
    p_uart_obj[uart_num] = NULL;

    return ESP_OK;
}

void uart_set_select_notif_callback(uart_port_t uart_num, uart_select_notif_callback_t uart_select_notif_callback)
{
    if (uart_num < UART_NUM_MAX && p_uart_obj[uart_num]) {
        p_uart_obj[uart_num]->uart_select_notif_callback = (uart_select_notif_callback_t) uart_select_notif_callback;
    }
}

esp_err_t uart_set_rx_timeout(uart_port_t uart_num, const uint8_t tout_thresh)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_ERR_INVALID_ARG);
    UART_CHECK((tout_thresh < 127), "tout_thresh max value is 126", ESP_ERR_INVALID_ARG);

    UART_ENTER_CRITICAL();

    // The tout_thresh = 1, defines TOUT interrupt timeout equal to
    // transmission time of one symbol (~11 bit) on current baudrate
    if (tout_thresh > 0) {
        UART[uart_num]->conf1.rx_tout_thrhd = (tout_thresh & 0x7f);
        UART[uart_num]->conf1.rx_tout_en = 1;
    } else {
        UART[uart_num]->conf1.rx_tout_en = 0;
    }

    UART_EXIT_CRITICAL();
    return ESP_OK;
}

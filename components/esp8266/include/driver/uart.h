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


#ifndef _DRIVER_UART_H_
#define _DRIVER_UART_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/queue.h"

#define UART_FIFO_LEN           (128)        /*!< Length of the hardware FIFO buffers */
#define UART_INTR_MASK          0x1ff        /*!< Mask of all UART interrupts */
#define UART_LINE_INV_MASK      (0x3f << 19) /*!< TBD */

#define UART_INVERSE_DISABLE    (0x0)        /*!< Disable UART signal inverse*/
#define UART_INVERSE_RXD        (BIT(19))    /*!< UART RXD input inverse*/
#define UART_INVERSE_CTS        (BIT(20))    /*!< UART CTS input inverse*/
#define UART_INVERSE_TXD        (BIT(22))    /*!< UART TXD output inverse*/
#define UART_INVERSE_RTS        (BIT(23))    /*!< UART RTS output inverse*/

/**
 * @brief UART mode selection
 */
typedef enum {
    UART_MODE_UART = 0x00,                      /*!< mode: regular UART mode*/
} uart_mode_t;

/**
 * @brief UART word length constants
 */
typedef enum {
    UART_DATA_5_BITS = 0x0,    /*!< word length: 5bits*/
    UART_DATA_6_BITS = 0x1,    /*!< word length: 6bits*/
    UART_DATA_7_BITS = 0x2,    /*!< word length: 7bits*/
    UART_DATA_8_BITS = 0x3,    /*!< word length: 8bits*/
    UART_DATA_BITS_MAX = 0x4,
} uart_word_length_t;

/**
 * @brief UART stop bits number
 */
typedef enum {
    UART_STOP_BITS_1   = 0x1,  /*!< stop bit: 1bit*/
    UART_STOP_BITS_1_5 = 0x2,  /*!< stop bit: 1.5bits*/
    UART_STOP_BITS_2   = 0x3,  /*!< stop bit: 2bits*/
    UART_STOP_BITS_MAX = 0x4,
} uart_stop_bits_t;

/**
 * @brief UART peripheral number
 */
typedef enum {
    UART_NUM_0 = 0x0,
    UART_NUM_1 = 0x1,
    UART_NUM_MAX,
} uart_port_t;

/**
 * @brief UART parity constants
 */
typedef enum {
    UART_PARITY_DISABLE = 0x0,  /*!< Disable UART parity*/
    UART_PARITY_EVEN = 0x2,     /*!< Enable UART even parity*/
    UART_PARITY_ODD  = 0x3      /*!< Enable UART odd parity*/
} uart_parity_t;

/**
 * @brief UART hardware flow control modes
 */
typedef enum {
    UART_HW_FLOWCTRL_DISABLE = 0x0,   /*!< disable hardware flow control*/
    UART_HW_FLOWCTRL_RTS     = 0x1,   /*!< enable RX hardware flow control (rts)*/
    UART_HW_FLOWCTRL_CTS     = 0x2,   /*!< enable TX hardware flow control (cts)*/
    UART_HW_FLOWCTRL_CTS_RTS = 0x3,   /*!< enable hardware flow control*/
    UART_HW_FLOWCTRL_MAX     = 0x4,
} uart_hw_flowcontrol_t;

/**
 * @brief UART configuration parameters for uart_param_config function
 */
typedef struct {
    int baud_rate;                      /*!< UART baud rate*/
    uart_word_length_t data_bits;       /*!< UART byte size*/
    uart_parity_t parity;               /*!< UART parity mode*/
    uart_stop_bits_t stop_bits;         /*!< UART stop bits*/
    uart_hw_flowcontrol_t flow_ctrl;    /*!< UART HW flow control mode (cts/rts)*/
    uint8_t rx_flow_ctrl_thresh;        /*!< UART HW RTS threshold*/
} uart_config_t;

/**
 * @brief UART interrupt configuration parameters for uart_intr_config function
 */
typedef struct {
    uint32_t intr_enable_mask;          /*!< UART interrupt enable mask, choose from UART_XXXX_INT_ENA_M under UART_INT_ENA_REG(i), connect with bit-or operator*/
    uint8_t  rx_timeout_thresh;         /*!< UART timeout interrupt threshold (unit: time of sending one byte)*/
    uint8_t  txfifo_empty_intr_thresh;  /*!< UART TX empty interrupt threshold.*/
    uint8_t  rxfifo_full_thresh;        /*!< UART RX full interrupt threshold.*/
} uart_intr_config_t;

/**
 * @brief UART event types used in the ring buffer
 */
typedef enum {
    UART_DATA,              /*!< UART data event*/
    UART_BUFFER_FULL,       /*!< UART RX buffer full event*/
    UART_FIFO_OVF,          /*!< UART FIFO overflow event*/
    UART_FRAME_ERR,         /*!< UART RX frame error event*/
    UART_PARITY_ERR,        /*!< UART RX parity event*/
    UART_EVENT_MAX,         /*!< UART event max index*/
} uart_event_type_t;

/**
 * @brief Event structure used in UART event queue
 */
typedef struct {
    uart_event_type_t type; /*!< UART event type */
    size_t size;            /*!< UART data size for UART_DATA event*/
} uart_event_t;


/**
 * @brief Set UART data bits.
 *
 * @param uart_num Uart port number.
 * @param data_bit Uart data bits.
 *
 * @return
 *     - ESP_OK success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t uart_set_word_length(uart_port_t uart_num, uart_word_length_t data_bit);

/**
 * @brief Get UART data bits.
 *
 * @param uart_num Uart port number.
 * @param data_bit Pointer to accept value of UART data bits.
 *
 * @return
 *     - ESP_OK success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t uart_get_word_length(uart_port_t uart_num, uart_word_length_t *data_bit);

/**
 * @brief Set UART stop bits.
 *
 * @param uart_num Uart port number
 * @param stop_bits Uart stop bits
 *
 * @return
 *     - ESP_OK success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t uart_set_stop_bits(uart_port_t uart_num, uart_stop_bits_t stop_bits);

/**
 * @brief Get UART stop bits.
 *
 * @param uart_num Uart port number.
 * @param stop_bits Pointer to accept value of UART stop bits.
 *
 * @return
 *     - ESP_OK success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t uart_get_stop_bits(uart_port_t uart_num, uart_stop_bits_t *stop_bits);

/**
 * @brief Set UART parity mode.
 *
 * @param uart_num Uart port number.
 * @param parity_mode The enum of uart parity configuration.
 *
 * @return
 *     - ESP_OK success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t uart_set_parity(uart_port_t uart_num, uart_parity_t parity_mode);

/**
 * @brief Get UART parity mode.
 *
 * @param uart_num Uart port number
 * @param parity_mode Pointer to accept value of UART parity mode.
 *
 * @return
 *     - ESP_OK success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t uart_get_parity(uart_port_t uart_num, uart_parity_t *parity_mode);

/**
 * @brief Set UART baud rate.
 *
 * @param uart_num Uart port number
 * @param baudrate UART baud rate.
 *
 * @return
 *     - ESP_OK success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t uart_set_baudrate(uart_port_t uart_num, uint32_t baudrate);

/**
 * @brief Get UART baud rate.
 *
 * @param uart_num Uart port number.
 * @param baudrate Pointer to accept value of Uart baud rate.
 *
 * @return
 *     - ESP_OK success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t uart_get_baudrate(uart_port_t uart_num, uint32_t *baudrate);

/**
 * @brief Set UART line inverse mode
 *
 * @param uart_num  UART_NUM_0
 * @param inverse_mask Choose the wires that need to be inverted.
 *        Inverse_mask should be chosen from
 *        UART_INVERSE_RXD / UART_INVERSE_TXD / UART_INVERSE_RTS / UART_INVERSE_CTS,
 *        combined with OR operation.
 *
 * @return
 *     - ESP_OK success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t uart_set_line_inverse(uart_port_t uart_num, uint32_t inverse_mask);

/**
  * @brief Configure Hardware flow control.
  *
  * @param uart_num Uart port number.
  * @param flow_ctrl Hardware flow control mode.
  * @param rx_thresh Threshold of Hardware flow control.
  *
  * @return
  *     - ESP_OK success
  *     - ESP_ERR_INVALID_ARG Parameter error
  */
esp_err_t uart_set_hw_flow_ctrl(uart_port_t uart_num, uart_hw_flowcontrol_t flow_ctrl, uint8_t rx_thresh);

/**
 * @brief Get hardware flow control mode
 *
 * @param uart_num Uart port number.
 * @param flow_ctrl Option for different flow control mode.
 *
 * @return
 *     - ESP_OK   Success, result will be put in (*flow_ctrl)
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t uart_get_hw_flow_ctrl(uart_port_t uart_num, uart_hw_flowcontrol_t *flow_ctrl);

/**
  * @brief  UART0 swap.
  *         Use MTCK as UART0 RX, MTDO as UART0 TX, so ROM log will not output from
  *         this new UART0. We also need to use MTDO (U0RTS) and MTCK (U0CTS) as UART0 in hardware.
  *
  * @return
  *     - ESP_OK Success
  */
esp_err_t uart_enable_swap(void);

/**
  * @brief  Disable UART0 swap.
  *         Use the original UART0, not MTCK and MTDO.
  *
  * @return
  *     - ESP_OK Success
  */
esp_err_t uart_disable_swap(void);

/**
  * @brief Clear uart interrupts status.
  *
  * @param uart_num Uart port number.
  * @param mask Uart interrupt bits mask.
  *
  * @return
  *     - ESP_OK success
  *     - ESP_ERR_INVALID_ARG Parameter error
  */
esp_err_t uart_clear_intr_status(uart_port_t uart_num, uint32_t mask);

/**
 * @brief Set UART interrupt enable
 *
 * @param uart_num     Uart port number
 * @param enable_mask  Bit mask of the enable bits.
 *                     The bit mask should be composed from the fields of register UART_INT_ENA_REG.
 *
 * @return
 *     - ESP_OK   Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t uart_enable_intr_mask(uart_port_t uart_num, uint32_t enable_mask);

/**
 * @brief Clear UART interrupt enable bits
 *
 * @param uart_num      Uart port number
 * @param disable_mask  Bit mask of the disable bits.
 *                      The bit mask should be composed from the fields of register UART_INT_ENA_REG.
 *
 * @return
 *     - ESP_OK   Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t uart_disable_intr_mask(uart_port_t uart_num, uint32_t disable_mask);

/**
 * @brief Enable UART RX interrupt (RX_FULL & RX_TIMEOUT INTERRUPT)
 *
 * @param uart_num  UART_NUM_0
 *
 * @return
 *     - ESP_OK   Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t uart_enable_rx_intr(uart_port_t uart_num);

/**
 * @brief Disable UART RX interrupt (RX_FULL & RX_TIMEOUT INTERRUPT)
 *
 * @param uart_num  UART_NUM_0
 *
 * @return
 *     - ESP_OK   Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t uart_disable_rx_intr(uart_port_t uart_num);

/**
 * @brief Disable UART TX interrupt (TX_FULL & TX_TIMEOUT INTERRUPT)
 *
 * @param uart_num  UART_NUM_0
 *
 * @return
 *     - ESP_OK   Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t uart_disable_tx_intr(uart_port_t uart_num);

/**
 * @brief Enable UART TX interrupt (TX_FULL & TX_TIMEOUT INTERRUPT)
 *
 * @param uart_num UART_NUM_0
 * @param enable  1: enable; 0: disable
 * @param thresh  Threshold of TX interrupt, 0 ~ UART_FIFO_LEN
 *
 * @return
 *     - ESP_OK   Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t uart_enable_tx_intr(uart_port_t uart_num, int enable, int thresh);

/**
 * @brief Register UART interrupt handler (ISR).
 *
 * @param uart_num UART_NUM_0
 * @param fn  Interrupt handler function.
 * @param arg parameter for handler function
 *
 * @return
 *     - ESP_OK   Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t uart_isr_register(uart_port_t uart_num, void (*fn)(void *), void *arg);

/**
  * @brief Config Common parameters of serial ports.
  *
  * @param uart_num Uart port number.
  * @param uart_conf Uart config parameters.
  *
  * @return
  *     - ESP_OK success
  *     - ESP_ERR_INVALID_ARG Parameter error
  */
esp_err_t uart_param_config(uart_port_t uart_num, uart_config_t *uart_conf);

/**
  * @brief Config types of uarts.
  *
  * @param uart_num Uart port number.
  * @param uart_intr_conf Uart interrupt config parameters.
  *
  * @return
  *     - ESP_OK success
  *     - ESP_ERR_INVALID_ARG Parameter error
  */
esp_err_t uart_intr_config(uart_port_t uart_num, uart_intr_config_t *uart_intr_conf);

/**
 * @brief Install UART driver.
 *
 * @note  Rx_buffer_size should be greater than UART_FIFO_LEN. Tx_buffer_size should be either zero or greater than UART_FIFO_LEN.
 *
 * @param uart_num Uart port number.
 * @param rx_buffer_size UART RX ring buffer size.
 * @param tx_buffer_size UART TX ring buffer size.
 *        If set to zero, driver will not use TX buffer, TX function will block task until all data have been sent out.
 * @param queue_size UART event queue size/depth.
 * @param uart_queue UART event queue handle (out param). On success, a new queue handle is written here to provide
 *        access to UART events. If set to NULL, driver will not use an event queue.
 * @param no_use Invalid parameters, just to fit some modules.
 * 
 * @return
 *     - ESP_OK   Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t uart_driver_install(uart_port_t uart_num, int rx_buffer_size, int tx_buffer_size, int queue_size, QueueHandle_t *uart_queue, int no_use);

/**
 * @brief Uninstall UART driver.
 *
 * @param uart_num Uart port number.
 *
 * @return
 *     - ESP_OK   Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t uart_driver_delete(uart_port_t uart_num);

/**
 * @brief Waiting for the last byte of data to be sent
 *
 * @param uart_num Uart port number.
 * @param ticks_to_wait Timeout, count in RTOS ticks
 *
 * @return
 *     - ESP_OK   Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t uart_wait_tx_done(uart_port_t uart_num, TickType_t ticks_to_wait);

/**
 * @brief Send data to the UART port from a given buffer and length.
 *
 * This function will not wait for enough space in TX FIFO. It will just fill the available TX FIFO and return when the FIFO is full.
 * @note This function should only be used when UART TX buffer is not enabled.
 *
 * @param uart_num Uart port number.
 * @param buffer data buffer address
 * @param len    data length to send
 *
 * @return
 *     - (-1)  Parameter error
 *     - OTHERS (>=0) The number of bytes pushed to the TX FIFO
 */
int uart_tx_chars(uart_port_t uart_num, const char *buffer, uint32_t len);

/**
 * @brief Send data to the UART port from a given buffer and length,
 *
 * If the UART driver's parameter 'tx_buffer_size' is set to zero:
 * This function will not return until all the data have been sent out, or at least pushed into TX FIFO.
 *
 * Otherwise, if the 'tx_buffer_size' > 0, this function will return after copying all the data to tx ring buffer,
 * UART ISR will then move data from the ring buffer to TX FIFO gradually.
 *
 * @param uart_num Uart port number.
 * @param src   data buffer address
 * @param size  data length to send
 *
 * @return
 *     - (-1) Parameter error
 *     - OTHERS (>=0) The number of bytes pushed to the TX FIFO
 */
int uart_write_bytes(uart_port_t uart_num, const char *src, size_t size);

/**
 * @brief UART read bytes from UART buffer
 *
 * @param uart_num Uart port number.
 * @param buf     pointer to the buffer.
 * @param length  data length
 * @param ticks_to_wait sTimeout, count in RTOS ticks
 *
 * @return
 *     - (-1) Error
 *     - OTHERS (>=0) The number of bytes read from UART FIFO
 */
int uart_read_bytes(uart_port_t uart_num, uint8_t *buf, uint32_t length, TickType_t ticks_to_wait);

/**
 * @brief Alias of uart_flush_input.
 *        UART ring buffer flush. This will discard all data in the UART RX buffer.
 * @note  Instead of waiting the data sent out, this function will clear UART rx buffer.
 *        In order to send all the data in tx FIFO, we can use uart_wait_tx_done function.
 * @param uart_num UART port number.
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t uart_flush(uart_port_t uart_num);

/**
 * @brief Clear input buffer, discard all the data is in the ring-buffer.
 * @note  In order to send all the data in tx FIFO, we can use uart_wait_tx_done function.
 * @param uart_num UART port number.
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t uart_flush_input(uart_port_t uart_num);

/**
 * @brief   UART get RX ring buffer cached data length
 *
 * @param   uart_num UART port number.
 * @param   size Pointer of size_t to accept cached data length
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t uart_get_buffered_data_len(uart_port_t uart_num, size_t *size);

/**
 * @brief UART set threshold timeout for TOUT feature
 *
 * @param uart_num     Uart number to configure
 * @param tout_thresh  This parameter defines timeout threshold in uart symbol periods. The maximum value of threshold is 126.
 *        tout_thresh = 1, defines TOUT interrupt timeout equal to transmission time of one symbol (~11 bit) on current baudrate.
 *        If the time is expired the UART_RXFIFO_TOUT_INT interrupt is triggered. If tout_thresh == 0,
 *        the TOUT feature is disabled.
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t uart_set_rx_timeout(uart_port_t uart_num, const uint8_t tout_thresh);

#ifdef __cplusplus
}
#endif

#endif // _DRIVER_UART_H_

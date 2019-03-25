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

#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief I2S bit width per sample.
 */
typedef enum {
    I2S_BITS_PER_SAMPLE_8BIT    = 8,        /*!< I2S bits per sample: 8-bits*/
    I2S_BITS_PER_SAMPLE_16BIT   = 16,       /*!< I2S bits per sample: 16-bits*/
    I2S_BITS_PER_SAMPLE_24BIT   = 24,       /*!< I2S bits per sample: 24-bits*/
} i2s_bits_per_sample_t;

/**
 * @brief I2S channel.
 */
typedef enum {
    I2S_CHANNEL_MONO        = 1,            /*!< I2S 1 channel (mono)*/
    I2S_CHANNEL_STEREO      = 2             /*!< I2S 2 channel (stereo)*/
} i2s_channel_t;

/**
 * @brief I2S communication standard format
 */
typedef enum {
    I2S_COMM_FORMAT_I2S         = 0x01, /*!< I2S communication format I2S*/
    I2S_COMM_FORMAT_I2S_MSB     = 0x02, /*!< I2S format MSB*/
    I2S_COMM_FORMAT_I2S_LSB     = 0x04, /*!< I2S format LSB*/
} i2s_comm_format_t;

/**
 * @brief I2S channel format type
 */
typedef enum {
    I2S_CHANNEL_FMT_RIGHT_LEFT = 0x00,
    I2S_CHANNEL_FMT_ALL_RIGHT,
    I2S_CHANNEL_FMT_ALL_LEFT,
    I2S_CHANNEL_FMT_ONLY_RIGHT,
    I2S_CHANNEL_FMT_ONLY_LEFT,
} i2s_channel_fmt_t;

/**
 * @brief I2S Peripheral, 0
 */
typedef enum {
    I2S_NUM_0 = 0x0,  /*!< I2S 0*/
    I2S_NUM_MAX,
} i2s_port_t;

/**
 * @brief I2S Mode, defaut is I2S_MODE_MASTER | I2S_MODE_TX
 */
typedef enum {
    I2S_MODE_MASTER = 1,
    I2S_MODE_SLAVE = 2,
    I2S_MODE_TX = 4,
    I2S_MODE_RX = 8,
} i2s_mode_t;

/**
 * @brief I2S configuration parameters for i2s_param_config function
 */
typedef struct {
    i2s_mode_t              mode;                   /*!< I2S work mode*/
    int                     sample_rate;            /*!< I2S sample rate*/
    i2s_bits_per_sample_t   bits_per_sample;        /*!< I2S bits per sample*/
    i2s_channel_fmt_t       channel_format;         /*!< I2S channel format */
    i2s_comm_format_t       communication_format;   /*!< I2S communication format */
    int                     dma_buf_count;          /*!< I2S DMA Buffer Count */
    int                     dma_buf_len;            /*!< I2S DMA Buffer Length */
    bool                    tx_desc_auto_clear;     /*!< I2S auto clear tx descriptor if there is underflow condition (helps in avoiding noise in case of data unavailability) */
} i2s_config_t;

/**
 * @brief I2S event types
 */
typedef enum {
    I2S_EVENT_DMA_ERROR,
    I2S_EVENT_TX_DONE,     /*!< I2S DMA finish sent 1 buffer*/
    I2S_EVENT_RX_DONE,     /*!< I2S DMA finish received 1 buffer*/
    I2S_EVENT_MAX,         /*!< I2S event max index*/
} i2s_event_type_t;

/**
 * @brief Event structure used in I2S event queue
 */
typedef struct {
    i2s_event_type_t    type;   /*!< I2S event type */
    size_t              size;   /*!< I2S data size for I2S_DATA event*/
} i2s_event_t;

/**
 * @brief I2S pin enable for i2s_set_pin
 */
typedef struct {
    int bck_o_en;      /*!< BCK out pin*/
    int ws_o_en;       /*!< WS out pin*/
    int bck_i_en;      /*!< BCK in pin*/
    int ws_i_en;       /*!< WS in pin*/
    int data_out_en;   /*!< DATA out pin*/
    int data_in_en;    /*!< DATA in pin*/
} i2s_pin_config_t;

/**
 * @brief Set I2S pin number
 *
 * @param   i2s_num     I2S_NUM_0
 * @param   pin         I2S Pin structure
 *
 * @return
 *     - ESP_OK              Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 *     - ESP_FAIL            IO error
 */
esp_err_t i2s_set_pin(i2s_port_t i2s_num, const i2s_pin_config_t *pin);

/**
 * @brief Install and start I2S driver.
 *
 * @note  This function must be called before any I2S driver read/write operations.
 *
 * @param i2s_num         I2S_NUM_0
 * @param i2s_config      I2S configurations - see i2s_config_t struct
 * @param queue_size      I2S event queue size/depth.
 * @param i2s_queue       I2S event queue handle, if set NULL, driver will not use an event queue.
 *
 * @return
 *     - ESP_OK              Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 *     - ESP_ERR_NO_MEM      Out of memory
 */
esp_err_t i2s_driver_install(i2s_port_t i2s_num, const i2s_config_t *i2s_config, int queue_size, void *i2s_queue);

/**
 * @brief Uninstall I2S driver.
 *
 * @param i2s_num  I2S_NUM_0
 *
 * @return
 *     - ESP_OK              Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t i2s_driver_uninstall(i2s_port_t i2s_num);

/**
 * @brief Write data to I2S DMA transmit buffer.
 *
 * @note many ticks pass without space becoming available in the DMA
 *       transmit buffer, then the function will return (note that if the
 *       data is written to the DMA buffer in pieces, the overall operation
 *       may still take longer than this timeout.) Pass portMAX_DELAY for no
 *       timeout.
 *
 * @param i2s_num             I2S_NUM_0
 * @param src                 Source address to write from
 * @param size                Size of data in bytes
 * @param[out] bytes_written  Number of bytes written, if timeout, the result will be less than the size passed in.
 * @param ticks_to_wait       TX buffer wait timeout in RTOS ticks. If this
 *
 * @return
 *     - ESP_OK               Success
 *     - ESP_ERR_INVALID_ARG  Parameter error
 */
esp_err_t i2s_write(i2s_port_t i2s_num, const void *src, size_t size, size_t *bytes_written, TickType_t ticks_to_wait);

/**
 * @brief Write data to I2S DMA transmit buffer while expanding the number of bits per sample. For example, expanding 16-bit PCM to 32-bit PCM.
 *
 * @note many ticks pass without space becoming available in the DMA
 *       transmit buffer, then the function will return (note that if the
 *       data is written to the DMA buffer in pieces, the overall operation
 *       may still take longer than this timeout.) Pass portMAX_DELAY for no
 *       timeout.
 *       Format of the data in source buffer is determined by the I2S
 *       configuration (see i2s_config_t).
 *
 * @param i2s_num             I2S_NUM_0
 * @param src                 Source address to write from
 * @param size                Size of data in bytes
 * @param src_bits            Source audio bit
 * @param aim_bits            Bit wanted, no more than 32, and must be greater than src_bits
 * @param[out] bytes_written  Number of bytes written, if timeout, the result will be less than the size passed in.
 * @param ticks_to_wait       TX buffer wait timeout in RTOS ticks. If this
 *
 * @return
 *     - ESP_OK              Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t i2s_write_expand(i2s_port_t i2s_num, const void *src, size_t size, size_t src_bits, size_t aim_bits, size_t *bytes_written, TickType_t ticks_to_wait);

/**
 * @brief Read data from I2S DMA receive buffer
 *
 * @note If the built-in ADC mode is enabled, we should call i2s_adc_start and i2s_adc_stop around the whole reading process,
 *       to prevent the data getting corrupted.
 *
 * @param i2s_num         I2S_NUM_0
 * @param dest            Destination address to read into
 * @param size            Size of data in bytes
 * @param[out] bytes_read Number of bytes read, if timeout, bytes read will be less than the size passed in.
 * @param ticks_to_wait   RX buffer wait timeout in RTOS ticks. If this many ticks pass without bytes becoming available in the DMA receive buffer, then the function will return (note that if data is read from the DMA buffer in pieces, the overall operation may still take longer than this timeout.) Pass portMAX_DELAY for no timeout.
 *
 * @return
 *     - ESP_OK               Success
 *     - ESP_ERR_INVALID_ARG  Parameter error
 */
esp_err_t i2s_read(i2s_port_t i2s_num, void *dest, size_t size, size_t *bytes_read, TickType_t ticks_to_wait);

/**
 * @brief Set sample rate used for I2S RX and TX.
 *
 * @note The bit clock rate is determined by the sample rate and i2s_config_t configuration parameters (number of channels, bits_per_sample).
 *       `bit_clock = rate * (number of channels) * bits_per_sample`
 *
 * @param i2s_num  I2S_NUM_0
 * @param rate I2S sample rate (ex: 8000, 44100...)
 *
 * @return
 *     - ESP_OK              Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 *     - ESP_ERR_NO_MEM      Out of memory
 */
esp_err_t i2s_set_sample_rates(i2s_port_t i2s_num, uint32_t rate);

/**
 * @brief Stop I2S driver
 *
 * @note Disables I2S TX/RX, until i2s_start() is called.
 *
 * @param i2s_num  I2S_NUM_0
 *
 * @return
 *     - ESP_OK              Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t i2s_stop(i2s_port_t i2s_num);

/**
 * @brief Start I2S driver
 *
 * @note It is not necessary to call this function after i2s_driver_install() (it is started automatically), however it is necessary to call it after i2s_stop().
 *
 * @param i2s_num  I2S_NUM_0
 *
* @return
 *     - ESP_OK              Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t i2s_start(i2s_port_t i2s_num);

/**
 * @brief Zero the contents of the TX DMA buffer.
 *
 * @note Pushes zero-byte samples into the TX DMA buffer, until it is full.
 *
 * @param i2s_num  I2S_NUM_0
 *
 * @return
 *     - ESP_OK              Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t i2s_zero_dma_buffer(i2s_port_t i2s_num);

/**
 * @brief Set clock & bit width used for I2S RX and TX.
 *
 * @note Similar to i2s_set_sample_rates(), but also sets bit width.
 *
 * @param i2s_num  I2S_NUM_0
 * @param rate I2S sample rate (ex: 8000, 44100...)
 * @param bits I2S bit width (I2S_BITS_PER_SAMPLE_16BIT, I2S_BITS_PER_SAMPLE_24BIT)
 * @param ch I2S channel, (I2S_CHANNEL_MONO, I2S_CHANNEL_STEREO)
 *
 * @return
 *     - ESP_OK              Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 *     - ESP_ERR_NO_MEM      Out of memory
 */
esp_err_t i2s_set_clk(i2s_port_t i2s_num, uint32_t rate, i2s_bits_per_sample_t bits, i2s_channel_t ch);

#ifdef __cplusplus
}
#endif
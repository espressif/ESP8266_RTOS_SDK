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

#define SPI_NUM_MAX 2

/* SPI bus CPOL and CPHA definition */
#define SPI_CPOL_LOW  0
#define SPI_CPOL_HIGH 1
#define SPI_CPHA_LOW  0
#define SPI_CPHA_HIGH 1

/* SPI bus data sequence definition */
#define SPI_BIT_ORDER_MSB_FIRST  1
#define SPI_BIT_ORDER_LSB_FIRST  0
#define SPI_BYTE_ORDER_MSB_FIRST 1
#define SPI_BYTE_ORDER_LSB_FIRST 0

/* SPI default bus interface parameter definition */
#define SPI_DEFAULT_INTERFACE   0x1C0    /* CS_EN:1, MISO_EN:1, MOSI_EN:1, BYTE_TX_ORDER:0, BYTE_TX_ORDER:0, BIT_RX_ORDER:0, BIT_TX_ORDER:0, CPHA:0, CPOL:0 */

/* SPI master default interrupt enable definition */
#define SPI_MASTER_DEFAULT_INTR_ENABLE 0x10    /* TRANS_DONE: true, WRITE_STATUS: false, READ_STATUS: false, WRITE_BUFFER: false, READ_BUFFER: false */

/* SPI slave default interrupt enable definition */
#define SPI_SLAVE_DEFAULT_INTR_ENABLE 0x0F    /* TRANS_DONE: false, WRITE_STATUS: true, READ_STATUS: true, WRITE_BUFFER: true, READ_BUFFER: ture */

/* SPI event definition */
#define SPI_INIT_EVENT        0
#define SPI_TRANS_START_EVENT 1
#define SPI_TRANS_DONE_EVENT  2
#define SPI_DEINIT_EVENT      3

/* SPI data cmd definition */
#define SPI_MASTER_WRITE_DATA_TO_SLAVE_CMD     2
#define SPI_MASTER_READ_DATA_FROM_SLAVE_CMD    3

/* SPI status cmd definition */
#define SPI_MASTER_WRITE_STATUS_TO_SLAVE_CMD   1
#define SPI_MASTER_READ_STATUS_FROM_SLAVE_CMD  4

/* SPI slave transfer done interrupt status definition */
#define SPI_SLV_RD_BUF_DONE (BIT(0))
#define SPI_SLV_WR_BUF_DONE (BIT(1))
#define SPI_SLV_RD_STA_DONE (BIT(2))
#define SPI_SLV_WR_STA_DONE (BIT(3))
#define SPI_TRANS_DONE      (BIT(4))

typedef void (*spi_event_callback_t)(int event, void *arg);

/**
 * @brief SPI peripheral enumeration
 * 
 * @note ESP8266 has two hardware SPI, CSPI and HSPI. Currently, HSPI can be used arbitrarily.
 */
typedef enum {
    CSPI_HOST = 0,
    HSPI_HOST
} spi_host_t;

/**
 * @brief SPI clock division factor enumeration
 */
typedef enum {
    SPI_2MHz_DIV  = 40,
    SPI_4MHz_DIV  = 20,
    SPI_5MHz_DIV  = 16,
    SPI_8MHz_DIV  = 10,
    SPI_10MHz_DIV = 8,
    SPI_16MHz_DIV = 5,
    SPI_20MHz_DIV = 4,
    SPI_40MHz_DIV = 2,
    SPI_80MHz_DIV = 1,
} spi_clk_div_t;

/**
 * @brief SPI working mode enumeration
 */
typedef enum {
    SPI_MASTER_MODE,
    SPI_SLAVE_MODE
} spi_mode_t;

/**
 * @brief SPI interrupt enable union type definition
 */
typedef union {
    struct {
        uint32_t read_buffer:  1;    /*!< configurate intterrupt to enable reading */ 
        uint32_t write_buffer: 1;    /*!< configurate intterrupt to enable writing */ 
        uint32_t read_status:  1;    /*!< configurate intterrupt to enable reading status */ 
        uint32_t write_status: 1;    /*!< configurate intterrupt to enable writing status */ 
        uint32_t trans_done:   1;    /*!< configurate intterrupt to enable transmission done */ 
        uint32_t reserved5:    27;   /*!< reserved */
    };                               /*!< not filled */
    uint32_t val;                    /*!< union fill */ 
} spi_intr_enable_t;

/**
 * @brief SPI bus interface parameter union type definition
 */
typedef union {
    struct {
        uint32_t cpol:          1;   /*!< Clock Polarity */
        uint32_t cpha:          1;   /*!< Clock Phase */
        uint32_t bit_tx_order:  1;   /*!< Tx bit order */
        uint32_t bit_rx_order:  1;   /*!< Rx bit order */
        uint32_t byte_tx_order: 1;   /*!< Tx byte order */
        uint32_t byte_rx_order: 1;   /*!< Rx byte order */
        uint32_t mosi_en:       1;   /*!< MOSI line enable */
        uint32_t miso_en:       1;   /*!< MISO line enable */
        uint32_t cs_en:         1;   /*!< CS line enable */
        uint32_t reserved9:    23;   /*!< resserved */
    };                               /*!< not filled */
    uint32_t val;                    /*!< union fill */ 
} spi_interface_t;

/**
 * @brief SPI transmission parameter structure type definition
 */
typedef struct {
    uint16_t *cmd;                  /*!< SPI transmission command */  
    uint32_t *addr;                 /*!< SPI transmission address */  
    uint32_t *mosi;                 /*!< SPI transmission MOSI buffer, in order to improve the transmission efficiency, it is recommended that the external incoming data is (uint32_t *) type data, do not use other type data. */  
    uint32_t *miso;                 /*!< SPI transmission MISO buffer, in order to improve the transmission efficiency, it is recommended that the external incoming data is (uint32_t *) type data, do not use other type data. */  
    union {
        struct {
            uint32_t cmd:   5;      /*!< SPI transmission command bits */  
            uint32_t addr:  7;      /*!< SPI transmission address bits */ 
            uint32_t mosi: 10;      /*!< SPI transmission MOSI buffer bits */  
            uint32_t miso: 10;      /*!< SPI transmission MISO buffer bits */ 
        };                          /*!< not filled */
        uint32_t val;               /*!< union fill */ 
    } bits;                         /*!< SPI transmission packet members' bits */  
} spi_trans_t;

/**
 * @brief SPI initialization parameter structure type definition
 */
typedef struct {
    spi_interface_t interface;      /*!< SPI bus interface */  
    spi_intr_enable_t intr_enable;  /*!< check if enable SPI interrupt */  
    spi_event_callback_t event_cb;  /*!< SPI interrupt event callback */  
    spi_mode_t mode;                /*!< SPI mode */  
    spi_clk_div_t clk_div;          /*!< SPI clock divider */     
} spi_config_t;

/**
  * @brief Get the SPI clock division factor
  *
  * @param host SPI peripheral number
  *     - CSPI_HOST SPI0
  *     - HSPI_HOST SPI1
  *
  * @param clk_div Pointer to accept clock division factor
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_FAIL spi has not been initialized yet
  */
esp_err_t spi_get_clk_div(spi_host_t host, spi_clk_div_t *clk_div);

/**
  * @brief Get SPI Interrupt Enable
  *
  * @param host SPI peripheral number
  *     - CSPI_HOST SPI0
  *     - HSPI_HOST SPI1
  *
  * @param intr_enable Pointer to accept interrupt enable
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_FAIL spi has not been initialized yet
  */
esp_err_t spi_get_intr_enable(spi_host_t host, spi_intr_enable_t *intr_enable);

/**
  * @brief Get SPI working mode
  *
  * @param host SPI peripheral number
  *     - CSPI_HOST SPI0
  *     - HSPI_HOST SPI1
  *
  * @param mode Pointer to accept working mode
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_FAIL spi has not been initialized yet
  */
esp_err_t spi_get_mode(spi_host_t host, spi_mode_t *mode);

/**
  * @brief Get SPI bus interface configuration
  *
  * @param host SPI peripheral number
  *     - CSPI_HOST SPI0
  *     - HSPI_HOST SPI1
  *
  * @param interface Pointer to accept bus interface configuration
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_FAIL spi has not been initialized yet
  */
esp_err_t spi_get_interface(spi_host_t host, spi_interface_t *interface);

/**
  * @brief Get the SPI event callback function
  *
  * @param host SPI peripheral number
  *     - CSPI_HOST SPI0
  *     - HSPI_HOST SPI1
  *
  * @param event_cb Pointer to accept event callback function
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_FAIL spi has not been initialized yet
  */
esp_err_t spi_get_event_callback(spi_host_t host, spi_event_callback_t *event_cb);

/**
  * @brief Set the SPI clock division factor
  *
  * @param host SPI peripheral number
  *     - CSPI_HOST SPI0
  *     - HSPI_HOST SPI1
  *
  * @param clk_div Pointer to deliver clock division factor
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_FAIL spi has not been initialized yet
  */
esp_err_t spi_set_clk_div(spi_host_t host, spi_clk_div_t *clk_div);

/**
  * @brief Set SPI interrupt enable
  *
  * @param host SPI peripheral number
  *     - CSPI_HOST SPI0
  *     - HSPI_HOST SPI1
  *
  * @param intr_enable Pointer to deliver interrupt enable
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_FAIL spi has not been initialized yet
  */
esp_err_t spi_set_intr_enable(spi_host_t host, spi_intr_enable_t *intr_enable);

/**
  * @brief Set the SPI mode of operation
  *
  * @param host SPI peripheral number
  *     - CSPI_HOST SPI0
  *     - HSPI_HOST SPI1
  *
  * @param mode Pointer to deliver working mode
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_FAIL spi has not been initialized yet
  */
esp_err_t spi_set_mode(spi_host_t host, spi_mode_t *mode);

/**
  * @brief Get SPI dummy bitlen
  *
  * @param host SPI peripheral number
  *     - CSPI_HOST SPI0
  *     - HSPI_HOST SPI1
  *
  * @param bitlen Pointer to accept dummy bitlen
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_FAIL spi has not been initialized yet
  */
esp_err_t spi_get_dummy(spi_host_t host, uint16_t *bitlen);

/**
  * @brief Set SPI dummy bitlen
  *
  * @param host SPI peripheral number
  *     - CSPI_HOST SPI0
  *     - HSPI_HOST SPI1
  *
  * @param bitlen Pointer to deliver dummy bitlen
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_FAIL spi has not been initialized yet
  */
esp_err_t spi_set_dummy(spi_host_t host, uint16_t *bitlen);

/**
  * @brief Set SPI bus interface configuration
  *
  * @param host SPI peripheral number
  *     - CSPI_HOST SPI0
  *     - HSPI_HOST SPI1
  *
  * @param interface Pointer to deliver bus interface configuration
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_FAIL spi has not been initialized yet
  */
esp_err_t spi_set_interface(spi_host_t host, spi_interface_t *interface);

/**
  * @brief Set the SPI event callback function
  *
  * @note  This event_cb will be called from an ISR. So there is a stack
  *        size limit (configurable as "ISR stack size" in menuconfig). This
  *        limit is smaller compared to a global SPI interrupt handler due
  *        to the additional level of indirection.
  *
  * @param host SPI peripheral number
  *     - CSPI_HOST SPI0
  *     - HSPI_HOST SPI1
  *
  * @param event_cb Pointer to deliver event callback function
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_FAIL spi has not been initialized yet
  */
esp_err_t spi_set_event_callback(spi_host_t host, spi_event_callback_t *event_cb);

/**
  * @brief Get SPI slave wr_status register
  *
  * @param host SPI peripheral number
  *     - CSPI_HOST SPI0
  *     - HSPI_HOST SPI1
  *
  * @param status Pointer to accept wr_status register
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_FAIL spi has not been initialized yet
  */
esp_err_t spi_slave_get_status(spi_host_t host, uint32_t *status);

/**
  * @brief Set SPI slave rd_status register
  *
  * @param host SPI peripheral number
  *     - CSPI_HOST SPI0
  *     - HSPI_HOST SPI1
  *
  * @param status Pointer to deliver rd_status register
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_FAIL spi has not been initialized yet
  */
esp_err_t spi_slave_set_status(spi_host_t host, uint32_t *status);

/**
  * @brief SPI data transfer function
  *
  * @note If the bit of the corresponding phase in the transmission parameter is 0, its data will not work.
  *       For example: trans.bits.cmd = 0, cmd will not be transmitted
  *
  * @param host SPI peripheral number
  *     - CSPI_HOST SPI0
  *     - HSPI_HOST SPI1
  *
  * @param trans Pointer to transmission parameter structure
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  *     - ESP_FAIL spi has not been initialized yet
  */
esp_err_t spi_trans(spi_host_t host, spi_trans_t *trans);

/**
  * @brief Deinit the spi
  *
  * @param host SPI peripheral number
  *     - CSPI_HOST SPI0
  *     - HSPI_HOST SPI1
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_FAIL spi has not been initialized yet
  */
esp_err_t spi_deinit(spi_host_t host);

/**
  * @brief Initialize the spi
  *
  * @note SPI0 has been used by FLASH and cannot be used by the user temporarily.
  *
  * @param host SPI peripheral number
  *     - CSPI_HOST SPI0
  *     - HSPI_HOST SPI1
  *
  * @param config Pointer to deliver initialize configuration parameter
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_NO_MEM malloc fail
  *     - ESP_FAIL spi has been initialized
  */
esp_err_t spi_init(spi_host_t host, spi_config_t *config);

#ifdef __cplusplus
}
#endif
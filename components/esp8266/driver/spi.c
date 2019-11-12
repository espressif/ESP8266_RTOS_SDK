// Copyright 2015-2018 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp8266/eagle_soc.h"
#include "esp8266/spi_struct.h"
#include "esp8266/pin_mux_register.h"
#include "esp_libc.h"
#include "esp_heap_caps.h"
#include "esp_attr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "rom/ets_sys.h"
#include "spi.h"


#define ENTER_CRITICAL() portENTER_CRITICAL()
#define EXIT_CRITICAL() portEXIT_CRITICAL()
#define SPI_CHECK(a, str, ret_val) \
    do { \
        if (!(a)) { \
            ESP_LOGE(TAG,"%s(%d): %s", __FUNCTION__, __LINE__, str); \
            return (ret_val); \
        } \
    } while(0)

#define portYIELD_FROM_ISR() taskYIELD()

#ifndef CONFIG_ESP8266_HSPI_HIGH_THROUGHPUT
#define ENTER_CRITICAL_HIGH_THROUGHPUT() ENTER_CRITICAL()
#define EXIT_CRITICAL_HIGH_THROUGHPUT() EXIT_CRITICAL()
#define SPI_HIGH_THROUGHPUT_ATTR
#define SPI_CHECK_HIGH_THROUGHPUT(a, str, ret_val)  SPI_CHECK(a, str, ret_val)

#else
#define SPI_HIGH_THROUGHPUT_ATTR                            IRAM_ATTR
#define ENTER_CRITICAL_HIGH_THROUGHPUT()                    do{} while(0)
#define EXIT_CRITICAL_HIGH_THROUGHPUT()                     do{} while(0)

#define SPI_CHECK_HIGH_THROUGHPUT(a, str, ret_val) \
    do { \
        if (!(a)) { \
            ets_printf("%s(%d): %s", __FUNCTION__, __LINE__, str); \
            return (ret_val); \
        } \
    } while(0)
#endif

static const char *TAG = "spi";

#define spi_intr_enable() _xt_isr_unmask(1 << ETS_SPI_INUM)
#define spi_intr_disable() _xt_isr_mask(1 << ETS_SPI_INUM)
#define spi_intr_register(a, b) _xt_isr_attach(ETS_SPI_INUM, (a), (b))

/* SPI interrupt status register address definition for determining the interrupt source */
#define DPORT_SPI_INT_STATUS_REG 0x3ff00020
#define DPORT_SPI_INT_STATUS_SPI0 BIT4
#define DPORT_SPI_INT_STATUS_SPI1 BIT7

typedef struct {
    spi_mode_t mode;
    spi_interface_t interface;
    SemaphoreHandle_t trans_mux;
    spi_event_callback_t event_cb;
    spi_intr_enable_t intr_enable;
    uint32_t *buf;
} spi_object_t;

static spi_object_t *spi_object[SPI_NUM_MAX] = {NULL, NULL};

/* DRAM_ATTR is required to avoid SPI array placed in flash, due to accessed from ISR */
static DRAM_ATTR spi_dev_t *const SPI[SPI_NUM_MAX] = {&SPI0, &SPI1};

esp_err_t spi_get_clk_div(spi_host_t host, spi_clk_div_t *clk_div)
{
    SPI_CHECK(host < SPI_NUM_MAX, "host num error", ESP_ERR_INVALID_ARG);
    SPI_CHECK(spi_object[host], "spi has not been initialized yet", ESP_FAIL);
    SPI_CHECK(clk_div, "parameter pointer is empty", ESP_ERR_INVALID_ARG);

    if (SPI[host]->clock.clk_equ_sysclk) {
        *clk_div = SPI_80MHz_DIV;
    }

    *clk_div = SPI[host]->clock.clkcnt_n + 1;
    return ESP_OK;
}

esp_err_t spi_set_clk_div(spi_host_t host, spi_clk_div_t *clk_div)
{
    SPI_CHECK(host < SPI_NUM_MAX, "host num error", ESP_ERR_INVALID_ARG);
    SPI_CHECK(spi_object[host], "spi has not been initialized yet", ESP_FAIL);
    SPI_CHECK(clk_div, "parameter pointer is empty", ESP_ERR_INVALID_ARG);

    ENTER_CRITICAL();

    if (SPI_MASTER_MODE == spi_object[host]->mode) {
        if (SPI_80MHz_DIV == *clk_div) {
            switch (host) {
                case CSPI_HOST: {
                    SET_PERI_REG_MASK(PERIPHS_IO_MUX_CONF_U, SPI0_CLK_EQU_SYS_CLK);
                }
                break;

                case HSPI_HOST: {
                    SET_PERI_REG_MASK(PERIPHS_IO_MUX_CONF_U, SPI1_CLK_EQU_SYS_CLK);
                }
                break;
            }

            SPI[host]->clock.clk_equ_sysclk = true;
        } else {
            // Configure the IO_MUX clock (required, otherwise the clock output will be confusing)
            switch (host) {
                case CSPI_HOST: {
                    CLEAR_PERI_REG_MASK(PERIPHS_IO_MUX_CONF_U, SPI0_CLK_EQU_SYS_CLK);
                }
                break;

                case HSPI_HOST: {
                    CLEAR_PERI_REG_MASK(PERIPHS_IO_MUX_CONF_U, SPI1_CLK_EQU_SYS_CLK);
                }
                break;
            }

            // FRE(SCLK) = clk_equ_sysclk ? 80MHz : APB_CLK(80MHz) / clkdiv_pre / clkcnt
            SPI[host]->clock.clk_equ_sysclk = false;
            SPI[host]->clock.clkdiv_pre = 0;
            SPI[host]->clock.clkcnt_n = *clk_div - 1;
            // In the master mode clkcnt_h = floor((clkcnt_n+1)/2-1). In the slave mode it must be 0
            SPI[host]->clock.clkcnt_h = *clk_div / 2 - 1;
            // In the master mode clkcnt_l = clkcnt_n. In the slave mode it must be 0
            SPI[host]->clock.clkcnt_l = *clk_div - 1;
        }
    } else {
        // Slave mode must be set to 0
        SPI[host]->clock.val = 0;
    }

    EXIT_CRITICAL();

    return ESP_OK;
}

esp_err_t spi_get_intr_enable(spi_host_t host, spi_intr_enable_t *intr_enable)
{
    SPI_CHECK(host < SPI_NUM_MAX, "host num error", ESP_ERR_INVALID_ARG);
    SPI_CHECK(spi_object[host], "spi has not been initialized yet", ESP_FAIL);
    SPI_CHECK(intr_enable, "parameter pointer is empty", ESP_ERR_INVALID_ARG);

    intr_enable->val = (SPI[host]->slave.val >> 5) & 0x1F;

    return ESP_OK;
}

esp_err_t spi_set_intr_enable(spi_host_t host, spi_intr_enable_t *intr_enable)
{
    SPI_CHECK(host < SPI_NUM_MAX, "host num error", ESP_ERR_INVALID_ARG);
    SPI_CHECK(spi_object[host], "spi has not been initialized yet", ESP_FAIL);
    SPI_CHECK(intr_enable, "parameter pointer is empty", ESP_ERR_INVALID_ARG);

    ENTER_CRITICAL();
    SPI[host]->slave.rd_buf_inten = intr_enable->read_buffer;
    SPI[host]->slave.wr_buf_inten = intr_enable->write_buffer;
    SPI[host]->slave.rd_sta_inten = intr_enable->read_status;
    SPI[host]->slave.wr_sta_inten = intr_enable->write_status;
    SPI[host]->slave.trans_inten  = intr_enable->trans_done;
    // Clear interrupt status register
    SPI[host]->slave.rd_buf_done  = false;
    SPI[host]->slave.wr_buf_done  = false;
    SPI[host]->slave.rd_sta_done  = false;
    SPI[host]->slave.wr_sta_done  = false;
    SPI[host]->slave.trans_done   = false;
    EXIT_CRITICAL();

    spi_object[host]->intr_enable.val = intr_enable->val;

    return ESP_OK;
}

esp_err_t spi_get_mode(spi_host_t host, spi_mode_t *mode)
{
    SPI_CHECK(host < SPI_NUM_MAX, "host num error", ESP_ERR_INVALID_ARG);
    SPI_CHECK(spi_object[host], "spi has not been initialized yet", ESP_FAIL);
    SPI_CHECK(mode, "parameter pointer is empty", ESP_ERR_INVALID_ARG);

    *mode = spi_object[host]->mode;

    return ESP_OK;
}

esp_err_t spi_set_mode(spi_host_t host, spi_mode_t *mode)
{
    SPI_CHECK(host < SPI_NUM_MAX, "host num error", ESP_ERR_INVALID_ARG);
    SPI_CHECK(spi_object[host], "spi has not been initialized yet", ESP_FAIL);
    SPI_CHECK(mode, "parameter pointer is empty", ESP_ERR_INVALID_ARG);

    spi_object[host]->mode = *mode;
    ENTER_CRITICAL();
    // Disable flash operation mode
    SPI[host]->user.flash_mode = false;

    if (SPI_MASTER_MODE == *mode) {
        // Set to Master mode
        SPI[host]->pin.slave_mode = false;
        SPI[host]->slave.slave_mode = false;
        // Master uses the entire hardware buffer to improve transmission speed
        SPI[host]->user.usr_mosi_highpart = false;
        SPI[host]->user.usr_miso_highpart = false;
        SPI[host]->user.usr_mosi = true;
        // Create hardware cs in advance
        SPI[host]->user.cs_setup = true;
        // Hysteresis to keep hardware cs
        SPI[host]->user.cs_hold = true;
        SPI[host]->user.duplex = true;
        SPI[host]->user.ck_i_edge = true;
        SPI[host]->ctrl2.mosi_delay_num = 0;
        SPI[host]->ctrl2.miso_delay_num = 1;
    } else {
        // Set to Slave mode
        SPI[host]->pin.slave_mode = true;
        SPI[host]->slave.slave_mode = true;
        SPI[host]->user.usr_mosi_highpart = false;
        SPI[host]->user.usr_miso_highpart = false;
        SPI[host]->user.usr_addr = 1;
        // MOSI signals are delayed by APB_CLK(80MHz) mosi_delay_num cycles
        SPI[host]->ctrl2.mosi_delay_num = 2;
        SPI[host]->ctrl2.miso_delay_num = 0;
        SPI[host]->slave.wr_rd_buf_en = 1;
        SPI[host]->slave.wr_rd_sta_en = 1;
        SPI[host]->slave1.status_bitlen = 31;
        SPI[host]->slave1.status_readback = 0;
        // Put the slave's miso on the highpart, so you can only send 512bits
        // In Slave mode miso, mosi length is the same
        SPI[host]->slave1.buf_bitlen = 511;
        SPI[host]->slave1.wr_addr_bitlen = 7;
        SPI[host]->slave1.rd_addr_bitlen = 7;
        SPI[host]->user1.usr_addr_bitlen = 7;
        SPI[host]->user1.usr_miso_bitlen = 31;
        SPI[host]->user1.usr_mosi_bitlen = 31;
        SPI[host]->user2.usr_command_bitlen = 7;
        SPI[host]->cmd.usr = 1;
    }

    SPI[host]->user.fwrite_dual = false;
    SPI[host]->user.fwrite_quad = false;
    SPI[host]->user.fwrite_dio  = false;
    SPI[host]->user.fwrite_qio  = false;
    SPI[host]->ctrl.fread_dual  = false;
    SPI[host]->ctrl.fread_quad  = false;
    SPI[host]->ctrl.fread_dio   = false;
    SPI[host]->ctrl.fread_qio   = false;
    SPI[host]->ctrl.fastrd_mode = true;
    SPI[host]->slave.sync_reset = 1;
    EXIT_CRITICAL();

    return ESP_OK;
}

esp_err_t spi_get_interface(spi_host_t host, spi_interface_t *interface)
{
    SPI_CHECK(host < SPI_NUM_MAX, "host num error", ESP_ERR_INVALID_ARG);
    SPI_CHECK(spi_object[host], "spi has not been initialized yet", ESP_FAIL);
    SPI_CHECK(interface, "parameter pointer is empty", ESP_ERR_INVALID_ARG);

    *interface = spi_object[host]->interface;

    return ESP_OK;
}

esp_err_t spi_set_interface(spi_host_t host, spi_interface_t *interface)
{
    SPI_CHECK(host < SPI_NUM_MAX, "host num error", ESP_ERR_INVALID_ARG);
    SPI_CHECK(spi_object[host], "spi has not been initialized yet", ESP_FAIL);
    SPI_CHECK(interface, "parameter pointer is empty", ESP_ERR_INVALID_ARG);

    spi_object[host]->interface = *interface;
    ENTER_CRITICAL();

    switch (host) {
        case CSPI_HOST: {
            // Initialize SPI IO
            PIN_PULLUP_EN(PERIPHS_IO_MUX_SD_CLK_U);
            PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_CLK_U, FUNC_SPICLK);

            if (interface->mosi_en) {
                PIN_PULLUP_EN(PERIPHS_IO_MUX_SD_DATA1_U);
                PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_DATA1_U, FUNC_SPID_MOSI);
            }

            if (interface->miso_en) {
                PIN_PULLUP_EN(PERIPHS_IO_MUX_SD_DATA0_U);
                PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_DATA0_U, FUNC_SPIQ_MISO);
            }

            if (interface->cs_en) {
                PIN_PULLUP_EN(PERIPHS_IO_MUX_SD_CMD_U);
                PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_CMD_U, FUNC_SPICS0);
            }
        }
        break;

        case HSPI_HOST: {
            // Initialize HSPI IO
            PIN_PULLUP_EN(PERIPHS_IO_MUX_MTMS_U);
            PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_HSPI_CLK); //GPIO14 is SPI CLK pin (Clock)

            if (interface->mosi_en) {
                PIN_PULLUP_EN(PERIPHS_IO_MUX_MTCK_U);
                PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_HSPID_MOSI); //GPIO13 is SPI MOSI pin (Master Data Out)
            }

            if (interface->miso_en) {
                PIN_PULLUP_EN(PERIPHS_IO_MUX_MTDI_U);
                PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_HSPIQ_MISO); //GPIO12 is SPI MISO pin (Master Data In)
            }

            if (interface->cs_en) {
                PIN_PULLUP_EN(PERIPHS_IO_MUX_MTDO_U);
                PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_HSPI_CS0);
            }
        }
        break;
    }

    // Set the clock polarity and phase
    SPI[host]->pin.ck_idle_edge = interface->cpol;

    if (interface->cpol == interface->cpha) {
        SPI[host]->user.ck_out_edge = false;
    } else {
        SPI[host]->user.ck_out_edge = true;
    }

    // Set data bit order
    SPI[host]->ctrl.wr_bit_order = interface->bit_tx_order;
    SPI[host]->ctrl.rd_bit_order = interface->bit_rx_order;
    // Set data byte order
    SPI[host]->user.wr_byte_order = interface->byte_tx_order;
    SPI[host]->user.rd_byte_order = interface->byte_rx_order;
    EXIT_CRITICAL();

    return ESP_OK;
}

esp_err_t spi_get_dummy(spi_host_t host, uint16_t *bitlen)
{
    SPI_CHECK(host < SPI_NUM_MAX, "host num error", ESP_ERR_INVALID_ARG);
    SPI_CHECK(spi_object[host], "spi has not been initialized yet", ESP_FAIL);
    SPI_CHECK(bitlen, "parameter pointer is empty", ESP_ERR_INVALID_ARG);

    if (SPI[host]->user.usr_dummy) {
        *bitlen = SPI[host]->user1.usr_dummy_cyclelen + 1;
    } else {
        *bitlen = 0;
    }

    return ESP_OK;
}

esp_err_t spi_set_dummy(spi_host_t host, uint16_t *bitlen)
{
    SPI_CHECK(host < SPI_NUM_MAX, "host num error", ESP_ERR_INVALID_ARG);
    SPI_CHECK(spi_object[host], "spi has not been initialized yet", ESP_FAIL);
    SPI_CHECK(bitlen, "parameter pointer is empty", ESP_ERR_INVALID_ARG);
    SPI_CHECK(*bitlen <= 256, "spi dummy must be shorter than 256 bits", ESP_ERR_INVALID_ARG);

    ENTER_CRITICAL();

    if (*bitlen) {
        SPI[host]->user.usr_dummy = 1;
        SPI[host]->user1.usr_dummy_cyclelen = *bitlen - 1;
    } else {
        SPI[host]->user.usr_dummy = 0;
    }

    EXIT_CRITICAL();

    return ESP_OK;
}

esp_err_t spi_get_event_callback(spi_host_t host, spi_event_callback_t *event_cb)
{
    SPI_CHECK(host < SPI_NUM_MAX, "host num error", ESP_ERR_INVALID_ARG);
    SPI_CHECK(spi_object[host], "spi has not been initialized yet", ESP_FAIL);
    SPI_CHECK(event_cb, "parameter pointer is empty", ESP_ERR_INVALID_ARG);

    *event_cb = spi_object[host]->event_cb;

    return ESP_OK;
}

esp_err_t spi_set_event_callback(spi_host_t host, spi_event_callback_t *event_cb)
{
    SPI_CHECK(host < SPI_NUM_MAX, "host num error", ESP_ERR_INVALID_ARG);
    SPI_CHECK(spi_object[host], "spi has not been initialized yet", ESP_FAIL);
    SPI_CHECK(event_cb, "parameter pointer is empty", ESP_ERR_INVALID_ARG);

    spi_object[host]->event_cb = *event_cb;

    return ESP_OK;
}

esp_err_t spi_slave_get_status(spi_host_t host, uint32_t *status)
{
    SPI_CHECK(host < SPI_NUM_MAX, "host num error", ESP_ERR_INVALID_ARG);
    SPI_CHECK(spi_object[host], "spi has not been initialized yet", ESP_FAIL);
    SPI_CHECK(SPI_SLAVE_MODE == spi_object[host]->mode, "this function must used by spi slave mode", ESP_FAIL);
    SPI_CHECK(status, "parameter pointer is empty", ESP_ERR_INVALID_ARG);

    ENTER_CRITICAL();

    *status = SPI[host]->wr_status;

    EXIT_CRITICAL();

    return ESP_OK;
}

esp_err_t SPI_HIGH_THROUGHPUT_ATTR spi_slave_set_status(spi_host_t host, uint32_t *status)
{
    SPI_CHECK_HIGH_THROUGHPUT(host < SPI_NUM_MAX, "host num error", ESP_ERR_INVALID_ARG);
    SPI_CHECK_HIGH_THROUGHPUT(spi_object[host], "spi has not been initialized yet", ESP_FAIL);
    SPI_CHECK_HIGH_THROUGHPUT(SPI_SLAVE_MODE == spi_object[host]->mode, "this function must used by spi slave mode", ESP_FAIL);
    SPI_CHECK_HIGH_THROUGHPUT(status, "parameter pointer is empty", ESP_ERR_INVALID_ARG);

    ENTER_CRITICAL_HIGH_THROUGHPUT();

    SPI[host]->rd_status.val = *status;

    EXIT_CRITICAL_HIGH_THROUGHPUT();

    return ESP_OK;
}

static esp_err_t SPI_HIGH_THROUGHPUT_ATTR spi_master_trans(spi_host_t host, spi_trans_t *trans)
{
    SPI_CHECK_HIGH_THROUGHPUT(trans->bits.cmd <= 16, "spi cmd must be shorter than 16 bits", ESP_ERR_INVALID_ARG);
    SPI_CHECK_HIGH_THROUGHPUT(trans->bits.addr <= 32, "spi addr must be shorter than 32 bits", ESP_ERR_INVALID_ARG);
    SPI_CHECK_HIGH_THROUGHPUT(trans->bits.mosi <= 512, "spi mosi must be shorter than 512 bits", ESP_ERR_INVALID_ARG);
    SPI_CHECK_HIGH_THROUGHPUT(trans->bits.miso <= 512, "spi miso must be shorter than 512 bits", ESP_ERR_INVALID_ARG);

    int x, y;

    // Waiting for an incomplete transfer
    while (SPI[host]->cmd.usr);

    ENTER_CRITICAL_HIGH_THROUGHPUT();

    // Set the cmd length and transfer cmd
    if (trans->bits.cmd && trans->cmd) {
        SPI[host]->user.usr_command = 1;
        SPI[host]->user2.usr_command_bitlen = trans->bits.cmd - 1;
        SPI[host]->user2.usr_command_value = *trans->cmd;
    } else {
        SPI[host]->user.usr_command = 0;
    }

    // Set addr length and transfer addr
    if (trans->bits.addr && trans->addr) {
        SPI[host]->user.usr_addr = 1;
        SPI[host]->user1.usr_addr_bitlen = trans->bits.addr - 1;
        SPI[host]->addr = *trans->addr;
    } else {
        SPI[host]->user.usr_addr = 0;
    }

    // Set mosi length and transmit mosi
    if (trans->bits.mosi && trans->mosi) {
        SPI[host]->user.usr_mosi = 1;
        SPI[host]->user1.usr_mosi_bitlen = trans->bits.mosi - 1;
        if ((uint32_t)(trans->mosi) % 4 == 0) {
            for (x = 0; x < trans->bits.mosi; x += 32) {
                y = x / 32;
                SPI[host]->data_buf[y] = trans->mosi[y];
            }
        } else {
            ESP_LOGW(TAG,"Using unaligned data may reduce transmission efficiency");
            memset(spi_object[host]->buf, 0, sizeof(uint32_t) * 16);
            memcpy(spi_object[host]->buf, trans->mosi, trans->bits.mosi / 8 + (trans->bits.mosi % 8) ? 1 : 0);
            for (x = 0; x < trans->bits.mosi; x += 32) {
                y = x / 32;
                SPI[host]->data_buf[y] = spi_object[host]->buf[y];
            }
        }

    } else {
        SPI[host]->user.usr_mosi = 0;
    }

    // Set the length of the miso
    if (trans->bits.miso && trans->miso) {
        SPI[host]->user.usr_miso = 1;
        SPI[host]->user1.usr_miso_bitlen = trans->bits.miso - 1;
    } else {
        SPI[host]->user.usr_miso = 0;
    }

    // Call the event callback function to send a transfer start event
    if (spi_object[host]->event_cb) {
        spi_object[host]->event_cb(SPI_TRANS_START_EVENT, NULL);
    }

    // Start transmission
    SPI[host]->cmd.usr = 1;

    // Receive miso data
    if (trans->bits.miso && trans->miso) {
        while (SPI[host]->cmd.usr);

        if ((uint32_t)(trans->miso) % 4 == 0) {
            for (x = 0; x < trans->bits.miso; x += 32) {
                y = x / 32;
                trans->miso[y] = SPI[host]->data_buf[y];
            }
        } else {
            ESP_LOGW(TAG,"Using unaligned data may reduce transmission efficiency");
            memset(spi_object[host]->buf, 0, sizeof(uint32_t) * 16);
            for (x = 0; x < trans->bits.miso; x += 32) {
                y = x / 32;
                spi_object[host]->buf[y] = SPI[host]->data_buf[y];
            }
            memcpy(trans->miso, spi_object[host]->buf, trans->bits.miso / 8 + (trans->bits.miso % 8) ? 1 : 0);
        }
    }

    EXIT_CRITICAL_HIGH_THROUGHPUT();

    return ESP_OK;
}

static esp_err_t SPI_HIGH_THROUGHPUT_ATTR spi_slave_trans(spi_host_t host, spi_trans_t *trans)
{
    SPI_CHECK_HIGH_THROUGHPUT(trans->bits.cmd >= 3 && trans->bits.cmd <= 16, "spi cmd must be longer than 3 bits and shorter than 16 bits", ESP_ERR_INVALID_ARG);
    SPI_CHECK_HIGH_THROUGHPUT(trans->bits.addr >= 1 && trans->bits.addr <= 32, "spi addr must be longer than 1 bits and shorter than 32 bits", ESP_ERR_INVALID_ARG);
    SPI_CHECK_HIGH_THROUGHPUT(trans->bits.miso <= 512, "spi miso must be shorter than 512 bits", ESP_ERR_INVALID_ARG);
    SPI_CHECK_HIGH_THROUGHPUT(trans->bits.mosi <= 512, "spi mosi must be shorter than 512 bits", ESP_ERR_INVALID_ARG);

    int x, y;
    ENTER_CRITICAL_HIGH_THROUGHPUT();
    // Set cmd length and receive cmd
    SPI[host]->user2.usr_command_bitlen = trans->bits.cmd - 1;

    if (trans->cmd) {
        *trans->cmd = SPI[host]->user2.usr_command_value;
    }

    // Set addr length and transfer addr
    SPI[host]->slave1.wr_addr_bitlen = trans->bits.addr - 1;
    SPI[host]->slave1.rd_addr_bitlen = trans->bits.addr - 1;

    if (trans->addr) {
        *trans->addr = SPI[host]->addr;
    }

    // Set the length of the miso and transfer the miso
    if (trans->bits.miso && trans->miso) {
        if ((uint32_t)(trans->miso) % 4 == 0) {
            for (x = 0; x < trans->bits.miso; x += 32) {
                y = x / 32;
                SPI[host]->data_buf[y] = trans->miso[y];
            }
        } else {
            ESP_LOGW(TAG,"Using unaligned data may reduce transmission efficiency");
            memset(spi_object[host]->buf, 0, sizeof(uint32_t) * 16);
            memcpy(spi_object[host]->buf, trans->miso, trans->bits.miso / 8 + (trans->bits.miso % 8) ? 1 : 0);
            for (x = 0; x < trans->bits.miso; x += 32) {
                y = x / 32;
                SPI[host]->data_buf[y] = spi_object[host]->buf[y];
            }
        }
    }

    // Receive mosi data
    if (trans->bits.mosi && trans->mosi) {
        if ((uint32_t)(trans->mosi) % 4 == 0) {
            for (x = 0; x < trans->bits.mosi; x += 32) {
                y = x / 32;
                trans->mosi[y] = SPI[host]->data_buf[y];
            }
        } else {
            ESP_LOGW(TAG,"Using unaligned data may reduce transmission efficiency");
            memset(spi_object[host]->buf, 0, sizeof(uint32_t) * 16);
            for (x = 0; x < trans->bits.mosi; x += 32) {
                y = x / 32;
                spi_object[host]->buf[y] = SPI[host]->data_buf[y];
            }
            memcpy(trans->mosi, spi_object[host]->buf, trans->bits.mosi / 8 + (trans->bits.mosi % 8) ? 1 : 0);
        }
    }

    EXIT_CRITICAL_HIGH_THROUGHPUT();

    return ESP_OK;
}

static esp_err_t SPI_HIGH_THROUGHPUT_ATTR spi_trans_static(spi_host_t host, spi_trans_t *trans)
{
    int ret;
    if (SPI_MASTER_MODE == spi_object[host]->mode) {
        ret = spi_master_trans(host, trans);
    } else {
        ret = spi_slave_trans(host, trans);
    }

    return ret;
}

esp_err_t SPI_HIGH_THROUGHPUT_ATTR spi_trans(spi_host_t host, spi_trans_t *trans)
{
    SPI_CHECK_HIGH_THROUGHPUT(host < SPI_NUM_MAX, "host num error", ESP_ERR_INVALID_ARG);
    SPI_CHECK_HIGH_THROUGHPUT(spi_object[host], "spi has not been initialized yet", ESP_FAIL);
    SPI_CHECK_HIGH_THROUGHPUT(trans->bits.val, "trans bits is empty", ESP_ERR_INVALID_ARG);

    int ret;
    if (xPortInIsrContext()) {
        /* In ISR Context */
        BaseType_t higher_task_woken = false;
        if (xSemaphoreTakeFromISR(spi_object[host]->trans_mux, NULL) != pdTRUE) {
            return ESP_FAIL;
        }
        ret = spi_trans_static(host, trans);
        xSemaphoreGiveFromISR(spi_object[host]->trans_mux, &higher_task_woken);
        if (higher_task_woken) {
            portYIELD_FROM_ISR();
        }
    }
    else {
        xSemaphoreTake(spi_object[host]->trans_mux, portMAX_DELAY);
        ret = spi_trans_static(host, trans);
        xSemaphoreGive(spi_object[host]->trans_mux);
    }
    return ret;
}

static IRAM_ATTR void spi_intr(void *arg)
{
    spi_host_t host;
    uint32_t trans_done;
    uint32_t cnt = 0;
    if (READ_PERI_REG(DPORT_SPI_INT_STATUS_REG) & DPORT_SPI_INT_STATUS_SPI0) { // DPORT_SPI_INT_STATUS_SPI0
        trans_done = SPI0.slave.val & 0x1F;
        SPI0.slave.val &= ~0x3FF;
        host = CSPI_HOST;
    } else if (READ_PERI_REG(DPORT_SPI_INT_STATUS_REG) & DPORT_SPI_INT_STATUS_SPI1) { // DPORT_SPI_INT_STATUS_SPI1
        trans_done = SPI1.slave.val & 0x1F;
        SPI1.slave.val &= ~0x1F;
        // Hardware issues: We need to wait for the hardware to clear the registers successfully.
        while ((SPI1.slave.val & 0x1F) != 0) {
            if (cnt >= 50) {
                ets_printf("WARNING: waiting too much time, maybe error\r\n");
                cnt = 0;
            }
            SPI1.slave.val &= ~0x1F;
            cnt++;
        }

        host = HSPI_HOST;
    } else {
        return;
    }

    if (spi_object[host]) {
        // Hardware has no interrupt flag, which can be generated by software.
        trans_done &=  spi_object[host]->intr_enable.val;
        if (spi_object[host]->event_cb && trans_done != 0) {
            spi_object[host]->event_cb(SPI_TRANS_DONE_EVENT, &trans_done);
        }
    }
}

esp_err_t spi_deinit(spi_host_t host)
{
    SPI_CHECK(host < SPI_NUM_MAX, "host num error", ESP_ERR_INVALID_ARG);
    SPI_CHECK(spi_object[host], "spi has not been initialized yet", ESP_FAIL);

    spi_intr_enable_t intr_enable;
    // Turn off the current host interrupt enable
    intr_enable.val = 0;
    spi_set_intr_enable(host, &intr_enable);

    // Turn off the SPI interrupt if all other hosts are not initialized
    if (host == CSPI_HOST) {
        if (spi_object[HSPI_HOST] == NULL) {
            spi_intr_disable();
        }
    } else {
        if (spi_object[CSPI_HOST] == NULL) {
            spi_intr_disable();
        }
    }

    // Waiting for all transfers to complete
    while (SPI[host]->cmd.usr);

    // Call the event callback function to send the SPI_DEINIT event
    if (spi_object[host]->event_cb) {
        spi_object[host]->event_cb(SPI_DEINIT_EVENT, NULL);
    }

    if (spi_object[host]->trans_mux) {
        vSemaphoreDelete(spi_object[host]->trans_mux);
    }
    heap_caps_free(spi_object[host]->buf);
    spi_object[host]->buf = NULL;

    heap_caps_free(spi_object[host]);
    spi_object[host] = NULL;

    return ESP_OK;
}

esp_err_t spi_init(spi_host_t host, spi_config_t *config)
{
    SPI_CHECK(host < SPI_NUM_MAX, "host num error", ESP_ERR_INVALID_ARG);
    SPI_CHECK(host > CSPI_HOST, "CSPI_HOST can't support now", ESP_FAIL);
    SPI_CHECK(NULL == spi_object[host], "spi has been initialized", ESP_FAIL);

    spi_object[host] = (spi_object_t *)heap_caps_malloc(sizeof(spi_object_t), MALLOC_CAP_8BIT);
    SPI_CHECK(spi_object[host], "malloc fail", ESP_ERR_NO_MEM);
    spi_object[host]->trans_mux = xSemaphoreCreateMutex();
#ifdef CONFIG_ESP8266_HSPI_HIGH_THROUGHPUT
    spi_object[host]->buf = (uint32_t *)heap_caps_malloc(sizeof(uint32_t) * 16, MALLOC_CAP_8BIT);
#else
    spi_object[host]->buf = (uint32_t *)heap_caps_malloc(sizeof(uint32_t) * 16, MALLOC_CAP_32BIT);
#endif
    if (NULL == spi_object[host]->trans_mux || NULL == spi_object[host]->buf) {
        spi_deinit(host);
        SPI_CHECK(false, "no memory", ESP_ERR_NO_MEM);
    }
    uint16_t dummy_bitlen = 0;
    spi_set_event_callback(host, &config->event_cb);
    spi_set_mode(host, &config->mode);
    spi_set_interface(host, &config->interface);
    spi_set_clk_div(host, &config->clk_div);
    spi_set_dummy(host, &dummy_bitlen);

    spi_set_intr_enable(host, &config->intr_enable);
    spi_intr_register(spi_intr, NULL);
    spi_intr_enable();

    if (spi_object[host]->event_cb) {
        spi_object[host]->event_cb(SPI_INIT_EVENT, NULL);
    }
    return ESP_OK;
}

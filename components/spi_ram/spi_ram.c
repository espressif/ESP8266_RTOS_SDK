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
#include <string.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp8266/eagle_soc.h"
#include "esp8266/spi_struct.h"
#include "esp8266/pin_mux_register.h"
#include "esp8266/spi_register.h"
#include "esp8266/gpio_struct.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "spi_ram.h"

#define SPI_RAM_ENTER_CRITICAL()              portENTER_CRITICAL()
#define SPI_RAM_EXIT_CRITICAL()               portEXIT_CRITICAL()

static const char *TAG = "spi_ram";

#define SPI_RAM_CHECK(a, str, ret_val) \
    if (!(a)) { \
        ESP_LOGE(TAG,"%s(%d): %s", __FUNCTION__, __LINE__, str); \
        return (ret_val); \
    }

typedef struct {
    uint32_t size;
    uint32_t read_wait_cycle;
    spi_ram_cmd_t cmd;
    uint32_t clock;
    bool qpi_flag;
    SemaphoreHandle_t mux;
} spi_ram_obj_t;

spi_ram_obj_t *spi_ram_obj[SPI_RAM_NUM_MAX] = {NULL};

static esp_err_t IRAM_ATTR spi_ram_write_cmd(spi_ram_num_t num, uint8_t cmd)
{
    SPI_RAM_CHECK((num < SPI_RAM_NUM_MAX), "spi ram num error", ESP_ERR_INVALID_ARG);
    SPI_RAM_CHECK(spi_ram_obj[num], "spi ram not installed yet", ESP_FAIL);

    xSemaphoreTake(spi_ram_obj[num]->mux, (portTickType)portMAX_DELAY);
    SPI_RAM_ENTER_CRITICAL();

    while (SPI1.cmd.usr);
    if (spi_ram_obj[num]->qpi_flag == false) {
        SPI1.user.val |= SPI_CS_SETUP | SPI_CS_HOLD | SPI_USR_COMMAND;
        SPI1.user.val &= ~(SPI_FLASH_MODE | SPI_USR_ADDR | SPI_USR_DUMMY | SPI_USR_MOSI | SPI_USR_MISO | SPI_FWRITE_QIO);
        // SPI_FLASH_USER2 bit28-31 is cmd length,cmd bit length is value(0-15)+1,
        // bit15-0 is cmd value.
        SPI1.user2.val = ((7 & SPI_USR_COMMAND_BITLEN) << SPI_USR_COMMAND_BITLEN_S) | ((uint32_t)cmd);
        SPI1.ctrl.val &= ~(SPI_QIO_MODE | SPI_FASTRD_MODE);
    } else {
        SPI1.user.val |= SPI_CS_SETUP | SPI_CS_HOLD | SPI_USR_MOSI | SPI_FWRITE_QIO;
        SPI1.user.val &= ~(SPI_FLASH_MODE | SPI_USR_COMMAND | SPI_USR_ADDR | SPI_USR_DUMMY | SPI_USR_MISO );
        SPI1.user1.usr_mosi_bitlen = 7; // 8bits cmd
        SPI1.data_buf[0] = cmd & 0xFF; // write cmd
        SPI1.ctrl.val |= SPI_QIO_MODE | SPI_FASTRD_MODE;
    }

    if (SPI_RAM_NUM_1 == num) {
        SPI1.pin.cs2_dis = true;

        while (SPI0.cmd.usr);

        GPIO.out_w1tc |= GPIO_Pin_5;
        SPI1.cmd.usr = true;

        while (SPI1.cmd.usr);

        GPIO.out_w1ts |= GPIO_Pin_5;
    } else {
        SPI1.pin.cs2_dis = false;
        SPI1.cmd.usr = true;
    }

    SPI_RAM_EXIT_CRITICAL();
    xSemaphoreGive(spi_ram_obj[num]->mux);
    return ESP_OK;
}

esp_err_t IRAM_ATTR spi_ram_write(spi_ram_num_t num, uint32_t addr, uint8_t *data, int len)
{
    SPI_RAM_CHECK((num < SPI_RAM_NUM_MAX), "spi ram num error", ESP_ERR_INVALID_ARG);
    SPI_RAM_CHECK(spi_ram_obj[num], "spi ram not installed yet", ESP_FAIL);
    SPI_RAM_CHECK(data, "param null", ESP_ERR_INVALID_ARG);
    SPI_RAM_CHECK(addr + len <= spi_ram_obj[num]->size, "Address out of range", ESP_ERR_INVALID_ARG);
    SPI_RAM_CHECK((len > 0) && (len <= 64), "len error", ESP_ERR_INVALID_ARG);
    uint32_t i;
    uint32_t buf;
    xSemaphoreTake(spi_ram_obj[num]->mux, (portTickType)portMAX_DELAY);
    SPI_RAM_ENTER_CRITICAL();

    while (SPI1.cmd.usr);

    SPI1.clock.val = spi_ram_obj[num]->clock;

    if (spi_ram_obj[num]->qpi_flag == false) {
        SPI1.user.val |= SPI_CS_SETUP | SPI_CS_HOLD | SPI_USR_COMMAND | SPI_USR_ADDR | SPI_USR_MOSI;
        SPI1.user.val &= ~(SPI_FLASH_MODE | SPI_USR_MISO | SPI_USR_DUMMY | SPI_FWRITE_QIO);
        SPI1.user1.val = ((((8 * len) - 1)&SPI_USR_MOSI_BITLEN) << SPI_USR_MOSI_BITLEN_S) | // len bitsbits of data out
                         ((0 & SPI_USR_MISO_BITLEN) << SPI_USR_MISO_BITLEN_S) | // no data in
                         ((23 & SPI_USR_ADDR_BITLEN) << SPI_USR_ADDR_BITLEN_S); // address is 24 bits A0-A23
        SPI1.addr = addr << 8; // write address
        SPI1.user2.val = (((7 & SPI_USR_COMMAND_BITLEN) << SPI_USR_COMMAND_BITLEN_S) | spi_ram_obj[num]->cmd.write);
        SPI1.ctrl.val &= ~(SPI_QIO_MODE | SPI_FASTRD_MODE);
    } else {
        SPI1.user.val |= SPI_CS_SETUP | SPI_CS_HOLD | SPI_USR_ADDR | SPI_USR_MOSI | SPI_FWRITE_QIO;
        SPI1.user.val &= ~(SPI_FLASH_MODE | SPI_USR_MISO | SPI_USR_COMMAND | SPI_USR_DUMMY);
        SPI1.user1.val = ((((8 * len) - 1)&SPI_USR_MOSI_BITLEN) << SPI_USR_MOSI_BITLEN_S) | // len bitsbits of data out
                         ((0 & SPI_USR_MISO_BITLEN) << SPI_USR_MISO_BITLEN_S) | // no data i
                         ((31 & SPI_USR_ADDR_BITLEN) << SPI_USR_ADDR_BITLEN_S); // 8bits command+address is 24 bits A0-A23
        SPI1.addr = addr | (spi_ram_obj[num]->cmd.write<<24); // write address
        SPI1.ctrl.val |= SPI_QIO_MODE | SPI_FASTRD_MODE;
    }

    // Assume unaligned src: Copy byte-wise.
    for (i = 0; i < (len + 3) / 4; i++) {
        buf = *(uint32_t *)(data + i*4);
        SPI1.data_buf[i] = buf;
    }

    if (SPI_RAM_NUM_1 == num) {
        SPI1.pin.cs2_dis = true;

        while (SPI0.cmd.usr);

        GPIO.out_w1tc |= GPIO_Pin_5;
        SPI1.cmd.usr = true;

        while (SPI1.cmd.usr);

        GPIO.out_w1ts |= GPIO_Pin_5;
    } else {
        SPI1.pin.cs2_dis = false;
        SPI1.cmd.usr = true;
    }

    SPI_RAM_EXIT_CRITICAL();
    xSemaphoreGive(spi_ram_obj[num]->mux);
    return ESP_OK;
}

esp_err_t IRAM_ATTR spi_ram_read(spi_ram_num_t num, uint32_t addr, uint8_t *data, int len)
{
    SPI_RAM_CHECK((num < SPI_RAM_NUM_MAX), "spi ram num error", ESP_ERR_INVALID_ARG);
    SPI_RAM_CHECK(spi_ram_obj[num], "spi ram not installed yet", ESP_FAIL);
    SPI_RAM_CHECK(data, "param null", ESP_ERR_INVALID_ARG);
    SPI_RAM_CHECK(addr + len <= spi_ram_obj[num]->size, "Address out of range", ESP_ERR_INVALID_ARG);
    SPI_RAM_CHECK((len > 0) && (len <= 64), "len error", ESP_ERR_INVALID_ARG);
    uint32_t buf = 0;
    int i = 0;
    xSemaphoreTake(spi_ram_obj[num]->mux, (portTickType)portMAX_DELAY);
    SPI_RAM_ENTER_CRITICAL();

    while (SPI1.cmd.usr);

    SPI1.clock.val = spi_ram_obj[num]->clock;

    if (spi_ram_obj[num]->qpi_flag == false) {
        SPI1.user.val |= SPI_CS_SETUP | SPI_CS_HOLD | SPI_USR_COMMAND | SPI_USR_ADDR | SPI_USR_MISO;
        SPI1.user.val &= ~(SPI_FLASH_MODE | SPI_USR_MOSI | SPI_FWRITE_QIO);
        SPI1.user1.val = ((0 & SPI_USR_MOSI_BITLEN) << SPI_USR_MOSI_BITLEN_S) | // no data out
                         ((((8 * len) - 1)&SPI_USR_MISO_BITLEN) << SPI_USR_MISO_BITLEN_S) | // len bits of data in
                         ((23 & SPI_USR_ADDR_BITLEN) << SPI_USR_ADDR_BITLEN_S); // address is 24 bits A0-A23
        if (spi_ram_obj[num]->read_wait_cycle != 0x0) {
            SPI1.user.usr_dummy = true;
            SPI1.user1.usr_dummy_cyclelen = spi_ram_obj[num]->read_wait_cycle - 1; // 1 dummy cycle = 1 spi clk cycle
        } else {
            SPI1.user.usr_dummy = false;
        }
        SPI1.addr = addr << 8; // write address
        SPI1.user2.val = (((7 & SPI_USR_COMMAND_BITLEN) << SPI_USR_COMMAND_BITLEN_S) | spi_ram_obj[num]->cmd.read);
        SPI1.ctrl.val &= ~(SPI_QIO_MODE | SPI_FASTRD_MODE);
    } else {
        SPI1.user.val |= SPI_CS_SETUP | SPI_CS_HOLD | SPI_USR_ADDR | SPI_USR_MISO | SPI_USR_DUMMY | SPI_FWRITE_QIO;
        SPI1.user.val &= ~(SPI_FLASH_MODE | SPI_USR_MOSI | SPI_USR_COMMAND);
        SPI1.user1.val = ((0 & SPI_USR_MOSI_BITLEN) << SPI_USR_MOSI_BITLEN_S) | // no data out
                         ((((8 * len) - 1)&SPI_USR_MISO_BITLEN) << SPI_USR_MISO_BITLEN_S) | // len bits of data in
                         ((31 & SPI_USR_ADDR_BITLEN) << SPI_USR_ADDR_BITLEN_S);           // 8bits command+address is 24 bits A0-A23
        if (spi_ram_obj[num]->read_wait_cycle != 0x0) {
            SPI1.user.usr_dummy = true;
            SPI1.user1.usr_dummy_cyclelen = spi_ram_obj[num]->read_wait_cycle - 1; // 1 dummy cycle = 1 spi clk cycle
        } else {
            SPI1.user.usr_dummy = false;
        }
        SPI1.addr = addr | (spi_ram_obj[num]->cmd.read<<24); // write address
        SPI1.ctrl.val |= SPI_QIO_MODE | SPI_FASTRD_MODE;
    }

    if (SPI_RAM_NUM_1 == num) {
        SPI1.pin.cs2_dis = true;

        while (SPI0.cmd.usr);

        GPIO.out_w1tc |= GPIO_Pin_5;
        SPI1.cmd.usr = true;

        while (SPI1.cmd.usr);

        GPIO.out_w1ts |= GPIO_Pin_5;
    } else {
        SPI1.pin.cs2_dis = false;
        SPI1.cmd.usr = true;

        while (SPI1.cmd.usr);
    }

    // Unaligned dest address. Copy 8bit at a time
    while (len > 0) {
        buf = SPI1.data_buf[i];
        data[i * 4 + 0] = (buf >> 0) & 0xff;

        if (len > 1) {
            data[i * 4 + 1] = (buf >> 8) & 0xff;
        }

        if (len > 2) {
            data[i * 4 + 2] = (buf >> 16) & 0xff;
        }

        if (len > 3) {
            data[i * 4 + 3] = (buf >> 24) & 0xff;
        }

        len -= 4;
        i++;
    }

    SPI_RAM_EXIT_CRITICAL();
    xSemaphoreGive(spi_ram_obj[num]->mux);
    return ESP_OK;
}

esp_err_t spi_ram_check(spi_ram_num_t num)
{
    SPI_RAM_CHECK((num < SPI_RAM_NUM_MAX), "spi ram num error", ESP_ERR_INVALID_ARG);
    SPI_RAM_CHECK(spi_ram_obj[num], "spi ram not installed yet", ESP_FAIL);
    int x;
    int err = 0;
    uint8_t a[64];
    uint8_t b[64];
    uint8_t aa, bb;

    for (x = 0; x < 64; x++) {
        a[x] = x ^ (x << 2);
        b[x] = 0xaa ^ x;
    }

    spi_ram_write(num, 0x0, a, 64);
    spi_ram_write(num, 0x100, b, 64);

    spi_ram_read(num, 0x0, a, 64);
    spi_ram_read(num, 0x100, b, 64);

    for (x = 0; x < 64; x++) {
        aa = x ^ (x << 2);
        bb = 0xaa ^ x;

        if (aa != a[x]) {
            err = 1;
            ESP_LOGE(TAG, "num[%d]: aa: 0x%x != 0x%x\n", num, aa, a[x]);
        }

        if (bb != b[x]) {
            err = 1;
            ESP_LOGE(TAG, "num[%d]: bb: 0x%x != 0x%x\n", num, bb, b[x]);
        }
    }

    if (err) {
        return ESP_FAIL;
    } else {
        return ESP_OK;
    }
}

esp_err_t spi_ram_set_clk_div(spi_ram_num_t num, spi_ram_clk_div_t *clk_div)
{
    SPI_RAM_CHECK((num < SPI_RAM_NUM_MAX), "spi ram num error", ESP_ERR_INVALID_ARG);
    SPI_RAM_CHECK(spi_ram_obj[num], "spi ram not installed yet", ESP_FAIL);
    SPI_RAM_CHECK(clk_div && *clk_div > 0, "parameter pointer is empty", ESP_ERR_INVALID_ARG);
#ifdef CONFIG_ESPTOOLPY_FLASHFREQ_80M
    SPI_RAM_CHECK(SPI_RAM_80MHz_DIV == *clk_div, "Since the SPI Flash clock frequency is set to 80MHz, the SPI ram clock frequency also needs to be set to 80MHz.", ESP_ERR_INVALID_ARG);
#else
    SPI_RAM_CHECK(SPI_RAM_80MHz_DIV != *clk_div, "SPI Flash clock frequency also need to be set to 80MHz", ESP_ERR_INVALID_ARG);
#endif
    SPI_RAM_ENTER_CRITICAL();

    if (SPI_RAM_80MHz_DIV == *clk_div) {
        SET_PERI_REG_MASK(PERIPHS_IO_MUX_CONF_U, SPI0_CLK_EQU_SYS_CLK);
        SPI1.clock.clk_equ_sysclk = true;
    } else {
        CLEAR_PERI_REG_MASK(PERIPHS_IO_MUX_CONF_U, SPI0_CLK_EQU_SYS_CLK);
        // FRE(SCLK) = clk_equ_sysclk ? 80MHz : APB_CLK(80MHz) / clkdiv_pre / clkcnt
        SPI1.clock.clk_equ_sysclk = false;
        SPI1.clock.clkdiv_pre = 0;
        SPI1.clock.clkcnt_n = *clk_div - 1;
        // In the master mode clkcnt_h = floor((clkcnt_n+1)/2-1). In the slave mode it must be 0
        SPI1.clock.clkcnt_h = *clk_div / 2 - 1;
        // In the master mode clkcnt_l = clkcnt_n. In the slave mode it must be 0
        SPI1.clock.clkcnt_l = *clk_div - 1;
    }

    spi_ram_obj[num]->clock = SPI1.clock.val;

    SPI_RAM_EXIT_CRITICAL();

    return ESP_OK;
}

esp_err_t spi_ram_deinit(spi_ram_num_t num)
{
    SPI_RAM_CHECK((num < SPI_RAM_NUM_MAX), "spi ram num error", ESP_ERR_INVALID_ARG);
    SPI_RAM_CHECK(spi_ram_obj[num], "spi ram not installed yet", ESP_FAIL);

    if (spi_ram_obj[num]->qpi_flag == true) {
        spi_ram_write_cmd(num, spi_ram_obj[num]->cmd.exit);
        spi_ram_obj[num]->qpi_flag = false;
    }

    if (spi_ram_obj[num]->mux) {
        xSemaphoreTake(spi_ram_obj[num]->mux, (portTickType)portMAX_DELAY);
    }

    vSemaphoreDelete(spi_ram_obj[num]->mux);
    heap_caps_free(spi_ram_obj[num]);
    spi_ram_obj[num] = NULL;

    return ESP_OK;
}

esp_err_t spi_ram_init(spi_ram_num_t num, spi_ram_config_t *config)
{
    SPI_RAM_CHECK((num < SPI_RAM_NUM_MAX), "spi ram num error", ESP_ERR_INVALID_ARG);
    SPI_RAM_CHECK(config, "param null", ESP_ERR_INVALID_ARG);
    SPI_RAM_CHECK(config->cmd.val != 0, "cmd error", ESP_ERR_INVALID_ARG);
    SPI_RAM_CHECK(spi_ram_obj[num] == NULL, "spi ram has been initialized", ESP_FAIL);

    spi_ram_obj[num] = (spi_ram_obj_t *)heap_caps_zalloc(sizeof(spi_ram_obj_t), MALLOC_CAP_8BIT);
    SPI_RAM_CHECK(spi_ram_obj[num], "malloc spi ram obj error", ESP_ERR_NO_MEM);
    spi_ram_obj[num]->mux = xSemaphoreCreateMutex();

    if (NULL == spi_ram_obj[num]->mux) {
        spi_ram_deinit(num);
        return ESP_ERR_NO_MEM;
    }

    spi_ram_obj[num]->size = config->size;
    spi_ram_obj[num]->cmd = config->cmd;
    spi_ram_obj[num]->read_wait_cycle = config->read_wait_cycle;
    uint8_t dummy[64];
    spi_ram_set_clk_div(num, &config->clk_div);
    SPI_RAM_ENTER_CRITICAL();
    // hspi overlap to spi, two spi masters on cspi
    SET_PERI_REG_MASK(HOST_INF_SEL, PERI_IO_CSPI_OVERLAP);
    // set higher priority for spi than hspi
    SPI0.ext3.val = 0x1;
    SPI1.ext3.val = 0x3;
    SPI1.user.cs_setup = true;
    SPI1.user.usr_mosi_highpart = false;
    SPI1.user.usr_miso_highpart = false;
    
    if (SPI_RAM_NUM_1 == num) {
        // Using GPIO5 to simulate CS
        WRITE_PERI_REG(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);
        GPIO.out_w1ts |= GPIO_Pin_5;
        GPIO.enable_w1ts |= GPIO_Pin_5;
        SPI1.pin.val |= SPI_CS0_DIS | SPI_CS1_DIS | SPI_CS2_DIS;
    } else {
        // select HSPI CS2 ,disable HSPI CS0 and CS1
        SPI1.pin.cs2_dis = false;
        SPI1.pin.val |= SPI_CS0_DIS | SPI_CS1_DIS;
        // SET IO MUX FOR GPIO0 , SELECT PIN FUNC AS SPI CS2
        // IT WORK AS HSPI CS2 AFTER OVERLAP(THERE IS NO PIN OUT FOR NATIVE HSPI CS1/2)
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_SPICS2);
    }

    if (spi_ram_obj[num]->cmd.start != 0x0) {
        spi_ram_write_cmd(num, spi_ram_obj[num]->cmd.start);
        spi_ram_obj[num]->qpi_flag = true;
    }

    SPI_RAM_EXIT_CRITICAL();

    //Dummy read to clear any weird state the SPI ram chip may be in
    spi_ram_read(num, 0x0, dummy, 64);

    return ESP_OK;
}
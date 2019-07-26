/* spi_oled example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp8266/gpio_struct.h"
#include "esp8266/spi_struct.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_libc.h"

#include "driver/gpio.h"
#include "driver/spi.h"

static const char *TAG = "spi_oled";

#define OLED_DC_GPIO     12
#define OLED_RST_GPIO    15
#define OLED_PIN_SEL  (1ULL<<OLED_DC_GPIO) | (1ULL<<OLED_RST_GPIO)

static uint8_t oled_dc_level = 0;

static esp_err_t oled_delay_ms(uint32_t time)
{
    vTaskDelay(time / portTICK_RATE_MS);
    return ESP_OK;
}

static esp_err_t oled_set_dc(uint8_t dc)
{
    oled_dc_level = dc;
    return ESP_OK;
}

// Write an 8-bit cmd
static esp_err_t oled_write_cmd(uint8_t data)
{
    uint32_t buf = data << 24; // In order to improve the transmission efficiency, it is recommended that the external incoming data is (uint32_t *) type data, do not use other type data.
    spi_trans_t trans = {0};
    trans.mosi = &buf;
    trans.bits.mosi = 8;
    oled_set_dc(0);
    spi_trans(HSPI_HOST, &trans);
    return ESP_OK;
}

static esp_err_t oled_rst()
{
    gpio_set_level(OLED_RST_GPIO, 0);
    oled_delay_ms(200);
    gpio_set_level(OLED_RST_GPIO, 1);
    oled_delay_ms(100);
    return ESP_OK;
}

static esp_err_t oled_init()
{
    oled_rst(); // Reset OLED
    oled_write_cmd(0xAE);    // Set Display ON/OFF (AEh/AFh)
    oled_write_cmd(0x00);    // Set Lower Column Start Address for Page Addressing Mode (00h~0Fh)
    oled_write_cmd(0x10);    // Set Higher Column Start Address for Page Addressing Mode (10h~1Fh)
    oled_write_cmd(0x40);    // Set Display Start Line (40h~7Fh)
    oled_write_cmd(0x81);    // Set Contrast Control for BANK0 (81h)
    oled_write_cmd(0xCF);    //
    oled_write_cmd(0xA1);    // Set Segment Re-map (A0h/A1h)
    oled_write_cmd(0xC8);    // Set COM Output Scan Direction (C0h/C8h)
    oled_write_cmd(0xA6);    // Set Normal/Inverse Display (A6h/A7h)
    oled_write_cmd(0xA8);    // Set Multiplex Ratio (A8h)
    oled_write_cmd(0x3F);    //
    oled_write_cmd(0xD3);    // Set Display Offset (D3h)
    oled_write_cmd(0x00);    // Set Lower Column Start Address for Page Addressing Mode (00h~0Fh)
    oled_write_cmd(0xD5);    // Set Display Clock Divide Ratio/ Oscillator Frequency (D5h)
    oled_write_cmd(0x80);    //
    oled_write_cmd(0xD9);    // Set Pre-charge Period (D9h)
    oled_write_cmd(0xF1);    //
    oled_write_cmd(0xDA);    // Set COM Pins Hardware Configuration (DAh)
    oled_write_cmd(0x12);    // Set Higher Column Start Address for Page Addressing Mode (10h~1Fh)
    oled_write_cmd(0xDB);    // Set VCOMH  Deselect Level (DBh)
    oled_write_cmd(0x40);    // Set Display Start Line (40h~7Fh)
    oled_write_cmd(0x20);    // Set Memory Addressing Mode (20h)
    oled_write_cmd(0x02);    // Set Lower Column Start Address for Page Addressing Mode (00h~0Fh)
    oled_write_cmd(0x8D);    //
    oled_write_cmd(0x14);    // Set Higher Column Start Address for Page Addressing Mode (10h~1Fh)
    oled_write_cmd(0xA4);    // Entire Display ON (A4h/A5h)
    oled_write_cmd(0xA6);    // Set Normal/Inverse Display (A6h/A7h)
    oled_write_cmd(0xAF);    // Set Display ON/OFF (AEh/AFh)
    return ESP_OK;
}

static esp_err_t oled_set_pos(uint8_t x_start, uint8_t y_start)
{
    oled_write_cmd(0xb0 + y_start);
    oled_write_cmd(((x_start & 0xf0) >> 4) | 0x10);
    oled_write_cmd((x_start & 0x0f) | 0x01);
    return ESP_OK;
}

static esp_err_t oled_clear(uint8_t data)
{
    uint8_t x;
    uint32_t buf[16];
    spi_trans_t trans = {0};
    trans.mosi = buf;
    trans.bits.mosi = 64 * 8;

    for (x = 0; x < 16; x++) {
        buf[x] = data << 24 | data << 16 | data << 8 | data;
    }

    // SPI transfers 64 bytes at a time, transmits twice, increasing the screen refresh rate
    for (x = 0; x < 8; x++) {
        oled_set_pos(0, x);
        oled_set_dc(1);
        spi_trans(HSPI_HOST, &trans);
        spi_trans(HSPI_HOST, &trans);
    }

    return ESP_OK;
}

static void IRAM_ATTR spi_event_callback(int event, void *arg)
{
    switch (event) {
        case SPI_INIT_EVENT: {

        }
        break;

        case SPI_TRANS_START_EVENT: {
            gpio_set_level(OLED_DC_GPIO, oled_dc_level);
        }
        break;

        case SPI_TRANS_DONE_EVENT: {

        }
        break;

        case SPI_DEINIT_EVENT: {
        }
        break;
    }
}

void app_main(void)
{
    uint8_t x = 0;

    ESP_LOGI(TAG, "init gpio");
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = OLED_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "init hspi");
    spi_config_t spi_config;
    // Load default interface parameters
    // CS_EN:1, MISO_EN:1, MOSI_EN:1, BYTE_TX_ORDER:1, BYTE_TX_ORDER:1, BIT_RX_ORDER:0, BIT_TX_ORDER:0, CPHA:0, CPOL:0
    spi_config.interface.val = SPI_DEFAULT_INTERFACE;
    // Load default interrupt enable
    // TRANS_DONE: true, WRITE_STATUS: false, READ_STATUS: false, WRITE_BUFFER: false, READ_BUFFER: false
    spi_config.intr_enable.val = SPI_MASTER_DEFAULT_INTR_ENABLE;
    // Cancel hardware cs
    spi_config.interface.cs_en = 0;
    // MISO pin is used for DC
    spi_config.interface.miso_en = 0;
    // CPOL: 1, CPHA: 1
    spi_config.interface.cpol = 1;
    spi_config.interface.cpha = 1;
    // Set SPI to master mode
    // 8266 Only support half-duplex
    spi_config.mode = SPI_MASTER_MODE;
    // Set the SPI clock frequency division factor
    spi_config.clk_div = SPI_10MHz_DIV;
    // Register SPI event callback function
    spi_config.event_cb = spi_event_callback;
    spi_init(HSPI_HOST, &spi_config);

    ESP_LOGI(TAG, "init oled");
    oled_init();
    oled_clear(0x00);

    while (1) {
        oled_clear(x);
        oled_delay_ms(1000);
        x++;
    }
}
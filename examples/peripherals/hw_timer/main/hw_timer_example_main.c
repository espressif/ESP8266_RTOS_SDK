/* hw_timer example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"

#include "driver/gpio.h"
#include "driver/hw_timer.h"

static const char *TAG = "hw_timer_example";

#define TEST_ONE_SHOT    false        // testing will be done without auto reload (one-shot)
#define TEST_RELOAD      true         // testing will be done with auto reload

#define GPIO_OUTPUT_IO_0    12
#define GPIO_OUTPUT_IO_1    15
#define GPIO_OUTPUT_PIN_SEL  ((1ULL << GPIO_OUTPUT_IO_0) | (1ULL << GPIO_OUTPUT_IO_1))

void hw_timer_callback1(void *arg)
{
    static int state = 0;

    gpio_set_level(GPIO_OUTPUT_IO_0, (state ++) % 2);
}

void hw_timer_callback2(void *arg)
{
    static int state = 0;

    gpio_set_level(GPIO_OUTPUT_IO_1, (state ++) % 2);
}

void hw_timer_callback3(void *arg)
{
    gpio_set_level(GPIO_OUTPUT_IO_0, 0);
    gpio_set_level(GPIO_OUTPUT_IO_1, 1);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Config gpio");
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "Initialize hw_timer for callback1");
    hw_timer_init(hw_timer_callback1, NULL);
    ESP_LOGI(TAG, "Set hw_timer timing time 100us with reload");
    hw_timer_alarm_us(100, TEST_RELOAD);
    vTaskDelay(1000 / portTICK_RATE_MS);
    ESP_LOGI(TAG, "Deinitialize hw_timer for callback1");
    hw_timer_deinit();

    ESP_LOGI(TAG, "Initialize hw_timer for callback2");
    hw_timer_init(hw_timer_callback2, NULL);
    ESP_LOGI(TAG, "Set hw_timer timing time 1ms with reload");
    hw_timer_alarm_us(1000, TEST_RELOAD);
    vTaskDelay(1000 / portTICK_RATE_MS);
    ESP_LOGI(TAG, "Set hw_timer timing time 10ms with reload");
    hw_timer_alarm_us(10000, TEST_RELOAD);
    vTaskDelay(2000 / portTICK_RATE_MS);
    ESP_LOGI(TAG, "Set hw_timer timing time 100ms with reload");
    hw_timer_alarm_us(100000, TEST_RELOAD);
    vTaskDelay(3000 / portTICK_RATE_MS);
    ESP_LOGI(TAG, "Cancel timing");
    hw_timer_disarm();
    hw_timer_deinit();

    ESP_LOGI(TAG, "Initialize hw_timer for callback3");
    hw_timer_init(hw_timer_callback3, NULL);

    ESP_LOGI(TAG, "Set hw_timer timing time 1ms with one-shot");
    hw_timer_alarm_us(1000, TEST_ONE_SHOT);   
}



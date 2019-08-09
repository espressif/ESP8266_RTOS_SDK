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
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"

#include "driver/ledc.h"

#define LEDC_HS_CH0_GPIO (13)
#define LEDC_HS_CH1_GPIO (14)
#define LEDC_LS_CH2_GPIO (15)
#define LEDC_LS_CH3_GPIO (12)

#define LEDC_HS_CH0_CHANNEL LEDC_CHANNEL_0
#define LEDC_HS_CH1_CHANNEL LEDC_CHANNEL_1
#define LEDC_LS_CH2_CHANNEL LEDC_CHANNEL_2
#define LEDC_LS_CH3_CHANNEL LEDC_CHANNEL_3

#define LEDC_TEST_CH_NUM (4)
#define LEDC_TEST_FREQ (1000) //1KHz
#define LEDC_TEST_DUTY (900) //duty = LEDC_TEST_DUTY / LEDC_TEST_PERIOD  & LEDC_TEST_PERIOD = 1,000,000 / LEDC_TEST_FREQ
#define LEDC_TEST_FADE_TIME (3000)

static const char* TAG = "main";

void app_main()
{
    int ch;
    /*
     * Prepare and set configuration of timers
     * that will be used by LED Controller
     */

    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_1_BIT, // resolution of PWM duty(Invalid parameter, compatible with esp32 API)
        .freq_hz = LEDC_TEST_FREQ, // frequency of PWM signal
        .speed_mode = 0, // timer mode (Invalid parameter, compatible with esp32 API)
        .timer_num = 0 // timer index (Invalid parameter, compatible with esp32 API)
    };
    // Set configuration of timer0 for high speed channels
    ledc_timer_config(&ledc_timer);

    /*
     * Prepare individual configuration
     * for each channel of LED Controller
     * by selecting:
     * - controller's channel number
     * - output duty cycle, set initially to 0
     * - GPIO number where LED is connected to
     * - speed mode, either high or low
     * - timer servicing selected channel
     *   Note: Speed_mode and timer_sel are only compatible with esp32, 
     *   but it is not required in esp8266, so it can be set to 0 when
     *   using esp8266
     */
    ledc_channel_config_t ledc_channel[LEDC_TEST_CH_NUM] = {
        { .channel = LEDC_HS_CH0_CHANNEL,
            .duty = 0,
            .gpio_num = LEDC_HS_CH0_GPIO,
            .speed_mode = 0,
            .timer_sel = 0 },
        { .channel = LEDC_HS_CH1_CHANNEL,
            .duty = 0,
            .gpio_num = LEDC_HS_CH1_GPIO,
            .speed_mode = 0,
            .timer_sel = 0 },
        { .channel = LEDC_LS_CH2_CHANNEL,
            .duty = 0,
            .gpio_num = LEDC_LS_CH2_GPIO,
            .speed_mode = 0,
            .timer_sel = 0 },
        { .channel = LEDC_LS_CH3_CHANNEL,
            .duty = 0,
            .gpio_num = LEDC_LS_CH3_GPIO,
            .speed_mode = 0,
            .timer_sel = 0 },
    };

    // Set LED Controller with previously prepared configuration
    for (ch = 0; ch < LEDC_TEST_CH_NUM; ch++) {
        ledc_channel_config(&ledc_channel[ch]);
    }

    // Initialize fade service.
    ledc_fade_func_install(0);

    while (1) {
        ESP_LOGI(TAG, "1. LEDC fade up to duty = %d\n", LEDC_TEST_DUTY);
        for (ch = 0; ch < LEDC_TEST_CH_NUM; ch++) {
            ledc_set_fade_with_time(ledc_channel[ch].speed_mode,
                ledc_channel[ch].channel, LEDC_TEST_DUTY, LEDC_TEST_FADE_TIME);
        }
        for (ch = 0; ch < LEDC_TEST_CH_NUM; ch++) {
            ledc_fade_start(ledc_channel[ch].speed_mode,
                ledc_channel[ch].channel, LEDC_FADE_NO_WAIT);
        }
        vTaskDelay(LEDC_TEST_FADE_TIME / portTICK_PERIOD_MS);
        vTaskDelay(LEDC_TEST_FADE_TIME / portTICK_PERIOD_MS);

        ESP_LOGI(TAG, "2. LEDC fade down to duty = 0\n");
        for (ch = 0; ch < LEDC_TEST_CH_NUM; ch++) {
            ledc_set_fade_with_time(ledc_channel[ch].speed_mode,
                ledc_channel[ch].channel, 0, LEDC_TEST_FADE_TIME);
        }
        for (ch = 0; ch < LEDC_TEST_CH_NUM; ch++) {
            ledc_fade_start(ledc_channel[ch].speed_mode,
                ledc_channel[ch].channel, LEDC_FADE_NO_WAIT);
        }
        vTaskDelay(LEDC_TEST_FADE_TIME / portTICK_PERIOD_MS);
        vTaskDelay(LEDC_TEST_FADE_TIME / portTICK_PERIOD_MS);
    }
}

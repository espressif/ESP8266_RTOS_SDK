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

#include "esp_err.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/pwm.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define LEDC_PRIORITY (2)
#define LEDC_CHANNEL_MAX (8)
#define LEDC_STEP_TIME (10)
#define LEDC_FLAG_ON (1)
#define LEDC_FLAG_OFF (0)
#define LEDC_TASK_STACK_DEPTH (1024)
#define LEDC_MAX_DUTY (8196)

static const char* LEDC_TAG = "ledc";

#define LEDC_CHECK(a, str, ret)                                                     \
    if (!(a)) {                                                                     \
        ESP_LOGE(LEDC_TAG, "%s:%d (%s):%s", __FILE__, __LINE__, __FUNCTION__, str); \
        return (ret);                                                               \
    }

typedef struct {
    ledc_channel_t channel_num; // Channel
    uint32_t duty_p; // Duty at present
    uint32_t duty; // Duty what we want to
    uint32_t step_duty; // Duty/10ms   means every 10ms change step_duty
    uint32_t step_01duty; // 0.1 of the duty value
    uint32_t step_001duty; // 0.01 of the duty value
    uint32_t gpio_num;//gpio pins
    int16_t phase; //init phase
    int fade_time; // Time to duty by fade  
} ledc_obj_t;

QueueHandle_t channel_queue;
static ledc_obj_t *p_ledc_obj[LEDC_CHANNEL_MAX] = { 0 };
static uint8_t ledc_usr_channel_max = 0; //This is to allocate some channels according to the channel used by the user
static uint32_t ledc_period;

/**
 * @brief  set down ledc duty by step
 * 
 * @param  channel   set channel to change duty
 * @param  flag      tells the caller whether the set duty cycle has been reached
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG Parameter error  
 */
static esp_err_t ledc_fade_down(ledc_channel_t channel, uint8_t* flag);

/**
 * @brief  set up ledc duty by step
 * 
 * @param  channel   set channel to change duty
 * @param  flag      tells the caller whether the set duty cycle has been reached
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG Parameter error  
 */
static esp_err_t ledc_fade_up(ledc_channel_t channel, uint8_t* flag);

esp_err_t ledc_timer_config(const ledc_timer_config_t* timer_conf)
{
    // Just freq_hz is useful
    // Hz to period
    LEDC_CHECK(timer_conf != NULL, "time_conf error", ESP_ERR_INVALID_ARG);

    ledc_period = 1000000 / timer_conf->freq_hz;

    return ESP_OK;
}

// The difference between the current duty cycle and the target duty cycle
esp_err_t ledc_set_duty(ledc_mode_t speed_mode, ledc_channel_t ledc_channel, uint32_t ledc_duty)
{
    LEDC_CHECK(ledc_channel < LEDC_CHANNEL_MAX, "ledc_channel error", ESP_ERR_INVALID_ARG);
    LEDC_CHECK(ledc_period * ledc_duty / LEDC_MAX_DUTY <= ledc_period, "ledc_duty error", ESP_ERR_INVALID_ARG);
    
    p_ledc_obj[ledc_channel]->channel_num = ledc_channel;
    p_ledc_obj[ledc_channel]->duty = ledc_period * ledc_duty / LEDC_MAX_DUTY;
    pwm_get_duty(ledc_channel, &p_ledc_obj[ledc_channel]->duty_p);

    p_ledc_obj[ledc_channel]->step_duty = (p_ledc_obj[ledc_channel]->duty_p > p_ledc_obj[ledc_channel]->duty ? p_ledc_obj[ledc_channel]->duty_p - p_ledc_obj[ledc_channel]->duty : p_ledc_obj[ledc_channel]->duty - p_ledc_obj[ledc_channel]->duty_p);
    //The duty print value for this channel is duty/period(Program internal value), corresponding to duty/ledc_max_duty
    ESP_LOGI(LEDC_TAG, "channel_num = %d | duty = %d; duty_p = %d | step_duty = %d;",
        p_ledc_obj[ledc_channel]->channel_num, p_ledc_obj[ledc_channel]->duty,
        p_ledc_obj[ledc_channel]->duty_p, p_ledc_obj[ledc_channel]->step_duty);

    return ESP_OK;
}

esp_err_t ledc_update_duty(ledc_mode_t speed_mode, ledc_channel_t ledc_channel)
{
    // send the queue
    LEDC_CHECK(ledc_channel < LEDC_CHANNEL_MAX, "ledc_channel error", ESP_ERR_INVALID_ARG);

    uint8_t ret = xQueueSend(channel_queue, &ledc_channel, 0);
    if (ret != pdPASS) {
        ESP_LOGE(LEDC_TAG, "xQueueSend err\r\n");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t ledc_set_fade_with_time(ledc_mode_t speed_mode, ledc_channel_t ledc_channel, uint32_t ledc_duty, int ledc_fade_time)
{
    //  For porting, speed_mode is not used
    LEDC_CHECK(ledc_channel < LEDC_CHANNEL_MAX, "ledc_channel error", ESP_ERR_INVALID_ARG);
    LEDC_CHECK(ledc_period * ledc_duty / LEDC_MAX_DUTY <= ledc_period, "ledc_duty error", ESP_ERR_INVALID_ARG);

    p_ledc_obj[ledc_channel]->channel_num = ledc_channel;
    p_ledc_obj[ledc_channel]->duty = ledc_period * ledc_duty / LEDC_MAX_DUTY;
    p_ledc_obj[ledc_channel]->fade_time = ledc_fade_time;
    pwm_get_duty(ledc_channel, &p_ledc_obj[ledc_channel]->duty_p);
    uint32_t duty_value = (p_ledc_obj[ledc_channel]->duty_p > p_ledc_obj[ledc_channel]->duty ? (p_ledc_obj[ledc_channel]->duty_p - p_ledc_obj[ledc_channel]->duty) : (p_ledc_obj[ledc_channel]->duty - p_ledc_obj[ledc_channel]->duty_p));

    p_ledc_obj[ledc_channel]->step_duty = duty_value / (ledc_fade_time / LEDC_STEP_TIME);
    p_ledc_obj[ledc_channel]->step_01duty = duty_value * 10 / (ledc_fade_time / LEDC_STEP_TIME) % 10;
    p_ledc_obj[ledc_channel]->step_001duty = duty_value * 100 / (ledc_fade_time / LEDC_STEP_TIME) % 10;

    //The duty print value for this channel is duty/period(Program internal value), corresponding to duty/ledc_max_duty
    ESP_LOGI(LEDC_TAG, "channel_num = %d | duty = %d; duty_p = %d | step_duty = %d | step_01duty = %d | step_001duty = %d",
        p_ledc_obj[ledc_channel]->channel_num, p_ledc_obj[ledc_channel]->duty,
        p_ledc_obj[ledc_channel]->duty_p, p_ledc_obj[ledc_channel]->step_duty, p_ledc_obj[ledc_channel]->step_01duty, p_ledc_obj[ledc_channel]->step_001duty);

    return ESP_OK;
}

esp_err_t ledc_fade_start(ledc_mode_t speed_mode, ledc_channel_t ledc_channel, ledc_fade_mode_t fade_mode)
{
    LEDC_CHECK(ledc_channel < LEDC_CHANNEL_MAX, "ledc_channel error", ESP_ERR_INVALID_ARG);

    esp_err_t ret;
    ret = ledc_update_duty(speed_mode, ledc_channel);
    if (ret == ESP_FAIL) {
        return ESP_FAIL;
    }

    if (fade_mode == LEDC_FADE_WAIT_DONE) {
        vTaskDelay(p_ledc_obj[ledc_channel]->fade_time / portTICK_PERIOD_MS);
    }
    return ESP_OK;
}

esp_err_t ledc_channel_config(const ledc_channel_config_t* ledc_conf)
{
    LEDC_CHECK(ledc_conf != NULL, "ledc_conf error", ESP_ERR_INVALID_ARG);
    LEDC_CHECK( ledc_conf->duty <= ledc_period, "ledc_duty error", ESP_ERR_INVALID_ARG);

      if (p_ledc_obj[ledc_usr_channel_max] == NULL){
            p_ledc_obj[ledc_usr_channel_max] = (ledc_obj_t *)calloc(1, sizeof(ledc_obj_t));
            if (p_ledc_obj[ledc_usr_channel_max] == NULL){
                ESP_LOGE(LEDC_TAG, "LEDC driver malloc error");
                return ESP_FAIL;
            }
        }
    p_ledc_obj[ledc_usr_channel_max]->gpio_num = ledc_conf->gpio_num;
    p_ledc_obj[ledc_usr_channel_max]->duty = ledc_period * ledc_conf->duty / LEDC_MAX_DUTY;
    ledc_usr_channel_max++;

    return ESP_OK;
}

esp_err_t ledc_fade_up(ledc_channel_t channel, uint8_t* flag)
{
    LEDC_CHECK(flag != NULL, "flag error", ESP_ERR_INVALID_ARG);
    LEDC_CHECK(channel < LEDC_CHANNEL_MAX, "ledc_channel error", ESP_ERR_INVALID_ARG);

    static uint8_t i[LEDC_CHANNEL_MAX] = { 0 };
    uint32_t duty_value = 0;

    duty_value = (i[channel] % 10 == 5 ? p_ledc_obj[channel]->step_duty + p_ledc_obj[channel]->step_01duty : p_ledc_obj[channel]->step_duty);
    duty_value += (i[channel] == 50 ? p_ledc_obj[channel]->step_001duty : 0);

    if (p_ledc_obj[channel]->duty_p < p_ledc_obj[channel]->duty) {
        if (p_ledc_obj[channel]->duty_p + duty_value > p_ledc_obj[channel]->duty) {
            p_ledc_obj[channel]->duty_p = p_ledc_obj[channel]->duty;
        } else {
            p_ledc_obj[channel]->duty_p += duty_value;
        }
        pwm_set_duty(channel, p_ledc_obj[channel]->duty_p);
        i[channel]++;
        if (i[channel] == 100) {
            i[channel] = 0;
        }
    }
    if (p_ledc_obj[channel]->duty_p == p_ledc_obj[channel]->duty) {
        *flag = LEDC_FLAG_OFF;
    }

    return ESP_OK;
}

esp_err_t ledc_fade_down(ledc_channel_t channel, uint8_t* flag)
{
    LEDC_CHECK(flag != NULL, "flag error", ESP_ERR_INVALID_ARG);
    LEDC_CHECK(channel < LEDC_CHANNEL_MAX, "ledc_channel error", ESP_ERR_INVALID_ARG);

    static uint8_t i[LEDC_CHANNEL_MAX] = { 0 };
    uint32_t duty_value = 0;

    duty_value = (i[channel] % 10 == 5 ? p_ledc_obj[channel]->step_duty + p_ledc_obj[channel]->step_01duty : p_ledc_obj[channel]->step_duty);
    duty_value += (i[channel] == 50 ? p_ledc_obj[channel]->step_001duty : 0);

    if (p_ledc_obj[channel]->duty_p > p_ledc_obj[channel]->duty) {
        // it is more smart than 'p_ledc_obj[channel].duty_p - p_ledc_obj[channel].step_duty < p_ledc_obj[channel].duty'
        if (p_ledc_obj[channel]->duty_p < p_ledc_obj[channel]->duty + duty_value) {
            p_ledc_obj[channel]->duty_p = p_ledc_obj[channel]->duty;
        } else {
            p_ledc_obj[channel]->duty_p -= duty_value;
        }
        pwm_set_duty(channel, p_ledc_obj[channel]->duty_p);
        i[channel]++;
        if (i[channel] == 100) {
            i[channel] = 0;
        }
    }
    if (p_ledc_obj[channel]->duty_p == p_ledc_obj[channel]->duty) {
        *flag = LEDC_FLAG_OFF;
    }

    return ESP_OK;
}

// Complete message queue reception while changing duty cycle
static void ledc_task(void* pvParameters)
{
    ledc_channel_t channel;
    uint8_t i;
    uint8_t flag[LEDC_CHANNEL_MAX] = { 0 };

    while (1) {

        while (pdTRUE == xQueueReceive(channel_queue, &channel, 0)) {
            flag[channel] = LEDC_FLAG_ON;
        }
        vTaskSuspendAll();
        for (i = 0; i < ledc_usr_channel_max; i++) {
            if (flag[i] == LEDC_FLAG_ON) {
                if (p_ledc_obj[i]->duty_p < p_ledc_obj[i]->duty) {
                    ledc_fade_up(i, &flag[i]);
                } else {
                    ledc_fade_down(i, &flag[i]);
                }
            }
        }
        pwm_start();
        xTaskResumeAll();
        vTaskDelay(LEDC_STEP_TIME / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

esp_err_t ledc_fade_func_install(int intr_alloc_flags)
{
    int16_t ledc_phase[LEDC_CHANNEL_MAX] = {0};
    uint32_t ledc_duty[LEDC_CHANNEL_MAX] = {0};
    uint32_t ledc_gpio_num[LEDC_CHANNEL_MAX] = {0};
    
    for (int i = 0; i < ledc_usr_channel_max; i++){
        ledc_gpio_num[i] = p_ledc_obj[i]->gpio_num;
        ledc_duty[i] = p_ledc_obj[i]->duty;
    }

    LEDC_CHECK(ledc_usr_channel_max < LEDC_CHANNEL_MAX, "flag error", ESP_ERR_INVALID_ARG);
    pwm_init(ledc_period, ledc_duty, ledc_usr_channel_max, ledc_gpio_num);
    ESP_LOGI(LEDC_TAG, "ledc_usr_channel_max:%d", ledc_usr_channel_max);
    for (int i = 0; i < ledc_usr_channel_max; i++) {
        ESP_LOGI(LEDC_TAG, "gpio:%d", ledc_gpio_num[i]);
    }
    
    pwm_set_phases(ledc_phase);
    channel_queue = xQueueCreate(ledc_usr_channel_max, sizeof(uint8_t));
    if (channel_queue == 0) {
        ESP_LOGE(LEDC_TAG, "xQueueCreate err\r\n");
        return ESP_ERR_INVALID_STATE;
    }

    xTaskCreate(ledc_task, "ledc_task", LEDC_TASK_STACK_DEPTH, NULL, LEDC_PRIORITY, NULL);

    return pwm_start();
}

esp_err_t ledc_fade_func_uninstall(void)
{
    for (int i = 0; i < ledc_usr_channel_max; i++){
        free(p_ledc_obj[i]);
        p_ledc_obj[i] = NULL;
    }
    return pwm_stop(0x00);
}

esp_err_t ledc_stop(ledc_mode_t speed_mode, ledc_channel_t channel, uint32_t idle_level)
{
    LEDC_CHECK(idle_level == 0 || idle_level == 1, "idle_level error", ESP_ERR_INVALID_ARG);

    static uint32_t stop_level_mask = 0x0;
    if (idle_level == 0){
        stop_level_mask = stop_level_mask | 0x1 << channel;
    }
    else if (idle_level == 1){
        stop_level_mask = stop_level_mask & ~(0x1 << channel);
    }

    pwm_stop(stop_level_mask);

    return ESP_OK;
}

int periph_module_enable(int none)
{
    return ESP_OK;
}
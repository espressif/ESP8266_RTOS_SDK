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
#include <stdlib.h>

#include "esp_err.h"
#include "esp_log.h"

#include "esp8266/eagle_soc.h"
#include "esp8266/gpio_register.h"
#include "esp8266/pin_mux_register.h"

#include "esp_heap_caps.h"

#include "driver/pwm.h"
#include "driver/gpio.h"

// Temporary use the FreeRTOS critical function
#include "FreeRTOS.h"

#define ENTER_CRITICAL() portENTER_CRITICAL()
#define EXIT_CRITICAL() portEXIT_CRITICAL()

static const char *TAG = "pwm";

#define PWM_CHECK(a, str, ret)  if(!(a)) {                                             \
        ESP_LOGE(TAG,"%s:%d (%s):%s", __FILE__, __LINE__, __FUNCTION__, str);      \
        return (ret);                                                                   \
    }

#define MAX_PWM_CHANNEL  (8)

#define US_TO_MAC_TICK(t) (t)
#define US_TO_TICKS(t) US_TO_MAC_TICK(t)
// Time to switch PWM ahead of time
#define AHEAD_TICKS0 0
// Advance timing of the timer
#define AHEAD_TICKS1 8
// The time that remains in the interrupt function (the adjacent target loops in the interrupt)
#define AHEAD_TICKS2 10
// Minimum timing time
#define AHEAD_TICKS3 2
#define MAX_TICKS    10000000ul

#define WDEVTSF0_TIME_LO     0x3ff21004
#define WDEVTSF0_TIME_HI     0x3ff21008
#define WDEVTSFSW0_LO        0x3ff21018
#define WDEVTSFSW0_HI        0x3ff2101C
#define WDEVTSF0_TIMER_LO    0x3ff2109c
#define WDEVTSF0_TIMER_HI    0x3ff210a0
#define WDEVTSF0TIMER_ENA    0x3ff21098
#define WDEV_TSF0TIMER_ENA   BIT(31)

#define PWM_VERSION          "PWM v3.2"

typedef struct {
    uint32_t duty;  /*!< pwm duty for each channel */
    int16_t phase;  /*!< pwm phase for each channel */
    uint8_t io_num; /*!< pwm io_num for each channel */
} pwm_info_t;

typedef struct  {
    uint16_t io_mask;        /*!< gpio num for each channel */
    uint32_t post_edg_time;  /*!< positive edge time for each channel */
    uint32_t neg_edg_time;   /*!< negative edge time for each channel */
} pwm_channel_param_t;

typedef struct {
    uint32_t edg_time;       /*!< change the output level at this edge time */
    uint16_t io_set_mask;    /*!< which gpio needs to change high level at this edge time */
    uint16_t io_clr_mask;    /*!< which gpio needs to change low level at this edge time */
} pwm_param_t;

typedef struct {
    uint8_t run_channel_num;     /*!< pwm run channel num */
    pwm_param_t *run_pwm_param;  /*!< pwm param for each channel */
} run_pwm_single_t;

typedef struct {
    uint32_t depth;
    uint8_t start_flag;
    uint8_t init_flag;
    uint16_t channel_invert_bitmap;
    pwm_channel_param_t *channel;
    pwm_param_t *param;
    pwm_info_t  *pwm_info;
    run_pwm_single_t run_pwm[2];
    uint32_t period;
    uint8_t channel_num;
    uint8_t update_done;
    uint8_t run_pwm_toggle;
    uint8_t current_channel;
    uint16_t start_set_mask;
    uint16_t start_clr_mask;
    uint16_t local_channel;
    uint16_t gpio_bit_mask;
    uint32_t this_target;
    run_pwm_single_t *single;
} pwm_obj_t;

pwm_obj_t *pwm_obj = NULL;

int wDev_MacTimSetFunc(void (*handle)(void));

static void pwm_phase_init(void)
{
    int32_t time_delay;
    uint8_t i;

    for (i = 0; i < pwm_obj->channel_num; i++) {
        if (-180 < pwm_obj->pwm_info[i].phase && pwm_obj->pwm_info[i].phase < 0) {
            time_delay = 0 - ((0 - pwm_obj->pwm_info[i].phase) * pwm_obj->depth / 360);
        } else if (pwm_obj->pwm_info[i].phase == 0) {
            continue;
        } else if (180 > pwm_obj->pwm_info[i].phase && pwm_obj->pwm_info[i].phase > 0) {
            time_delay = pwm_obj->pwm_info[i].phase * pwm_obj->depth / 360;
        } else {
            ESP_LOGE(TAG, "channel[%d]  phase error %d, valid ramge from (-180,180)\n", i, pwm_obj->pwm_info[i].phase);
            continue;
        }

        pwm_obj->channel[i].post_edg_time = (time_delay + pwm_obj->channel[i].post_edg_time + pwm_obj->depth) % pwm_obj->depth;
        pwm_obj->channel[i].neg_edg_time = (time_delay + pwm_obj->channel[i].neg_edg_time + pwm_obj->depth) % pwm_obj->depth;
    }
}

static void pwm_insert_sort(void)
{
    uint8_t i;
    pwm_param_t tmp;

    for (i = 1; i < pwm_obj->channel_num * 2; i++) {
        if (pwm_obj->param[i].edg_time < pwm_obj->param[i - 1].edg_time) {
            int32_t j = i - 1;
            memcpy((void *)&tmp, (void *)&pwm_obj->param[i], sizeof(pwm_param_t));

            while (j >= 0 && pwm_obj->param[j].edg_time > tmp.edg_time) {
                memcpy(&pwm_obj->param[j + 1], &pwm_obj->param[j], sizeof(pwm_param_t));
                j--;
            }

            memcpy(&pwm_obj->param[j + 1], &tmp, sizeof(pwm_param_t));
        }
    }

}

esp_err_t pwm_set_period(uint32_t period)
{
    PWM_CHECK(period >= 10, "period setting is too short", ESP_ERR_INVALID_ARG);

    pwm_obj->depth = period;    // For ease of conversion, let the depth equal the period
    pwm_obj->period = period;

    return ESP_OK;
}

esp_err_t pwm_get_period(uint32_t *period_p)
{
    PWM_CHECK(NULL != period_p, "Pointer is empty", ESP_ERR_INVALID_ARG);

    *period_p = pwm_obj->period;

    return ESP_OK;
}

esp_err_t pwm_set_channel_invert(uint16_t channel_mask)
{
    PWM_CHECK((channel_mask  >> pwm_obj->channel_num) == 0, "invalid channel_mask", ESP_ERR_INVALID_ARG);

    pwm_obj->channel_invert_bitmap = channel_mask;
    return ESP_OK;
}

esp_err_t pwm_clear_channel_invert(uint16_t channel_mask)
{
    PWM_CHECK((channel_mask  >> pwm_obj->channel_num) == 0, "invalid channel_mask", ESP_ERR_INVALID_ARG);

    pwm_obj->channel_invert_bitmap = pwm_obj->channel_invert_bitmap & (~channel_mask);
    return ESP_OK;
}

esp_err_t pwm_set_duty(uint8_t channel_num, uint32_t duty)
{
    PWM_CHECK(channel_num < pwm_obj->channel_num, "Channel num error", ESP_ERR_INVALID_ARG);

    pwm_obj->pwm_info[channel_num].duty = duty;
    return ESP_OK;
}

esp_err_t pwm_set_duties(uint32_t *duties)
{
    uint8_t i;
    PWM_CHECK(NULL != duties, "Pointer is empty", ESP_ERR_INVALID_ARG);

    for (i = 0; i < pwm_obj->channel_num; i++) {
        pwm_obj->pwm_info[i].duty = duties[i];
    }

    return ESP_OK;
}

esp_err_t pwm_get_duty(uint8_t channel_num, uint32_t *duty_p)
{
    PWM_CHECK(channel_num < pwm_obj->channel_num, "Channel num error", ESP_ERR_INVALID_ARG);
    PWM_CHECK(NULL != duty_p, "Pointer is empty", ESP_ERR_INVALID_ARG);

    *duty_p = pwm_obj->pwm_info[channel_num].duty;

    return ESP_OK;
}

esp_err_t pwm_set_period_duties(uint32_t period, uint32_t *duties)
{
    PWM_CHECK(NULL != duties, "Pointer is empty", ESP_ERR_INVALID_ARG);

    pwm_set_period(period);
    pwm_set_duties(duties);

    return ESP_OK;
}

esp_err_t pwm_set_phase(uint8_t channel_num, int16_t phase)
{
    PWM_CHECK(channel_num < pwm_obj->channel_num, "Channel num error", ESP_ERR_INVALID_ARG);

    pwm_obj->pwm_info[channel_num].phase = phase;

    return ESP_OK;
}

esp_err_t pwm_set_phases(int16_t *phases)
{
    uint8_t i;
    PWM_CHECK(NULL != phases, "Pointer is empty", ESP_ERR_INVALID_ARG);

    for (i = 0; i < pwm_obj->channel_num; i++) {
        pwm_obj->pwm_info[i].phase = phases[i];

    }

    return ESP_OK;
}

esp_err_t pwm_get_phase(uint8_t channel_num, uint16_t *phase_p)
{
    PWM_CHECK(channel_num < pwm_obj->channel_num, "Channel num error", ESP_ERR_INVALID_ARG);
    PWM_CHECK(NULL != phase_p, "Pointer is empty", ESP_ERR_INVALID_ARG);

    *phase_p = pwm_obj->pwm_info[channel_num].phase;

    return ESP_OK;
}

static void pwm_timer_enable(uint8_t enable)
{
    if (0 == enable) {
        ENTER_CRITICAL();
        REG_WRITE(WDEVTSF0TIMER_ENA, REG_READ(WDEVTSF0TIMER_ENA) & (~WDEV_TSF0TIMER_ENA));
        EXIT_CRITICAL();
    } else {
        REG_WRITE(WDEVTSF0TIMER_ENA, WDEV_TSF0TIMER_ENA);
    }
}

static void IRAM_ATTR pwm_timer_intr_handler(void)
{
    //process continous event
    uint32_t mask = REG_READ(PERIPHS_GPIO_BASEADDR + GPIO_OUT_ADDRESS);

    // In the interrupt handler, first check for data updates, then switch to the updated array at the end of a cycle, start outputting new PWM waveforms, and clear the update flag.
    while (1) {
        if (REG_READ(WDEVTSF0_TIME_LO)  + AHEAD_TICKS2 < pwm_obj->this_target) {
            break;
        } else {
            //wait timer comes
            while ((REG_READ(WDEVTSF0_TIME_LO) + AHEAD_TICKS0) < pwm_obj->this_target && (pwm_obj->this_target < MAX_TICKS));

            if (pwm_obj->current_channel ==  0) {
                if (pwm_obj->update_done == 1) {
                    pwm_obj->single =  &pwm_obj->run_pwm[pwm_obj->run_pwm_toggle];
                    pwm_obj->run_pwm_toggle = (pwm_obj->run_pwm_toggle ^ 0x1);
                    pwm_obj->update_done = 0;
                }
                mask = mask & (~pwm_obj->single->run_pwm_param[pwm_obj->single->run_channel_num - 1].io_clr_mask);
                mask = mask | pwm_obj->single->run_pwm_param[pwm_obj->single->run_channel_num - 1].io_set_mask;
                REG_WRITE(PERIPHS_GPIO_BASEADDR + GPIO_OUT_ADDRESS, mask);
            } else {
                mask = mask & (~(pwm_obj->single->run_pwm_param[pwm_obj->current_channel - 1].io_clr_mask));
                mask = mask | (pwm_obj->single->run_pwm_param[pwm_obj->current_channel - 1].io_set_mask);
                REG_WRITE(PERIPHS_GPIO_BASEADDR + GPIO_OUT_ADDRESS, mask);
            }

            pwm_obj->this_target += pwm_obj->single->run_pwm_param[pwm_obj->current_channel].edg_time;
            pwm_obj->current_channel++;

            if (pwm_obj->current_channel >= pwm_obj->single->run_channel_num) {
                pwm_obj->current_channel = 0;
            }
        }
    }

    pwm_obj->this_target -= REG_READ(WDEVTSF0_TIME_LO);

    if (pwm_obj->this_target >= MAX_TICKS || pwm_obj->this_target < AHEAD_TICKS1 + AHEAD_TICKS3) {
        pwm_obj->this_target = AHEAD_TICKS1 + AHEAD_TICKS3;
    }

    REG_WRITE(WDEVTSFSW0_LO, 0);
    //WARNING, pwm_obj->this_target - AHEAD_TICKS1 should be bigger than 2
    REG_WRITE(WDEVTSF0_TIMER_LO, pwm_obj->this_target - AHEAD_TICKS1);
    REG_WRITE(WDEVTSF0TIMER_ENA, WDEV_TSF0TIMER_ENA);
}

static void pwm_timer_start(uint32_t period)
{
    // suspend all task to void timer interrupt missed
    // TODO, do we need lock interrupt here, I think interrupt context will not take 1ms long
    // time low field to 0
    REG_WRITE(WDEVTSFSW0_LO, 0);
    // time high field to 0
    REG_WRITE(WDEVTSFSW0_HI, 0);
    // time low field to 0 again
    REG_WRITE(WDEVTSFSW0_LO, 0);
    // target timer low field to 0
    REG_WRITE(WDEVTSF0_TIMER_LO, 0);
    // target timer high field to 0
    REG_WRITE(WDEVTSF0_TIMER_HI, 0);
    // target low to the target value, with ahead time AHEAD_TICKS1
    pwm_obj->this_target = US_TO_TICKS(period);
    // WARNING: pwm_obj->this_target should bigger than AHEAD_TICKS1
    REG_WRITE(WDEVTSF0_TIMER_LO, pwm_obj->this_target - AHEAD_TICKS1);
    // enable timer
    pwm_timer_enable(1);
}

static void pwm_timer_register(void (*handle)(void))
{
    wDev_MacTimSetFunc(handle);
}

esp_err_t pwm_start(void)
{
    uint8_t i, j;
    PWM_CHECK((pwm_obj != NULL), "PWM has not been initialized yet.", ESP_FAIL);

    pwm_obj->start_set_mask = 0;
    pwm_obj->start_clr_mask = 0;

    // Each PWM waveform has two edges in a cycle, rising edge and falling edge.
    // Each PWM waveform has a level change at the edge time.
    // Therefore, the duty cycle and phase can be adjusted as long as the corresponding edge is generated on the corresponding channel at each edge time.
    // What the program needs to do is to establish the relationship between the edge time and the corresponding GPIO level changes.
    // Then the edge time is sorted from small to large and the same edge time is merged. The time difference between two adjacent edge times
    for (i = 0; i < pwm_obj->channel_num; i++) {
        pwm_obj->channel[i].io_mask = (0x1 << pwm_obj->pwm_info[i].io_num);

        if (pwm_obj->channel_invert_bitmap & (0x1 << i)) {
            if (pwm_obj->pwm_info[i].duty == 0) {
                pwm_obj->start_set_mask |= pwm_obj->channel[i].io_mask;
                pwm_obj->channel[i].io_mask = 0;
                pwm_obj->channel[i].post_edg_time = 0;
                pwm_obj->channel[i].neg_edg_time = 0;
            } else if (pwm_obj->pwm_info[i].duty == pwm_obj->depth) {
                pwm_obj->start_clr_mask |= pwm_obj->channel[i].io_mask;
                pwm_obj->channel[i].io_mask = 0;
                pwm_obj->channel[i].post_edg_time = 0;
                pwm_obj->channel[i].neg_edg_time = 0;
            } else {
                pwm_obj->channel[i].neg_edg_time = 0;
                pwm_obj->channel[i].post_edg_time = US_TO_TICKS(pwm_obj->period) * ((float)pwm_obj->pwm_info[i].duty / pwm_obj->depth);
            }
        } else {
            if (pwm_obj->pwm_info[i].duty == 0) {
                pwm_obj->start_clr_mask |= pwm_obj->channel[i].io_mask;
                pwm_obj->channel[i].io_mask = 0;
                pwm_obj->channel[i].post_edg_time = 0;
                pwm_obj->channel[i].neg_edg_time = 0;
            } else if (pwm_obj->pwm_info[i].duty == pwm_obj->depth) {
                pwm_obj->start_set_mask |= pwm_obj->channel[i].io_mask;
                pwm_obj->channel[i].io_mask = 0;
                pwm_obj->channel[i].post_edg_time = 0;
                pwm_obj->channel[i].neg_edg_time = 0;
            } else {
                pwm_obj->channel[i].post_edg_time = 0;
                pwm_obj->channel[i].neg_edg_time = US_TO_TICKS(pwm_obj->period) * ((float)pwm_obj->pwm_info[i].duty / pwm_obj->depth);
            }
        }
    }

    pwm_phase_init();

    for (i = 0; i < pwm_obj->channel_num; i++) {
        if (pwm_obj->channel[i].post_edg_time < pwm_obj->channel[i].neg_edg_time) {
            if (pwm_obj->channel[i].post_edg_time == 0) {
                pwm_obj->start_set_mask |= pwm_obj->channel[i].io_mask;
            } else {
                pwm_obj->start_clr_mask |= pwm_obj->channel[i].io_mask;
            }
        } else if (pwm_obj->channel[i].post_edg_time > pwm_obj->channel[i].neg_edg_time) {
            if (pwm_obj->channel[i].neg_edg_time == 0) {
                pwm_obj->start_clr_mask |= pwm_obj->channel[i].io_mask;
            } else {
                pwm_obj->start_set_mask |= pwm_obj->channel[i].io_mask;
            }
        }
    }

    for (i = 0; i < pwm_obj->channel_num * 2; i += 2) {
        pwm_obj->param[i].edg_time = pwm_obj->channel[i >> 1].post_edg_time;
        pwm_obj->param[i].io_set_mask = pwm_obj->channel[i >> 1].io_mask;
        pwm_obj->param[i].io_clr_mask = 0;
        pwm_obj->param[i + 1].edg_time = pwm_obj->channel[i >> 1].neg_edg_time;
        pwm_obj->param[i + 1].io_set_mask = 0;
        pwm_obj->param[i + 1].io_clr_mask = pwm_obj->channel[i >> 1].io_mask;
    }

    pwm_obj->local_channel = pwm_obj->channel_num * 2 + 1;
    pwm_obj->param[pwm_obj->local_channel - 1].edg_time = pwm_obj->depth;

    // All edges are sorted from small to large.
    pwm_insert_sort();

    // Merge the same edges.
    for (i = pwm_obj->channel_num * 2; i > 0; i--) {
        if (pwm_obj->param[i].edg_time == pwm_obj->param[i - 1].edg_time) {
            pwm_obj->param[i - 1].io_set_mask |= pwm_obj->param[i].io_set_mask;
            pwm_obj->param[i - 1].io_clr_mask |= pwm_obj->param[i].io_clr_mask;

            for (j = i + 1; j <= pwm_obj->channel_num * 2; j++) {
                memcpy(&pwm_obj->param[j - 1], &pwm_obj->param[j], sizeof(pwm_param_t));
            }

            pwm_obj->local_channel--;
        }
    }

    for (i = pwm_obj->local_channel - 1; i > 0; i--) {
        pwm_obj->param[i].edg_time =  pwm_obj->param[i].edg_time - pwm_obj->param[i - 1].edg_time;
    }

    if (pwm_obj->param[0].edg_time == 0) {
        pwm_obj->start_set_mask |= pwm_obj->param[0].io_set_mask;
        pwm_obj->start_clr_mask |= pwm_obj->param[0].io_clr_mask;

        for (i = 1; i < pwm_obj->local_channel; i++) {
            memcpy(&pwm_obj->param[i - 1], &pwm_obj->param[i], sizeof(pwm_param_t));
        }

        pwm_obj->local_channel--;
    }

    pwm_obj->param[pwm_obj->local_channel - 1].io_set_mask = pwm_obj->start_set_mask;
    pwm_obj->param[pwm_obj->local_channel - 1].io_clr_mask = pwm_obj->start_clr_mask;

    if (pwm_obj->start_flag != 1) {
        pwm_obj->start_flag = 1;
        pwm_obj->run_pwm_toggle = 0;
        pwm_obj->single = &pwm_obj->run_pwm[0];
        pwm_obj->current_channel = 0;
        memcpy(pwm_obj->run_pwm[0].run_pwm_param, pwm_obj->param, sizeof(pwm_param_t) * pwm_obj->local_channel);
        pwm_obj->run_pwm[pwm_obj->run_pwm_toggle].run_channel_num = pwm_obj->local_channel;
        pwm_obj->update_done = 1;
        pwm_timer_start(pwm_obj->period);
    } else {
        if (pwm_obj->update_done != 1) {
            memcpy(pwm_obj->run_pwm[pwm_obj->run_pwm_toggle].run_pwm_param, pwm_obj->param, sizeof(pwm_param_t) * pwm_obj->local_channel);
            pwm_obj->run_pwm[pwm_obj->run_pwm_toggle].run_channel_num = pwm_obj->local_channel;
            pwm_obj->update_done = 1;
        }
    }

    return ESP_OK;
}

static esp_err_t pwm_obj_free(void)
{
    PWM_CHECK((pwm_obj != NULL), "PWM has not been initialized yet.", ESP_FAIL);

    if (pwm_obj->run_pwm[1].run_pwm_param) {
        heap_caps_free(pwm_obj->run_pwm[1].run_pwm_param);
    }

    if (pwm_obj->run_pwm[0].run_pwm_param) {
        heap_caps_free(pwm_obj->run_pwm[0].run_pwm_param);
    }

    if (pwm_obj->pwm_info) {
        heap_caps_free(pwm_obj->pwm_info);
    }

    if (pwm_obj->param) {
        heap_caps_free(pwm_obj->param);
    }

    if (pwm_obj->channel) {
        heap_caps_free(pwm_obj->channel);
    }

    heap_caps_free(pwm_obj);
    pwm_obj = NULL;

    return ESP_OK;
}

static esp_err_t pwm_obj_malloc(uint8_t channel_num)
{
    pwm_obj = (pwm_obj_t *)heap_caps_malloc(sizeof(pwm_obj_t), MALLOC_CAP_8BIT);

    if (NULL == pwm_obj) {
        return ESP_ERR_NO_MEM;
    } else {
        memset(pwm_obj, 0, sizeof(pwm_obj_t));
        pwm_obj->channel                  = (pwm_channel_param_t *)heap_caps_malloc(sizeof(pwm_channel_param_t) * channel_num, MALLOC_CAP_8BIT);
        pwm_obj->param                    = (pwm_param_t *)heap_caps_malloc(sizeof(pwm_param_t) * channel_num * 2 + sizeof(pwm_param_t), MALLOC_CAP_8BIT);
        pwm_obj->pwm_info                 = (pwm_info_t *)heap_caps_malloc(sizeof(pwm_info_t) * channel_num, MALLOC_CAP_8BIT);
        pwm_obj->run_pwm[0].run_pwm_param = (pwm_param_t *)heap_caps_malloc(sizeof(pwm_param_t) * channel_num * 2 + sizeof(pwm_param_t), MALLOC_CAP_8BIT);
        pwm_obj->run_pwm[1].run_pwm_param = (pwm_param_t *)heap_caps_malloc(sizeof(pwm_param_t) * channel_num * 2 + sizeof(pwm_param_t), MALLOC_CAP_8BIT);
    }

    if (pwm_obj->channel && pwm_obj->param && pwm_obj->pwm_info && pwm_obj->run_pwm[0].run_pwm_param && pwm_obj->run_pwm[1].run_pwm_param) {
        return ESP_OK;
    } else {
        pwm_obj_free();
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

esp_err_t pwm_init(uint32_t period, uint32_t *duties, uint8_t channel_num, const uint32_t *pin_num)
{
    PWM_CHECK(pwm_obj == NULL, "pwm has been initialized", ESP_FAIL);
    PWM_CHECK(channel_num <= MAX_PWM_CHANNEL, "Channel num out of range", ESP_ERR_INVALID_ARG);
    PWM_CHECK(NULL != duties, "duties pointer is empty", ESP_ERR_INVALID_ARG);
    PWM_CHECK(NULL != pin_num, "Pointer is empty", ESP_ERR_INVALID_ARG);
    PWM_CHECK(period >= 10, "period setting is too short", ESP_ERR_INVALID_ARG);

    uint8_t i;

    if (ESP_ERR_NO_MEM == pwm_obj_malloc(channel_num)) {
        pwm_obj_free();
        return ESP_ERR_NO_MEM;
    }

    pwm_obj->channel_num = channel_num;

    for (i = 0; i < channel_num; i++) {
        pwm_obj->pwm_info[i].io_num =  pin_num[i];
        pwm_obj->gpio_bit_mask |= (0x1 << pin_num[i]);
    }
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = pwm_obj->gpio_bit_mask;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    GPIO_REG_WRITE(GPIO_ENABLE_W1TS_ADDRESS, pwm_obj->gpio_bit_mask);
    pwm_set_period_duties(period, duties);
    pwm_timer_register(pwm_timer_intr_handler);
    ESP_LOGI(TAG, "--- %s\n", PWM_VERSION);
    pwm_obj->init_flag = 1;
    return ESP_OK;
}

esp_err_t pwm_stop(uint32_t stop_level_mask)
{
    int16_t i = 0;

    pwm_timer_enable(0);
    uint32_t level_set = REG_READ(PERIPHS_GPIO_BASEADDR + GPIO_OUT_ADDRESS);

    for (i = 0; i < pwm_obj->channel_num; i++) {
        if (stop_level_mask & (0x1 << i)) {
            level_set |= 0x1 << pwm_obj->pwm_info[i].io_num;
        }  else {
            level_set &= (~(0x1 << pwm_obj->pwm_info[i].io_num));
        }
    }

    REG_WRITE(PERIPHS_GPIO_BASEADDR + GPIO_OUT_ADDRESS, level_set);
    pwm_obj->start_flag = 0;

    return ESP_OK;
}

esp_err_t pwm_deinit(void)
{
    PWM_CHECK((pwm_obj != NULL), "PWM has not been initialized yet.", ESP_FAIL);
    PWM_CHECK((pwm_obj->init_flag), "PWM has been deleted.", ESP_FAIL);

    pwm_obj->init_flag = 0;

    pwm_stop(0xFF); // stop all channel
    pwm_timer_register(NULL);
    pwm_obj_free();

    return ESP_OK;
}

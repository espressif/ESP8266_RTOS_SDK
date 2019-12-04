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
#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"

#include "rom/ets_sys.h"
#include "esp8266/eagle_soc.h"
#include "esp8266/timer_struct.h"

#include "esp_heap_caps.h"
#include "esp_attr.h"
#include "esp_err.h"
#include "esp_log.h"

#include "driver/hw_timer.h"

#define ENTER_CRITICAL() portENTER_CRITICAL()
#define EXIT_CRITICAL() portEXIT_CRITICAL()

static const char *TAG = "hw_timer";

#define HW_TIMER_CHECK(a, str, ret_val) \
    if (!(a)) { \
        ESP_LOGE(TAG,"%s(%d): %s", __FUNCTION__, __LINE__, str); \
        return (ret_val); \
    }

#define hw_timer_intr_enable() _xt_isr_unmask(1 << ETS_FRC_TIMER1_INUM)

#define hw_timer_intr_disable() _xt_isr_mask(1 << ETS_FRC_TIMER1_INUM)

#define hw_timer_intr_register(a, b) _xt_isr_attach(ETS_FRC_TIMER1_INUM, (a), (b))


typedef struct {
    hw_timer_callback_t cb;
} hw_timer_obj_t;

hw_timer_obj_t *hw_timer_obj = NULL;

esp_err_t hw_timer_set_clkdiv(hw_timer_clkdiv_t clkdiv)
{
    HW_TIMER_CHECK(hw_timer_obj, "hw_timer has not been initialized yet", ESP_FAIL);
    HW_TIMER_CHECK(clkdiv >= TIMER_CLKDIV_1 && clkdiv <= TIMER_CLKDIV_256, "clkdiv is out of range.", ESP_ERR_INVALID_ARG);

    ENTER_CRITICAL();
    frc1.ctrl.div = clkdiv;
    EXIT_CRITICAL();

    return ESP_OK;
}

uint32_t hw_timer_get_clkdiv()
{
    return frc1.ctrl.div;
}

esp_err_t hw_timer_set_intr_type(hw_timer_intr_type_t intr_type)
{
    HW_TIMER_CHECK(hw_timer_obj, "hw_timer has not been initialized yet", ESP_FAIL);
    HW_TIMER_CHECK(intr_type >= TIMER_EDGE_INT && intr_type <= TIMER_LEVEL_INT, "intr_type is out of range.", ESP_ERR_INVALID_ARG);

    ENTER_CRITICAL();
    frc1.ctrl.intr_type = intr_type;
    EXIT_CRITICAL();

    return ESP_OK;
}

uint32_t hw_timer_get_intr_type()
{
    return frc1.ctrl.intr_type;
}

esp_err_t hw_timer_set_reload(bool reload)
{
    HW_TIMER_CHECK(hw_timer_obj, "hw_timer has not been initialized yet", ESP_FAIL);

    ENTER_CRITICAL();
    if (true == reload) {
        frc1.ctrl.reload = 0x01;
    } else {
        frc1.ctrl.reload = 0x00;
    }
    EXIT_CRITICAL();

    return ESP_OK;
    
}

bool hw_timer_get_reload()
{
    if (frc1.ctrl.reload) {
        return true;
    } else {
        return false;
    }
}

esp_err_t hw_timer_enable(bool en)
{
    HW_TIMER_CHECK(hw_timer_obj, "hw_timer has not been initialized yet", ESP_FAIL);

    ENTER_CRITICAL();
    if (true == en) {
        frc1.ctrl.en = 0x01;
    } else {
        frc1.ctrl.en = 0x00;
    }
    EXIT_CRITICAL();

    return ESP_OK;
}

bool hw_timer_get_enable()
{
    if (frc1.ctrl.en) {
        return true;
    } else {
        return false;
    }
}

esp_err_t hw_timer_set_load_data(uint32_t load_data)
{
    HW_TIMER_CHECK(hw_timer_obj, "hw_timer has not been initialized yet", ESP_FAIL);
    HW_TIMER_CHECK(load_data < 0x1000000, "load_data is out of range.", ESP_ERR_INVALID_ARG);

    ENTER_CRITICAL();
    frc1.load.data = load_data;  
    EXIT_CRITICAL();

    return ESP_OK;
}

uint32_t hw_timer_get_load_data()
{
    return frc1.load.data;
}

uint32_t hw_timer_get_count_data()
{
    return frc1.count.data;
}

static void IRAM_ATTR hw_timer_isr_cb(void* arg)
{
    if (!frc1.ctrl.reload) {
        frc1.ctrl.en = 0;
    }
    if (hw_timer_obj->cb != NULL) {
        hw_timer_obj->cb(arg);
    }
}

esp_err_t hw_timer_disarm(void)
{
    HW_TIMER_CHECK(hw_timer_obj, "hw_timer has not been initialized yet", ESP_FAIL);

    frc1.ctrl.val = 0;

    return ESP_OK;
}

esp_err_t hw_timer_alarm_us(uint32_t value, bool reload)
{
    HW_TIMER_CHECK(hw_timer_obj, "hw_timer has not been initialized yet", ESP_FAIL);
    HW_TIMER_CHECK( (reload ? ((value > 50) ? 1 : 0) : ((value > 10) ? 1 : 0)) && (value <= 0x199999), "value is out of range.", ESP_ERR_INVALID_ARG);
    
    hw_timer_set_reload(reload);
    hw_timer_set_clkdiv(TIMER_CLKDIV_16);
    hw_timer_set_intr_type(TIMER_EDGE_INT);
    hw_timer_set_load_data(((TIMER_BASE_CLK >> hw_timer_get_clkdiv()) / 1000000) * value);   // Calculate the number of timer clocks required for timing, ticks = (80MHz / div) * t
    hw_timer_enable(true);

    return ESP_OK;
}

static void hw_timer_obj_delete(void)
{
    if (NULL == hw_timer_obj) {
        return;
    }
    hw_timer_obj->cb = NULL;
    heap_caps_free(hw_timer_obj);
    hw_timer_obj = NULL;
}

static esp_err_t hw_timer_obj_create(void)
{
    hw_timer_obj = (hw_timer_obj_t *)heap_caps_malloc(sizeof(hw_timer_obj_t), MALLOC_CAP_8BIT);

    if (NULL == hw_timer_obj) {
        hw_timer_obj_delete();
        return ESP_FAIL;
    }
    hw_timer_obj->cb = NULL;

    return ESP_OK;
}

esp_err_t hw_timer_deinit(void)
{
    HW_TIMER_CHECK(hw_timer_obj, "hw_timer has not been initialized yet", ESP_FAIL);

    hw_timer_disarm();
    hw_timer_intr_disable();
    TM1_EDGE_INT_DISABLE();
    hw_timer_intr_register(NULL, NULL);
    hw_timer_obj_delete();

    return ESP_OK;
}

esp_err_t hw_timer_init(hw_timer_callback_t callback, void *arg)
{
    HW_TIMER_CHECK(hw_timer_obj == NULL, "hw_timer has been initialized", ESP_FAIL);
    HW_TIMER_CHECK(callback != NULL, "callback pointer NULL", ESP_ERR_INVALID_ARG);

    if (ESP_FAIL == hw_timer_obj_create()) {
        return ESP_FAIL;
    }
    hw_timer_obj->cb = callback;
    hw_timer_intr_register(hw_timer_isr_cb, arg);
    TM1_EDGE_INT_ENABLE();
    hw_timer_intr_enable();

    return ESP_OK;
}
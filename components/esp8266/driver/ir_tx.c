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

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ir_tx.h"
#include "driver/hw_timer.h"
#include "driver/i2s.h"
#include "esp8266/gpio_struct.h"

static const char *TAG = "ir tx";

#define WDEVTSF0_TIME_LO     0x3ff21004
#define WDEVTSF0_TIME_HI     0x3ff21008
#define WDEVTSFSW0_LO        0x3ff21018
#define WDEVTSFSW0_HI        0x3ff2101C
#define WDEVTSF0_TIMER_LO    0x3ff2109c
#define WDEVTSF0_TIMER_HI    0x3ff210a0
#define WDEVTSF0TIMER_ENA    0x3ff21098
#define WDEV_TSF0TIMER_ENA   BIT(31)

int wDev_MacTimSetFunc(void (*handle)(void));

#define IR_TX_CHECK(a, str, ret_val) \
    if (!(a)) { \
        ESP_LOGE(TAG,"%s(%d): %s", __FUNCTION__, __LINE__, str); \
        return (ret_val); \
    }

typedef enum {
    TX_BIT_CARRIER,
    TX_BIT_LOW,
} ir_tx_bit_state_t;

typedef enum {
    IR_TX_IDLE,
    IR_TX_HEADER,
    IR_TX_DATA,
    IR_TX_REP,
} ir_tx_state_t;

/**
 * @brief IR TX transmission parameter structure type definition
 */
typedef struct {
    ir_tx_nec_data_t data;
    uint8_t repeat;
} ir_tx_trans_t;

typedef struct {
    uint32_t io_num;
    uint32_t freq;
    SemaphoreHandle_t done_sem;
    SemaphoreHandle_t send_mux;
    ir_tx_trans_t trans;
    ir_tx_timer_t timer;
} ir_tx_obj_t;

ir_tx_obj_t *ir_tx_obj = NULL;

static ir_tx_state_t ir_tx_state = IR_TX_IDLE;

static void inline ir_tx_clear_carrier()
{
    switch (ir_tx_obj->io_num) {
        case 2: {
            GPIO.out_w1tc |= 0x4; // GPIO 2
            PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
        }
        break;

        case 14: {
            GPIO.out_w1tc |= 0x4000; // GPIO 14
            PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14);
        }
        break;
    }
}

static void inline ir_tx_gen_carrier()
{
    switch (ir_tx_obj->io_num) {
        case 2: {
            GPIO.out_w1ts |= 0x4; // GPIO 2
            PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_I2SO_WS);
        }
        break;

        case 14: {
            GPIO.out_w1ts |= 0x4000; // GPIO 14
            PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_I2SI_WS);
        }
        break;
    }
}

static void inline ir_tx_timer_alarm(uint32_t val)
{
    if (ir_tx_obj->timer == IR_TX_WDEV_TIMER) {
        REG_WRITE(WDEVTSFSW0_LO, 0);
        REG_WRITE(WDEVTSFSW0_HI, 0);
        REG_WRITE(WDEVTSFSW0_LO, 0);
        REG_WRITE(WDEVTSF0_TIMER_LO, 0);
        REG_WRITE(WDEVTSF0_TIMER_HI, 0);
        REG_WRITE(WDEVTSF0_TIMER_LO, val - IR_TX_WDEV_TIMER_ERROR_US);
        REG_WRITE(WDEVTSF0TIMER_ENA, WDEV_TSF0TIMER_ENA);
    } else {
        hw_timer_alarm_us(val - IR_TX_HW_TIMER_ERROR_US, false);
    }
}

void IRAM_ATTR ir_tx_handler()
{
    uint32_t t_expire = 0;
    static uint32_t rep_expire_us = IR_TX_NEC_REP_CYCLE; //for nec 32bit mode
    static uint16_t data_tmp = 0;
    static uint8_t ir_tx_bit_num = 0;
    static uint8_t ir_bit_state = TX_BIT_CARRIER;

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (ir_tx_obj->timer == IR_TX_WDEV_TIMER) {
        REG_WRITE(WDEVTSF0TIMER_ENA, REG_READ(WDEVTSF0TIMER_ENA) & (~WDEV_TSF0TIMER_ENA));
    }
    switch (ir_tx_state) {
        case IR_TX_IDLE: {
            ir_tx_gen_carrier();
            ir_tx_timer_alarm(IR_TX_NEC_HEADER_HIGH_US);
            ir_tx_state = IR_TX_HEADER;
            break;
        }

        case IR_TX_HEADER: {
            ir_tx_clear_carrier();
            ir_tx_timer_alarm(IR_TX_NEC_HEADER_LOW_US);
            ir_tx_state = IR_TX_DATA;
            ir_bit_state = TX_BIT_CARRIER;
            data_tmp = ir_tx_obj->trans.data.addr1;
            rep_expire_us -= (IR_TX_NEC_HEADER_HIGH_US + IR_TX_NEC_HEADER_LOW_US);
            break;
        }

        case IR_TX_DATA: {
            if (ir_bit_state == TX_BIT_CARRIER) {
                t_expire = IR_TX_NEC_DATA_HIGH_US;
                ir_bit_state = TX_BIT_LOW;
                ir_tx_gen_carrier();
            } else if (ir_bit_state == TX_BIT_LOW) {
                ir_tx_clear_carrier();

                if ((data_tmp >> (ir_tx_bit_num % IR_TX_NEC_BIT_NUM)) & 0x1) {
                    t_expire = IR_TX_NEC_DATA_LOW_1_US;
                } else {
                    t_expire = IR_TX_NEC_DATA_LOW_0_US;
                }

                ir_tx_bit_num++;

                if (ir_tx_bit_num == IR_TX_NEC_BIT_NUM) {
                    data_tmp = ir_tx_obj->trans.data.addr2;
                } else if (ir_tx_bit_num == IR_TX_NEC_BIT_NUM * 2) {
                    data_tmp = ir_tx_obj->trans.data.cmd1;
                } else if (ir_tx_bit_num == IR_TX_NEC_BIT_NUM * 3) {
                    data_tmp = ir_tx_obj->trans.data.cmd2;
                } else if ((ir_tx_bit_num == (IR_TX_NEC_BIT_NUM * 4 + 1))) {
                    //clean up state for next or for repeat
                    ir_tx_bit_num = 0;
                    ir_bit_state = TX_BIT_CARRIER;

                    if (ir_tx_obj->trans.repeat > 0) {
                        t_expire = (rep_expire_us - 5);
                        ir_tx_timer_alarm(t_expire);
                        rep_expire_us = IR_TX_NEC_REP_CYCLE;
                        ir_tx_state = IR_TX_REP;
                    } else {
                        rep_expire_us = IR_TX_NEC_REP_CYCLE;
                        ir_tx_state = IR_TX_IDLE;
                        xSemaphoreGiveFromISR(ir_tx_obj->done_sem, &xHigherPriorityTaskWoken);
                    }

                    break;

                }

                ir_bit_state = TX_BIT_CARRIER;
            } else {
            }

            rep_expire_us -= t_expire;
            ir_tx_timer_alarm(t_expire);
            break;
        }

        case IR_TX_REP: {
            if (ir_tx_obj->trans.repeat > 0) {
                if (ir_tx_bit_num == 0) {
                    ir_tx_gen_carrier();
                    t_expire = IR_TX_NEC_REP_HIGH_US ;
                } else if (ir_tx_bit_num == 1) {
                    ir_tx_clear_carrier();
                    t_expire = IR_TX_NEC_REP_LOW_US ;
                } else if (ir_tx_bit_num == 2) {
                    ir_tx_gen_carrier();
                    t_expire = IR_TX_NEC_REP_STOP_US;
                } else if (ir_tx_bit_num == 3) {
                    ir_tx_clear_carrier();
                    ir_tx_obj->trans.repeat--;

                    if (ir_tx_obj->trans.repeat > 0) {
                        t_expire = rep_expire_us ;
                        rep_expire_us = IR_TX_NEC_REP_CYCLE;
                    } else {
                        ir_tx_bit_num = 0;
                        rep_expire_us = IR_TX_NEC_REP_CYCLE;
                        ir_tx_state = IR_TX_IDLE;
                        ir_bit_state = TX_BIT_CARRIER;
                        xSemaphoreGiveFromISR(ir_tx_obj->done_sem, &xHigherPriorityTaskWoken);
                        break;
                    }
                } else {
                }

                ir_tx_bit_num++;//bit num reuse for repeat wave form

                if (ir_tx_bit_num == 4) {
                    ir_tx_bit_num = 0;
                    rep_expire_us = IR_TX_NEC_REP_CYCLE;
                } else {
                    rep_expire_us -=  t_expire;
                }

                ir_tx_timer_alarm(t_expire);
            }

            break;
        }

        default:
            break;
    }

    if (xHigherPriorityTaskWoken == pdTRUE) {
        taskYIELD();
    }
}

static esp_err_t ir_tx_trans(ir_tx_nec_data_t data, uint8_t repeat, uint32_t *timeout_ticks)
{
    int ret;
    uint32_t ticks_escape = 0, ticks_last = 0;
    struct timeval now;

    if (*timeout_ticks != portMAX_DELAY) {
        gettimeofday(&now, NULL);
        ticks_last = (now.tv_sec * 1000 + now.tv_usec / 1000) / portTICK_RATE_MS;
    }

    if (ir_tx_state != IR_TX_IDLE) {
        IR_TX_CHECK(false, "When transmission begins, the state must be idle", ESP_FAIL);
    }

    ir_tx_obj->trans.data = data;
    ir_tx_obj->trans.repeat = repeat;
    xSemaphoreTake(ir_tx_obj->done_sem, 0); // Clear possible semaphore
    ir_tx_handler();

    if (ir_tx_state != IR_TX_IDLE) {
        ret = xSemaphoreTake(ir_tx_obj->done_sem, *timeout_ticks);

        if (ret != pdTRUE) {
            IR_TX_CHECK(false, "Waiting for done_sem error", ESP_ERR_TIMEOUT);
        }
    }

    if (*timeout_ticks != portMAX_DELAY) {
        gettimeofday(&now, NULL);
        ticks_escape = (now.tv_sec * 1000 + now.tv_usec / 1000) / portTICK_RATE_MS - ticks_last;

        if (*timeout_ticks <= ticks_escape) {
            IR_TX_CHECK(false, "timeout", ESP_ERR_TIMEOUT);
        } else {
            *timeout_ticks -= ticks_escape;
        }
    }

    return ESP_OK;
}

int ir_tx_send_data(ir_tx_nec_data_t *data, size_t len, uint32_t timeout_ticks)
{
    IR_TX_CHECK(ir_tx_obj, "ir tx has not been initialized yet.", ESP_FAIL);
    int ret;
    int x, y;
    uint32_t ticks_escape = 0, ticks_last = 0;
    struct timeval now;

    if (timeout_ticks != portMAX_DELAY) {
        gettimeofday(&now, NULL);
        ticks_last = (now.tv_sec * 1000 + now.tv_usec / 1000) / portTICK_RATE_MS;
    }

    ret = xSemaphoreTake(ir_tx_obj->send_mux, timeout_ticks);

    if (ret != pdTRUE) {
        IR_TX_CHECK(false, "SemaphoreTake error", -1);
    }

    if (timeout_ticks != portMAX_DELAY) {
        gettimeofday(&now, NULL);
        ticks_escape = (now.tv_sec * 1000 + now.tv_usec / 1000) / portTICK_RATE_MS - ticks_last;

        if (timeout_ticks <= ticks_escape) {
            xSemaphoreGive(ir_tx_obj->send_mux);
            IR_TX_CHECK(false, "timeout", -1);
        } else {
            timeout_ticks -= ticks_escape;
        }
    }

    for (x = 0; x < len;) {
        for (y = 1; y < len - x; y++) {
            if (data[y + x].val != data[x].val) { // search repeat
                break;
            }
        }

        ret = ir_tx_trans(data[x], y - 1, &timeout_ticks);

        if (ret != ESP_OK) {
            if (ret == ESP_ERR_TIMEOUT) {
                x += y;
            }

            xSemaphoreGive(ir_tx_obj->send_mux);
            IR_TX_CHECK(false, "trans data error", x);
        }

        x += y;
    }

    xSemaphoreGive(ir_tx_obj->send_mux);

    return len;
}

esp_err_t ir_tx_deinit()
{
    IR_TX_CHECK(ir_tx_obj, "ir tx has not been initialized yet.", ESP_FAIL);

    if (ir_tx_obj->done_sem) {
        vSemaphoreDelete(ir_tx_obj->done_sem);
        ir_tx_obj->done_sem = NULL;
    }

    if (ir_tx_obj->send_mux) {
        vSemaphoreDelete(ir_tx_obj->send_mux);
        ir_tx_obj->send_mux = NULL;
    }

    if (ir_tx_obj->timer == IR_TX_WDEV_TIMER) {
        REG_WRITE(WDEVTSF0TIMER_ENA, REG_READ(WDEVTSF0TIMER_ENA) & (~WDEV_TSF0TIMER_ENA));
        wDev_MacTimSetFunc(NULL);
    } else {
        hw_timer_deinit();
    }

    i2s_driver_uninstall(I2S_NUM_0);

    heap_caps_free(ir_tx_obj);
    ir_tx_obj = NULL;
    return ESP_OK;
}

esp_err_t ir_tx_init(ir_tx_config_t *config)
{
    IR_TX_CHECK(config, "config error", ESP_ERR_INVALID_ARG);
    IR_TX_CHECK((config->io_num == 2) || (config->io_num == 14), "Only supports io2 and io14 as carrier outputs", ESP_ERR_INVALID_ARG);
    IR_TX_CHECK(NULL == ir_tx_obj, "ir tx has been initialized", ESP_FAIL);

    ir_tx_obj = heap_caps_malloc(sizeof(ir_tx_obj_t), MALLOC_CAP_8BIT);
    IR_TX_CHECK(ir_tx_obj, "ir tx object malloc error", ESP_ERR_NO_MEM);
    ir_tx_obj->io_num = config->io_num;
    ir_tx_obj->freq = config->freq;
    ir_tx_obj->timer = config->timer;
    ir_tx_obj->done_sem = xSemaphoreCreateBinary();
    ir_tx_obj->send_mux = xSemaphoreCreateMutex();

    if (NULL == ir_tx_obj->done_sem || NULL == ir_tx_obj->send_mux) {
        ir_tx_deinit();
        IR_TX_CHECK(false, "Semaphore create fail", ESP_ERR_NO_MEM);
    }

    // init default data
    ir_tx_obj->trans.data.addr1 = (uint8_t)0xee; //addr code
    ir_tx_obj->trans.data.addr2 = (uint8_t)~0xee;
    ir_tx_obj->trans.data.cmd1 = (uint8_t)0x5a; //cmd code
    ir_tx_obj->trans.data.cmd2 = (uint8_t)~0x5a;
    ir_tx_obj->trans.repeat = 5;       //repeat number

    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 1 << ir_tx_obj->io_num;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER,    // Only carrier mode
        .sample_rate = ir_tx_obj->freq,
        .bits_per_sample = 16,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,                           // 2-channels
        .communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,
        .dma_buf_count = 2, // no use
        .dma_buf_len = 8 // no use
    };
    i2s_pin_config_t pin_config = {
        .bck_o_en = -1,
        .ws_o_en = (ir_tx_obj->io_num == 2) ? 1 : -1,
        .bck_i_en = -1,
        .ws_i_en = (ir_tx_obj->io_num == 14) ? 1 : -1,
        .data_out_en = -1,
        .data_in_en = -1
    };

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
    if (ir_tx_obj->timer == IR_TX_WDEV_TIMER) {
        wDev_MacTimSetFunc(ir_tx_handler);
    } else {
        hw_timer_init(ir_tx_handler, NULL);
        hw_timer_disarm();
    }
    ir_tx_clear_carrier();

    return ESP_OK;
}

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

#include <stdint.h>
#include <stdlib.h>

#include "esp8266/eagle_soc.h"
#include "esp8266/pin_mux_register.h"
#include "esp8266/gpio_struct.h"

#include "rom/ets_sys.h"

#include "driver/gpio.h"

#include "esp_err.h"
#include "esp_log.h"    //TODO:No dependence on RTOS

// Temporary use the FreeRTOS critical function
#include "FreeRTOS.h"
#define ENTER_CRITICAL() portENTER_CRITICAL()
#define EXIT_CRITICAL() portEXIT_CRITICAL()

static const char *GPIO_TAG = "gpio";

#define GPIO_CHECK(a, str, ret_val) \
    if (!(a)) { \
        ESP_LOGE(GPIO_TAG,"%s(%d): %s", __FUNCTION__, __LINE__, str); \
        return (ret_val); \
    }

#define GPIO_PIN_REG_0          PERIPHS_IO_MUX_GPIO0_U
#define GPIO_PIN_REG_1          PERIPHS_IO_MUX_U0TXD_U
#define GPIO_PIN_REG_2          PERIPHS_IO_MUX_GPIO2_U
#define GPIO_PIN_REG_3          PERIPHS_IO_MUX_U0RXD_U
#define GPIO_PIN_REG_4          PERIPHS_IO_MUX_GPIO4_U
#define GPIO_PIN_REG_5          PERIPHS_IO_MUX_GPIO5_U
#define GPIO_PIN_REG_6          PERIPHS_IO_MUX_SD_CLK_U
#define GPIO_PIN_REG_7          PERIPHS_IO_MUX_SD_DATA0_U
#define GPIO_PIN_REG_8          PERIPHS_IO_MUX_SD_DATA1_U
#define GPIO_PIN_REG_9          PERIPHS_IO_MUX_SD_DATA2_U
#define GPIO_PIN_REG_10         PERIPHS_IO_MUX_SD_DATA3_U
#define GPIO_PIN_REG_11         PERIPHS_IO_MUX_SD_CMD_U
#define GPIO_PIN_REG_12         PERIPHS_IO_MUX_MTDI_U
#define GPIO_PIN_REG_13         PERIPHS_IO_MUX_MTCK_U
#define GPIO_PIN_REG_14         PERIPHS_IO_MUX_MTMS_U
#define GPIO_PIN_REG_15         PERIPHS_IO_MUX_MTDO_U
#define GPIO_PIN_REG_16         PAD_XPD_DCDC_CONF

#define GPIO_PIN_REG(i) \
    (i==0) ? GPIO_PIN_REG_0:  \
    (i==1) ? GPIO_PIN_REG_1:  \
    (i==2) ? GPIO_PIN_REG_2:  \
    (i==3) ? GPIO_PIN_REG_3:  \
    (i==4) ? GPIO_PIN_REG_4:  \
    (i==5) ? GPIO_PIN_REG_5:  \
    (i==6) ? GPIO_PIN_REG_6:  \
    (i==7) ? GPIO_PIN_REG_7:  \
    (i==8) ? GPIO_PIN_REG_8:  \
    (i==9) ? GPIO_PIN_REG_9:  \
    (i==10)? GPIO_PIN_REG_10: \
    (i==11)? GPIO_PIN_REG_11: \
    (i==12)? GPIO_PIN_REG_12: \
    (i==13)? GPIO_PIN_REG_13: \
    (i==14)? GPIO_PIN_REG_14: \
    (i==15)? GPIO_PIN_REG_15: \
    GPIO_PIN_REG_16

typedef struct {
    gpio_isr_t fn;   /*!< isr function */
    void *args;      /*!< isr function args */
} gpio_isr_func_t;

static gpio_isr_func_t *gpio_isr_func = NULL;

esp_err_t gpio_pullup_en(gpio_num_t gpio_num)
{
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);

    if (RTC_GPIO_IS_VALID_GPIO(gpio_num)) {
        return ESP_OK;
    }

    gpio_pin_reg_t pin_reg;
    pin_reg.val = READ_PERI_REG(GPIO_PIN_REG(gpio_num));
    pin_reg.pullup = 1;
    WRITE_PERI_REG(GPIO_PIN_REG(gpio_num), pin_reg.val);
    return ESP_OK;
}

esp_err_t gpio_pullup_dis(gpio_num_t gpio_num)
{
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);

    if (RTC_GPIO_IS_VALID_GPIO(gpio_num)) {
        return ESP_OK;
    }

    gpio_pin_reg_t pin_reg;
    pin_reg.val = READ_PERI_REG(GPIO_PIN_REG(gpio_num));
    pin_reg.pullup = 0;
    WRITE_PERI_REG(GPIO_PIN_REG(gpio_num), pin_reg.val);
    return ESP_OK;
}

esp_err_t gpio_pulldown_en(gpio_num_t gpio_num)
{
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);

    if (!RTC_GPIO_IS_VALID_GPIO(gpio_num)) {
        return ESP_OK;
    }

    gpio_pin_reg_t pin_reg;
    pin_reg.val = READ_PERI_REG(GPIO_PIN_REG(gpio_num));
    pin_reg.rtc_pin.pulldown = 1;
    WRITE_PERI_REG(GPIO_PIN_REG(gpio_num), pin_reg.val);
    return ESP_OK;
}

esp_err_t gpio_pulldown_dis(gpio_num_t gpio_num)
{
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);

    if (!RTC_GPIO_IS_VALID_GPIO(gpio_num)) {
        return ESP_OK;
    }

    gpio_pin_reg_t pin_reg;
    pin_reg.val = READ_PERI_REG(GPIO_PIN_REG(gpio_num));
    pin_reg.rtc_pin.pulldown = 0;
    WRITE_PERI_REG(GPIO_PIN_REG(gpio_num), pin_reg.val);
    return ESP_OK;
}

esp_err_t gpio_set_intr_type(gpio_num_t gpio_num, gpio_int_type_t intr_type)
{
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);
    GPIO_CHECK(!RTC_GPIO_IS_VALID_GPIO(gpio_num), "GPIO is RTC GPIO", ESP_ERR_INVALID_ARG);
    GPIO_CHECK(intr_type < GPIO_INTR_MAX, "GPIO interrupt type error", ESP_ERR_INVALID_ARG);

    GPIO.pin[gpio_num].int_type = intr_type;
    return ESP_OK;
}

static esp_err_t gpio_output_disable(gpio_num_t gpio_num)
{
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);

    if (RTC_GPIO_IS_VALID_GPIO(gpio_num)) {
        WRITE_PERI_REG(PAD_XPD_DCDC_CONF, ((READ_PERI_REG(PAD_XPD_DCDC_CONF) & (uint32_t)0xffffffbc)) | (uint32_t)0x1); 	// mux configuration for XPD_DCDC and rtc_gpio0 connection
        CLEAR_PERI_REG_MASK(RTC_GPIO_CONF, 0x1);    //mux configuration for out enable
        CLEAR_PERI_REG_MASK(RTC_GPIO_ENABLE, 0x1);   //out disable
    } else {
        GPIO.enable_w1tc |= (0x1 << gpio_num);
    }

    return ESP_OK;
}

static esp_err_t gpio_output_enable(gpio_num_t gpio_num)
{
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);

    if (RTC_GPIO_IS_VALID_GPIO(gpio_num)) {
        WRITE_PERI_REG(PAD_XPD_DCDC_CONF, ((READ_PERI_REG(PAD_XPD_DCDC_CONF) & (uint32_t)0xffffffbc)) | (uint32_t)0x1); // mux configuration for XPD_DCDC and rtc_gpio0 connection
        CLEAR_PERI_REG_MASK(RTC_GPIO_CONF, 0x1);                                                                        //mux configuration for out enable
        SET_PERI_REG_MASK(RTC_GPIO_ENABLE, 0x1);                                                                        //out enable
    } else {
        GPIO.enable_w1ts |= (0x1 << gpio_num);
    }

    return ESP_OK;
}

esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level)
{
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);

    if (RTC_GPIO_IS_VALID_GPIO(gpio_num)) {
        if (level) {
            SET_PERI_REG_MASK(RTC_GPIO_OUT, 0x1);    //set_high_level
        } else {
            CLEAR_PERI_REG_MASK(RTC_GPIO_OUT, 0x1);    //set_low_level
        }
    } else {
        if (level) {
            GPIO.out_w1ts |= (0x1 << gpio_num);
        } else {
            GPIO.out_w1tc |= (0x1 << gpio_num);
        }
    }

    return ESP_OK;
}

int gpio_get_level(gpio_num_t gpio_num)
{
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);

    if (RTC_GPIO_IS_VALID_GPIO(gpio_num)) {
        return READ_PERI_REG(RTC_GPIO_IN_DATA) & 0x1;
    } else {
        return (GPIO.in >> gpio_num) & 0x1;
    }
}

esp_err_t gpio_set_pull_mode(gpio_num_t gpio_num, gpio_pull_mode_t pull)
{
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);
    GPIO_CHECK(pull <= GPIO_FLOATING, "GPIO pull mode error", ESP_ERR_INVALID_ARG);

    esp_err_t ret = ESP_OK;

    switch (pull) {
        case GPIO_PULLUP_ONLY:
            gpio_pulldown_dis(gpio_num);
            gpio_pullup_en(gpio_num);
            break;

        case GPIO_PULLDOWN_ONLY:
            gpio_pulldown_en(gpio_num);
            gpio_pullup_dis(gpio_num);
            break;

        case GPIO_FLOATING:
            gpio_pulldown_dis(gpio_num);
            gpio_pullup_dis(gpio_num);
            break;

        default:
            ESP_LOGE(GPIO_TAG, "Unknown pull up/down mode,gpio_num=%u,pull=%u", gpio_num, pull);
            ret = ESP_ERR_INVALID_ARG;
            break;
    }

    return ret;
}

esp_err_t gpio_set_direction(gpio_num_t gpio_num, gpio_mode_t mode)
{
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);

    // esp8266 input is always connected
    if (mode & GPIO_MODE_DEF_OUTPUT) {
        gpio_output_enable(gpio_num);
    } else {
        gpio_output_disable(gpio_num);
    }

    if ((mode & GPIO_MODE_DEF_OD) && !RTC_GPIO_IS_VALID_GPIO(gpio_num)) {
        GPIO.pin[gpio_num].driver = 1;
    } else {
        GPIO.pin[gpio_num].driver = 0;
    }

    return ESP_OK;
}

esp_err_t gpio_config(const gpio_config_t *gpio_cfg)
{
    uint32_t gpio_pin_mask = (gpio_cfg->pin_bit_mask);
    uint32_t io_reg = 0;
    uint32_t io_num = 0;
    uint8_t input_en = 0;
    uint8_t output_en = 0;
    uint8_t od_en = 0;
    uint8_t pu_en = 0;
    uint8_t pd_en = 0;
    gpio_pin_reg_t pin_reg;

    if (gpio_cfg->pin_bit_mask == 0 || gpio_cfg->pin_bit_mask >= (((uint32_t) 1) << GPIO_PIN_COUNT)) {
        ESP_LOGE(GPIO_TAG, "GPIO_PIN mask error ");
        return ESP_ERR_INVALID_ARG;
    }

    do {
        io_reg = GPIO_PIN_REG(io_num);

        if (((gpio_pin_mask >> io_num) & BIT(0))) {
            if (!io_reg) {
                ESP_LOGE(GPIO_TAG, "IO%d is not a valid GPIO", io_num);
                return ESP_ERR_INVALID_ARG;
            }

            if (gpio_cfg->mode & GPIO_MODE_OUTPUT) {
                output_en = 1;
            } else {
                input_en = 1;
            }

            if ((gpio_cfg->mode & GPIO_MODE_DEF_OD) && !RTC_GPIO_IS_VALID_GPIO(io_num)) {
                od_en = 1;
            }

            gpio_set_direction(io_num, gpio_cfg->mode);

            if (!RTC_GPIO_IS_VALID_GPIO(io_num)) {
                if (gpio_cfg->pull_up_en) {
                    pu_en = 1;
                    gpio_pullup_en(io_num);
                } else {
                    gpio_pullup_dis(io_num);
                }
            }

            if (RTC_GPIO_IS_VALID_GPIO(io_num)) {
                if (gpio_cfg->pull_down_en) {
                    pd_en = 1;
                    gpio_pulldown_en(io_num);
                } else {
                    gpio_pulldown_dis(io_num);
                }
            }

            ESP_LOGI(GPIO_TAG, "GPIO[%d]| InputEn: %d| OutputEn: %d| OpenDrain: %d| Pullup: %d| Pulldown: %d| Intr:%d ", io_num, input_en, output_en, od_en, pu_en, pd_en, gpio_cfg->intr_type);

            if (!RTC_GPIO_IS_VALID_GPIO(io_num)) {
                gpio_set_intr_type(io_num, gpio_cfg->intr_type);
            }

            pin_reg.val = READ_PERI_REG(GPIO_PIN_REG(io_num));

            // It should be noted that GPIO0, 2, 4, and 5 need to set the func register to 0,
            // and the other GPIO needs to be set to 3 so that IO can be GPIO function.
            if ((0x1 << io_num) & (GPIO_Pin_0 | GPIO_Pin_2 | GPIO_Pin_4 | GPIO_Pin_5)) {
                pin_reg.rtc_pin.func_low_bit = 0;
                pin_reg.rtc_pin.func_high_bit = 0;
            } else {
                pin_reg.func_low_bit = 3;
                pin_reg.func_high_bit = 0;
            }

            WRITE_PERI_REG(GPIO_PIN_REG(io_num), pin_reg.val);
        }

        io_num++;
    } while (io_num < GPIO_PIN_COUNT);

    return ESP_OK;
}

void IRAM_ATTR gpio_intr_service(void *arg)
{
    //GPIO intr process
    uint32_t gpio_num = 0;
    //read status to get interrupt status for GPIO0-15
    uint32_t gpio_intr_status = GPIO.status;

    if (gpio_isr_func == NULL) {
        return;
    }

    do {
        if (gpio_num < GPIO_PIN_COUNT - 1) {
            if (gpio_intr_status & BIT(gpio_num)) { //gpio0-gpio15
                GPIO.status_w1tc = BIT(gpio_num);
                if (gpio_isr_func[gpio_num].fn != NULL) {
                    gpio_isr_func[gpio_num].fn(gpio_isr_func[gpio_num].args);
                }
            }
        }
    } while (++gpio_num < GPIO_PIN_COUNT - 1);
}

esp_err_t gpio_isr_handler_add(gpio_num_t gpio_num, gpio_isr_t isr_handler, void *args)
{
    GPIO_CHECK(gpio_isr_func != NULL, "GPIO isr service is not installed, call gpio_install_isr_service() first", ESP_ERR_INVALID_STATE);
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);
    GPIO_CHECK(!RTC_GPIO_IS_VALID_GPIO(gpio_num), "GPIO is RTC GPIO", ESP_ERR_INVALID_ARG);

    ENTER_CRITICAL();
    _xt_isr_mask(1 << ETS_GPIO_INUM);

    if (gpio_isr_func) {
        gpio_isr_func[gpio_num].fn = isr_handler;
        gpio_isr_func[gpio_num].args = args;
    }

    _xt_isr_unmask(1 << ETS_GPIO_INUM);
    EXIT_CRITICAL();
    return ESP_OK;
}

esp_err_t gpio_isr_handler_remove(gpio_num_t gpio_num)
{
    GPIO_CHECK(gpio_isr_func != NULL, "GPIO isr service is not installed, call gpio_install_isr_service() first", ESP_ERR_INVALID_STATE);
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);
    GPIO_CHECK(!RTC_GPIO_IS_VALID_GPIO(gpio_num), "GPIO is RTC GPIO", ESP_ERR_INVALID_ARG);

    ENTER_CRITICAL();
    _xt_isr_mask(1 << ETS_GPIO_INUM);

    if (gpio_isr_func) {
        gpio_isr_func[gpio_num].fn = NULL;
        gpio_isr_func[gpio_num].args = NULL;
    }

    _xt_isr_unmask(1 << ETS_GPIO_INUM);
    EXIT_CRITICAL();
    return ESP_OK;
}

esp_err_t gpio_isr_register(void (*fn)(void *), void *arg, int no_use, gpio_isr_handle_t *handle_no_use)
{
    GPIO_CHECK(fn, "GPIO ISR null", ESP_ERR_INVALID_ARG);

    _xt_isr_attach(ETS_GPIO_INUM, fn, arg);
    return ESP_OK;
}

esp_err_t gpio_install_isr_service(int no_use)
{
    GPIO_CHECK(gpio_isr_func == NULL, "GPIO isr service already installed", ESP_FAIL);

    esp_err_t ret;
    ENTER_CRITICAL();
    gpio_isr_func = (gpio_isr_func_t *) calloc(GPIO_PIN_COUNT - 1, sizeof(gpio_isr_func_t));

    if (gpio_isr_func == NULL) {
        ret = ESP_ERR_NO_MEM;
    } else {
        ret = gpio_isr_register(gpio_intr_service, NULL, 0, NULL);
    }

    EXIT_CRITICAL();
    return ret;
}

void gpio_uninstall_isr_service()
{
    if (gpio_isr_func == NULL) {
        return;
    }

    ENTER_CRITICAL();
    _xt_isr_mask(1 << ETS_GPIO_INUM);
    _xt_isr_attach(ETS_GPIO_INUM, NULL, NULL);
    free(gpio_isr_func);
    gpio_isr_func = NULL;
    EXIT_CRITICAL();
    return;
}

/*only level interrupt can be used for wake-up function*/
esp_err_t gpio_wakeup_enable(gpio_num_t gpio_num, gpio_int_type_t intr_type)
{
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);
    GPIO_CHECK(!RTC_GPIO_IS_VALID_GPIO(gpio_num), "RTC IO can not use the wakeup function", ESP_ERR_INVALID_ARG);

    esp_err_t ret = ESP_OK;

    if ((intr_type == GPIO_INTR_LOW_LEVEL) || (intr_type == GPIO_INTR_HIGH_LEVEL)) {
        GPIO.pin[gpio_num].int_type = intr_type;
        GPIO.pin[gpio_num].wakeup_enable = 0x1;
    } else {
        ret = ESP_ERR_INVALID_ARG;
    }

    return ret;
}

esp_err_t gpio_wakeup_disable(gpio_num_t gpio_num)
{
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);
    GPIO_CHECK(!RTC_GPIO_IS_VALID_GPIO(gpio_num), "RTC IO can not use the wakeup function", ESP_ERR_INVALID_ARG);

    GPIO.pin[gpio_num].wakeup_enable = 0;
    return ESP_OK;
}

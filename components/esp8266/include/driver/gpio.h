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

#ifndef __GPIO_H__
#define __GPIO_H__

#include <stdint.h>
#include "esp_err.h"

#include "esp8266/eagle_soc.h"
#include "esp8266/pin_mux_register.h"
#include "esp8266/gpio_register.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BIT
#define BIT(x) (1 << (x))
#endif

#define GPIO_Pin_0              (BIT(0))  /* Pin 0 selected */
#define GPIO_Pin_1              (BIT(1))  /* Pin 1 selected */
#define GPIO_Pin_2              (BIT(2))  /* Pin 2 selected */
#define GPIO_Pin_3              (BIT(3))  /* Pin 3 selected */
#define GPIO_Pin_4              (BIT(4))  /* Pin 4 selected */
#define GPIO_Pin_5              (BIT(5))  /* Pin 5 selected */
#define GPIO_Pin_6              (BIT(6))  /* Pin 6 selected */
#define GPIO_Pin_7              (BIT(7))  /* Pin 7 selected */
#define GPIO_Pin_8              (BIT(8))  /* Pin 8 selected */
#define GPIO_Pin_9              (BIT(9))  /* Pin 9 selected */
#define GPIO_Pin_10             (BIT(10)) /* Pin 10 selected */
#define GPIO_Pin_11             (BIT(11)) /* Pin 11 selected */
#define GPIO_Pin_12             (BIT(12)) /* Pin 12 selected */
#define GPIO_Pin_13             (BIT(13)) /* Pin 13 selected */
#define GPIO_Pin_14             (BIT(14)) /* Pin 14 selected */
#define GPIO_Pin_15             (BIT(15)) /* Pin 15 selected */
#define GPIO_Pin_16             (BIT(16)) /* Pin 16 selected */
#define GPIO_Pin_All            (0x1FFFF)  /* All pins selected */

#define GPIO_MODE_DEF_DISABLE         (0)
#define GPIO_MODE_DEF_INPUT           (BIT(0))
#define GPIO_MODE_DEF_OUTPUT          (BIT(1))
#define GPIO_MODE_DEF_OD              (BIT(2))

#define GPIO_PIN_COUNT              17

#define GPIO_IS_VALID_GPIO(gpio_num)      ((gpio_num < GPIO_PIN_COUNT))   /*!< Check whether it is a valid GPIO number */
#define RTC_GPIO_IS_VALID_GPIO(gpio_num)     ((gpio_num == 16))    /*!< Check whether it is a valid RTC GPIO number */

typedef enum {
    GPIO_NUM_0 = 0,     /*!< GPIO0, input and output */
    GPIO_NUM_1 = 1,     /*!< GPIO1, input and output */
    GPIO_NUM_2 = 2,     /*!< GPIO2, input and output */
    GPIO_NUM_3 = 3,     /*!< GPIO3, input and output */
    GPIO_NUM_4 = 4,     /*!< GPIO4, input and output */
    GPIO_NUM_5 = 5,     /*!< GPIO5, input and output */
    GPIO_NUM_6 = 6,     /*!< GPIO6, input and output */
    GPIO_NUM_7 = 7,     /*!< GPIO7, input and output */
    GPIO_NUM_8 = 8,     /*!< GPIO8, input and output */
    GPIO_NUM_9 = 9,     /*!< GPIO9, input and output */
    GPIO_NUM_10 = 10,   /*!< GPIO10, input and output */
    GPIO_NUM_11 = 11,   /*!< GPIO11, input and output */
    GPIO_NUM_12 = 12,   /*!< GPIO12, input and output */
    GPIO_NUM_13 = 13,   /*!< GPIO13, input and output */
    GPIO_NUM_14 = 14,   /*!< GPIO14, input and output */
    GPIO_NUM_15 = 15,   /*!< GPIO15, input and output */
    GPIO_NUM_16 = 16,   /*!< GPIO16, input and output */
    GPIO_NUM_MAX = 17,
    /** @endcond */
} gpio_num_t;

typedef enum {
    GPIO_INTR_DISABLE = 0,    /*!< Disable GPIO interrupt */
    GPIO_INTR_POSEDGE = 1,    /*!< GPIO interrupt type : rising edge */
    GPIO_INTR_NEGEDGE = 2,    /*!< GPIO interrupt type : falling edge */
    GPIO_INTR_ANYEDGE = 3,    /*!< GPIO interrupt type : both rising and falling edge */
    GPIO_INTR_LOW_LEVEL = 4,  /*!< GPIO interrupt type : input low level trigger */
    GPIO_INTR_HIGH_LEVEL = 5, /*!< GPIO interrupt type : input high level trigger */
    GPIO_INTR_MAX,
} gpio_int_type_t;

typedef enum {
    GPIO_MODE_DISABLE = GPIO_MODE_DEF_DISABLE,                          /*!< GPIO mode : disable input and output */
    GPIO_MODE_INPUT = GPIO_MODE_DEF_INPUT,                              /*!< GPIO mode : input only */
    GPIO_MODE_OUTPUT = GPIO_MODE_DEF_OUTPUT,                            /*!< GPIO mode : output only mode */
    GPIO_MODE_OUTPUT_OD = ((GPIO_MODE_DEF_OUTPUT) | (GPIO_MODE_DEF_OD)), /*!< GPIO mode : output only with open-drain mode */
} gpio_mode_t;

typedef enum {
    GPIO_PULLUP_ONLY,     /*!< Pad pull up */
    GPIO_PULLDOWN_ONLY,   /*!< Pad pull down */
    GPIO_FLOATING,        /*!< Pad floating */
} gpio_pull_mode_t;

typedef enum {
    GPIO_PULLUP_DISABLE = 0x0, /*!< Disable GPIO pull-up resistor */
    GPIO_PULLUP_ENABLE = 0x1,  /*!< Enable GPIO pull-up resistor */
} gpio_pullup_t;

typedef enum {
    GPIO_PULLDOWN_DISABLE = 0x0,   /*!< Disable GPIO pull-down resistor */
    GPIO_PULLDOWN_ENABLE = 0x1,    /*!< Enable GPIO pull-down resistor  */
} gpio_pulldown_t;

/**
 * @brief Configuration parameters of GPIO pad for gpio_config function
 */
typedef struct {
    uint32_t pin_bit_mask;          /*!< GPIO pin: set with bit mask, each bit maps to a GPIO */
    gpio_mode_t mode;               /*!< GPIO mode: set input/output mode */
    gpio_pullup_t pull_up_en;       /*!< GPIO pull-up */
    gpio_pulldown_t pull_down_en;   /*!< GPIO pull-down */
    gpio_int_type_t intr_type;      /*!< GPIO interrupt type */
} gpio_config_t;

typedef void (*gpio_isr_t)(void *);
typedef void *gpio_isr_handle_t;

/**
 * @brief GPIO common configuration
 *
 * Configure GPIO's Mode,pull-up,PullDown,IntrType
 *
 * @param  gpio_cfg  Pointer to GPIO configure struct
 *
 * @return
 *     - ESP_OK success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t gpio_config(const gpio_config_t *gpio_cfg);

/**
 * @brief  GPIO set interrupt trigger type
 *
 * @param  gpio_num GPIO number. If you want to set the trigger type of e.g. of GPIO12, gpio_num should be GPIO_NUM_12 (12);
 * @param  intr_type Interrupt type, select from gpio_int_type_t
 *
 * @return
 *     - ESP_OK  Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t gpio_set_intr_type(gpio_num_t gpio_num, gpio_int_type_t intr_type);

/**
 * @brief  GPIO set output level
 *
 * @param  gpio_num GPIO number. If you want to set the output level of e.g. GPIO16, gpio_num should be GPIO_NUM_16 (16);
 * @param  level Output level. 0: low ; 1: high
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG GPIO number error
 */
esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level);

/**
 * @brief  GPIO get input level
 *
 * @note   If the pad is not configured for input (or input and output) the returned value is always 0.
 *
 * @param  gpio_num GPIO number. If you want to get the logic level of e.g. pin GPIO16, gpio_num should be GPIO_NUM_16 (16);
 *
 * @return
 *     - 0 the GPIO input level is 0
 *     - 1 the GPIO input level is 1
 */
int gpio_get_level(gpio_num_t gpio_num);

/**
 * @brief  GPIO set direction
 *
 * Configure GPIO direction,such as output_only,input_only
 *
 * @param  gpio_num  Configure GPIO pins number, it should be GPIO number. If you want to set direction of e.g. GPIO16, gpio_num should be GPIO_NUM_16 (16);
 * @param  mode GPIO direction
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG GPIO error
 */
esp_err_t gpio_set_direction(gpio_num_t gpio_num, gpio_mode_t mode);

/**
 * @brief  Configure GPIO pull-up/pull-down resistors
 *
 * @param  gpio_num GPIO number. If you want to set pull up or down mode for e.g. GPIO16, gpio_num should be GPIO_NUM_16 (16);
 * @param  pull GPIO pull up/down mode.
 *
 * @note   The GPIO of esp8266 can not be pulled down except RTC GPIO which can not be pulled up.
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG : Parameter error
 */
esp_err_t gpio_set_pull_mode(gpio_num_t gpio_num, gpio_pull_mode_t pull);

/**
  * @brief Enable GPIO wake-up function.
  * 
  * @note RTC IO can not use the wakeup function
  *
  * @param gpio_num GPIO number.
  *
  * @param intr_type GPIO wake-up type. Only GPIO_INTR_LOW_LEVEL or GPIO_INTR_HIGH_LEVEL can be used.
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  */
esp_err_t gpio_wakeup_enable(gpio_num_t gpio_num, gpio_int_type_t intr_type);

/**
  * @brief Disable GPIO wake-up function.
  *
  * @note RTC IO can not use the wakeup function
  * 
  * @param gpio_num GPIO number
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  */
esp_err_t gpio_wakeup_disable(gpio_num_t gpio_num);

/**
 * @brief   Register GPIO interrupt handler, the handler is an ISR.
 *
 * This ISR function is called whenever any GPIO interrupt occurs. See
 * the alternative gpio_install_isr_service() and
 * gpio_isr_handler_add() API in order to have the driver support
 * per-GPIO ISRs.
 *
 * @param  fn  Interrupt handler function.
 * @param  no_use In order to be compatible with esp32, the parameter has no practical meaning and can be filled with 0.
 * @param  arg  Parameter for handler function
 * @param  handle_no_use Pointer to return handle. In order to be compatible with esp32,the parameter has no practical meaning and can be filled with NULL.
 *
 * @return
 *     - ESP_OK Success ;
 *     - ESP_ERR_INVALID_ARG GPIO error
 *     - ESP_ERR_NOT_FOUND No free interrupt found with the specified flags
 */
esp_err_t gpio_isr_register(void (*fn)(void *), void *arg, int no_use, gpio_isr_handle_t *handle_no_use);

/**
  * @brief Enable pull-up on GPIO.
  *
  * @param gpio_num GPIO number
  *
  * @note  The GPIO of esp8266 can not be pulled down except RTC GPIO which can not be pulled up.
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  */
esp_err_t gpio_pullup_en(gpio_num_t gpio_num);

/**
  * @brief Disable pull-up on GPIO.
  *
  * @param gpio_num GPIO number
  *
  * @note  The GPIO of esp8266 can not be pulled down except RTC GPIO which can not be pulled up.
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  */
esp_err_t gpio_pullup_dis(gpio_num_t gpio_num);

/**
  * @brief Enable pull-down on GPIO.
  *
  * @param gpio_num GPIO number
  *
  * @note  The GPIO of esp8266 can not be pulled down except RTC GPIO which can not be pulled up.
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  */
esp_err_t gpio_pulldown_en(gpio_num_t gpio_num);

/**
  * @brief Disable pull-down on GPIO.
  *
  * @param gpio_num GPIO number
  *
  * @note  The GPIO of esp8266 can not be pulled down except RTC GPIO which can not be pulled up.
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_ARG Parameter error
  */
esp_err_t gpio_pulldown_dis(gpio_num_t gpio_num);

/**
  * @brief Install the driver's GPIO ISR handler service, which allows per-pin GPIO interrupt handlers.
  *
  * This function is incompatible with gpio_isr_register() - if that function is used, a single global ISR is registered for all GPIO interrupts. If this function is used, the ISR service provides a global GPIO ISR and individual pin handlers are registered via the gpio_isr_handler_add() function.
  *
  * @param  no_use In order to be compatible with esp32, the parameter has no practical meaning and can be filled with 0.
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_NO_MEM No memory to install this service
  *     - ESP_ERR_INVALID_STATE ISR service already installed.
  *     - ESP_ERR_NOT_FOUND No free interrupt found with the specified flags
  *     - ESP_ERR_INVALID_ARG GPIO error
  */
esp_err_t gpio_install_isr_service(int no_use);

/**
  * @brief Uninstall the driver's GPIO ISR service, freeing related resources.
  */
void gpio_uninstall_isr_service();

/**
  * @brief Add ISR handler for the corresponding GPIO pin.
  *
  * Call this function after using gpio_install_isr_service() to
  * install the driver's GPIO ISR handler service.
  *
  * This ISR handler will be called from an ISR. So there is a stack
  * size limit (configurable as "ISR stack size" in menuconfig). This
  * limit is smaller compared to a global GPIO interrupt handler due
  * to the additional level of indirection.
  *
  * @param gpio_num GPIO number
  * @param isr_handler ISR handler function for the corresponding GPIO number.
  * @param args parameter for ISR handler.
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_STATE Wrong state, the ISR service has not been initialized.
  *     - ESP_ERR_INVALID_ARG Parameter error
  */
esp_err_t gpio_isr_handler_add(gpio_num_t gpio_num, gpio_isr_t isr_handler, void *args);

/**
  * @brief Remove ISR handler for the corresponding GPIO pin.
  *
  * @param gpio_num GPIO number
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_ERR_INVALID_STATE Wrong state, the ISR service has not been initialized.
  *     - ESP_ERR_INVALID_ARG Parameter error
  */
esp_err_t gpio_isr_handler_remove(gpio_num_t gpio_num);



#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_GPIO_H_ */
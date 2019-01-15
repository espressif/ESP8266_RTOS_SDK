// Copyright 2018-2019 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef _ROM_GPIO_H_
#define _ROM_GPIO_H_

#include <stdint.h>

#include "esp8266/pin_mux_register.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup gpio_apis, uart configuration and communication related apis
  * @brief gpio apis
  */

/** @addtogroup gpio_apis
  * @{
  */

#define GPIO_INPUT_GET(gpio_no)     ((gpio_input_get() >> (gpio_no)) & BIT0)

/**
  * @brief Change GPIO(0-15) pin output by setting, clearing, or disabling pins, GPIO0<->BIT(0).
  *         There is no particular ordering guaranteed; so if the order of writes is significant,
  *         calling code should divide a single call into multiple calls.
  *
  * @param  uint32_t set_mask : the gpios that need high level.
  *
  * @param  uint32_t clear_mask : the gpios that need low level.
  *
  * @param  uint32_t enable_mask : the gpios that need be changed.
  *
  * @param  uint32_t disable_mask : the gpios that need diable output.
  *
  * @return None
  */
void gpio_output_set(uint32_t set_mask, uint32_t clear_mask, uint32_t enable_mask, uint32_t disable_mask);

/**
  * @brief Sample the value of GPIO input pins(0-31) and returns a bitmask.
  *
  * @param None
  *
  * @return uint32_t : bitmask for GPIO input pins, BIT(0) for GPIO0.
  */
uint32_t gpio_input_get(void);

/**
  * @brief Select pad as a gpio function from IOMUX.
  *
  * @param uint32_t gpio_num : gpio number, 0~15
  *
  * @return None
  */
static inline void gpio_pad_select_gpio(uint32_t gpio_num)
{
    uint32_t gpio_mux_reg = PERIPHS_GPIO_MUX_REG(gpio_num);

    if (gpio_num == 0 || gpio_num == 2 || gpio_num == 4 || gpio_num == 5) {
        PIN_FUNC_SELECT(gpio_mux_reg, 0);
    } else {
        PIN_FUNC_SELECT(gpio_mux_reg, 3);
    }
}

/**
  * @brief Pull up the pad from gpio number.
  *
  * @param uint32_t gpio_num : gpio number, 0~15
  *
  * @return None
  */
static inline void gpio_pad_pullup(uint32_t gpio_num)
{
    uint32_t gpio_mux_reg = PERIPHS_GPIO_MUX_REG(gpio_num);

    PIN_PULLUP_EN(gpio_mux_reg);
}

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* _ROM_GPIO_H_ */

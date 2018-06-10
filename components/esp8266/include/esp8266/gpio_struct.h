/*
 * Copyright 2018 Espressif Systems (Shanghai) PTE LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef _GPIO_STRUCT_H_
#define _GPIO_STRUCT_H_

#include <stdint.h>
#include "esp8266/eagle_soc.h"

#ifdef __cplusplus
extern "C" {
#endif


/* ESP8266 GPIO Register Definitions */

typedef union {
    __RW__ struct {
        uint32_t sleep_oe:      1;
        uint32_t sleep_sel:     1;
        uint32_t reserved1:     1;
        uint32_t sleep_pullup:  1;
        uint32_t func_low_bit:  2;
        uint32_t reserved2:     1;
        uint32_t pullup:        1;
        uint32_t func_high_bit: 1;
    };
    __RW__ struct {
        uint32_t func_low_bit:  2;
        uint32_t reserved1:     1;
        uint32_t pulldown:      1;
        uint32_t reserved2:     1;
        uint32_t sleep_pulldown: 1;
        uint32_t func_high_bit: 1;
    } rtc_pin;
    __RW__ uint32_t val;
} gpio_pin_reg_t;

typedef struct {
    __RO__ uint32_t out;
    __WO__ uint32_t out_w1ts;
    __WO__ uint32_t out_w1tc;

    __RO__ uint32_t enable;
    __WO__ uint32_t enable_w1ts;
    __WO__ uint32_t enable_w1tc;

    __RO__ uint32_t in;

    __RO__ uint32_t status;
    __WO__ uint32_t status_w1ts;
    __WO__ uint32_t status_w1tc;

    __RW__ union {
        struct {
            uint32_t source:        1;
            uint32_t reserved1:     1;
            uint32_t driver:        1;
            uint32_t reserved2:     4;
            uint32_t int_type:      3;
            uint32_t wakeup_enable: 1;
            uint32_t reserved3:     21;
        };
        uint32_t val;
    } pin[16];

    __RW__ uint32_t sigma_delta;

    __RW__ uint32_t rtc_calib_sync;
    __RW__ uint32_t rtc_calib_value;
} gpio_struct_t;

extern volatile gpio_struct_t GPIO;

#ifdef __cplusplus
}
#endif  /* end of __cplusplus */

#endif  /* _GPIO_STRUCT_H_ */

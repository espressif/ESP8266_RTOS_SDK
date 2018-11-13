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

#pragma once

#include <stdint.h>
#include "esp8266/eagle_soc.h"

#ifdef __cplusplus
extern "C" {
#endif


/* ESP8266 FRC Register Definitions */

// FRC1 is a 24-bit countdown timer, triggers interrupt when reaches zero.
typedef struct {
    union {
        struct {
            uint32_t data:        23;
            uint32_t reserved23:   9;
        };
        uint32_t val;
    } load;

    union {
        struct {
            uint32_t data:        23;
            uint32_t reserved23:   9;
        };
        uint32_t val;
    } count;

    union {
        struct {
            uint32_t div:          6;
            uint32_t reload:       1;
            uint32_t en:           1;
            uint32_t intr_type:    1;
            uint32_t reserved24:  23;
        };
        uint32_t val;
    } ctrl;

    union {
        struct {
            uint32_t clr:          1;
            uint32_t reserved1:   31;
        };
        uint32_t val;
    } intr;
} frc1_struct_t;

extern volatile frc1_struct_t frc1;

#ifdef __cplusplus
}
#endif  /* end of __cplusplus */


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

/* ESP8266 I2S Register Definitions */

typedef volatile struct {
    uint32_t tx_fifo;
    uint32_t rx_fifo;
    union {
        struct {
            uint32_t tx_reset:       1;
            uint32_t rx_reset:       1;
            uint32_t tx_fifo_reset:  1;
            uint32_t rx_fifo_reset:  1;
            uint32_t tx_slave_mod:   1;
            uint32_t rx_slave_mod:   1;
            uint32_t right_first:    1;
            uint32_t msb_right:      1;
            uint32_t tx_start:       1;
            uint32_t rx_start:       1;
            uint32_t tx_msb_shift:   1;
            uint32_t rx_msb_shift:   1;
            uint32_t bits_mod:       4;
            uint32_t clkm_div_num:   6;
            uint32_t bck_div_num:    6;
            uint32_t reserved28:     4;
        };
        uint32_t val;
    } conf;
    union {
        struct {
            uint32_t rx_take_data:  1;
            uint32_t tx_put_data:   1;
            uint32_t rx_wfull:      1;
            uint32_t rx_rempty:     1;
            uint32_t tx_wfull:      1;
            uint32_t tx_rempty:     1;
            uint32_t reserved6:    26;
        };
        uint32_t val;
    } int_raw;
    union {
        struct {
            uint32_t rx_take_data:  1;
            uint32_t tx_put_data:   1;
            uint32_t rx_wfull:      1;
            uint32_t rx_rempty:     1;
            uint32_t tx_wfull:      1;
            uint32_t tx_rempty:     1;
            uint32_t reserved6:    26;
        };
        uint32_t val;
    } int_st;
    union {
        struct {
            uint32_t rx_take_data:  1;
            uint32_t tx_put_data:   1;
            uint32_t rx_wfull:      1;
            uint32_t rx_rempty:     1;
            uint32_t tx_wfull:      1;
            uint32_t tx_rempty:     1;
            uint32_t reserved6:    26;
        };
        uint32_t val;
    } int_ena;
    union {
        struct {
            uint32_t rx_take_data:  1;
            uint32_t tx_put_data:   1;
            uint32_t rx_wfull:      1;
            uint32_t rx_rempty:     1;
            uint32_t tx_wfull:      1;
            uint32_t tx_rempty:     1;
            uint32_t reserved6:    26;
        };
        uint32_t val;
    } int_clr;
    union {
        struct {
            uint32_t tx_bck_in_delay:   2;
            uint32_t tx_ws_in_delay:    2;
            uint32_t rx_bck_in_delay:   2;
            uint32_t rx_ws_in_delay:    2;
            uint32_t rx_sd_in_delay:    2;
            uint32_t tx_bck_out_delay:  2;
            uint32_t tx_ws_out_delay:   2;
            uint32_t tx_sd_out_delay:   2;
            uint32_t rx_ws_out_delay:   2;
            uint32_t rx_bck_out_delay:  2;
            uint32_t tx_dsync_sw:       1;
            uint32_t rx_dsync_sw:       1;
            uint32_t tx_bck_in_inv:     1;
            uint32_t reserved23:        9;
        };
        uint32_t val;
    } timing;
    union {
        struct {
            uint32_t rx_data_num:          6;
            uint32_t tx_data_num:          6;
            uint32_t dscr_en:              1;
            uint32_t tx_fifo_mod:          3;
            uint32_t rx_fifo_mod:          3;
            uint32_t reserved19:          13;
        };
        uint32_t val;
    } fifo_conf;
    uint32_t rx_eof_num;
    uint32_t conf_single_data;
    union {
        struct {
            uint32_t tx_chan_mod: 3;
            uint32_t rx_chan_mod: 2;
            uint32_t reserved5:  27;
        };
        uint32_t val;
    } conf_chan;
} i2s_struct_t;

extern volatile i2s_struct_t I2S0;

#ifdef __cplusplus
}
#endif  /* end of __cplusplus */


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


/* ESP8266 SLC Register Definitions */

typedef struct {
    union {
        struct {
            uint32_t tx_rst:            1;
            uint32_t rx_rst:            1;
            uint32_t ahbm_fifo_rst:     1;
            uint32_t ahbm_rst:          1;
            uint32_t tx_loop_test:      1;
            uint32_t rx_loop_test:      1;
            uint32_t rx_auto_wrback:    1;
            uint32_t rx_no_restart_clr: 1;
            uint32_t rxdscr_burst_en:   1;
            uint32_t rxdata_burst_en:   1;
            uint32_t rxlink_auto_ret:   1;
            uint32_t txlink_auto_ret:   1;
            uint32_t txdscr_burst_en:   1;
            uint32_t txdata_burst_en:   1;
            uint32_t reserved14:       18;
        };
        uint32_t val;
    } conf0;
    union {
        struct {
            uint32_t frhost_bit0:             1;
            uint32_t frhost_bit1:             1;
            uint32_t frhost_bit2:             1;
            uint32_t frhost_bit3:             1;
            uint32_t frhost_bit4:             1;
            uint32_t frhost_bit5:             1;
            uint32_t frhost_bit6:             1;
            uint32_t frhost_bit7:             1;
            uint32_t rx_start:                1;
            uint32_t tx_start:                1;
            uint32_t rx_udf:                  1;
            uint32_t tx_ovf:                  1;
            uint32_t token0_1to0:             1;
            uint32_t token1_1to0:             1;
            uint32_t tx_done:                 1;
            uint32_t tx_suc_eof:              1;
            uint32_t rx_done:                 1;
            uint32_t rx_eof:                  1;
            uint32_t tohost:                  1;
            uint32_t tx_dscr_err:             1;
            uint32_t rx_dscr_err:             1;
            uint32_t tx_dscr_empty:           1;
            uint32_t reserved22:             10;
        };
        uint32_t val;
    } int_raw;
    union {
        struct {
            uint32_t frhost_bit0:            1;
            uint32_t frhost_bit1:            1;
            uint32_t frhost_bit2:            1;
            uint32_t frhost_bit3:            1;
            uint32_t frhost_bit4:            1;
            uint32_t frhost_bit5:            1;
            uint32_t frhost_bit6:            1;
            uint32_t frhost_bit7:            1;
            uint32_t rx_start:               1;
            uint32_t tx_start:               1;
            uint32_t rx_udf:                 1;
            uint32_t tx_ovf:                 1;
            uint32_t token0_1to0:            1;
            uint32_t token1_1to0:            1;
            uint32_t tx_done:                1;
            uint32_t tx_suc_eof:             1;
            uint32_t rx_done:                1;
            uint32_t rx_eof:                 1;
            uint32_t tohost:                 1;
            uint32_t tx_dscr_err:            1;
            uint32_t rx_dscr_err:            1;
            uint32_t tx_dscr_empty:          1;
            uint32_t reserved22:            10;
        };
        uint32_t val;
    } int_st;
    union {
        struct {
            uint32_t frhost_bit0:             1;
            uint32_t frhost_bit1:             1;
            uint32_t frhost_bit2:             1;
            uint32_t frhost_bit3:             1;
            uint32_t frhost_bit4:             1;
            uint32_t frhost_bit5:             1;
            uint32_t frhost_bit6:             1;
            uint32_t frhost_bit7:             1;
            uint32_t rx_start:                1;
            uint32_t tx_start:                1;
            uint32_t rx_udf:                  1;
            uint32_t tx_ovf:                  1;
            uint32_t token0_1to0:             1;
            uint32_t token1_1to0:             1;
            uint32_t tx_done:                 1;
            uint32_t tx_suc_eof:              1;
            uint32_t rx_done:                 1;
            uint32_t rx_eof:                  1;
            uint32_t tohost:                  1;
            uint32_t tx_dscr_err:             1;
            uint32_t rx_dscr_err:             1;
            uint32_t tx_dscr_empty:           1;
            uint32_t reserved22:             10;
        };
        uint32_t val;
    } int_ena;
    union {
        struct {
            uint32_t frhost_bit0:             1;
            uint32_t frhost_bit1:             1;
            uint32_t frhost_bit2:             1;
            uint32_t frhost_bit3:             1;
            uint32_t frhost_bit4:             1;
            uint32_t frhost_bit5:             1;
            uint32_t frhost_bit6:             1;
            uint32_t frhost_bit7:             1;
            uint32_t rx_start:                1;
            uint32_t tx_start:                1;
            uint32_t rx_udf:                  1;
            uint32_t tx_ovf:                  1;
            uint32_t token0_1to0:             1;
            uint32_t token1_1to0:             1;
            uint32_t tx_done:                 1;
            uint32_t tx_suc_eof:              1;
            uint32_t rx_done:                 1;
            uint32_t rx_eof:                  1;
            uint32_t tohost:                  1;
            uint32_t tx_dscr_err:             1;
            uint32_t rx_dscr_err:             1;
            uint32_t tx_dscr_empty:           1;
            uint32_t reserved22:             10;
        };
        uint32_t val;
    } int_clr;
    union {
        struct {
            uint32_t rx_full:     1;
            uint32_t rx_empty:    1;
            uint32_t reserved2:  30;
        };
        uint32_t val;
    } rx_status;
    union {
        struct {
            uint32_t rxfifo_wdata:   9;
            uint32_t reserved9:      7;
            uint32_t rxfifo_push:    1;
            uint32_t reserved17:    15;
        };
        uint32_t val;
    } rxfifo_push;
    union {
        struct {
            uint32_t tx_full:    1;
            uint32_t tx_empty:   1;
            uint32_t reserved2:  30;
        };
        uint32_t val;
    } tx_status;
    union {
        struct {
            uint32_t txfifo_rdata:  11;
            uint32_t reserved11:     5;
            uint32_t txfifo_pop:     1;
            uint32_t reserved17:    15;
        };
        uint32_t val;
    } txfifo_pop;
    union {
        struct {
            uint32_t addr:            20;
            uint32_t reserved20:       8;
            uint32_t stop:             1;
            uint32_t start:            1;
            uint32_t restart:          1;
            uint32_t park:             1;
        };
        uint32_t val;
    } rx_link;
    union {
        struct {
            uint32_t addr:            20;
            uint32_t reserved20:       8;
            uint32_t stop:             1;
            uint32_t start:            1;
            uint32_t restart:          1;
            uint32_t park:             1;
        };
        uint32_t val;
    } tx_link;
    union {
        struct {
            uint32_t intvec:      8;
            uint32_t reserved8:  24;
        };
        uint32_t val;
    } intvec_tohost;
    union {
        struct {
            uint32_t wdata:            12;
            uint32_t wr:                1;
            uint32_t inc:               1;
            uint32_t inc_more:          1;
            uint32_t reserved15:        1;
            uint32_t token0:           12;
            uint32_t reserved28:        4;
        };
        uint32_t val;
    } token0;
    union {
        struct {
            uint32_t wdata:            12;
            uint32_t wr:                1;
            uint32_t inc:               1;
            uint32_t inc_more:          1;
            uint32_t reserved15:        1;
            uint32_t token1:           12;
            uint32_t reserved28:        4;
        };
        uint32_t val;
    } token1;
    uint32_t conf1;
    uint32_t state0;
    uint32_t state1;
    union {
        struct {
            uint32_t txeof_ena:           6;
            uint32_t reserved6:           2;
            uint32_t fifo_map_ena:        4;
            uint32_t tx_dummy_mode:       1;
            uint32_t reserved13:          3;
            uint32_t tx_push_idle_num:   16;
        };
        uint32_t val;
    } bridge_conf;
    uint32_t rx_eof_des_addr;
    uint32_t tx_eof_des_addr;
    uint32_t to_eof_bfr_des_addr;
    union {
        struct {
            uint32_t mode:         3;
            uint32_t reserved3:    1;
            uint32_t addr:         2;
            uint32_t reserved6:   26;
        };
        uint32_t val;
    } ahb_test;
    union {
        struct {
            uint32_t cmd_st:          3;
            uint32_t reserved3:       1;
            uint32_t func_st:         4;
            uint32_t sdio_wakeup:     1;
            uint32_t reserved9:       3;
            uint32_t bus_st:          3;
            uint32_t reserved15:     17;
        };
        uint32_t val;
    } sdio_st;
    union {
        struct {
            uint32_t pop_idle_cnt:     16;
            uint32_t token_no_replace:  1;
            uint32_t infor_no_replace:  1;
            uint32_t rx_fill_mode:      1;
            uint32_t rx_eof_mode:       1;
            uint32_t rx_fill_en:        1;
            uint32_t reserved21:       11;
        };
        uint32_t val;
    } rx_dscr_conf;
    uint32_t txlink_dscr;
    uint32_t txlink_dscr_bf0;
    uint32_t txlink_dscr_bf1;
    uint32_t rxlink_dscr;
    uint32_t rxlink_dscr_bf0;
    uint32_t rxlink_dscr_bf1;
    uint32_t date;
    uint32_t id;
    uint32_t reserved_80[2];
    union {
        struct {
            uint32_t reserved0:      23;
            uint32_t intr_ena:        1;
            uint32_t reserved24:      8;
        };
        uint32_t val;
    } host_intr_raw;
    uint32_t reserved_8C[2];
    uint32_t host_conf_w0;
    uint32_t host_conf_w1;
    uint32_t host_intr_status;
    uint32_t host_conf_w2;
    uint32_t host_conf_w3;
    uint32_t host_conf_w4;
    uint32_t reserved_AC[1];
    union {
        struct {
            uint32_t reserved0:       12;
            uint32_t sof_bit:          1;
            uint32_t reserved13:      19;
        };
        uint32_t val;
    } host_intr_clr;
    union {
        struct {
            uint32_t tohost_bit0:     1;
            uint32_t reserved1:      22;
            uint32_t rx_new_packet:   1;
            uint32_t reserved24:      8;
        };
        uint32_t val;
    } host_intr_ena;
    uint32_t reserved_BC[1];
    uint32_t host_conf_w5;
} slc_struct_t;

extern volatile slc_struct_t SLC0;

#ifdef __cplusplus
}
#endif  /* end of __cplusplus */


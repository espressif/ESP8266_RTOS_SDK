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

#ifndef _UART_STRUCT_H_
#define _UART_STRUCT_H_

#include <stdint.h>
#include "esp8266/eagle_soc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ESP8266 UART Register Definitions */

typedef volatile struct {
    union {
        struct {
            uint8_t rw_byte;                 /*This register stores one byte data  read by rx fifo.*/
            uint8_t reserved[3];
        };
        __RO__ uint32_t val;
    } fifo;
    union {
        struct {
            uint32_t rxfifo_full:      1;           /*This interrupt raw bit turns to high level when receiver receives more data than (rx_flow_thrhd_h3 rx_flow_thrhd).*/
            uint32_t txfifo_empty:     1;           /*This interrupt raw bit turns to high level when the amount of data in transmitter's fifo is less than ((tx_mem_cnttxfifo_cnt) .*/
            uint32_t parity_err:       1;           /*This interrupt raw bit turns to high level when receiver detects the parity error of data.*/
            uint32_t frm_err:          1;           /*This interrupt raw bit turns to high level when receiver detects data's frame error .*/
            uint32_t rxfifo_ovf:       1;           /*This interrupt raw bit turns to high level when receiver receives more data than the fifo can store.*/
            uint32_t dsr_chg:          1;           /*This interrupt raw bit turns to high level when receiver detects the edge change of dsrn signal.*/
            uint32_t cts_chg:          1;           /*This interrupt raw bit turns to high level when receiver detects the edge change of ctsn signal.*/
            uint32_t brk_det:          1;           /*This interrupt raw bit turns to high level when receiver detects the 0 after the stop bit.*/
            uint32_t rxfifo_tout:      1;           /*This interrupt raw bit turns to high level when receiver takes more time than rx_tout_thrhd to receive a byte.*/
            uint32_t reserved9:       23;
        };
        __RO__ uint32_t val;
    } int_raw;
    union {
        struct {
            uint32_t rxfifo_full:      1;            /*This is the status bit for rxfifo_full_int_raw when rxfifo_full_int_ena is set to 1.*/
            uint32_t txfifo_empty:     1;            /*This is the status bit for  txfifo_empty_int_raw  when txfifo_empty_int_ena is set to 1.*/
            uint32_t parity_err:       1;            /*This is the status bit for parity_err_int_raw when parity_err_int_ena is set to 1.*/
            uint32_t frm_err:          1;            /*This is the status bit for frm_err_int_raw when fm_err_int_ena is set to 1.*/
            uint32_t rxfifo_ovf:       1;            /*This is the status bit for rxfifo_ovf_int_raw when rxfifo_ovf_int_ena is set to 1.*/
            uint32_t dsr_chg:          1;            /*This is the status bit for dsr_chg_int_raw when dsr_chg_int_ena is set to 1.*/
            uint32_t cts_chg:          1;            /*This is the status bit for cts_chg_int_raw when cts_chg_int_ena is set to 1.*/
            uint32_t brk_det:          1;            /*This is the status bit for brk_det_int_raw when brk_det_int_ena is set to 1.*/
            uint32_t rxfifo_tout:      1;            /*This is the status bit for rxfifo_tout_int_raw when rxfifo_tout_int_ena is set to 1.*/
            uint32_t reserved9:       23;
        };
        __RO__ uint32_t val;
    } int_st;
    union {
        struct {
            uint32_t rxfifo_full:      1;           /*This is the enable bit for rxfifo_full_int_st register.*/
            uint32_t txfifo_empty:     1;           /*This is the enable bit for rxfifo_full_int_st register.*/
            uint32_t parity_err:       1;           /*This is the enable bit for parity_err_int_st register.*/
            uint32_t frm_err:          1;           /*This is the enable bit for frm_err_int_st register.*/
            uint32_t rxfifo_ovf:       1;           /*This is the enable bit for rxfifo_ovf_int_st register.*/
            uint32_t dsr_chg:          1;           /*This is the enable bit for dsr_chg_int_st register.*/
            uint32_t cts_chg:          1;           /*This is the enable bit for cts_chg_int_st register.*/
            uint32_t brk_det:          1;           /*This is the enable bit for brk_det_int_st register.*/
            uint32_t rxfifo_tout:      1;           /*This is the enable bit for rxfifo_tout_int_st register.*/
            uint32_t reserved9:       23;
        };
        __RW__ uint32_t val;
    } int_ena;
    union {
        struct {
            uint32_t rxfifo_full:      1;           /*Set this bit to clear the rxfifo_full_int_raw interrupt.*/
            uint32_t txfifo_empty:     1;           /*Set this bit to clear txfifo_empty_int_raw interrupt.*/
            uint32_t parity_err:       1;           /*Set this bit to clear parity_err_int_raw interrupt.*/
            uint32_t frm_err:          1;           /*Set this bit to clear frm_err_int_raw interrupt.*/
            uint32_t rxfifo_ovf:       1;           /*Set this bit to clear rxfifo_ovf_int_raw interrupt.*/
            uint32_t dsr_chg:          1;           /*Set this bit to clear the dsr_chg_int_raw interrupt.*/
            uint32_t cts_chg:          1;           /*Set this bit to clear the cts_chg_int_raw interrupt.*/
            uint32_t brk_det:          1;           /*Set this bit to clear the brk_det_int_raw interrupt.*/
            uint32_t rxfifo_tout:      1;           /*Set this bit to clear the rxfifo_tout_int_raw interrupt.*/
            uint32_t reserved9:       23;
        };
        __WO__ uint32_t val;
    } int_clr;
    union {
        struct {
            uint32_t div_int:    20;                /*The register value is  the  integer part of the frequency divider's factor.*/
            uint32_t reserved20: 12;
        };
        __RW__ uint32_t val;
    } clk_div;
    union {
        struct {
            uint32_t en: 1;                         /*This is the enable bit for detecting baudrate.*/
            uint32_t reserved1:   7;
            uint32_t glitch_filt: 8;                /*when input pulse width is lower then this value ignore this pulse.this register is used in auto-baud detect process.*/
            uint32_t reserved16: 16;
        };
        __RW__ uint32_t val;
    } auto_baud;
    union {
        struct {
            uint32_t rxfifo_cnt: 8;                 /*(rx_mem_cnt rxfifo_cnt) stores the byte number of valid data in receiver's fifo. rx_mem_cnt register stores the 3 most significant bits  rxfifo_cnt stores the 8 least significant bits.*/
            uint32_t reserved8:  5;
            uint32_t dsrn:       1;                 /*This register stores the level value of the internal uart dsr signal.*/
            uint32_t ctsn:       1;                 /*This register stores the level value of the internal uart cts signal.*/
            uint32_t rxd:        1;                 /*This register stores the level value of the internal uart rxd signal.*/
            uint32_t txfifo_cnt: 8;                 /*(tx_mem_cnt txfifo_cnt) stores the byte number of valid data in transmitter's fifo.tx_mem_cnt stores the 3 most significant bits  txfifo_cnt stores the 8 least significant bits.*/
            uint32_t reserved24: 5;
            uint32_t dtrn:       1;                 /*The register represent the level value of the internal uart dsr signal.*/
            uint32_t rtsn:       1;                 /*This register represent the level value of the internal uart cts signal.*/
            uint32_t txd:        1;                 /*This register represent the  level value of the internal uart rxd signal.*/
        };
        __RO__ uint32_t val;
    } status;
    union {
        struct {
            uint32_t parity:             1;         /*This register is used to configure the parity check mode.  0:even 1:odd*/
            uint32_t parity_en:          1;         /*Set this bit to enable uart parity check.*/
            uint32_t bit_num:            2;         /*This register is used to set the length of data:  0:5bits 1:6bits 2:7bits 3:8bits*/
            uint32_t stop_bit_num:       2;         /*This register is used to set the length of  stop bit. 1:1bit  2:1.5bits  3:2bits*/
            uint32_t sw_rts:             1;         /*This register is used to configure the software rts signal which is used in software flow control.*/
            uint32_t sw_dtr:             1;         /*This register is used to configure the software dtr signal which is used in software flow control..*/
            uint32_t txd_brk:            1;         /*Set this bit to enable transmitter to  send 0 when the process of sending data is done.*/
            uint32_t irda_dplx:          1;         /*Set this bit to enable irda loop-back mode.*/
            uint32_t irda_tx_en:         1;         /*This is the start enable bit for irda transmitter.*/
            uint32_t irda_wctl:          1;         /*1：the irda transmitter's 11th bit is the same to the 10th bit. 0：set irda transmitter's 11th bit to 0.*/
            uint32_t irda_tx_inv:        1;         /*Set this bit to inverse the level value of  irda transmitter's level.*/
            uint32_t irda_rx_inv:        1;         /*Set this bit to inverse the level value of irda receiver's level.*/
            uint32_t loopback:           1;         /*Set this bit to enable uart loop-back test mode.*/
            uint32_t tx_flow_en:         1;         /*Set this bit to enable transmitter's flow control function.*/
            uint32_t irda_en:            1;         /*Set this bit to enable irda protocol.*/
            uint32_t rxfifo_rst:         1;         /*Set this bit to reset uart receiver's fifo.*/
            uint32_t txfifo_rst:         1;         /*Set this bit to reset uart transmitter's fifo.*/
            uint32_t rxd_inv:            1;         /*Set this bit to inverse the level value of uart rxd signal.*/
            uint32_t cts_inv:            1;         /*Set this bit to inverse the level value of uart cts signal.*/
            uint32_t dsr_inv:            1;         /*Set this bit to inverse the level value of uart dsr signal.*/
            uint32_t txd_inv:            1;         /*Set this bit to inverse the level value of uart txd signal.*/
            uint32_t rts_inv:            1;         /*Set this bit to inverse the level value of uart rts signal.*/
            uint32_t dtr_inv:            1;         /*Set this bit to inverse the level value of uart dtr signal.*/
            uint32_t reserved25:         7;
        };
        __RW__ uint32_t val;
    } conf0;
    union {
        struct {
            uint32_t rxfifo_full_thrhd:  7;         /*When receiver receives more data than its threshold value，receiver will produce rxfifo_full_int_raw interrupt.the threshold value is (rx_flow_thrhd_h3 rxfifo_full_thrhd).*/
            uint32_t reserved7:           1;
            uint32_t txfifo_empty_thrhd: 7;         /*when the data amount in transmitter fifo is less than its threshold value， it will produce txfifo_empty_int_raw interrupt. the threshold value is (tx_mem_empty_thresh txfifo_empty_thrhd)*/
            uint32_t reserved15:          1;
            uint32_t rx_flow_thrhd:      7;         /*when receiver receives more data than its threshold value， receiver produce signal to tell the transmitter stop transferring data. the threshold value is (rx_flow_thrhd_h3 rx_flow_thrhd).*/
            uint32_t rx_flow_en:          1;         /*This is the flow enable bit for uart receiver. 1:choose software flow control with configuring sw_rts signal*/
            uint32_t rx_tout_thrhd:      7;         /*This register is used to configure the timeout value for uart receiver receiving a byte.*/
            uint32_t rx_tout_en:          1;         /*This is the enable bit for uart receiver's timeout function.*/
        };
        __RW__ uint32_t val;
    } conf1;
    union {
        struct {
            uint32_t min_cnt:     20;               /*This register stores the value of the minimum duration time for the low level pulse， it is used in baudrate-detect process.*/
            uint32_t reserved20:  12;
        };
        __RO__ uint32_t val;
    } lowpulse;
    union {
        struct {
            uint32_t min_cnt:     20;               /*This register stores  the value of the maximum duration time for the high level pulse， it is used in baudrate-detect process.*/
            uint32_t reserved20:  12;
        };
        __RO__ uint32_t val;
    } highpulse;
    union {
        struct {
            uint32_t edge_cnt:    10;               /*This register stores the count of rxd edge change， it is used in baudrate-detect process.*/
            uint32_t reserved10:  22;
        };
        __RO__ uint32_t val;
    } rxd_cnt;
    uint32_t  reserved[18];
    __RW__ uint32_t date;                           /**/
    __RW__ uint32_t id;                             /**/
} uart_dev_t;

extern uart_dev_t uart0;
extern uart_dev_t uart1;

#ifdef __cplusplus
}
#endif  /* end of __cplusplus */

#endif  /* _UART_STRUCT_H_ */

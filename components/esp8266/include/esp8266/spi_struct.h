// Copyright 2010-2018 Espressif Systems (Shanghai) PTE LTD
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

#pragma once

#include <stdint.h>
#include "esp8266/eagle_soc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef volatile struct {
    union {
        struct {
            uint32_t reserved0: 18;                         /*reserved*/
            uint32_t usr:        1;                         /*User define command enable.  An operation will be triggered when the bit is set. The bit will be cleared once the operation done.1: enable 0: disable.*/
            uint32_t flash_hpm:  1;                         /*Drive Flash into high performance mode.  The bit will be cleared once the operation done.1: enable 0: disable.*/
            uint32_t flash_res:  1;                         /*This bit combined with reg_resandres bit releases Flash from the power-down state or high performance mode and obtains the devices ID. The bit will be cleared once the operation done.1: enable 0: disable.*/
            uint32_t flash_dp:   1;                         /*Drive Flash into power down.  An operation will be triggered when the bit is set. The bit will be cleared once the operation done.1: enable 0: disable.*/
            uint32_t flash_ce:   1;                         /*Chip erase enable. Chip erase operation will be triggered when the bit is set. The bit will be cleared once the operation done.1: enable 0: disable.*/
            uint32_t flash_be:   1;                         /*Block erase enable(32KB) .  Block erase operation will be triggered when the bit is set. The bit will be cleared once the operation done.1: enable 0: disable.*/
            uint32_t flash_se:   1;                         /*Sector erase enable(4KB). Sector erase operation will be triggered when the bit is set. The bit will be cleared once the operation done.1: enable 0: disable.*/
            uint32_t flash_pp:   1;                         /*Page program enable(1 byte ~256 bytes data to be programmed). Page program operation  will be triggered when the bit is set. The bit will be cleared once the operation done .1: enable 0: disable.*/
            uint32_t flash_wrsr: 1;                         /*Write status register enable.   Write status operation  will be triggered when the bit is set. The bit will be cleared once the operation done.1: enable 0: disable.*/
            uint32_t flash_rdsr: 1;                         /*Read status register-1.  Read status operation will be triggered when the bit is set. The bit will be cleared once the operation done.1: enable 0: disable.*/
            uint32_t flash_rdid: 1;                         /*Read JEDEC ID . Read ID command will be sent when the bit is set. The bit will be cleared once the operation done. 1: enable 0: disable.*/
            uint32_t flash_wrdi: 1;                         /*Write flash disable. Write disable command will be sent when the bit is set. The bit will be cleared once the operation done. 1: enable 0: disable.*/
            uint32_t flash_wren: 1;                         /*Write flash enable.  Write enable command will be sent when the bit is set. The bit will be cleared once the operation done. 1: enable 0: disable.*/
            uint32_t flash_read: 1;                         /*Read flash enable. Read flash operation will be triggered when the bit is set. The bit will be cleared once the operation done. 1: enable 0: disable.*/
        };
        uint32_t val;
    } cmd;
    uint32_t addr;                                          /*addr to slave / from master. SPI transfer from the MSB to the LSB. If length > 32 bits, then address continues from MSB of wr_status.*/
    union {
        struct {
            uint32_t clkcnt_l:           4;
            uint32_t clkcnt_h:           4;
            uint32_t clkcnt_n:           4;
            uint32_t clk_equ_sysclk:     1;
            uint32_t fastrd_mode:        1;                 /*This bit enable the bits: spi_fread_qio  spi_fread_dio  spi_fread_qout and spi_fread_dout. 1: enable 0: disable.*/
            uint32_t fread_dual:         1;                 /*In the read operations  read-data phase apply 2 signals. 1: enable 0: disable.*/
            uint32_t reserved15:         5;                 /*reserved*/
            uint32_t fread_quad:         1;                 /*In the read operations read-data phase apply 4 signals. 1: enable 0: disable.*/
            uint32_t reserved21:         2;                 /*reserved*/
            uint32_t fread_dio:          1;                 /*In the read operations address phase and read-data phase apply 2 signals. 1: enable 0: disable.*/
            uint32_t fread_qio:          1;                 /*In the read operations address phase and read-data phase apply 4 signals. 1: enable 0: disable.*/
            uint32_t rd_bit_order:       1;                 /*In read-data (MISO) phase 1: LSB first 0: MSB first*/
            uint32_t wr_bit_order:       1;                 /*In command address write-data (MOSI) phases 1: LSB firs 0: MSB first*/
            uint32_t reserved27:         5;                 /*reserved*/
        };
        uint32_t val;
    } ctrl;
    union {
        struct {
            uint32_t reserved0:         16;                 /*reserved*/
            uint32_t cs_hold_delay_res: 12;                 /*Delay cycles of resume Flash when resume Flash is enable by spi clock.*/
            uint32_t cs_hold_delay:      4;                 /*SPI cs signal is delayed by spi clock cycles*/
        };
        uint32_t val;
    } ctrl1;
    union {
        struct {
            uint32_t status:    16;                         /*In the slave mode, it is the status for master to read out.*/
            uint32_t wb_mode:    8;                         /*Mode bits in the flash fast read mode, it is combined with spi_fastrd_mode bit.*/
            uint32_t status_ext: 8;                         /*In the slave mode,it is the status for master to read out.*/
        };
        uint32_t val;
    } rd_status;                                     /*In the slave mode, this register are the status register for the master to read out.*/
    union {
        struct {
            uint32_t reserved0:       16;                   /*reserved*/
            uint32_t miso_delay_mode:  2;                   /*MISO signals are delayed by spi_clk. 0: zero  1: if spi_ck_out_edge or spi_ck_i_edge is set 1  delayed by half cycle    else delayed by one cycle  2: if spi_ck_out_edge or spi_ck_i_edge is set 1  delayed by one cycle  else delayed by half cycle  3: delayed one cycle*/
            uint32_t miso_delay_num:   3;                   /*MISO signals are delayed by system clock cycles*/
            uint32_t mosi_delay_mode:  2;                   /*MOSI signals are delayed by spi_clk. 0: zero  1: if spi_ck_out_edge or spi_ck_i_edge is set 1  delayed by half cycle    else delayed by one cycle  2: if spi_ck_out_edge or spi_ck_i_edge is set 1  delayed by one cycle  else delayed by half cycle  3: delayed one cycle*/
            uint32_t mosi_delay_num:   3;                   /*MOSI signals are delayed by system clock cycles*/
            uint32_t cs_delay_mode:    2;                   /*spi_cs signal is delayed by spi_clk . 0: zero  1: if spi_ck_out_edge or spi_ck_i_edge is set 1  delayed by half cycle    else delayed by one cycle  2: if spi_ck_out_edge or spi_ck_i_edge is set 1  delayed by one cycle   else delayed by half cycle  3: delayed one cycle*/
            uint32_t cs_delay_num:     4;                   /*spi_cs signal is delayed by system clock cycles*/
        };
        uint32_t val;
    } ctrl2;
    union {
        struct {
            uint32_t clkcnt_l:       6;                     /*In the master mode it must be equal to spi_clkcnt_n. In the slave mode it must be 0.*/
            uint32_t clkcnt_h:       6;                     /*In the master mode it must be floor((spi_clkcnt_n+1)/2-1). In the slave mode it must be 0.*/
            uint32_t clkcnt_n:       6;                     /*In the master mode it is the divider of spi_clk. So spi_clk frequency is system/(spi_clkdiv_pre+1)/(spi_clkcnt_n+1)*/
            uint32_t clkdiv_pre:    13;                     /*In the master mode it is pre-divider of spi_clk.*/
            uint32_t clk_equ_sysclk: 1;                     /*In the master mode 1: spi_clk is eqaul to system 0: spi_clk is divided from system clock.*/
        };
        uint32_t val;
    } clock;
    union {
        struct {
            uint32_t duplex:            1;                  
            uint32_t reserved1:         1;                  /*reserved*/
            uint32_t flash_mode:        1;
            uint32_t reserved3:         1;                  /*reserved*/
            uint32_t cs_hold:           1;                  /*spi cs keep low when spi is in done phase. 1: enable 0: disable.*/
            uint32_t cs_setup:          1;                  /*spi cs is enable when spi is in prepare phase. 1: enable 0: disable.*/
            uint32_t ck_i_edge:         1;                  /*In the slave mode  the bit is same as spi_ck_out_edge in master mode. It is combined with  spi_miso_delay_mode bits.*/
            uint32_t ck_out_edge:       1;                  /*the bit combined with spi_mosi_delay_mode bits to set mosi signal delay mode.*/
            uint32_t reserved8:         2;                  /*reserved*/
            uint32_t rd_byte_order:     1;                  /*In read-data (MISO) phase 1: big-endian 0: little_endian*/
            uint32_t wr_byte_order:     1;                  /*In command address write-data (MOSI) phases 1: big-endian 0: litte_endian*/
            uint32_t fwrite_dual:       1;                  /*In the write operations write-data phase apply 2 signals*/
            uint32_t fwrite_quad:       1;                  /*In the write operations write-data phase apply 4 signals*/
            uint32_t fwrite_dio:        1;                  /*In the write operations address phase and write-data phase apply 2 signals.*/
            uint32_t fwrite_qio:        1;                  /*In the write operations address phase and write-data phase apply 4 signals.*/
            uint32_t sio:               1;                  /*Set the bit to enable 3-line half duplex communication  mosi and miso signals share the same pin. 1: enable 0: disable.*/
            uint32_t reserved17:        7;                  /*reserved*/
            uint32_t usr_miso_highpart: 1;                  /*read-data phase only access to high-part of the buffer spi_w8~spi_w15. 1: enable 0: disable.*/
            uint32_t usr_mosi_highpart: 1;                  /*write-data phase only access to high-part of the buffer spi_w8~spi_w15. 1: enable 0: disable.*/
            uint32_t reserved26:        1;                  /*reserved*/
            uint32_t usr_mosi:          1;                  /*This bit enable the write-data phase of an operation.*/
            uint32_t usr_miso:          1;                  /*This bit enable the read-data phase of an operation.*/
            uint32_t usr_dummy:         1;                  /*This bit enable the dummy phase of an operation.*/
            uint32_t usr_addr:          1;                  /*This bit enable the address phase of an operation.*/
            uint32_t usr_command:       1;                  /*This bit enable the command phase of an operation.*/
        };
        uint32_t val;
    } user;
    union {
        struct {
            uint32_t usr_dummy_cyclelen: 8;                 /*The length in spi_clk cycles of dummy phase. The register value shall be (cycle_num-1).*/
            uint32_t usr_miso_bitlen:    9;                 /*The length in bits of  read-data. The register value shall be (bit_num-1).*/
            uint32_t usr_mosi_bitlen:    9;                 /*The length in bits of write-data. The register value shall be (bit_num-1).*/
            uint32_t usr_addr_bitlen:    6;                 /*The length in bits of address phase. The register value shall be (bit_num-1).*/
        };
        uint32_t val;
    } user1;
    union {
        struct {
            uint32_t usr_command_value: 16;                 /*The value of  command. Output sequence: bit 7-0 and then 15-8.*/
            uint32_t reserved16:        12;                 /*reserved*/
            uint32_t usr_command_bitlen: 4;                 /*The length in bits of command phase. The register value shall be (bit_num-1)*/
        };
        uint32_t val;
    } user2;
    uint32_t wr_status;                                 /*In the slave mode this register are the status register for the master to write into. In the master mode this register are the higher 32bits in the 64 bits address condition.*/
    union {
        struct {
            uint32_t cs0_dis:        1;                     /*SPI CS0 pin enable, 1: disable CS0, 0: spi_cs0 signal is from/to CS0 pin*/
            uint32_t cs1_dis:        1;                     /*SPI CS1 pin enable, 1: disable CS1, 0: spi_cs1 signal is from/to CS1 pin*/
            uint32_t cs2_dis:        1;                     /*SPI CS2 pin enable, 1: disable CS2, 0: spi_cs2 signal is from/to CS2 pin*/
            uint32_t reserved3:     16;                     /*reserved*/
            uint32_t slave_mode:     1;                     /*1: Both CLK and CS are input, 0: Both CLK and CS are output*/
            uint32_t reserved20:     9;                     /*reserved*/
            uint32_t ck_idle_edge:   1;                     /*1: spi clk line is high when idle     0: spi clk line is low when idle*/
            uint32_t reserved30:     2;                     /*reserved*/
        };
        uint32_t val;
    } pin;
    union {
        struct {
            uint32_t rd_buf_done:  1;                       /*The interrupt raw bit for the completion of read-buffer operation in the slave mode.*/
            uint32_t wr_buf_done:  1;                       /*The interrupt raw bit for the completion of write-buffer operation in the slave mode.*/
            uint32_t rd_sta_done:  1;                       /*The interrupt raw bit for the completion of read-status operation in the slave mode.*/
            uint32_t wr_sta_done:  1;                       /*The interrupt raw bit for the completion of write-status operation in the slave mode.*/
            uint32_t trans_done:   1;                       /*The interrupt raw bit for the completion of any operation in both the master mode and the slave mode.*/
            uint32_t rd_buf_inten: 1;                       /*The interrupt enable bit for the completion of read-buffer operation in the slave mode.*/
            uint32_t wr_buf_inten: 1;                       /*The interrupt enable bit for the completion of write-buffer operation in the slave mode.*/
            uint32_t rd_sta_inten: 1;                       /*The interrupt enable bit for the completion of read-status operation in the slave mode.*/
            uint32_t wr_sta_inten: 1;                       /*The interrupt enable bit for the completion of write-status operation in the slave mode.*/
            uint32_t trans_inten:  1;                       /*The interrupt enable bit for the completion of any operation in both the master mode and the slave mode.*/
            uint32_t reserved10:  13;                       /*reserved*/
            uint32_t trans_cnt:    4;                       /*The operations counter in both the master mode and the slave mode.*/
            uint32_t cmd_define:   1;                       /*1: slave mode commands are defined in SPI_SLAVE3.  0: slave mode commands are fixed as: 1: write-status 2: write-buffer and 3: read-buffer 4: read-status*/
            uint32_t wr_rd_sta_en: 1;                       /*write and read status enable  in the slave mode*/
            uint32_t wr_rd_buf_en: 1;                       /*write and read buffer enable in the slave mode*/
            uint32_t slave_mode:   1;                       /*1: slave mode 0: master mode.*/
            uint32_t sync_reset:   1;                       /*Software reset enable, reset the spi clock line cs line and data lines.*/
        };
        uint32_t val;
    } slave;
    union {
        struct {
            uint32_t rdbuf_dummy_en:  1;                    /*In the slave mode it is the enable bit of dummy phase for read-buffer operations.*/
            uint32_t wrbuf_dummy_en:  1;                    /*In the slave mode it is the enable bit of dummy phase for write-buffer operations.*/
            uint32_t rdsta_dummy_en:  1;                    /*In the slave mode it is the enable bit of dummy phase for read-status operations.*/
            uint32_t wrsta_dummy_en:  1;                    /*In the slave mode it is the enable bit of dummy phase for write-status operations.*/
            uint32_t wr_addr_bitlen:  6;                    /*In the slave mode it is the address length in bits for write-buffer operation. The register value shall be (bit_num-1).*/
            uint32_t rd_addr_bitlen:  6;                    /*In the slave mode it is the address length in bits for read-buffer operation. The register value shall be (bit_num-1).*/
            uint32_t buf_bitlen:      9;                    /*In the slave mode it is the length of buffer bit.*/
            uint32_t status_readback: 1;                    /*In the slave mode it is the bit decide whether master reads rd_status register or wr_status register data. 0: rd_status: 1: wr_status*/
            uint32_t reserved25:      1;                    /*reserved*/
            uint32_t status_bitlen:   5;                    /*In the slave mode it is the length of status bit.*/
        };
        uint32_t val;
    } slave1;
    union {
        struct {
            uint32_t rdsta_dummy_cyclelen: 8;               /*In the slave mode it is the length in spi_clk cycles of dummy phase for read-status operations. The register value shall be (cycle_num-1).*/
            uint32_t wrsta_dummy_cyclelen: 8;               /*In the slave mode it is the length in spi_clk cycles of dummy phase for write-status operations. The register value shall be (cycle_num-1).*/
            uint32_t rdbuf_dummy_cyclelen: 8;               /*In the slave mode it is the length in spi_clk cycles of dummy phase for read-buffer operations. The register value shall be (cycle_num-1).*/
            uint32_t wrbuf_dummy_cyclelen: 8;               /*In the slave mode it is the length in spi_clk cycles of dummy phase for write-buffer operations. The register value shall be (cycle_num-1).*/
        };
        uint32_t val;
    } slave2;
    union {
        struct {
            uint32_t rdbuf_cmd_value: 8;                    /*In the slave mode it is the value of read-buffer command.*/
            uint32_t wrbuf_cmd_value: 8;                    /*In the slave mode it is the value of write-buffer command.*/
            uint32_t rdsta_cmd_value: 8;                    /*In the slave mode it is the value of read-status command.*/
            uint32_t wrsta_cmd_value: 8;                    /*In the slave mode it is the value of write-status command.*/
        };
        uint32_t val;
    } slave3;
    uint32_t data_buf[16];                                  /*data buffer*/
    uint32_t reserved_80[30];
    uint32_t ext2;
    union {
        struct {
            uint32_t int_hold_ena: 2;                       /*This register is for two SPI masters to share the same cs clock and data signals. The bits of one SPI are set  if the other SPI is busy  the SPI will be hold. 1(3): hold at ,idle, phase 2: hold at ,prepare, phase.*/
            uint32_t reserved2:   30;                       /*reserved*/
        };
        uint32_t val;
    } ext3;
} spi_dev_t;

extern spi_dev_t SPI0;
extern spi_dev_t SPI1;

#ifdef __cplusplus
}
#endif

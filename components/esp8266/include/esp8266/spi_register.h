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

#include "esp8266/eagle_soc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define REG_SPI_BASE(i)                       (0x60000200-i*0x100)
#define SPI_CMD(i)                            (REG_SPI_BASE(i)  + 0x0)

#define SPI_FLASH_READ                         BIT31
#define SPI_FLASH_WREN                         BIT30
#define SPI_FLASH_WRDI                         BIT29
#define SPI_FLASH_RDID                         BIT28
#define SPI_FLASH_RDSR                         BIT27
#define SPI_FLASH_WRSR                         BIT26
#define SPI_FLASH_PP                           BIT25
#define SPI_FLASH_SE                           BIT24
#define SPI_FLASH_BE                           BIT23
#define SPI_FLASH_CE                           BIT22
#define SPI_FLASH_RES                          BIT20

#define SPI_USR (BIT(18))

#define SPI_ADDR(i)                           (REG_SPI_BASE(i)  + 0x4)

#define SPI_CTRL(i)                           (REG_SPI_BASE(i)  + 0x8)
#define SPI_WR_BIT_ORDER (BIT(26))
#define SPI_RD_BIT_ORDER (BIT(25))
#define SPI_QIO_MODE (BIT(24))
#define SPI_DIO_MODE (BIT(23))
#define SPI_QOUT_MODE (BIT(20))
#define SPI_DOUT_MODE (BIT(14))
#define SPI_FASTRD_MODE (BIT(13))

#define SPI_CTRL1(i)                           (REG_SPI_BASE(i)  + 0xc)
#define  SPI_CS_HOLD_DELAY  0xf
#define  SPI_CS_HOLD_DELAY_S   28
#define  SPI_CS_HOLD_DELAY_RES  0xfff
#define  SPI_CS_HOLD_DELAY_RES_S   16


#define SPI_RD_STATUS(i)                       (REG_SPI_BASE(i)  + 0x10)

#define SPI_CTRL2(i)                           (REG_SPI_BASE(i)  + 0x14)

#define SPI_CS_DELAY_NUM 0x0000000F
#define SPI_CS_DELAY_NUM_S 28
#define SPI_CS_DELAY_MODE 0x00000003
#define SPI_CS_DELAY_MODE_S 26
#define SPI_MOSI_DELAY_NUM 0x00000007
#define SPI_MOSI_DELAY_NUM_S 23
#define SPI_MOSI_DELAY_MODE 0x00000003
#define SPI_MOSI_DELAY_MODE_S 21
#define SPI_MISO_DELAY_NUM 0x00000007
#define SPI_MISO_DELAY_NUM_S 18
#define SPI_MISO_DELAY_MODE 0x00000003
#define SPI_MISO_DELAY_MODE_S 16
#define SPI_CLOCK(i)                          (REG_SPI_BASE(i)  + 0x18)
#define SPI_CLK_EQU_SYSCLK (BIT(31))
#define SPI_CLKDIV_PRE 0x00001FFF
#define SPI_CLKDIV_PRE_S 18
#define SPI_CLKCNT_N 0x0000003F
#define SPI_CLKCNT_N_S 12
#define SPI_CLKCNT_H 0x0000003F
#define SPI_CLKCNT_H_S 6
#define SPI_CLKCNT_L 0x0000003F
#define SPI_CLKCNT_L_S 0

#define SPI_USER(i)                           (REG_SPI_BASE(i)  + 0x1C)
#define SPI_USR_COMMAND (BIT(31))
#define SPI_USR_ADDR (BIT(30))
#define SPI_USR_DUMMY (BIT(29))
#define SPI_USR_MISO (BIT(28))
#define SPI_USR_MOSI (BIT(27))

#define SPI_USR_MOSI_HIGHPART (BIT(25))
#define SPI_USR_MISO_HIGHPART (BIT(24))


#define SPI_SIO (BIT(16))
#define SPI_FWRITE_QIO (BIT(15))
#define SPI_FWRITE_DIO (BIT(14))
#define SPI_FWRITE_QUAD (BIT(13))
#define SPI_FWRITE_DUAL (BIT(12))
#define SPI_WR_BYTE_ORDER (BIT(11))
#define SPI_RD_BYTE_ORDER (BIT(10))
#define SPI_CK_OUT_EDGE (BIT(7))
#define SPI_CK_I_EDGE (BIT(6))
#define SPI_CS_SETUP (BIT(5))
#define SPI_CS_HOLD (BIT(4))
#define SPI_FLASH_MODE (BIT(2))

#define SPI_USER1(i)                          (REG_SPI_BASE(i) + 0x20)
#define SPI_USR_ADDR_BITLEN 0x0000003F
#define SPI_USR_ADDR_BITLEN_S 26
#define SPI_USR_MOSI_BITLEN 0x000001FF
#define SPI_USR_MOSI_BITLEN_S 17
#define SPI_USR_MISO_BITLEN 0x000001FF
#define SPI_USR_MISO_BITLEN_S 8

#define SPI_USR_DUMMY_CYCLELEN 0x000000FF
#define SPI_USR_DUMMY_CYCLELEN_S 0

#define SPI_USER2(i)                          (REG_SPI_BASE(i)  + 0x24)
#define SPI_USR_COMMAND_BITLEN 0x0000000F
#define SPI_USR_COMMAND_BITLEN_S 28
#define SPI_USR_COMMAND_VALUE 0x0000FFFF
#define SPI_USR_COMMAND_VALUE_S 0

#define SPI_WR_STATUS(i)                      (REG_SPI_BASE(i)  + 0x28)
#define SPI_PIN(i)                            (REG_SPI_BASE(i)  + 0x2C)
#define SPI_IDLE_EDGE (BIT(29))
#define SPI_CS2_DIS (BIT(2))
#define SPI_CS1_DIS (BIT(1))
#define SPI_CS0_DIS (BIT(0))

#define SPI_SLAVE(i)                          (REG_SPI_BASE(i)  + 0x30)
#define SPI_SYNC_RESET (BIT(31))
#define SPI_SLAVE_MODE (BIT(30))
#define SPI_SLV_WR_RD_BUF_EN (BIT(29))
#define SPI_SLV_WR_RD_STA_EN (BIT(28))
#define SPI_SLV_CMD_DEFINE (BIT(27))
#define SPI_TRANS_CNT 0x0000000F
#define SPI_TRANS_CNT_S 23
#define SPI_TRANS_DONE_EN (BIT(9))
#define SPI_SLV_WR_STA_DONE_EN (BIT(8))
#define SPI_SLV_RD_STA_DONE_EN (BIT(7))
#define SPI_SLV_WR_BUF_DONE_EN (BIT(6))
#define SPI_SLV_RD_BUF_DONE_EN (BIT(5))



#define SLV_SPI_INT_EN   0x0000001f
#define SLV_SPI_INT_EN_S 5

#define SPI_TRANS_DONE (BIT(4))
#define SPI_SLV_WR_STA_DONE (BIT(3))
#define SPI_SLV_RD_STA_DONE (BIT(2))
#define SPI_SLV_WR_BUF_DONE (BIT(1))
#define SPI_SLV_RD_BUF_DONE (BIT(0))

#define SPI_SLAVE1(i)                         (REG_SPI_BASE(i)  + 0x34)
#define SPI_SLV_STATUS_BITLEN 0x0000001F
#define SPI_SLV_STATUS_BITLEN_S 27
#define SPI_SLV_BUF_BITLEN 0x000001FF
#define SPI_SLV_BUF_BITLEN_S 16
#define SPI_SLV_RD_ADDR_BITLEN 0x0000003F
#define SPI_SLV_RD_ADDR_BITLEN_S 10
#define SPI_SLV_WR_ADDR_BITLEN 0x0000003F
#define SPI_SLV_WR_ADDR_BITLEN_S 4

#define SPI_SLV_WRSTA_DUMMY_EN (BIT(3))
#define SPI_SLV_RDSTA_DUMMY_EN (BIT(2))
#define SPI_SLV_WRBUF_DUMMY_EN (BIT(1))
#define SPI_SLV_RDBUF_DUMMY_EN (BIT(0))



#define SPI_SLAVE2(i)  (REG_SPI_BASE(i)  + 0x38)
#define SPI_SLV_WRBUF_DUMMY_CYCLELEN  0X000000FF
#define SPI_SLV_WRBUF_DUMMY_CYCLELEN_S 24
#define SPI_SLV_RDBUF_DUMMY_CYCLELEN  0X000000FF
#define SPI_SLV_RDBUF_DUMMY_CYCLELEN_S 16
#define SPI_SLV_WRSTR_DUMMY_CYCLELEN  0X000000FF
#define SPI_SLV_WRSTR_DUMMY_CYCLELEN_S  8
#define SPI_SLV_RDSTR_DUMMY_CYCLELEN  0x000000FF
#define SPI_SLV_RDSTR_DUMMY_CYCLELEN_S 0

#define SPI_SLAVE3(i)                         (REG_SPI_BASE(i)  + 0x3C)
#define SPI_SLV_WRSTA_CMD_VALUE 0x000000FF
#define SPI_SLV_WRSTA_CMD_VALUE_S 24
#define SPI_SLV_RDSTA_CMD_VALUE 0x000000FF
#define SPI_SLV_RDSTA_CMD_VALUE_S 16
#define SPI_SLV_WRBUF_CMD_VALUE 0x000000FF
#define SPI_SLV_WRBUF_CMD_VALUE_S 8
#define SPI_SLV_RDBUF_CMD_VALUE 0x000000FF
#define SPI_SLV_RDBUF_CMD_VALUE_S 0

#define SPI_W0(i)                           (REG_SPI_BASE(i) +0x40)
#define SPI_W1(i)                           (REG_SPI_BASE(i) +0x44)
#define SPI_W2(i)                           (REG_SPI_BASE(i) +0x48)
#define SPI_W3(i)                           (REG_SPI_BASE(i) +0x4C)
#define SPI_W4(i)                           (REG_SPI_BASE(i) +0x50)
#define SPI_W5(i)                           (REG_SPI_BASE(i) +0x54)
#define SPI_W6(i)                           (REG_SPI_BASE(i) +0x58)
#define SPI_W7(i)                           (REG_SPI_BASE(i) +0x5C)
#define SPI_W8(i)                           (REG_SPI_BASE(i) +0x60)
#define SPI_W9(i)                           (REG_SPI_BASE(i) +0x64)
#define SPI_W10(i)                          (REG_SPI_BASE(i) +0x68)
#define SPI_W11(i)                          (REG_SPI_BASE(i) +0x6C)
#define SPI_W12(i)                          (REG_SPI_BASE(i) +0x70)
#define SPI_W13(i)                          (REG_SPI_BASE(i) +0x74)
#define SPI_W14(i)                          (REG_SPI_BASE(i) +0x78)
#define SPI_W15(i)                          (REG_SPI_BASE(i) +0x7C)

#define SPI_EXT2(i)                         (REG_SPI_BASE(i)  + 0xF8)

#define SPI_EXT3(i)                         (REG_SPI_BASE(i)  + 0xFC)
#define SPI_INT_HOLD_ENA 0x00000003
#define SPI_INT_HOLD_ENA_S 0

#define SPI_EXT2(i)                             (REG_SPI_BASE(i) + 0xF8)
#define SPI_EXT3(i)                             (REG_SPI_BASE(i) + 0xFC)

#define SPI_ENABLE_AHB                          BIT17

#define SPI_FLASH_CLK_EQU_SYSCLK                BIT12

//SPI flash command
#define SPI_FLASH_READ                          BIT31
#define SPI_FLASH_WREN                          BIT30
#define SPI_FLASH_WRDI                          BIT29
#define SPI_FLASH_RDID                          BIT28
#define SPI_FLASH_RDSR                          BIT27
#define SPI_FLASH_WRSR                          BIT26
#define SPI_FLASH_PP                            BIT25
#define SPI_FLASH_SE                            BIT24
#define SPI_FLASH_BE                            BIT23
#define SPI_FLASH_CE                            BIT22
#define SPI_FLASH_RES                           BIT20
#define SPI_FLASH_DPD                           BIT21
#define SPI_FLASH_HPM                           BIT19

//SPI address register
#define SPI_FLASH_BYTES_LEN                     24
#define SPI_BUFF_BYTE_NUM                       32
#define IODATA_START_ADDR                       BIT0

//SPI status register
#define  SPI_FLASH_BUSY_FLAG                    BIT0
#define  SPI_FLASH_WRENABLE_FLAG                BIT1
#define  SPI_FLASH_BP0                          BIT2
#define  SPI_FLASH_BP1                          BIT3
#define  SPI_FLASH_BP2                          BIT4
#define  SPI_FLASH_TOP_BOT_PRO_FLAG             BIT5
#define  SPI_FLASH_STATUS_PRO_FLAG              BIT7

#define  FLASH_WR_PROTECT                       (SPI_FLASH_BP0|SPI_FLASH_BP1|SPI_FLASH_BP2)

#define SPI 0

#define PERIPHS_SPI_FLASH_C0                    SPI_W0(SPI)
#define PERIPHS_SPI_FLASH_CTRL                  SPI_CTRL(SPI)
#define PERIPHS_SPI_FLASH_CMD                   SPI_CMD(SPI)

#define SPI0_CLK_EQU_SYSCLK                     BIT8

#define PERIPHS_SPI_FLASH_USRREG                (0x60000200 + 0x1c)

#define CACHE_MAP_1M_HIGH                       BIT25
#define CACHE_MAP_2M                            BIT24
#define CACHE_MAP_SEGMENT_S                     16
#define CACHE_MAP_SEGMENT_MASK                  0x3
#define CACHE_BASE_ADDR                         0x40200000
#define CACHE_2M_SIZE                           0x00200000
#define CACHE_1M_SIZE                           0x00100000

#ifdef __cplusplus
}
#endif

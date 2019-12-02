/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef _EAGLE_SOC_H_
#define _EAGLE_SOC_H_

#include "sdkconfig.h"
#include <stdint.h>
#include <stddef.h>
#include "driver/soc.h"

/* IO definitions (access restrictions to peripheral registers) */

#define __RO__        volatile const     /*!< Defines 'read only' permissions                 */
#define __WO__        volatile           /*!< Defines 'write only' permissions                */
#define __RW__        volatile           /*!< Defines 'read / write' permissions              */

//Register Bits{{
#define BIT31   0x80000000
#define BIT30   0x40000000
#define BIT29   0x20000000
#define BIT28   0x10000000
#define BIT27   0x08000000
#define BIT26   0x04000000
#define BIT25   0x02000000
#define BIT24   0x01000000
#define BIT23   0x00800000
#define BIT22   0x00400000
#define BIT21   0x00200000
#define BIT20   0x00100000
#define BIT19   0x00080000
#define BIT18   0x00040000
#define BIT17   0x00020000
#define BIT16   0x00010000
#define BIT15   0x00008000
#define BIT14   0x00004000
#define BIT13   0x00002000
#define BIT12   0x00001000
#define BIT11   0x00000800
#define BIT10   0x00000400
#define BIT9    0x00000200
#define BIT8    0x00000100
#define BIT7    0x00000080
#define BIT6    0x00000040
#define BIT5    0x00000020
#define BIT4    0x00000010
#define BIT3    0x00000008
#define BIT2    0x00000004
#define BIT1    0x00000002
#define BIT0    0x00000001
//}}

#define BIT(nr)     (1UL << (nr))

#define REG_WRITE(_r, _v)       (*(volatile uint32_t *)(_r)) = (_v)
#define REG_READ(_r)            (*(volatile uint32_t *)(_r))

#define REG_SET_BIT(_r, _b)     (*(volatile uint32_t *)(_r) |= (_b))
#define REG_CLR_BIT(_r, _b)     (*(volatile uint32_t *)(_r) &= ~(_b))

//Registers Operation {{
#define ETS_UNCACHED_ADDR(addr) (addr)
#define ETS_CACHED_ADDR(addr)   (addr)

#define READ_PERI_REG(addr)                             (*((volatile uint32_t *)ETS_UNCACHED_ADDR(addr)))
#define WRITE_PERI_REG(addr, val)                       (*((volatile uint32_t *)ETS_UNCACHED_ADDR(addr))) = (uint32_t)(val)
#define CLEAR_PERI_REG_MASK(reg, mask)                  WRITE_PERI_REG((reg), (READ_PERI_REG(reg) & (~(mask))))
#define SET_PERI_REG_MASK(reg, mask)                    WRITE_PERI_REG((reg), (READ_PERI_REG(reg) | (mask)))
#define GET_PERI_REG_BITS(reg, hipos, lowpos)           ((READ_PERI_REG(reg) >> (lowpos)) & ((1 << ((hipos) - (lowpos) + 1)) - 1))
#define SET_PERI_REG_BITS(reg, bit_map, value, shift)   (WRITE_PERI_REG((reg), (READ_PERI_REG(reg) & (~((bit_map) << (shift)))) | ((value) << (shift)) ))
//}}

//Periheral Clock {{
#define CPU_CLK_FREQ                80 * 1000000       // unit: Hz
#define APB_CLK_FREQ                CPU_CLK_FREQ
#define UART_CLK_FREQ               APB_CLK_FREQ
#define TIMER_CLK_FREQ              (APB_CLK_FREQ >> 8) // divided by 256

#define FREQ_1MHZ                   (1000 * 1000)
#define FREQ_1KHZ                   (1000)

#define CPU_FREQ_160MHZ             (160 * 1000 * 1000)
#define CPU_FREQ_80MHz              (80 * 1000 * 1000)

#define CPU_160M_TICKS_PRT_MS       (CPU_FREQ_160MHZ / FREQ_1KHZ)
#define CPU_80M_TICKS_PRT_MS        (CPU_FREQ_80MHz / FREQ_1KHZ)

#define CPU_160M_TICKS_PRT_US       (CPU_FREQ_160MHZ / FREQ_1MHZ)
#define CPU_80M_TICKS_PRT_US        (CPU_FREQ_80MHz / FREQ_1MHZ)
//}}

//Peripheral device base address define{{
#define PERIPHS_DPORT_BASEADDR      0x3ff00000
#define PERIPHS_RTC_BASEADDR        0x60000700
//}}

//DPORT{{
#define HOST_INF_SEL                (PERIPHS_DPORT_BASEADDR + 0x28)
#define DPORT_LINK_DEVICE_SEL       0x000000FF
#define DPORT_LINK_DEVICE_SEL_S     8
#define DPORT_PERI_IO_SWAP          0x000000FF
#define DPORT_PERI_IO_SWAP_S        0
#define PERI_IO_CSPI_OVERLAP        (BIT(7)) // two spi masters on cspi
#define PERI_IO_HSPI_OVERLAP        (BIT(6)) // two spi masters on hspi
#define PERI_IO_HSPI_PRIO           (BIT(5)) // hspi is with the higher prior
#define PERI_IO_UART1_PIN_SWAP      (BIT(3)) // swap uart1 pins (u1rxd <-> u1cts), (u1txd <-> u1rts)
#define PERI_IO_UART0_PIN_SWAP      (BIT(2)) // swap uart0 pins (u0rxd <-> u0cts), (u0txd <-> u0rts)
#define PERI_IO_SPI_PORT_SWAP       (BIT(1)) // swap two spi
#define PERI_IO_UART_PORT_SWAP      (BIT(0)) // swap two uart
//}}

//Interrupt remap control registers define{{
#define EDGE_INT_ENABLE_REG         (PERIPHS_DPORT_BASEADDR + 0x04)
#define WDT_EDGE_INT_ENABLE()       SET_PERI_REG_MASK(EDGE_INT_ENABLE_REG, BIT0)
#define TM1_EDGE_INT_ENABLE()       SET_PERI_REG_MASK(EDGE_INT_ENABLE_REG, BIT1)
#define TM1_EDGE_INT_DISABLE()      CLEAR_PERI_REG_MASK(EDGE_INT_ENABLE_REG, BIT1)
//}}

#define DPORT_CTL_REG               (PERIPHS_DPORT_BASEADDR + 0x14)  
#define DPORT_CTL_DOUBLE_CLK        BIT0

#define INT_ENA_WDEV                0x3ff20c18
#define WDEV_TSF0_REACH_INT         (BIT(27))

#define WDEV_COUNT_REG              (0x3ff20c00)

//Watch dog reg {{
#define PERIPHS_WDT_BASEADDR        0x60000900

#define WDT_CTL_ADDRESS             0
#define WDT_OP_ADDRESS              0x4
#define WDT_OP_ND_ADDRESS           0x8
#define WDT_RST_ADDRESS             0x14

#define WDT_CTL_RSTLEN_MASK         0x38
#define WDT_CTL_RSPMOD_MASK         0x6
#define WDT_CTL_EN_MASK             0x1

#define WDT_CTL_RSTLEN_LSB          0x3
#define WDT_CTL_RSPMOD_LSB          0x1
#define WDT_CTL_EN_LSB              0

#define WDT_FEED_VALUE              0x73

#define WDT_REG_READ(_reg)                  REG_READ(PERIPHS_WDT_BASEADDR + _reg)
#define WDT_REG_WRITE(_reg, _val)           REG_WRITE(PERIPHS_WDT_BASEADDR + _reg, _val)
#define CLEAR_WDT_REG_MASK(_reg, _mask)     WDT_REG_WRITE(_reg, WDT_REG_READ(_reg) & (~_mask))
#define WDT_FEED()                          WDT_REG_WRITE(WDT_RST_ADDRESS, WDT_FEED_VALUE)

//}}

//RTC reg {{
#define REG_RTC_BASE                PERIPHS_RTC_BASEADDR

#define RTC_SLP_VAL                     (REG_RTC_BASE + 0x004) // the target value of RTC_COUNTER for wakeup from light-sleep/deep-sleep
#define RTC_SLP_CNT_VAL                 (REG_RTC_BASE + 0x01C) // the current value of RTC_COUNTER

#define RTC_SCRATCH0                    (REG_RTC_BASE + 0x030) // the register for software to save some values for watchdog reset
#define RTC_SCRATCH1                    (REG_RTC_BASE + 0x034) // the register for software to save some values for watchdog reset
#define RTC_SCRATCH2                    (REG_RTC_BASE + 0x038) // the register for software to save some values for watchdog reset
#define RTC_SCRATCH3                    (REG_RTC_BASE + 0x03C) // the register for software to save some values for watchdog reset

#define RTC_GPIO_OUT                    (REG_RTC_BASE + 0x068) // used by gpio16
#define RTC_GPIO_ENABLE                 (REG_RTC_BASE + 0x074)
#define RTC_GPIO_IN_DATA                (REG_RTC_BASE + 0x08C)
#define RTC_GPIO_CONF                   (REG_RTC_BASE + 0x090)
#define PAD_XPD_DCDC_CONF               (REG_RTC_BASE + 0x0A0)
//}}

//CACHE{{
#define CACHE_FLASH_CTRL_REG            (0x3ff00000 + 0x0c)
#define CACHE_READ_EN_BIT               BIT8
//}}

#define DRAM_BASE                       (0x3FFE8000)
#define DRAM_SIZE                       (96 * 1024)

#define IRAM_BASE                       (0x40100000)
#define IRAM_SIZE                       (CONFIG_SOC_IRAM_SIZE)

#define FLASH_BASE                      (0x40200000)
#define FLASH_SIZE                      (1 * 1024 * 1024)

#define RTC_SYS_BASE                    (0x60001000)
#define RTC_SYS_SIZE                    (0x200)

#define RTC_USER_BASE                   (0x60001200)
#define RTC_USER_SIZE                   (0x200)

#define ROM_BASE                        (0x40000000)
#define ROM_SIZE                        (0x10000)

#define IS_DRAM(a)                      ((size_t)(a) >= DRAM_BASE && (size_t)(a) < (DRAM_BASE + DRAM_SIZE))
#define IS_IRAM(a)                      ((size_t)(a) >= IRAM_BASE && (size_t)(a) < (IRAM_BASE + IRAM_SIZE))
#define IS_FLASH(a)                     ((size_t)(a) >= FLASH_BASE && (size_t)(a) < (FLASH_BASE + FLASH_SIZE))
#define IS_USR_RTC(a)                   ((size_t)(a) >= RTC_USER_BASE && (size_t)(a) < (RTC_USER_BASE + RTC_USER_SIZE))
#define IS_ROM(a)                       ((size_t)(a) >= ROM_BASE && (size_t)(a) < (ROM_BASE + ROM_SIZE))

#endif //_EAGLE_SOC_H_

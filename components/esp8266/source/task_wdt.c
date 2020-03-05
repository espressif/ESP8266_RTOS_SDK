// Copyright 2018-2019 Espressif Systems (Shanghai) PTE LTD
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

#include <stdint.h>

#include "esp_log.h"
#include "esp_libc.h"
#include "esp_task_wdt.h"
#include "esp_attr.h"
#include "portmacro.h"
#include "esp8266/rom_functions.h"
#include "esp8266/eagle_soc.h"
#include "driver/soc.h"

static const char *TAG = "wdt";

#ifdef CONFIG_ESP_TASK_WDT_PANIC
/**
  * @brief  Task watch dog interrupt function and it should do panic
  */
static void esp_task_wdt_isr(void *param)
{
    extern void panicHandler(void *frame, int wdt);
    extern void *__wifi_task_top_sp(void);

    panicHandler(__wifi_task_top_sp(), 1);
}
#endif

#ifdef CONFIG_ESP8266_NMI_WDT

#if CONFIG_ESP_TASK_WDT_TIMEOUT_S == 13
#define NMI_WD_TOTAL_PERIOD (6553600)
#elif CONFIG_ESP_TASK_WDT_TIMEOUT_S == 14
#define NMI_WD_TOTAL_PERIOD (13107200)
#elif CONFIG_ESP_TASK_WDT_TIMEOUT_S == 15
#define NMI_WD_TOTAL_PERIOD (26214400)
#endif

#define NMI_WD_CHECK_PERIOD (1 * 1000 * 1000)

static int s_nmi_wd_state;

static void nmi_panic_wd(void)
{
    extern uint32_t _chip_nmi_cnt;
    extern uint8_t _chip_nmi_stk[];
    extern void panicHandler(void *frame, int wdt);
    uint32_t *p;

    if (_chip_nmi_cnt == 1) {
        p = (uint32_t *)&_chip_nmi_stk[512];
    } else {
        p = (uint32_t *)&_chip_nmi_stk[512 + 124 + 256];
    }

    panicHandler(p - 1, 1);
}

static void IRAM_ATTR nmi_set_wd_time(uint32_t us)
{
    REG_WRITE(WDEVTSF0TIMER_ENA, REG_READ(WDEVTSF0TIMER_ENA) & (~WDEV_TSF0TIMER_ENA));

    REG_WRITE(WDEVTSFSW0_LO, 0);
    REG_WRITE(WDEVTSFSW0_HI, 0);
    REG_WRITE(WDEVTSFSW0_LO, 0);

    REG_WRITE(WDEVTSF0_TIMER_LO, 0);
    REG_WRITE(WDEVTSF0_TIMER_HI, 0);

    REG_WRITE(WDEVTSF0_TIMER_LO, us);

    REG_WRITE(WDEVTSF0TIMER_ENA, WDEV_TSF0TIMER_ENA);
}

static void IRAM_ATTR nmi_check_wd(void)
{
    switch (s_nmi_wd_state) {
        case 0:
            s_nmi_wd_state = 1;
            nmi_set_wd_time(NMI_WD_CHECK_PERIOD);
            break;
        case 1:
            s_nmi_wd_state = 2;
            nmi_set_wd_time(NMI_WD_TOTAL_PERIOD - NMI_WD_CHECK_PERIOD);
            break;
        case 2:
            Cache_Read_Enable_New();
            nmi_panic_wd();
            break;
        default:
            break;
    }
}
#endif

/**
  * @brief  Just for pass compiling and mark wdt calling line
  */
esp_err_t esp_task_wdt_init(void)
{
    CLEAR_WDT_REG_MASK(WDT_CTL_ADDRESS, BIT0);

#ifdef CONFIG_ESP_TASK_WDT_PANIC
    const uint32_t panic_time_param = 11;

    // Just for soft restart
    soc_clear_int_mask(1 << ETS_WDT_INUM);

    _xt_isr_attach(ETS_WDT_INUM, esp_task_wdt_isr, NULL);
    _xt_isr_unmask(1 << ETS_WDT_INUM);

    WDT_EDGE_INT_ENABLE();

    ESP_LOGD(TAG, "Enable task watch dog panic, panic time parameter is %u", panic_time_param);
#else
    const uint32_t panic_time_param = 1;
#endif

    ESP_LOGD(TAG, "task watch dog trigger time parameter is %u", CONFIG_ESP_TASK_WDT_TIMEOUT_S);

    WDT_REG_WRITE(WDT_OP_ADDRESS, CONFIG_ESP_TASK_WDT_TIMEOUT_S);   // 2^n * 0.8ms, mask 0xf, n = 13 -> (2^13 = 8192) * 0.8 * 0.001 = 6.5536
    WDT_REG_WRITE(WDT_OP_ND_ADDRESS, panic_time_param);         // 2^n * 0.8ms, mask 0xf, n = 11 -> (2^11 = 2048) * 0.8 * 0.001 = 1.6384

    SET_PERI_REG_BITS(PERIPHS_WDT_BASEADDR + WDT_CTL_ADDRESS, WDT_CTL_RSTLEN_MASK, 7 << WDT_CTL_RSTLEN_LSB, 0);
    // interrupt then reset
    SET_PERI_REG_BITS(PERIPHS_WDT_BASEADDR + WDT_CTL_ADDRESS, WDT_CTL_RSPMOD_MASK, 0 << WDT_CTL_RSPMOD_LSB, 0);
    // start task watch dog1
    SET_PERI_REG_BITS(PERIPHS_WDT_BASEADDR + WDT_CTL_ADDRESS, WDT_CTL_EN_MASK, 1 << WDT_CTL_EN_LSB, 0);

    WDT_FEED();

#ifdef CONFIG_ESP8266_NMI_WDT
    {
        extern void wDev_MacTimSetFunc(void *func);

        wDev_MacTimSetFunc(nmi_check_wd);;
        nmi_set_wd_time(NMI_WD_CHECK_PERIOD);
    }
#endif

    return 0;
}

/**
  * @brief  Reset(Feed) the Task Watchdog Timer (TWDT) on behalf of the currently
  *         running task
  */
void esp_task_wdt_reset(void)
{
    WDT_FEED();

#ifdef CONFIG_ESP8266_NMI_WDT
    s_nmi_wd_state = 0;
#endif
}

/**
  * @brief  Just for pass compiling and mark wdt calling line
  */
void pp_soft_wdt_stop(void)
{

}

/**
  * @brief  Just for pass compiling and mark wdt calling line
  */
void pp_soft_wdt_restart(void)
{

}

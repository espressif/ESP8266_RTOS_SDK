// Copyright 2019-2020 Espressif Systems (Shanghai) PTE LTD
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
#include <sys/param.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_sleep.h"
#include "esp_wifi.h"
#include "FreeRTOS.h"
#include "freertos/task.h"
#include "driver/soc.h"
#include "driver/gpio.h"
#include "esp8266/timer_struct.h"
#include "esp8266/gpio_struct.h"
#include "esp8266/rom_functions.h"
#include "driver/rtc.h"
#include "rom/uart.h"
#include "internal/phy_init_data.h"

#define FRC2_LOAD               (0x60000620)
#define FRC2_COUNT              (0x60000624)
#define FRC2_CTL                (0x60000628)
#define FRC2_ALARM              (0x60000630)

#define FRC2_CNTL_ENABLE        BIT7

#define FRC2_TICKS_PER_US       (5)
#define FRC2_TICKS_MAX          (UINT32_MAX / 4)

#define SLEEP_PROC_TIME         (2735)
#define WAKEUP_EARLY_TICKS      (264) // PLL and STAL wait ticks
#define MIN_SLEEP_US            (6500)
#define RTC_TICK_CAL            (100) 
#define RTC_TICK_OFF            (1245 + RTC_TICK_CAL)

#define TAG                     "esp8266_pm"

typedef struct pm_soc_clk {
    uint32_t    ccount;

    uint32_t    frc2_enable;
    uint32_t    frc2_cnt;

    uint32_t    wdev_cnt;

    uint32_t    rtc_val;
    uint32_t    cal_period;

    uint32_t    sleep_us;
} pm_soc_clk_t;

typedef struct sleep_proc {
    uint32_t    sleep_us;               // sleep microsecond

    uint32_t    wait_int : 1;           // wait interrupt
    uint32_t    check_mode : 1;         // check sleep mode
    uint32_t    flush_uart : 1;         // flush UART
} sleep_proc_t;

static uint16_t s_lock_cnt = 1;
static esp_sleep_mode_t s_sleep_mode = ESP_CPU_WAIT;
static uint32_t s_sleep_wakup_triggers;
static uint32_t s_sleep_duration;

static inline uint32_t save_local_wdev(void)
{
    extern uint32_t WDEV_INTEREST_EVENT;

    uint32_t reg = WDEV_INTEREST_EVENT;

    REG_WRITE(INT_ENA_WDEV, WDEV_TSF0_REACH_INT);

    return reg;
}

static inline void restore_local_wdev(uint32_t reg)
{
    REG_WRITE(INT_ENA_WDEV, reg);
}

uint32_t rtc_clk_to_us(uint32_t rtc_cycles, uint32_t period)
{
    return (uint64_t)rtc_cycles * period / 4096;
}

uint32_t rtc_us_to_clk(uint32_t us, uint32_t period)
{
    return (uint64_t)us * 4096 / period;
}

static inline void save_soc_clk(pm_soc_clk_t *clk)
{
    clk->rtc_val = REG_READ(RTC_SLP_CNT_VAL);

    clk->ccount = soc_get_ccount();

    clk->frc2_enable = REG_READ(FRC2_CTL) & FRC2_CNTL_ENABLE;
    if (clk->frc2_enable)
        clk->frc2_cnt = REG_READ(FRC2_COUNT);

    clk->wdev_cnt = REG_READ(WDEV_COUNT_REG);
}

static inline uint32_t min_sleep_us(pm_soc_clk_t *clk)
{
    const uint32_t os_idle_ticks = prvGetExpectedIdleTime();
    const int32_t os_sleep_us = ((int32_t)soc_get_ccompare() - (int32_t)clk->ccount) / g_esp_ticks_per_us +
                                        (os_idle_ticks ? os_idle_ticks - 1 : 0) * portTICK_RATE_MS * 1000;
    const uint32_t ccompare_sleep_us = os_sleep_us > 0 ? os_sleep_us : 0;

    if (clk->frc2_enable) {
        const uint32_t frc2_sleep_ticks = REG_READ(FRC2_ALARM) - clk->frc2_cnt;
        const uint32_t frc2_sleep_us = frc2_sleep_ticks < FRC2_TICKS_MAX ? frc2_sleep_ticks / FRC2_TICKS_PER_US : 0;

        clk->sleep_us = MIN(ccompare_sleep_us, frc2_sleep_us);
    } else {
        clk->sleep_us = ccompare_sleep_us;
    }

    return clk->sleep_us;
}

static inline uint32_t sleep_rtc_ticks(pm_soc_clk_t *clk)
{
    uint32_t rtc_ticks;

    clk->cal_period = pm_rtc_clock_cali_proc();

    rtc_ticks = rtc_us_to_clk(clk->sleep_us - SLEEP_PROC_TIME, clk->cal_period);

    return rtc_ticks;
}

static inline void update_soc_clk(pm_soc_clk_t *clk)
{
    extern uint32_t WdevTimOffSet;

    uint32_t slept_us;

    if (s_sleep_wakup_triggers & RTC_GPIO_TRIG_EN) {
        uint32_t total_rtc = 0, end_rtc = REG_READ(RTC_SLP_CNT_VAL);

        if (end_rtc > clk->rtc_val)
            total_rtc = end_rtc - clk->rtc_val;
        else
            total_rtc = UINT32_MAX - clk->rtc_val + end_rtc;
        slept_us = rtc_clk_to_us(total_rtc, clk->cal_period) + RTC_TICK_OFF;

        if (slept_us >= clk->sleep_us)
            slept_us = clk->sleep_us;
        else
            slept_us -= RTC_TICK_CAL;
    } else {
        slept_us = clk->sleep_us;
    }

    const uint32_t os_ccount = slept_us * g_esp_ticks_per_us + clk->ccount;

    if (os_ccount >= _xt_tick_divisor) 
        soc_set_ccompare(os_ccount + 32);
    soc_set_ccount(os_ccount);

    if (clk->frc2_enable) {
        const uint32_t frc2_cnt = slept_us * FRC2_TICKS_PER_US + clk->frc2_cnt - 1;

        REG_WRITE(FRC2_LOAD,  frc2_cnt);
    }

    uint32_t wdev_us;
    uint32_t wdev_cnt = REG_READ(WDEV_COUNT_REG);

    if (clk->wdev_cnt < wdev_cnt)
        wdev_us = slept_us - (wdev_cnt - clk->wdev_cnt);
    else
        wdev_us = slept_us - (UINT32_MAX - clk->wdev_cnt + wdev_cnt);

    WdevTimOffSet += wdev_us;
}

static int cpu_is_wait_mode(void)
{
    return (s_sleep_mode == ESP_CPU_WAIT) || s_lock_cnt;
}

static int cpu_reject_sleep(void)
{
    int ret = 0;

    if (s_sleep_wakup_triggers & RTC_GPIO_TRIG_EN) {
        for (int gpio = 0; gpio < 16; gpio++) {
            if (!GPIO.pin[gpio].wakeup_enable)
                continue;

            if (GPIO.pin[gpio].int_type == GPIO_INTR_LOW_LEVEL) {
                if (!((GPIO.in >> gpio) & 1)) {
                    ret = 1;
                    break;
                }
            } else if (GPIO.pin[gpio].int_type == GPIO_INTR_HIGH_LEVEL) {
                if ((GPIO.in >> gpio) & 1) {
                    ret = 1;
                    break;
                }
            }
        }
    }

    return ret;
}

rtc_cpu_freq_t rtc_clk_cpu_freq_get(void)
{
    rtc_cpu_freq_t freq;
    uint32_t reg = REG_READ(DPORT_CTL_REG);

    if (reg & DPORT_CTL_DOUBLE_CLK)
        freq = RTC_CPU_FREQ_160M;
    else
        freq = RTC_CPU_FREQ_80M;

    return freq;
}

void rtc_clk_cpu_freq_set(rtc_cpu_freq_t cpu_freq)
{
    if (RTC_CPU_FREQ_80M == cpu_freq)
        REG_CLR_BIT(DPORT_CTL_REG, DPORT_CTL_DOUBLE_CLK);
    else
        REG_SET_BIT(DPORT_CTL_REG, DPORT_CTL_DOUBLE_CLK);
}

esp_err_t esp_sleep_enable_timer_wakeup(uint32_t time_in_us)
{
    if (time_in_us <= MIN_SLEEP_US)
        return ESP_ERR_INVALID_ARG;

    s_sleep_duration = time_in_us;
    s_sleep_wakup_triggers |= RTC_TIMER_TRIG_EN;

    return ESP_OK;
}

esp_err_t esp_sleep_enable_gpio_wakeup(void)
{
    s_sleep_wakup_triggers |= RTC_GPIO_TRIG_EN;

    return ESP_OK;
}

esp_err_t esp_sleep_disable_wakeup_source(esp_sleep_source_t source)
{
    // For most of sources it is enough to set trigger mask in local
    // configuration structure. The actual RTC wake up options
    // will be updated by esp_sleep_start().
    if (source == ESP_SLEEP_WAKEUP_ALL) {
        s_sleep_wakup_triggers = 0;
    } else if (ESP_SLEEP_WAKEUP_TIMER == source) {
        s_sleep_wakup_triggers &= ~RTC_TIMER_TRIG_EN;
        s_sleep_duration = 0;
    } else if (ESP_SLEEP_WAKEUP_GPIO == source) {
        s_sleep_wakup_triggers &= ~RTC_GPIO_TRIG_EN;
    } else {
        ESP_LOGE(TAG, "Incorrect wakeup source (%d) to disable.", (int) source);
        return ESP_ERR_INVALID_STATE;
    }

    return ESP_OK;
}

static int esp_light_sleep_internal(const sleep_proc_t *proc)
{
    pm_soc_clk_t clk;
    esp_irqflag_t irqflag;
    uint32_t wdevflag;
    uint32_t sleep_us;
    uint64_t start_us;
    int ret = ESP_OK;
    int cpu_wait = proc->wait_int;

    start_us = esp_timer_get_time();

    if (proc->check_mode && cpu_is_wait_mode()) {
        if (proc->wait_int)
            soc_wait_int();
        return ESP_ERR_INVALID_ARG;
    }

    if (cpu_reject_sleep()) {
        if (proc->wait_int)
            soc_wait_int();
        return ESP_ERR_INVALID_STATE;
    }

    if (proc->flush_uart) {
        uart_tx_wait_idle(0);
        uart_tx_wait_idle(1);
    }

    irqflag = soc_save_local_irq();
    wdevflag = save_local_wdev();

    if (proc->check_mode && cpu_is_wait_mode()) {
        ret = ESP_ERR_INVALID_ARG;
        goto exit;
    }

    if (cpu_reject_sleep()) {
        ret = ESP_ERR_INVALID_STATE;
        goto exit;
    }

    save_soc_clk(&clk);
    if (!proc->sleep_us)
        sleep_us = min_sleep_us(&clk);
    else {
        uint64_t total_us = esp_timer_get_time() - start_us;

        if (total_us >= proc->sleep_us) {
            ret = ESP_ERR_INVALID_ARG;
            goto exit;
        } else
            sleep_us = clk.sleep_us = proc->sleep_us - total_us;
    }

    if (sleep_us > MIN_SLEEP_US) {
        uint32_t rtc_ticks = sleep_rtc_ticks(&clk);

        if (rtc_ticks > WAKEUP_EARLY_TICKS + 1) {
            rtc_cpu_freq_t cpu_freq = rtc_clk_cpu_freq_get();

            pm_set_sleep_mode(2);
            pm_set_sleep_cycles(rtc_ticks - WAKEUP_EARLY_TICKS);
            rtc_light_sleep_start(s_sleep_wakup_triggers | RTC_TIMER_TRIG_EN, 0);
            rtc_wakeup_init();

            rtc_clk_cpu_freq_set(cpu_freq);

            update_soc_clk(&clk);

            cpu_wait = 0;
        } else
            ret = ESP_ERR_INVALID_ARG;
    } else
        ret = ESP_ERR_INVALID_ARG;

exit:
    restore_local_wdev(wdevflag);
    soc_restore_local_irq(irqflag);

    if (cpu_wait)
        soc_wait_int();

    return ret;
}

esp_err_t esp_light_sleep_start(void)
{
    const sleep_proc_t proc = {
        .sleep_us = s_sleep_duration,
        .wait_int = 0,
        .check_mode = 0,
        .flush_uart = 1
    };

    if (esp_wifi_get_state() >= WIFI_STATE_START) {
        return ESP_ERR_INVALID_STATE;
    }

    return esp_light_sleep_internal(&proc);
}

void esp_sleep_lock(void)
{
    const esp_irqflag_t irqflag = soc_save_local_irq();
    s_lock_cnt++;
    soc_restore_local_irq(irqflag);
}

void esp_sleep_unlock(void)
{
    const esp_irqflag_t irqflag = soc_save_local_irq();
    s_lock_cnt--;
    soc_restore_local_irq(irqflag);
}

void esp_sleep_set_mode(esp_sleep_mode_t mode)
{
    s_sleep_mode = mode;
}

void esp_sleep_start(void)
{
    const sleep_proc_t proc = {
        .sleep_us = 0,
        .wait_int = 1,
        .check_mode = 1,
        .flush_uart = 1,
    };

    esp_light_sleep_internal(&proc);
}

esp_err_t esp_pm_configure(const void* vconfig)
{
#ifndef CONFIG_PM_ENABLE
    return ESP_ERR_NOT_SUPPORTED;
#endif

    const esp_pm_config_esp8266_t* config = (const esp_pm_config_esp8266_t*) vconfig;
    if (config->light_sleep_enable) {
        s_sleep_mode = ESP_CPU_LIGHTSLEEP;
    } else {
        s_sleep_mode = ESP_CPU_WAIT;
    }
    return ESP_OK;
}

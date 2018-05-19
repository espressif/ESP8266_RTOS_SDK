// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
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

#include "esp_common.h"
#include "freertos/portmacro.h"

#define US_TO_RTC_TIMER_TICKS(t)          \
    ((t) ?          \
     (((t) > 0x35A) ?            \
      (((t) >> 2) * ((APB_CLK_FREQ >> 4) / 250000) + ((t)&0x3) * ((APB_CLK_FREQ >> 4) / 1000000)) : \
      (((t) *(APB_CLK_FREQ>>4)) / 1000000)) :    \
     0)

#define FRC1_ENABLE_TIMER   BIT7
#define FRC1_AUTO_LOAD      BIT6

typedef enum {          // timer provided mode
    DIVDED_BY_1   = 0,  // timer clock
    DIVDED_BY_16  = 4,  // divided by 16
    DIVDED_BY_256 = 8,  // divided by 256
} TIMER_PREDIVED_MODE;

typedef enum {          // timer interrupt mode
    TM_LEVEL_INT = 1,   // level interrupt
    TM_EDGE_INT  = 0,   // edge interrupt
} TIMER_INT_MODE;

#define RTC_REG_WRITE(addr, val)    WRITE_PERI_REG(addr, val)

static void (* user_hw_timer_cb)(void) = NULL;

bool frc1_auto_load = false;

static void hw_timer_isr_cb(void* arg)
{
    if (frc1_auto_load == false) {
        RTC_REG_WRITE(FRC1_CTRL_ADDRESS,
                      DIVDED_BY_16 | TM_EDGE_INT);
    }

    if (user_hw_timer_cb != NULL) {
        (*(user_hw_timer_cb))();
    }
}

void hw_timer_disarm(void)
{
    RTC_REG_WRITE(FRC1_CTRL_ADDRESS, 0);
}

void hw_timer_arm(uint32_t val, bool req)
{
    frc1_auto_load = req;

    if (frc1_auto_load == true) {
        RTC_REG_WRITE(FRC1_CTRL_ADDRESS,
                      FRC1_AUTO_LOAD | DIVDED_BY_16 | FRC1_ENABLE_TIMER | TM_EDGE_INT);
    } else {
        RTC_REG_WRITE(FRC1_CTRL_ADDRESS,
                      DIVDED_BY_16 | FRC1_ENABLE_TIMER | TM_EDGE_INT);
    }

    RTC_REG_WRITE(FRC1_LOAD_ADDRESS, US_TO_RTC_TIMER_TICKS(val));
}

void hw_timer_set_func(void (* user_hw_timer_cb_set)(void))
{
    user_hw_timer_cb = user_hw_timer_cb_set;
}

void hw_timer_init(void)
{
#if 0

    if (req == 1) {
        RTC_REG_WRITE(FRC1_CTRL_ADDRESS,
                      FRC1_AUTO_LOAD | DIVDED_BY_16 | FRC1_ENABLE_TIMER | TM_EDGE_INT);
    } else {
        RTC_REG_WRITE(FRC1_CTRL_ADDRESS,
                      DIVDED_BY_16 | FRC1_ENABLE_TIMER | TM_EDGE_INT);
    }

#endif
    _xt_isr_attach(ETS_FRC_TIMER1_INUM, hw_timer_isr_cb, NULL);

    TM1_EDGE_INT_ENABLE();
    _xt_isr_unmask(1 << ETS_FRC_TIMER1_INUM);
}

//-------------------------------Test Code Below--------------------------------------
#if 0
#include "hw_timer.h"

#define REG_WRITE(_r,_v)    (*(volatile uint32_t *)(_r)) = (_v)
#define REG_READ(_r)        (*(volatile uint32_t *)(_r))
#define WDEV_NOW()          REG_READ(0x3ff20c00)

uint32_t tick_now2 = 0;
void hw_test_timer_cb(void)
{
    static uint16 j = 0;
    j++;

    if ((WDEV_NOW() - tick_now2) >= 1000000) {
        static uint32_t idx = 1;
        tick_now2 = WDEV_NOW();
        os_printf("b%u:%d\n", idx++, j);
        j = 0;
    }

    //hw_timer_arm(50);
}

void user_init(void)
{
    hw_timer_init();
    hw_timer_set_func(hw_test_timer_cb, 1);
    hw_timer_arm(100);
}
#endif


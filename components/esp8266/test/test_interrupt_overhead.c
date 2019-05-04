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

#include <assert.h>
#include <esp_spi_flash.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <unity.h>

#include "FreeRTOS.h"
#include <driver/hw_timer.h>
#include "esp8266/timer_register.h"

#define TIMER1_ENABLE_TIMER 0x0080
#define TIMER1_INTERRUPT_TYPE_LEVEL 1
#define hw_timer_intr_register(a, b) \
  _xt_isr_attach(ETS_FRC_TIMER1_INUM, (a), (b))

struct timer_regs {
  uint32_t frc1_load;  /* 0x60000600 */
  uint32_t frc1_count; /* 0x60000604 */
  uint32_t frc1_ctrl;  /* 0x60000608 */
  uint32_t frc1_int;   /* 0x6000060C */
};
static struct timer_regs *timer = (struct timer_regs *)(PERIPHS_TIMER_BASEDDR);

static void test_timer_cb(void *cpu_clk_cnt) {
  *(uint32_t *)cpu_clk_cnt = soc_get_ccount();
}

TEST_CASE("Test interrupt overhead time", "[log]") {
  volatile uint32_t cpu_clk_cnt_interrupt_enter;
  uint32_t cpu_clk_cnt_start;
  uint32_t cpu_clk_cnt_stop;

  // Execute the test twice and only take the second result to make sure
  // that IRAM has cached all instructions.
  for (int i = 0; i < 2; i++) {
    cpu_clk_cnt_interrupt_enter = 0;
    hw_timer_init(test_timer_cb, (void *)&cpu_clk_cnt_interrupt_enter);
    hw_timer_intr_register(test_timer_cb, (void *)&cpu_clk_cnt_interrupt_enter);

    // Setup the timer, so that it'll trigger exactly once.
    timer->frc1_int = 0;   // reset the interrupt status
    timer->frc1_load = 0;  // trigger the timer interrupt immediately
    timer->frc1_ctrl = TIMER1_ENABLE_TIMER | TIMER1_INTERRUPT_TYPE_LEVEL;
    cpu_clk_cnt_start = soc_get_ccount();

    // busy wait until the interrupt triggered and updated the variable
    while (!cpu_clk_cnt_interrupt_enter)
      ;

    cpu_clk_cnt_stop = soc_get_ccount();
    hw_timer_deinit();
  }

  uint32_t overhead_enter = cpu_clk_cnt_interrupt_enter - cpu_clk_cnt_start;
  uint32_t overhead_total = cpu_clk_cnt_stop - cpu_clk_cnt_start;

#if CONFIG_OPTIMIZATION_LEVEL_DEBUG
#  define INTERRUPT_OVERHEAD_ENTER_TIME 334
#  define INTERRUPT_OVERHEAD_TOTAL_TIME 459
#else // CONFIG_OPTIMIZATION_LEVEL_RELEASE
#  define INTERRUPT_OVERHEAD_ENTER_TIME 258
#  define INTERRUPT_OVERHEAD_TOTAL_TIME 385
#endif

  if (overhead_enter != INTERRUPT_OVERHEAD_ENTER_TIME ||
      overhead_total != INTERRUPT_OVERHEAD_TOTAL_TIME) {
    char buf[128];
    snprintf(buf, sizeof(buf),
             "interrupt overhead times changed. expected (enter=%d, total=%d), "
             "but got (enter=%d, total=%d)",
             INTERRUPT_OVERHEAD_ENTER_TIME, INTERRUPT_OVERHEAD_TOTAL_TIME,
             overhead_enter, overhead_total);
    TEST_FAIL_MESSAGE(buf);
  }
}

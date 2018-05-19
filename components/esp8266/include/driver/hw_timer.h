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

#ifndef __HW_TIMER_H__
#define __HW_TIMER_H__

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup HW_Timer_APIs Hardware timer APIs
  * @brief Hardware timer APIs
  *
  * @attention Hardware timer can not interrupt other ISRs.
  *
  */

/** @addtogroup HW_Timer_APIs
  * @{
  */


/**
  * @brief   Initialize the hardware ISR timer.
  *
  * @param   null
  *
  * @return  null
  */
void hw_timer_init(void);

/**
  * @brief   Set a trigger timer delay to enable this timer.
  *
  * @param   uint32 val : Timing
  *    - In autoload mode, range : 50 ~ 0x7fffff
  *    - In non-autoload mode, range : 10 ~ 0x7fffff
  *
  * @param   uint8 req : 0, not autoload; 1, autoload mode.
  *
  * @return  null
  */
void hw_timer_arm(uint32 val, bool req);

/**
  * @brief   disable this timer.
  *
  * @param   null
  *
  * @return  null
  */
void hw_timer_disarm(void);

/**
  * @brief   Set timer callback function.
  *
  *         For enabled timer, timer callback has to be set.
  *
  * @param   uint32 val : Timing
  *    - In autoload mode, range : 50 ~ 0x7fffff
  *    - In non-autoload mode, range : 10 ~ 0x7fffff
  *
  * @return  null
  */
void hw_timer_set_func(void (* user_hw_timer_cb_set)(void));

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif

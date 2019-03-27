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

#include "stdlib.h"

#include "esp_attr.h"
#include "esp_libc.h"

#include "esp8266/eagle_soc.h"
#include "esp8266/rom_functions.h"
#include "rom/ets_sys.h"
#include "esp_err.h"

#include "FreeRTOS.h"
#include "task.h"
#include "private/list.h"
#include "private/portable.h"

#define STACK_VOL_NUM 16

#ifndef DISABLE_FREERTOS
/*
 * @Note: If freeRTOS is updated, the structure must be checked.
 */
typedef struct task_info
{
    volatile StackType_t	*pxTopOfStack;

#if ( portUSING_MPU_WRAPPERS == 1 )
    xMPU_SETTINGS	xMPUSettings;
#endif

    ListItem_t			xStateListItem;
    ListItem_t			xEventListItem;
    UBaseType_t			uxPriority;
    StackType_t			*pxStack;
    char				pcTaskName[ configMAX_TASK_NAME_LEN ];

#if configRECORD_STACK_HIGH_ADDRESS != 1
#error "configRECORD_STACK_HIGH_ADDRESS must enable"
#endif

#if portSTACK_GROWTH >= 0
#error "Task stack must decrease growing."
#endif

    StackType_t		*pxEndOfStack;
} task_info_t;

static inline uint32_t *__get_stack_head(void *task)
{
    return (uint32_t *)((task_info_t *)task)->pxStack;
}

static inline uint32_t *__get_stack_tail(void *task)
{
    return (uint32_t *)((task_info_t *)task)->pxEndOfStack;
} 
#else
typedef void*   task_info_t;

extern uint32_t *__get_stack_head(void *task);
extern uint32_t *__get_stack_tail(void *task);
#endif

static void panic_stack(const uint32_t *reg, const uint32_t *start_stk, const uint32_t *end_stk)
{
    const uint32_t *stk_ptr = (const uint32_t *)reg[4];

    if (stk_ptr <= start_stk || stk_ptr >= end_stk) {
        ets_printf("register map is %x error\n", stk_ptr);
        while (1);
    } else {
        start_stk = (const uint32_t *)((uint32_t)stk_ptr & (~(STACK_VOL_NUM * sizeof(const uint32_t *) - 1)));
    }

    size_t size = end_stk - start_stk + 1;

    if (size < STACK_VOL_NUM) {
        start_stk = start_stk - (STACK_VOL_NUM - size);
        size = STACK_VOL_NUM;
    }

    ets_printf("%10s", " ");
    for (int i = 0; i < STACK_VOL_NUM; i++) {
        ets_printf("  %8x ", i * sizeof(void *));
    }
    ets_printf("\r\n\r\n");

    for (int i = 0; i < size; i += STACK_VOL_NUM) {
        size_t len = size > i ? size - i : STACK_VOL_NUM - (i - size);

        if (len > STACK_VOL_NUM)
            len = STACK_VOL_NUM;

        ets_printf("%08x  ", start_stk + i);

        for (int j = 0; j < len; j++) {
            ets_printf("0x%08x ", start_stk[i + j]);
        }
        ets_printf("\r\n");
    }
}

/*
 * @brief output xtensa register value map when crash
 * 
 * @param frame xtensa register value map pointer
 * 
 * @return none
 */
static __attribute__((noreturn)) void panic_info(void *frame, int wdt)
{
    extern int _chip_nmi_cnt;

    task_info_t *task;
    uint32_t *regs = (uint32_t *)frame;
    int x, y;
    const char *sdesc[] = {
        "PC",   "PS",   "A0",   "A1",
        "A2",   "A3",   "A4",   "A5",
        "A6",   "A7",   "A8",   "A9",
        "A10",  "A11",  "A12",  "A13",
        "A14",  "A15",  "SAR",  "EXCCAUSE"
    };

    ets_printf("\r\n\r\n");

    if (wdt) {
        ets_printf("Task watchdog got triggered.\r\n\r\n");
    }
    
    if (_chip_nmi_cnt) {
        extern const uint32_t _chip_nmi_stk, LoadStoreErrorHandlerStack;

        _chip_nmi_cnt = 0;
        ets_printf("Core 0 was running in NMI context:\r\n\r\n");

        panic_stack(regs, &_chip_nmi_stk, &LoadStoreErrorHandlerStack);
    } else {
        if (xPortInIsrContext() && !wdt) {
            extern const uint32_t _chip_interrupt_stk, _chip_interrupt_tmp;

            ets_printf("Core 0 was running in ISR context:\r\n\r\n");

            panic_stack(regs, &_chip_interrupt_stk, &_chip_interrupt_tmp);
        } else {
            if ((task = (task_info_t *)xTaskGetCurrentTaskHandle())) {
                const uint32_t *pdata = __get_stack_head(task);
                const uint32_t *end = __get_stack_tail(task);

                ets_printf("Task stack [%s] stack from [%p] to [%p], total [%d] size\r\n\r\n", task->pcTaskName, 
                            pdata, end, (end - pdata + 1) * sizeof(const uint32_t *));

                panic_stack(regs, pdata, end);

                ets_printf("\r\n\r\n");
            } else {
                ets_printf("No task\r\n\r\n");
            }
        }
    }

    for (x = 0; x < 20; x += 4) {
        for (y = 0; y < 4; y++) {
            ets_printf("%10s: 0x%08x", sdesc[x + y], regs[x + y + 1]);
        }
        ets_printf("\r\n");
    }

    /*
     * Todo: add more option to select here to 'Kconfig':
     *     1. blocking
     *     2. restart
     *     3. GBD break
     */
    while (1);
}

void __attribute__((noreturn)) panicHandler(void *frame, int wdt)
{
    /* NMI can interrupt exception. */
    vPortEnterCritical();
    do {
        REG_WRITE(INT_ENA_WDEV, 0);
    } while (REG_READ(INT_ENA_WDEV) != 0);

    panic_info(frame, wdt);
}

void __attribute__((noreturn)) _esp_error_check_failed(esp_err_t rc, const char *file, int line, const char *function, const char *expression)
{
    printf("ESP_ERROR_CHECK failed: esp_err_t 0x%x at %p\n", rc, __builtin_return_address(0));
    printf("file: \"%s\" line %d\nfunc: %s\nexpression: %s\n", file, line, function, expression);
    abort();
}

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

static void panic_data32(uint32_t data, int hex)
{
    char buf[12];
    size_t off = 0;

    if (!data)
        buf[off++] = '0';
    else {
        while (data) {
            char tmp = data % hex;

            if (tmp >= 10)
                tmp = tmp - 10 + 'a';
            else
                tmp = tmp + '0';

            data = data / hex;

            buf[off++] = tmp;
        }
    }

    if (hex == 16) {
        while (off < 8)
            buf[off++] = '0';
    }

    while (off)
        ets_putc(buf[--off]);
}

static void panic_str(const char *s)
{
    while (*s)
        ets_putc(*s++);
}

static void panic_stack(StackType_t *start_stk, StackType_t *end_stk)
{
    uint32_t *start = (uint32_t *)start_stk, *end = (uint32_t *)end_stk;
    size_t i, j;
    size_t size = end - start;

    panic_str("          ");
    for (i = 0; i < STACK_VOL_NUM; i++) {
        panic_data32(i * sizeof(void *), 16);
        panic_str(" ");
    }
    panic_str("\r\n\r\n");

    for (i = 0; i < size; i += STACK_VOL_NUM) {
        size_t len = size > i ? size - i : STACK_VOL_NUM - (i - size);

        if (len > STACK_VOL_NUM)
            len = STACK_VOL_NUM;

        panic_data32((uint32_t)&start[i], 16);
        panic_str("  ");

        for (j = 0; j < len; j++) {
            panic_data32((uint32_t)start[i + j], 16);
            panic_str(" ");
        }
        panic_str("\r\n");
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
    int *regs = (int *)frame;
    int x, y;
    const char *sdesc[] = {
        "      PC",   "      PS",   "      A0",   "      A1",
        "      A2",   "      A3",   "      A4",   "      A5",
        "      A6",   "      A7",   "      A8",   "      A9",
        "     A10",   "     A11",   "     A12",   "     A13",
        "     A14",   "     A15",   "     SAR",   "EXCCAUSE"
    };

    panic_str("\r\n\r\n");

    if (wdt) {
        panic_str("Task watchdog got triggered.\r\n\r\n");
    }
    
    if (_chip_nmi_cnt) {
        extern StackType_t _chip_nmi_stk, LoadStoreErrorHandlerStack;

        _chip_nmi_cnt = 0;
        panic_str("Core 0 was running in NMI context:\r\n\r\n");

        panic_stack(&_chip_nmi_stk, &LoadStoreErrorHandlerStack);
    } else {
        if (xPortInIsrContext() && !wdt) {
            extern StackType_t _chip_interrupt_stk, _chip_interrupt_tmp;

            panic_str("Core 0 was running in ISR context:\r\n\r\n");

            panic_stack(&_chip_interrupt_stk, &_chip_interrupt_tmp);
        } else {
            if ((task = (task_info_t *)xTaskGetCurrentTaskHandle())) {
                StackType_t *pdata = task->pxStack;
                StackType_t *end = task->pxEndOfStack + 4;

                // "Task stack [%s] stack from [%p] to [%p], total [%d] size\r\n\r\n"
                panic_str("Task stack [");
                panic_str(task->pcTaskName);
                panic_str("] stack from [");
                panic_data32((uint32_t)pdata, 16);
                panic_str("] to [");
                panic_data32((uint32_t)end, 16);
                panic_str("], total [");
                panic_data32((uint32_t)(end - pdata), 10);
                panic_str("] size\r\n\r\n");

                panic_stack(pdata, end);

                panic_str("\r\n\r\n");
            } else {
                panic_str("No task\r\n\r\n");
            }
        }
    }

    for (x = 0; x < 20; x += 4) {
        for (y = 0; y < 4; y++) {
            panic_str(sdesc[x + y]);
            panic_str(": 0x");
            panic_data32((uint32_t)regs[x + y + 1], 16);
            panic_str(" ");
        }
        panic_str("\r\n");
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
    int cnt = 10;

    /* NMI can interrupt exception. */
    vPortEnterCritical();
    while (cnt--) {
        REG_WRITE(INT_ENA_WDEV, 0);
    }

    panic_info(frame, wdt);
}

void __attribute__((noreturn)) _esp_error_check_failed(esp_err_t rc, const char *file, int line, const char *function, const char *expression)
{
    printf("ESP_ERROR_CHECK failed: esp_err_t 0x%x at %p\n", rc, __builtin_return_address(0));
    printf("file: \"%s\" line %d\nfunc: %s\nexpression: %s\n", file, line, function, expression);
    abort();
}
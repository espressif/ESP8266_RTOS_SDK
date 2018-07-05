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

#include "esp_attr.h"
#include "esp_libc.h"

#include "esp8266/eagle_soc.h"
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

static void IRAM_ATTR panic_stack(StackType_t *start, StackType_t *end)
{
    size_t i, j;
    size_t size = end - start + 1;

    ets_printf("%10s", " ");
    for (i = 0; i < STACK_VOL_NUM; i++) {
        ets_printf("%-8x ", i * sizeof(StackType_t));
    }
    ets_printf("\n\n");

    for (i = 0; i < size; i += STACK_VOL_NUM) {
        size_t len = size > i ? size - i : STACK_VOL_NUM - (i - size);

        if (len > STACK_VOL_NUM)
            len = STACK_VOL_NUM;

        ets_printf("%-10x", &start[i]);

        for (j = 0; j < len; j++) {
            ets_printf("%08x ",start[i + j]);
        }
        ets_printf("\n");
    }
}

/*
 * @brief output xtensa register value map when crash
 * 
 * @param frame xtensa register value map pointer
 * 
 * @return none
 */
void IRAM_ATTR panicHandler(void *frame)
{
    // for panic the function that disable cache
    Cache_Read_Enable_New();

    int *regs = (int *)frame;
    int x, y;
    const char *sdesc[] = {
        "PC",   "PS",   "A0",   "A1",
        "A2",   "A3",   "A4",   "A5",
        "A6",   "A7",   "A8",   "A9",
        "A10",  "A11",  "A12",  "A13",
        "A14",  "A15",  "SAR",  "EXCCAUSE"
    };

    extern int _Pri_3_NMICount;

    /* NMI can interrupt exception. */
    ETS_INTR_LOCK();

    if (_Pri_3_NMICount == -1) {
        ets_printf("\nWatch dog triggle:\n\n");
        show_critical_info();
    } else if (xPortInIsrContext()) {
        ets_printf("\nISR:\n\n");
    } else {
        task_info_t *task = xTaskGetCurrentTaskHandle();

        if (task) {
            
            StackType_t *pdata = task->pxStack;
            StackType_t *end = task->pxEndOfStack + 1;

            ets_printf("\nTask stack [%s] stack from [%p] to [%p], total [%d] size\n\n",
                        task->pcTaskName, pdata, end, end - pdata + 1);

            panic_stack(pdata, end);

            ets_printf("\n");
        } else {
            ets_printf("\nNo task\n\n");
        }
    }

    for (x = 0; x < 20; x += 4) {
        for (y = 0; y < 4; y++) {
            ets_printf("%8s: 0x%08x  ", sdesc[x + y], regs[x + y + 1]);
        }
        ets_printf("\n");
    }

    /*
     * Todo: add more option to select here to 'Kconfig':
     *     1. blocking
     *     2. restart
     *     3. GBD break
     */
    while (1);
}

void _esp_error_check_failed(esp_err_t rc, const char *file, int line, const char *function, const char *expression)
{
    printf("ESP_ERROR_CHECK failed: esp_err_t 0x%x at %p\n", rc, __builtin_return_address(0));
    printf("file: \"%s\" line %d\nfunc: %s\nexpression: %s\n", file, line, function, expression);
    abort();
}
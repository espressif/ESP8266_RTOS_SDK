/*
 * FreeRTOS Kernel V10.0.1
 * Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 * Additions Copyright 2018 Espressif Systems (Shanghai) PTE LTD
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/* Scheduler includes. */

#include <stdint.h>

#include <xtensa/config/core.h>
#include <xtensa/tie/xt_interrupt.h>
#include <xtensa/tie/xt_timer.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/xtensa_rtos.h"

#include "esp_attr.h"
#include "esp_libc.h"

#include "esp8266/eagle_soc.h"
#include "rom/ets_sys.h"
#include "esp8266/rom_functions.h"

#define PORT_ASSERT(x) do { if (!(x)) {ets_printf("%s %u\n", "rtos_port", __LINE__); while(1){}; }} while (0)

extern char NMIIrqIsOn;
static uint8_t HdlMacSig = 0;
static uint8_t SWReq = 0;

unsigned cpu_sr;

/* Each task maintains its own interrupt status in the critical nesting
variable. */
static uint32_t uxCriticalNesting = 0;

void vPortEnterCritical(void);
void vPortExitCritical(void);

void _xt_timer_int1(void);


/*
 * See header file for description.
 */
StackType_t *pxPortInitialiseStack(StackType_t *pxTopOfStack, pdTASK_CODE pxCode, void *pvParameters)
{
#define SET_STKREG(r,v) sp[(r) >> 2] = (unsigned long)(v)
    unsigned long *sp, *tp, *stk = (unsigned long *)pxTopOfStack;

    /* Create interrupt stack frame aligned to 16 byte boundary */
    sp = (unsigned long *)(((INT32U)(stk + 1) - XT_CP_SIZE - XT_STK_FRMSZ) & ~0xf);

    /* Clear the entire frame (do not use memset() because we don't depend on C library) */
    for (tp = sp; tp <= stk; ++tp) {
        *tp = 0;
    }

    /* Explicitly initialize certain saved registers */
    SET_STKREG(XT_STK_PC,   pxCode);                        /* task entrypoint                  */
    SET_STKREG(XT_STK_A0,   0);                             /* to terminate GDB backtrace       */
    SET_STKREG(XT_STK_A1,   (INT32U)sp + XT_STK_FRMSZ);     /* physical top of stack frame      */
    SET_STKREG(XT_STK_A2,   pvParameters);                  /* parameters      */
    SET_STKREG(XT_STK_EXIT, _xt_user_exit);                 /* user exception exit dispatcher   */

    /* Set initial PS to int level 0, EXCM disabled ('rfe' will enable), user mode. */
#ifdef __XTENSA_CALL0_ABI__
    SET_STKREG(XT_STK_PS,      PS_UM | PS_EXCM);
#else
    /* + for windowed ABI also set WOE and CALLINC (pretend task was 'call4'd). */
    SET_STKREG(XT_STK_PS,      PS_UM | PS_EXCM | PS_WOE | PS_CALLINC(1));
#endif

    return (StackType_t *)sp;
}

void IRAM_ATTR PendSV(char req)
{
    if (req == 1) {
        vPortEnterCritical();
        SWReq = 1;
        xthal_set_intset(1 << ETS_SOFT_INUM);
        vPortExitCritical();
    } else if (req == 2) {
        HdlMacSig = 1;
        xthal_set_intset(1 << ETS_SOFT_INUM);
    }
}

void IRAM_ATTR HDL_MAC_SIG_IN_LV1_ISR(void)
{
    PendSV(2);
}

extern portBASE_TYPE MacIsrSigPostDefHdl(void);

void SoftIsrHdl(void* arg)
{
    ETS_NMI_LOCK();

    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    if (HdlMacSig == 1) {
        HdlMacSig = 0;
        xHigherPriorityTaskWoken = MacIsrSigPostDefHdl();
    }

    if (xHigherPriorityTaskWoken || (SWReq == 1)) {
        _xt_timer_int1();
        SWReq = 0;
    }

    ETS_NMI_UNLOCK();
}

void xPortSysTickHandle(void)
{
    if (xTaskIncrementTick() != pdFALSE) {
        vTaskSwitchContext();
    }
}

/*
 * See header file for description.
 */
portBASE_TYPE xPortStartScheduler(void)
{
    /*
     * TAG 1.2.3 FreeRTOS call "portDISABLE_INTERRUPTS" at file tasks.c line 1973, this is not at old one.
     * This makes it to be a wrong value.
     * 
     * So we should initialize global value "cpu_sr" with a right value.
     * 
     * Todo: Remove this one when refactor startup function.
     */
    cpu_sr = 0x20;

    /*******software isr*********/
    _xt_isr_attach(ETS_SOFT_INUM, SoftIsrHdl, NULL);
    _xt_isr_unmask(1 << ETS_SOFT_INUM);

    /* Initialize system tick timer interrupt and schedule the first tick. */
    _xt_tick_divisor_init();
    _xt_tick_timer_init();

    vTaskSwitchContext();

    /* Restore the context of the first task that is going to run. */
    _xt_enter_first_task();

    /* Should not get here as the tasks are now running! */
    return pdTRUE;
}

void vPortEndScheduler(void)
{
    /* It is unlikely that the CM3 port will require this function as there
    is nothing to return to.  */
}
/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/

static char ClosedLv1Isr = 0;

void IRAM_ATTR vPortEnterCritical(void)
{
    if (NMIIrqIsOn == 0) {
        if (ClosedLv1Isr != 1) {
            portDISABLE_INTERRUPTS();
            ClosedLv1Isr = 1;
        }
        uxCriticalNesting++;
    }
}
/*-----------------------------------------------------------*/

void IRAM_ATTR vPortExitCritical(void)
{
    if (NMIIrqIsOn == 0) {
        if (uxCriticalNesting > 0) {
            uxCriticalNesting--;

            if (uxCriticalNesting == 0) {
                if (ClosedLv1Isr == 1) {
                    ClosedLv1Isr = 0;
                    portENABLE_INTERRUPTS();
                }
            }
        } else {
            ets_printf(DRAM_STR("E:C:%u\n"), uxCriticalNesting);
            PORT_ASSERT((uxCriticalNesting > 0));
        }
    }
}

void show_critical_info(void)
{
    ets_printf("ShowCritical:%u\n", uxCriticalNesting);
    ets_printf("HdlMacSig:%u\n", HdlMacSig);
    ets_printf("SWReq:%u\n", SWReq);
}

void IRAM_ATTR vPortETSIntrLock(void)
{
    ETS_INTR_LOCK();
}

void IRAM_ATTR vPortETSIntrUnlock(void)
{
    ETS_INTR_UNLOCK();
}

void PortDisableInt_NoNest(void)
{
    if (NMIIrqIsOn == 0) {
        if (ClosedLv1Isr != 1) {
            portDISABLE_INTERRUPTS();
            ClosedLv1Isr = 1;
        }
    }
}

void PortEnableInt_NoNest(void)
{
    if (NMIIrqIsOn == 0) {
        if (ClosedLv1Isr == 1) {
            ClosedLv1Isr = 0;
            portENABLE_INTERRUPTS();
        }
    }
}

/*-----------------------------------------------------------*/
void ResetCcountVal(unsigned int cnt_val)
{
    asm volatile("wsr a2, ccount");
}

/*
 * @brief check if CPU core interrupt is disable
 */
bool interrupt_is_disable(void)
{
    uint32_t tmp;

    __asm__ __volatile__ (
        "rsr %0, PS\n"
        : "=a"(tmp) : : "memory");

    return tmp & 0xFUL ? true : false;
}

_xt_isr_entry isr[16];
char _xt_isr_status = 0;

void _xt_isr_attach(uint8_t i, _xt_isr func, void* arg)
{
    isr[i].handler = func;
    isr[i].arg = arg;
}

uint16_t _xt_isr_handler(uint16_t i)
{
    uint8_t index;

    if (i & (1 << ETS_WDT_INUM)) {
        index = ETS_WDT_INUM;
    } else if (i & (1 << ETS_GPIO_INUM)) {
        index = ETS_GPIO_INUM;
    } else {
        index = __builtin_ffs(i) - 1;

        if (index == ETS_MAX_INUM) {
            i &= ~(1 << ETS_MAX_INUM);
            index = __builtin_ffs(i) - 1;
        }
    }

    _xt_clear_ints(1 << index);

    _xt_isr_status = 1;
    isr[index].handler(isr[index].arg);
    _xt_isr_status = 0;

    return i & ~(1 << index);
}

int xPortInIsrContext(void)
{
    return _xt_isr_status != 0;
}

void __attribute__((weak, noreturn)) vApplicationStackOverflowHook(xTaskHandle xTask, const char *pcTaskName)
{
    ets_printf("***ERROR*** A stack overflow in task %s has been detected.\r\n", pcTaskName);

    abort();
}

signed portBASE_TYPE xTaskGenericCreate(TaskFunction_t pxTaskCode,
                                        const signed char * const pcName,
                                        unsigned short usStackDepth,
                                        void *pvParameters,
                                        unsigned portBASE_TYPE uxPriority,
                                        TaskHandle_t *pxCreatedTask,
                                        StackType_t *puxStackBuffer,
                                        const MemoryRegion_t * const xRegions)
{
    (void)puxStackBuffer;
    (void)xRegions;
    return xTaskCreate(pxTaskCode, (const char * const)pcName, usStackDepth,
                       pvParameters, uxPriority, pxCreatedTask);
}

BaseType_t xQueueGenericReceive(QueueHandle_t xQueue, void * const pvBuffer,
                                TickType_t xTicksToWait, const BaseType_t xJustPeeking)
{
    configASSERT(xJustPeeking == 0);
    return xQueueReceive(xQueue, pvBuffer, xTicksToWait);
}

void vApplicationIdleHook(void)
{
    extern void pmIdleHook(void);
    extern void esp_task_wdt_reset(void);

    pmIdleHook();
    esp_task_wdt_reset();
}

uint32_t xPortGetTickRateHz(void)
{
    return (uint32_t)configTICK_RATE_HZ;
}

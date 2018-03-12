/*
    FreeRTOS V7.5.2 - Copyright (C) 2013 Real Time Engineers Ltd.

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that has become a de facto standard.             *
     *                                                                       *
     *    Help yourself get started quickly and support the FreeRTOS         *
     *    project by purchasing a FreeRTOS tutorial book, reference          *
     *    manual, or both from: http://www.FreeRTOS.org/Documentation        *
     *                                                                       *
     *    Thank you!                                                         *
     *                                                                       *
    ***************************************************************************

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>!AND MODIFIED BY!<< the FreeRTOS exception.

    >>! NOTE: The modification to the GPL is included to allow you to distribute
    >>! a combined work that includes FreeRTOS without being obliged to provide
    >>! the source code for proprietary components outside of the FreeRTOS
    >>! kernel.

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available from the following
    link: http://www.freertos.org/a00114.html

    1 tab == 4 spaces!

    ***************************************************************************
     *                                                                       *
     *    Having a problem?  Start by reading the FAQ "My application does   *
     *    not run, what could be wrong?"                                     *
     *                                                                       *
     *    http://www.FreeRTOS.org/FAQHelp.html                               *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org - Documentation, books, training, latest versions,
    license and Real Time Engineers Ltd. contact details.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.OpenRTOS.com - Real Time Engineers ltd license FreeRTOS to High
    Integrity Systems to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

/*-----------------------------------------------------------
 * Implementation of functions defined in portable.h for the ARM CM3 port.
 *----------------------------------------------------------*/

/* Scheduler includes. */
#include <xtensa/config/core.h>
#include <xtensa/tie/xt_interrupt.h>
#include <xtensa/tie/xt_timer.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/xtensa_rtos.h"

#define PORT_ASSERT(x) do { if (!(x)) {ets_printf("%s %u\n", "rtos_port", __LINE__); while(1){}; }} while (0)

extern char NMIIrqIsOn;
static char HdlMacSig = 0;
static char SWReq = 0;

unsigned cpu_sr;

/* Each task maintains its own interrupt status in the critical nesting
variable. */
static unsigned portBASE_TYPE uxCriticalNesting = 0;

void vPortEnterCritical(void);
void vPortExitCritical(void);
/*
 * See header file for description.
 */
portSTACK_TYPE* ICACHE_FLASH_ATTR
pxPortInitialiseStack(portSTACK_TYPE* pxTopOfStack, pdTASK_CODE pxCode, void* pvParameters)
{
#define SET_STKREG(r,v) sp[(r) >> 2] = (portSTACK_TYPE)(v)
    portSTACK_TYPE* sp, *tp;

    /* Create interrupt stack frame aligned to 16 byte boundary */
    sp = (portSTACK_TYPE*)(((INT32U)(pxTopOfStack + 1) - XT_CP_SIZE - XT_STK_FRMSZ) & ~0xf);

    /* Clear the entire frame (do not use memset() because we don't depend on C library) */
    for (tp = sp; tp <= pxTopOfStack; ++tp) {
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

    return sp;
}

void PendSV(char req)
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
portBASE_TYPE ICACHE_FLASH_ATTR
xPortStartScheduler(void)
{
    /*******software isr*********/
    _xt_isr_attach(ETS_SOFT_INUM, SoftIsrHdl, NULL);
    _xt_isr_unmask(1 << ETS_SOFT_INUM);

    /* Initialize system tick timer interrupt and schedule the first tick. */
    _xt_tick_timer_init();

    vTaskSwitchContext();

    /* Restore the context of the first task that is going to run. */
    XT_RTOS_INT_EXIT();

    /* Should not get here as the tasks are now running! */
    return pdTRUE;
}

void ICACHE_FLASH_ATTR
vPortEndScheduler(void)
{
    /* It is unlikely that the CM3 port will require this function as there
    is nothing to return to.  */
}
/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/

static char ClosedLv1Isr = 0;

void vPortEnterCritical(void)
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

void vPortExitCritical(void)
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
            ets_printf("E:C:%d\n", uxCriticalNesting);
            PORT_ASSERT((uxCriticalNesting > 0));
        }
    }
}

void ICACHE_FLASH_ATTR ShowCritical(void)
{
    os_printf("ShowCritical:%d\n", uxCriticalNesting);
    os_printf("HdlMacSig:%d\n", HdlMacSig);
    os_printf("SWReq:%d\n", SWReq);

    ets_delay_us(50000);
}

void vPortETSIntrLock(void)
{
    ETS_INTR_LOCK();
}

void vPortETSIntrUnlock(void)
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
void ICACHE_FLASH_ATTR ResetCcountVal(unsigned int cnt_val)
{
    asm volatile("wsr a2, ccount");
}

_xt_isr_entry isr[16];
char _xt_isr_status = 0;

void ICACHE_FLASH_ATTR
_xt_isr_attach(uint8 i, _xt_isr func, void* arg)
{
    isr[i].handler = func;
    isr[i].arg = arg;
}

uint16 _xt_isr_handler(uint16 i)
{
    uint8 index;

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

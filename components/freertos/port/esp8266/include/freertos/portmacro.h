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

#ifndef PORTMACRO_H
#define PORTMACRO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "esp_attr.h"

#include    <xtensa/xtruntime.h>
#include    "xtensa_rtos.h"

#if defined(configUSE_NEWLIB_REENTRANT) && configUSE_NEWLIB_REENTRANT == 1
#ifndef CONFIG_NEWLIB_LIBRARY_CUSTOMER
#include "esp_newlib.h"

#define _impure_ptr _global_impure_ptr

#undef _REENT_INIT_PTR
#define _REENT_INIT_PTR(p) esp_reent_init(p)
#endif
#endif

/*-----------------------------------------------------------
 * Port specific definitions.
 *
 * The settings in this file configure FreeRTOS correctly for the
 * given hardware and compiler.
 *
 * These settings should not be altered.
 *-----------------------------------------------------------
 */

/* Type definitions. */
#define portCHAR        char
#define portFLOAT       float
#define portDOUBLE      double
#define portLONG        long
#define portSHORT       short
#define portSTACK_TYPE  unsigned char
#define portBASE_TYPE   long

#define BaseType_t      portBASE_TYPE
#define TickType_t      unsigned portLONG
#define UBaseType_t     unsigned portBASE_TYPE
#define StackType_t     portSTACK_TYPE

typedef unsigned portLONG portTickType;
typedef unsigned int INT32U;
#define portMAX_DELAY ( portTickType ) 0xffffffff
/*-----------------------------------------------------------*/

/* Architecture specifics. */
#define portSTACK_GROWTH			( -1 )
#define portTICK_PERIOD_MS			( ( portTickType ) 1000 / configTICK_RATE_HZ )
#define portBYTE_ALIGNMENT			4
/*-----------------------------------------------------------*/

/* Scheduler utilities. */
extern void PendSV(int req);
//#define portYIELD()	vPortYield()
#define portYIELD()	PendSV(1)

#if 0
#define portEND_SWITCHING_ISR( xSwitchRequired ) \
	if(xSwitchRequired) PendSV(1)
#endif

/* Task utilities. */
#define portEND_SWITCHING_ISR( xSwitchRequired ) 	\
{													\
extern void vTaskSwitchContext( void );				\
													\
	if( xSwitchRequired ) 							\
	{												\
		vTaskSwitchContext();						\
	}												\
}


/*-----------------------------------------------------------*/
extern unsigned cpu_sr;

/* Critical section management. */
extern void vPortEnterCritical( void );
extern void vPortExitCritical( void );

//DYC_ISR_DBG
void PortDisableInt_NoNest( void );
void PortEnableInt_NoNest( void );

/* Disable interrupts, saving previous state in cpu_sr */
#define  portDISABLE_INTERRUPTS() \
            __asm__ volatile ("rsil %0, " XTSTR(XCHAL_EXCM_LEVEL) : "=a" (cpu_sr) :: "memory")

/* Restore interrupts to previous level saved in cpu_sr */
#define  portENABLE_INTERRUPTS() __asm__ volatile ("wsr %0, ps" :: "a" (cpu_sr) : "memory")

#define portENTER_CRITICAL()                vPortEnterCritical()
#define portEXIT_CRITICAL()                 vPortExitCritical()

#define xPortGetCoreID()                    0

// no need to disable/enable lvl1 isr again in ISR
//#define  portSET_INTERRUPT_MASK_FROM_ISR()		PortDisableInt_NoNest()
//#define  portCLEAR_INTERRUPT_MASK_FROM_ISR(x)		PortEnableInt_NoNest()


/*-----------------------------------------------------------*/

/* Tickless idle/low power functionality. */

/*-----------------------------------------------------------*/

/* Port specific optimisations. */

/*-----------------------------------------------------------*/

/* Task function macros as described on the FreeRTOS.org WEB site.  These are
not necessary for to use this port.  They are defined so the common demo files
(which build with all the ports) will build. */
#define portTASK_FUNCTION_PROTO( vFunction, pvParameters ) void vFunction( void *pvParameters )
#define portTASK_FUNCTION( vFunction, pvParameters ) void vFunction( void *pvParameters )
/*-----------------------------------------------------------*/

void        _xt_user_exit           (void);
void        _xt_tick_timer_init   (void);
void        _xt_isr_unmask       (uint32_t unmask);
void        _xt_isr_mask       (uint32_t mask);

/* interrupt related */
typedef void (* _xt_isr)(void *arg);

void        _xt_isr_attach          (uint8_t i, _xt_isr func, void *arg);

typedef struct _xt_isr_entry_ {
    _xt_isr handler;
    void *  arg;
} _xt_isr_entry;

void show_critical_info(void);

/*
 * @brief add trace information to allocated memory
 * 
 * @param ptr memory pointer allocated by "os_maloc", "malloc" and so on
 * @param trace trace information, file name(__ESP_FILE__) or "__builtin_return_address(0)" is OK
 * @param no number of trace information, file line(__LINE__) or -1(using "__builtin_return_address(0)")
 */
void esp_mem_trace(const void *ptr, const char *trace, int no);

/*
 * @brief add file trace information to allocated memory
 * 
 * @param ptr memory pointer allocated by "os_maloc", "malloc" and so on
 */
#define esp_mem_mark_file(ptr) esp_mem_trace((ptr), __ESP_FILE__, LINE__)

/*
 * @brief check if CPU core interrupt is disable
 *
 * @return true if interrupt is disable or false
 */
bool interrupt_is_disable(void);

/* Get tick rate per second */
uint32_t xPortGetTickRateHz(void);

void _xt_enter_first_task(void);

void esp_increase_tick_cnt(const TickType_t ticks);

/* API compatible with esp-idf  */
#define xTaskCreatePinnedToCore(pvTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pvCreatedTask, tskNO_AFFINITY) \
        xTaskCreate(pvTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pvCreatedTask)

extern void esp_vApplicationIdleHook( void );
extern void esp_vApplicationTickHook( void );

extern const uint32_t g_esp_ticks_per_us;

/*
 * @brief Get FreeRTOS system idle ticks
 *
 * @return idle ticks
 */
TickType_t prvGetExpectedIdleTime(void);

#ifdef __cplusplus
}
#endif

#endif /* PORTMACRO_H */


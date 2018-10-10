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
#if defined(CONFIG_NEWLIB_LIBRARY_LEVEL_NORMAL) || defined(CONFIG_NEWLIB_LIBRARY_LEVEL_NANO)
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
extern void PendSV(char req);
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
uint32_t    _xt_read_ints (void);
void        _xt_clear_ints(uint32_t mask);

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

#ifdef __cplusplus
}
#endif

#endif /* PORTMACRO_H */


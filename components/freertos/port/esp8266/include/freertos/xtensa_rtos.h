/*******************************************************************************
Copyright (c) 2006-2009 by Tensilica Inc.  ALL RIGHTS RESERVED.
These coded instructions, statements, and computer programs are the
copyrighted works and confidential proprietary information of Tensilica Inc.
They may not be modified, copied, reproduced, distributed, or disclosed to
third parties in any manner, medium, or form, in whole or in part, without
the prior written consent of Tensilica Inc.
--------------------------------------------------------------------------------

        RTOS-SPECIFIC INFORMATION FOR XTENSA RTOS ASSEMBLER SOURCES

This header is the primary glue between generic Xtensa RTOS support
sources and a specific RTOS port for Xtensa.  It contains definitions
and macros for use primarily by Xtensa assembly coded source files.

Macros in this header map callouts from generic Xtensa files to specific
RTOS functions. It may also be included in C source files.

Xtensa RTOS ports support all RTOS-compatible configurations of the Xtensa 
architecture, using the Xtensa hardware abstraction layer (HAL) to deal 
with configuration specifics.

Should be included by all Xtensa generic and RTOS port-specific sources.

*******************************************************************************/

#ifndef XTENSA_RTOS_H
#define XTENSA_RTOS_H

#ifdef __ASSEMBLER__
#include    <xtensa/coreasm.h>
#else
#include    <xtensa/config/core.h>
#endif

#include    <xtensa/corebits.h>
#include    <xtensa/config/system.h>
#include    <xtensa/simcall.h>

/*
Include any RTOS specific definitions that are needed by this header.
*/

#ifdef XCHAL_EXCM_LEVEL
#undef XCHAL_EXCM_LEVEL
#define XCHAL_EXCM_LEVEL 3
#endif

/*
Name of RTOS (for messages).
*/
#define XT_RTOS_NAME    FreeRTOS

/*
Check some Xtensa configuration requirements and report error if not met.
Error messages can be customize to the RTOS port.
*/

#if !XCHAL_HAVE_XEA2
#error "FreeRTOS/Xtensa requires XEA2 (exception architecture 2)."
#endif


/*******************************************************************************

RTOS CALLOUT MACROS MAPPED TO RTOS PORT-SPECIFIC FUNCTIONS.

Define callout macros used in generic Xtensa code to interact with the RTOS.
The macros are simply the function names for use in calls from assembler code.
Some of these functions may call back to generic functions in xtensa_context.h .

*******************************************************************************/

/*
Inform RTOS of entry into an interrupt handler that will affect it. 
Allows RTOS to manage switch to any system stack and count nesting level.
Called after minimal context has been saved, with interrupts disabled.
RTOS port can call0 _xt_context_save to save the rest of the context.
May only be called from assembly code by the 'call0' instruction.
*/
#define XT_RTOS_INT_ENTER   _xt_int_enter
#ifndef __ASSEMBLER__
void XT_RTOS_INT_ENTER();
#endif

/*
Inform RTOS of completion of an interrupt handler, and give control to
RTOS to perform thread/task scheduling, switch back from any system stack
and restore the context, and return to the exit dispatcher saved in the
stack frame at XT_STK_EXIT. RTOS port can call0 _xt_context_restore
to save the context saved in XT_RTOS_INT_ENTER via _xt_context_save,
leaving only a minimal part of the context to be restored by the exit
dispatcher. This function does not return to the place it was called from.
May only be called from assembly code by the 'call0' instruction.
*/
#define XT_RTOS_INT_EXIT    _xt_int_exit
#ifndef __ASSEMBLER__
void XT_RTOS_INT_EXIT();
#endif

/*
Inform RTOS of the occurrence of a tick timer interrupt.
If RTOS has no tick timer, leave XT_RTOS_TIMER_INT undefined.
May be coded in or called from C or assembly, per ABI conventions.
RTOS may optionally define XT_TICK_PER_SEC in its own way (eg. macro).
*/
#define XT_RTOS_TIMER_INT   _xt_timer_int
#ifndef __ASSEMBLER__
void XT_RTOS_TIMER_INT();
#endif

/*
Return in a15 the base address of the co-processor state save area for the 
thread that triggered a co-processor exception, or 0 if no thread was running.
The state save area is structured as defined in xtensa_context.h and has size 
XT_CP_SIZE. Co-processor instructions should only be used in thread code, never
in interrupt handlers or the RTOS kernel. May only be called from assembly code
and by the 'call0' instruction. A result of 0 indicates an unrecoverable error. 
The implementation may use only a2-4, a15 (all other regs must be preserved).
*/
// void* XT_RTOS_CP_STATE(void)

/*******************************************************************************

HOOKS TO DYNAMICALLY INSTALL INTERRUPT AND EXCEPTION HANDLERS PER LEVEL.

This Xtensa RTOS port provides hooks for dynamically installing exception
and interrupt handlers to facilitate automated testing where each test
case can install its own handler for user exceptions and each interrupt
priority (level). This consists of an array of function pointers indexed
by interrupt priority, with index 0 being the user exception handler hook.
Each entry in the array is initially 0, and may be replaced by a function 
pointer of type XT_INTEXC_HOOK. A handler may be uninstalled by installing 0.

The handler for low and medium priority obeys ABI conventions so may be coded
in C. For the exception handler, the cause is the contents of the EXCCAUSE
reg, and the result is -1 if handled, else the cause (still needs handling).
For interrupt handlers, the cause is a mask of pending enabled interrupts at
that level, and the result is the same mask with the bits for the handled
interrupts cleared (those not cleared still need handling). This allows a test
case to either pre-handle or override the default handling for the exception
or interrupt level (see xtensa_vectors.S).

High priority handlers (including NMI) must be coded in assembly, are always
called by 'call0' regardless of ABI, must preserve all registers except a0,
and must not use or modify the interrupted stack. The hook argument 'cause'
is not passed and the result is ignored, so as not to burden the caller with
saving and restoring a2 (it assumes only one interrupt per level - see the
discussion in high priority interrupts in xtensa_vectors.S). The handler
therefore should be coded to prototype 'void h(void)' even though it plugs
into an array of handlers of prototype 'unsigned h(unsigned)'.

To enable interrupt/exception hooks, compile the RTOS with '-DXT_INTEXC_HOOKS'.

*******************************************************************************/

#define XT_INTEXC_HOOK_NUM  (1 + XCHAL_NUM_INTLEVELS + XCHAL_HAVE_NMI)

#ifndef __ASSEMBLER__
typedef unsigned (*XT_INTEXC_HOOK)(unsigned cause);
extern  volatile XT_INTEXC_HOOK _xt_intexc_hooks[XT_INTEXC_HOOK_NUM];
#endif


/*******************************************************************************

CONVENIENCE INCLUSIONS.

Ensures RTOS specific files need only include this one Xtensa-generic header.
These headers are included last so they can use the RTOS definitions above.

*******************************************************************************/

#include    "xtensa_context.h"

#ifdef XT_RTOS_TIMER_INT
#include    "xtensa_timer.h"
#endif


#endif /* XTENSA_RTOS_H */

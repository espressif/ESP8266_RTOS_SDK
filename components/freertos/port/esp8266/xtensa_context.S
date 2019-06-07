/*******************************************************************************
Copyright (c) 2006-2009 by Tensilica Inc.  ALL RIGHTS RESERVED.
These coded instructions, statements, and computer programs are the
copyrighted works and confidential proprietary information of Tensilica Inc.
They may not be modified, copied, reproduced, distributed, or disclosed to
third parties in any manner, medium, or form, in whole or in part, without
the prior written consent of Tensilica Inc.
--------------------------------------------------------------------------------

        XTENSA CONTEXT SAVE AND RESTORE ROUTINES

Low-level Call0 functions for handling generic context save and restore of
registers not specifically addressed by the interrupt vectors and handlers.
Those registers (not handled by these functions) are PC, PS, A0, A1 (SP).
Except for the calls to RTOS functions, this code is generic to Xtensa.

Note that in Call0 ABI, interrupt handlers are expected to preserve the callee-
save regs (A12-A15), which is always the case if the handlers are coded in C.
However A12, A13 are made available as scratch registers for interrupt dispatch
code, so are presumed saved anyway, and are always restored even in Call0 ABI.
Only A14, A15 are truly handled as callee-save regs.

Because Xtensa is a configurable architecture, this port supports all user
generated configurations (except restrictions stated in the release notes).
This is accomplished by conditional compilation using macros and functions
defined in the Xtensa HAL (hardware adaptation layer) for your configuration.
Only the processor state included in your configuration is saved and restored,
including any processor state added by user configuration options or TIE.

*******************************************************************************/

/*  Warn nicely if this file gets named with a lowercase .s instead of .S:  */
#define NOERROR #
NOERROR: .error "C preprocessor needed for this file: make sure its filename\
 ends in uppercase .S, or use xt-xcc's -x assembler-with-cpp option."


#include "freertos/xtensa_rtos.h"

    .section    .text

/*******************************************************************************

_xt_context_save

    !! MUST BE CALLED ONLY BY 'CALL0' INSTRUCTION !!

Saves all Xtensa processor state except PC, PS, A0, A1 (SP), A12, A13, in the
interrupt stack frame defined in xtensa_rtos.h.
Its counterpart is _xt_context_restore (which also restores A12, A13).

Caller is expected to have saved PC, PS, A0, A1 (SP), A12, A13 in the frame.
This function preserves A12 & A13 in order to provide the caller with 2 scratch 
regs that need not be saved over the call to this function. The choice of which
2 regs to provide is governed by xthal_window_spill_nw and xthal_save_extra_nw,
to avoid moving data more than necessary. Caller can assign regs accordingly.

Entry Conditions:
    A0  = Return address in caller.
    A1  = Stack pointer of interrupted thread or handler ("interruptee").
    Original A12, A13 have already been saved in the interrupt stack frame.
    Other processor state except PC, PS, A0, A1 (SP), A12, A13, is as at the 
    point of interruption.
    If windowed ABI, PS.EXCM = 1 (exceptions disabled).

Exit conditions:
    A0  = Return address in caller.
    A1  = Stack pointer of interrupted thread or handler ("interruptee").
    A12, A13 as at entry (preserved).
    If windowed ABI, PS.EXCM = 1 (exceptions disabled).

*******************************************************************************/

    .global _xt_context_save
    .type   _xt_context_save,@function
    .align  4
_xt_context_save:
    s32i    a2,  sp,  XT_STK_A2
    s32i    a3,  sp,  XT_STK_A3
    s32i    a4,  sp,  XT_STK_A4
    s32i    a5,  sp,  XT_STK_A5
    s32i    a6,  sp,  XT_STK_A6
    s32i    a7,  sp,  XT_STK_A7
    s32i    a8,  sp,  XT_STK_A8
    s32i    a9,  sp,  XT_STK_A9
    s32i    a10, sp,  XT_STK_A10
    s32i    a11, sp,  XT_STK_A11

    rsr     a3,  SAR
    s32i    a3,  sp,  XT_STK_SAR

    ret

/*******************************************************************************

_xt_context_restore

    !! MUST BE CALLED ONLY BY 'CALL0' INSTRUCTION !!

Restores all Xtensa processor state except PC, PS, A0, A1 (SP) (and in Call0
ABI, A14, A15 which are preserved by all interrupt handlers) from an interrupt 
stack frame defined in xtensa_rtos.h .
Its counterpart is _xt_context_save (whose caller saved A12, A13).

Caller is responsible to restore PC, PS, A0, A1 (SP).

Entry Conditions:
    A0  = Return address in caller.
    A1  = Stack pointer of interrupted thread or handler ("interruptee").

Exit conditions:
    A0  = Return address in caller.
    A1  = Stack pointer of interrupted thread or handler ("interruptee").
    Other processor state except PC, PS, A0, A1 (SP), is as at the point 
    of interruption.

*******************************************************************************/

    .global _xt_context_restore
    .type   _xt_context_restore,@function
    .align  4
_xt_context_restore:
    l32i    a3,  sp,  XT_STK_SAR
    l32i    a2,  sp,  XT_STK_A2
    wsr     a3,  SAR
    l32i    a3,  sp,  XT_STK_A3
    l32i    a4,  sp,  XT_STK_A4
    l32i    a5,  sp,  XT_STK_A5
    l32i    a6,  sp,  XT_STK_A6
    l32i    a7,  sp,  XT_STK_A7
    l32i    a8,  sp,  XT_STK_A8
    l32i    a9,  sp,  XT_STK_A9
    l32i    a10, sp,  XT_STK_A10
    l32i    a11, sp,  XT_STK_A11

    /*
    Call0 ABI callee-saved regs a12-15 do not need to be restored here.
    However a12-13 were saved for scratch before XT_RTOS_INT_ENTER(), 
    so need to be restored anyway, despite being callee-saved in Call0.
    */
    l32i    a12, sp,  XT_STK_A12
    l32i    a13, sp,  XT_STK_A13

    ret

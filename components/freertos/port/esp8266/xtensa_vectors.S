/*******************************************************************************
Copyright (c) 2006-2008 by Tensilica Inc.  ALL RIGHTS RESERVED.
These coded instructions, statements, and computer programs are the
copyrighted works and confidential proprietary information of Tensilica Inc.
They may not be modified, copied, reproduced, distributed, or disclosed to
third parties in any manner, medium, or form, in whole or in part, without
the prior written consent of Tensilica Inc.
--------------------------------------------------------------------------------

        XTENSA VECTORS AND LOW LEVEL HANDLERS FOR AN RTOS

Xtensa low level exception and interrupt vectors and handlers for an RTOS.

Interrupt handlers and user exception handlers support interaction with
the RTOS by calling XT_RTOS_INT_ENTER and XT_RTOS_INT_EXIT before and
after user's specific interrupt handlers. These macros are defined in
xtensa_<rtos>.h to call suitable functions in a specific RTOS. The user
may insert application-specific handlers at indicated places for each
hardware interrupt priority/level. These handlers may be coded in C and
called from the Xtensa low level handlers. Optional hooks are provided
to install a handler per level at run-time, made available by compiling
this source file with '-DXT_INTEXC_HOOKS' (useful for automated testing).

!!  This file is a template that usually needs to be modified to handle       !!
!!  application specific interrupts. Search USER_EDIT for helpful comments    !!
!!  on where to insert handlers and how to write them.                        !!

Because Xtensa is a configurable architecture, this port supports all user
generated configurations (except restrictions stated in the release notes).
This is accomplished by conditional compilation using macros and functions
defined in the Xtensa HAL (hardware adaptation layer) for your configuration.
Only the relevant parts of this file will be included in your RTOS build.
For example, this file provides interrupt vector templates for all types and
all priority levels, but only the ones in your configuration are built.

NOTES on the use of 'call0' for long jumps instead of 'j':
 1. This file should be assembled with the -mlongcalls option to xt-xcc.
 2. The -mlongcalls compiler option causes 'call0 dest' to be expanded to
    a sequence 'l32r a0, dest' 'callx0 a0' which works regardless of the
    distance from the call to the destination. The linker then relaxes
    it back to 'call0 dest' if it determines that dest is within range.
    This allows more flexibility in locating code without the performance
    overhead of the 'l32r' literal data load in cases where the destination
    is in range of 'call0'. There is an additional benefit in that 'call0'
    has a longer range than 'j' due to the target being word-aligned, so 
    the 'l32r' sequence is less likely needed.
 3. The use of 'call0' with -mlongcalls requires that register a0 not be 
    live at the time of the call, which is always the case for a function 
    call but needs to be ensured if 'call0' is used as a jump in lieu of 'j'.
 4. This use of 'call0' is independent of the C function call ABI.

*******************************************************************************/
    #include <xtensa/coreasm.h>
    #include <xtensa/config/system.h>
    #include <xtensa/config/specreg.h>
    #include <xtensa/xtruntime-frames.h>
    #include "freertos/xtensa_rtos.h"

/*******************************************************************************

Add to compile passing.

*******************************************************************************/

#undef HAVE_XSR
#define HAVE_XSR 1

#define _JOIN2(a, b) a ## b
#define _JOIN3(a, b, c) a ## b ## c

#define JOIN2(a, b) _JOIN2(a, b)
#define JOIN3(a, b, c) _JOIN3(a, b, c)

#define LABEL(a, b) _JOIN3(a, 3, b)

#define EXCSAVE_LEVEL _JOIN2(EXCSAVE_, 3)

/*******************************************************************************/

#define _INTERRUPT_LEVEL 3

STRUCT_BEGIN
STRUCT_FIELD (long,4,HESF_,SAR)
STRUCT_FIELD (long,4,HESF_,EPC1)
STRUCT_FIELD (long,4,HESF_,EXCCAUSE)
STRUCT_FIELD (long,4,HESF_,EXCVADDR)
STRUCT_FIELD (long,4,HESF_,EXCSAVE1)
STRUCT_FIELD (long,4,HESF_,EPC3)
STRUCT_FIELD (long,4,HESF_,EPS3)
STRUCT_AFIELD(long,4,HESF_,AREG, 16)    /* address registers ar0..ar15 */
#define HESF_AR(n)  HESF_AREG+((n)*4)
STRUCT_END(HighPriFrame)
#define HESF_TOTALSIZE  HighPriFrameSize+32 /* 32 bytes for interrupted code's save areas under SP */


#define PRI_N_STACK_SIZE     512    /* default to 1 kB stack for each level-N handling */
#define PRI_N_STACK_SIZE2    256    /* default to 1 kB stack for each level-N handling */

#define PANIC_STK_FRMSZ      (XT_STK_FRMSZ + 0x10)

    .global panicHandler

    //  Allocate save area and stack:
    //  (must use .bss, not .comm, because the subsequent .set does not work otherwise)
    .global _chip_nmi_stk
    .section .bss, "aw"
    .align  16
_chip_nmi_stk:    .space  PRI_N_STACK_SIZE + HESF_TOTALSIZE + PRI_N_STACK_SIZE2 + HESF_TOTALSIZE

    .global LoadStoreErrorHandlerStack
    .balign 16
LoadStoreErrorHandlerStack:
    .word   0       # a0
    .word   0       # (unused)
    .word   0       # a2
    .word   0       # a3
    .word   0       # a4
    .word   0       # a5
    .word   0       # a6

LoadStoreErrorHandlerStack_reentry:
    .word   0       # a0
    .word   0       # (unused)
    .word   0       # a2
    .word   0       # a3
    .word   0       # a4
    .word   0       # a5
    .word   0       # a6

#if HAVE_XSR
    .data
    .global LABEL(_Pri_,_HandlerAddress)
LABEL(_Pri_,_HandlerAddress):   .space 4
    .global _chip_nmi_cnt
_chip_nmi_cnt:   .space 4
#endif

    .section    .data, "aw"
    .global     _chip_interrupt_stk, _chip_interrupt_tmp
    .align      16
_chip_interrupt_stk:
    .space       CONFIG_FREERTOS_ISR_STACKSIZE
_chip_interrupt_tmp:
    .word        0

/*************************** LoadStoreError Handler **************************/

        .section    .text

/* Xtensa "Load/Store Exception" handler:
 * Completes L8/L16 load instructions from Instruction address space, for which
 * the architecture only supports 32-bit reads.
 *
 * Called from UserExceptionVector if EXCCAUSE is LoadStoreErrorCause
 *
 * (Fast path (no branches) is for L8UI)
 */
        .literal_position
        .type   LoadStoreErrorHandler, @function
        .balign 4
LoadStoreErrorHandler:
        rsr     a0,  excsave1                       # restore a0 saved by UserExceptionVector
        wsr     a1,  excsave1                       # save a1 to excsave1, a1 can be used as varalbe

        movi    a1,  _chip_nmi_cnt
        l32i    a1,  a1,  0

        bnez    a1,  LoadStoreErrorHandler_reentry
        movi    sp,  LoadStoreErrorHandlerStack
        j       LoadStoreErrorHandler_common
LoadStoreErrorHandler_reentry:
        movi    sp,  LoadStoreErrorHandlerStack_reentry
LoadStoreErrorHandler_common:

        /* Registers are saved in the address corresponding to their register
         * number times 4.  This allows a quick and easy mapping later on when
         * needing to store the value to a particular register number. */

        s32i    a0,  sp,  0
        s32i    a2,  sp,  0x08
        s32i    a3,  sp,  0x0c
        s32i    a4,  sp,  0x10
        rsr     a0,  sar                            # Save SAR in a0 to restore later

        /* Examine the opcode which generated the exception */
        /* Note: Instructions are in this order to avoid pipeline stalls. */
        rsr     a2, epc1
        movi    a3, ~3
        ssa8l   a2                                  # sar is now correct shift for aligned read
        and     a2,  a2,  a3                        # a2 now 4-byte aligned address of instruction
        l32i    a4,  a2,  0
        l32i    a2,  a2,  4
        movi    a3,  0x00700F                       # opcode mask for l8ui/l16si/l16ui
        src     a2,  a2,  a4                        # a2 now instruction that failed
        and     a3,  a2,  a3                        # a3 is masked instruction

        # This is store instruction
        movi   a4, 0x004002
        beq    a3, a4, .LSE_check_s8i_store         # s8i

        movi    a4,  0x005002
        beq     a3,  a4, .LSE_check_s16i_store      # s16i

        bnei    a3,  0x000002,  .LSE_check_l16

        /* Note: At this point, opcode could technically be one of two things:
         *   xx0xx2 (L8UI)
         *   xx8xx2 (Reserved (invalid) opcode)
         * It is assumed that we'll never get to this point from an illegal
         * opcode, so we don't bother to check for that case and presume this
         * is always an L8UI. */

        movi    a4,  ~3
        rsr     a3,  excvaddr                       # read faulting address
        and     a4,  a3,  a4                        # a4 now word aligned read address

        l32i    a4,  a4,  0                         # perform the actual read
        ssa8l   a3                                  # sar is now shift to extract a3's byte
        srl     a3,  a4                             # shift right correct distance
        extui   a4,  a3,  0,  8                     # mask off bits we need for an l8

.LSE_post_fetch:
        /* We jump back here after either the L8UI or the L16*I routines do the
         * necessary work to read the value from memory.
         * At this point, a2 holds the faulting instruction and a4 holds the
         * correctly read value.

         * Restore original SAR value (saved in a0) and update EPC so we'll
         * return back to the instruction following the one we just emulated */

        /* Note: Instructions are in this order to avoid pipeline stalls */
        rsr     a3,  epc1
        wsr     a0,  sar
        addi    a3,  a3,  0x3
        wsr     a3,  epc1

        /* Stupid opcode tricks: The jumptable we use later on needs 16 bytes
         * per entry (so we can avoid a second jump by just doing a RFE inside
         * each entry).  Unfortunately, however, Xtensa doesn't have an addx16
         * operation to make that easy for us.  Luckily, all of the faulting
         * opcodes we're processing are guaranteed to have bit 3 be zero, which
         * means if we just shift the register bits of the opcode down by 3
         * instead of 4, we will get the register number multiplied by 2.  This
         * combined with an addx8 will give us an effective addx16 without
         * needing any extra shift operations. */
        extui   a2,  a2,  3,  5                     # a2 is now destination register 0-15 times 2

        bgei    a2,  10, .LSE_assign_reg            # a5..a15 use jumptable
        beqi    a2,  2, .LSE_assign_a1              # a1 uses a special routine

        /* We're storing into a0 or a2..a4, which are all saved in our "stack"
         * area.  Calculate the correct address and stick the value in there,
         * then just do our normal restore and RFE (no jumps required, which
         * actually makes a0..a4 substantially faster). */
        addx2   a2,  a2,  sp
        s32i    a4,  a2,  0

        /* Restore all regs and return */
        l32i    a0,  sp,  0
        l32i    a2,  sp,  0x08
        l32i    a3,  sp,  0x0c
        l32i    a4,  sp,  0x10
        rsr     a1,  excsave1                       # restore a1 saved by UserExceptionVector
        rfe

.LSE_assign_reg:
        /* At this point, a2 contains the register number times 2, a4 is the
         * read value. */

        /* Calculate the jumptable address, and restore all regs except a2 and
         * a4 so we have less to do after jumping. */
        /* Note: Instructions are in this order to avoid pipeline stalls. */
        movi    a3,  .LSE_jumptable_base
        l32i    a0,  sp,  0
        addx8   a2,  a2,  a3                        # a2 is now the address to jump to
        l32i    a3,  sp,  0x0c

        jx      a2

        .balign 4
.LSE_check_l16:
        /* At this point, a2 contains the opcode, a3 is masked opcode */
        movi    a4,  0x001002                       # l16si or l16ui opcode after masking
        bne     a3,  a4,  .LSE_wrong_opcode

        /* Note: At this point, the opcode could be one of two things:
         *   xx1xx2 (L16UI)
         *   xx9xx2 (L16SI)
         * Both of these we can handle. */

        movi    a4,  ~3
        rsr     a3,  excvaddr                       # read faulting address
        and     a4,  a3,  a4                        # a4 now word aligned read address

        l32i    a4,  a4,  0                         # perform the actual read
        ssa8l   a3                                  # sar is now shift to extract a3's bytes
        srl     a3,  a4                             # shift right correct distance
        extui   a4,  a3,  0, 16                     # mask off bits we need for an l16

        bbci    a2,  15,  .LSE_post_fetch           # Not a signed op
        bbci    a4,  15,  .LSE_post_fetch           # Value does not need sign-extension

        movi    a3, 0xFFFF0000
        or      a4,  a3,  a4                        # set 32-bit sign bits
        j       .LSE_post_fetch

        .balign 4
.LSE_check_s8i_store:
        s32i    a5,  sp,  0x14
        s32i    a6,  sp,  0x18
        movi    a5,0xff
        j       .LSE_check_store

.LSE_check_s16i_store:
        s32i    a5,  sp,  0x14
        s32i    a6,  sp,  0x18
        movi    a5,  0xffff
        j       .LSE_check_store

.LSE_check_store:
        movi    a4, ~3
        rsr     a3, excvaddr                        # write faulting address
        and     a4, a3, a4                          # a4 now word aligned write address
        ssa8b   a3
        l32i    a3,  a4,  0                         # perform the actual read

        mov     a4,  a5
        sll     a4,  a4
        movi    a6,  -1
        xor     a4,  a6,  a4
        and     a3,  a3,  a4

        movi    a4,  ~3
        rsr     a6,  excvaddr                       # write faulting address
        and     a4,  a6, a4                         # a4 now word aligned write address

        extui   a2,  a2,  4,  4                     # a2 is now destination register 0-15 times 2

        bgei    a2,  7,   .LSE_big_reg
        movi    a6,  4
        mull    a6,  a2,  a6
        add     a2,  a6,  sp
        l32i    a2,  a2,  0
        j       .Write_data

.LSE_big_reg:
        movi    a6,  7
        sub     a2,  a2,  a6
        movi    a6,  8
        mull    a2,  a2,  a6

        movi    a6,.LSE_big_reg_table
        add     a2,  a2,  a6
        jx      a2

.balign 4
.LSE_big_reg_table:
        .org    .LSE_big_reg_table + (0*(2*4))
        mov     a2,  a7
        j       .Write_data

        .org    .LSE_big_reg_table + (1*(2*4))
        mov     a2,  a8
        j       .Write_data

        .org    .LSE_big_reg_table + (2*(2*4))
        mov     a2,  a9
        j       .Write_data

        .org    .LSE_big_reg_table + (3*(2*4))
        mov     a2,  a10
        j       .Write_data

        .org    .LSE_big_reg_table + (4*(2*4))
        mov     a2,  a11
        j       .Write_data

        .org    .LSE_big_reg_table + (5*(2*4))
        mov     a2,  a12
        j       .Write_data

        .org    .LSE_big_reg_table + (6*(2*4))
        mov     a2,  a13
        j       .Write_data

        .org    .LSE_big_reg_table + (7*(2*4))
        mov     a2,  a14
        j       .Write_data

        .org    .LSE_big_reg_table + (8*(2*4))
        mov     a2,  a15
        j       .Write_data

.Write_data:
        and     a2,  a2,  a5
        sll     a2,  a2
        or      a3,  a3,  a2

        s32i    a3,  a4,  0

        rsr     a3,  epc1
        wsr     a0,  sar
        addi    a3,  a3,  0x3
        wsr     a3,  epc1

        /* Restore all regs and return */
        l32i    a0,  sp,  0
        l32i    a2,  sp,  0x08
        l32i    a3,  sp,  0x0c
        l32i    a4,  sp,  0x10
        l32i    a5,  sp,  0x14
        l32i    a6,  sp,  0x18
        rsr     a1,  excsave1    # restore a1 saved by UserExceptionVector
        rfe

.LSE_wrong_opcode:
        /* If we got here it's not an opcode we can try to fix, so bomb out.
         * Restore registers so any dump the fatal exception routine produces
         * will have correct values */
        wsr     a0,  sar
        l32i    a0,  sp,  0
        l32i    a2,  sp,  0x08
        l32i    a3,  sp,  0x0c
        l32i    a4,  sp,  0x10
        rsr     a1, excsave1
        call0   _xt_ext_panic

        .balign 4
.LSE_assign_a1:
        /* a1 is saved in excsave1, so just update that with the value, */
        s32i    a4,  sp,  0x04
        /* Then restore all regs and return */
        l32i    a0,  sp,  0
        l32i    a2,  sp,  0x08
        l32i    a3,  sp,  0x0c
        l32i    a4,  sp,  0x10
        l32i    a1,  sp,  0x04
        rfe

        .balign 4
.LSE_jumptable:
        /* The first 5 entries (80 bytes) of this table are unused (registers
         * a0..a4 are handled separately above).  Rather than have a whole bunch
         * of wasted space, we just pretend that the table starts 80 bytes
         * earlier in memory. */
        .set    .LSE_jumptable_base, .LSE_jumptable - (16 * 5)

        .org    .LSE_jumptable_base + (16 * 5)
        mov     a5,  a4
        l32i    a2,  sp,  0x08
        l32i    a4,  sp,  0x10
        rsr     a1,  excsave1
        rfe

        .org    .LSE_jumptable_base + (16 * 6)
        mov     a6,  a4
        l32i    a2,  sp,  0x08
        l32i    a4,  sp,  0x10
        rsr     a1,  excsave1
        rfe

        .org    .LSE_jumptable_base + (16 * 7)
        mov     a7,  a4
        l32i    a2,  sp,  0x08
        l32i    a4,  sp,  0x10
        rsr     a1,  excsave1
        rfe

        .org    .LSE_jumptable_base + (16 * 8)
        mov     a8,  a4
        l32i    a2,  sp,  0x08
        l32i    a4,  sp,  0x10
        rsr     a1,  excsave1
        rfe

        .org    .LSE_jumptable_base + (16 * 9)
        mov     a9,  a4
        l32i    a2,  sp,  0x08
        l32i    a4,  sp,  0x10
        rsr     a1,  excsave1
        rfe

        .org    .LSE_jumptable_base + (16 * 10)
        mov     a10, a4
        l32i    a2,  sp,  0x08
        l32i    a4,  sp,  0x10
        rsr     a1,  excsave1
        rfe

        .org    .LSE_jumptable_base + (16 * 11)
        mov     a11, a4
        l32i    a2,  sp,  0x08
        l32i    a4,  sp,  0x10
        rsr     a1,  excsave1
        rfe

        .org    .LSE_jumptable_base + (16 * 12)
        mov     a12, a4
        l32i    a2,  sp,  0x08
        l32i    a4,  sp,  0x10
        rsr     a1,  excsave1
        rfe

        .org    .LSE_jumptable_base + (16 * 13)
        mov     a13, a4
        l32i    a2,  sp,  0x08
        l32i    a4,  sp,  0x10
        rsr     a1,  excsave1
        rfe

        .org    .LSE_jumptable_base + (16 * 14)
        mov     a14, a4
        l32i    a2,  sp,  0x08
        l32i    a4,  sp,  0x10
        rsr     a1,  excsave1
        rfe

        .org    .LSE_jumptable_base + (16 * 15)
        mov     a15, a4
        l32i    a2,  sp,  0x08
        l32i    a4,  sp,  0x10
        rsr     a1,  excsave1
        rfe


/*******************************************************************************

EXCEPTION AND LEVEL 1 INTERRUPT VECTORS AND LOW LEVEL HANDLERS
(except window exception vectors).

Each vector goes at a predetermined location according to the Xtensa
hardware configuration, which is ensured by its placement in a special
section known to the Xtensa linker support package (LSP). It performs
the minimum necessary before jumping to the handler in the .text section.

The corresponding handler goes in the normal .text section. It sets up
the appropriate stack frame, saves a few vector-specific registers and
calls XT_RTOS_INT_ENTER to save the rest of the interrupted context
and enter the RTOS, then sets up a C environment. It then calls the
user's interrupt handler code (which may be coded in C) and finally 
calls XT_RTOS_INT_EXIT to transfer control to the RTOS for scheduling.

While XT_RTOS_INT_EXIT does not return directly to the interruptee,
eventually the RTOS scheduler will want to dispatch the interrupted
task or handler. The scheduler will return to the exit point that was
saved in the interrupt stack frame at XT_STK_EXIT.

*******************************************************************************/

/*
--------------------------------------------------------------------------------
Debug Exception.
--------------------------------------------------------------------------------
*/

    .begin      literal_prefix .DebugExceptionVector
    .section    .DebugExceptionVector.text, "ax"
    .global     _DebugExceptionVector
    .align      4
    .literal_position
_DebugExceptionVector:
    wsr     a0,  EXCSAVE + XCHAL_DEBUGLEVEL         /* save original a0 somewhere */
    wsr     a0,  EXCSAVE_1
    wsr     a1,  EXCSAVE_2
    call0   _xt_ext_panic                           /* does not return */
    rfi     XCHAL_DEBUGLEVEL                        /* make a0 point here not later */

    .end        literal_prefix


/*
--------------------------------------------------------------------------------
Double Exception.
Double exceptions are not a normal occurrence. They indicate a bug of some kind.
--------------------------------------------------------------------------------
*/

    .begin      literal_prefix .DoubleExceptionVector
    .section    .DoubleExceptionVector.text, "ax"
    .global     _DoubleExceptionVector
    .align      4
    .literal_position
_DoubleExceptionVector:
    wsr     a0,  EXCSAVE_1
    wsr     a1,  EXCSAVE_2
    j       _xt_ext_panic
    rfde                                        /* make a0 point here not later */

    .end        literal_prefix


/*
--------------------------------------------------------------------------------
Kernel Exception (including Level 1 Interrupt from kernel mode).
--------------------------------------------------------------------------------
*/

    .begin      literal_prefix .KernelExceptionVector
    .section    .KernelExceptionVector.text, "ax"
    .global     _KernelExceptionVector
    .align      4
    .literal_position
_KernelExceptionVector:
    wsr     a0,  EXCSAVE_1                          /* preserve a0 */
    wsr     a1,  EXCSAVE_2
    j       _xt_ext_panic

    /* never returns here - call0 is used as a jump (see note at top) */

    .end        literal_prefix


/*
--------------------------------------------------------------------------------
User Exception (including Level 1 Interrupt from user mode).
--------------------------------------------------------------------------------
*/

    .begin      literal_prefix .UserExceptionVector
    .section    .UserExceptionVector.text, "ax"
    .global     _UserExceptionVector
    .type       _UserExceptionVector,@function
    .align      4
    .literal_position
_UserExceptionVector:

    wsr     a0,  EXCSAVE_1                      /* preserve a0 */
    wsr     a1,  EXCSAVE_2
    call0   _xt_user_exc                        /* user exception handler */
    /* never returns here - call0 is used as a jump (see note at top) */

    .end        literal_prefix


    /* User exception handler begins here. */
    .section    .text
    .type       _xt_user_exc,@function
    .align      4
_xt_user_exc:
    /*
    Handle alloca and syscall exceptions before allocating stack frame and
    interacting with RTOS.
    */
    rsr     a0,  EXCCAUSE
    beqi    a0,  EXCCAUSE_LOAD_STORE_ERROR,  LoadStoreErrorHandler
    beqi    a0,  EXCCAUSE_LEVEL1INTERRUPT,  _xt_user_entry1

    j   _xt_ext_panic

_xt_user_entry1:
    /* Allocate interrupt stack frame and save minimal context. */
    mov     a0,  sp                             /* sp == a1 */
    addi    sp,  sp,  -XT_STK_FRMSZ             /* allocate interrupt stack frame */
    s32i    a0,  sp,  XT_STK_A1                 /* save pre-interrupt SP */
    rsr     a0,  PS                             /* save interruptee's PS */
    s32i    a0,  sp,  XT_STK_PS
    rsr     a0,  EPC_1                          /* save interruptee's PC */
    s32i    a0,  sp,  XT_STK_PC
    rsr     a0,  EXCSAVE_1                      /* save interruptee's a0 */
    s32i    a0,  sp,  XT_STK_A0
    movi    a0,  _xt_user_exit                  /* save exit point for dispatch */
    s32i    a0,  sp,  XT_STK_EXIT

    /* Save rest of interrupt context and enter RTOS. */
    call0   XT_RTOS_INT_ENTER                   /* common RTOS interrupt entry */

    /* !! We are now on the RTOS system stack !! */

    /* Set up PS for C, reenable hi-pri interrupts, and clear EXCM. */
    movi    a0,  PS_INTLEVEL(XCHAL_EXCM_LEVEL) | PS_UM
    wsr     a0,  PS
    rsync

    /* !! It is OK to call C handlers after this point. !! */

    /* Handle level 1 interrupts. No need to enable med-pri interrupts now. */
.L_xt_user_int:

    /* USER_EDIT:
    ADD LOW PRIORITY LEVEL 1 INTERRUPT HANDLER CODE HERE, OR CALL C HANDLER.
    At this point, a2 contains a mask of pending, enabled ints at this level.
    Note on Call0 ABI: Callee-saved regs (a12-15) have not yet been saved,
    so should not be corrupted here. A C handler will not corrupt them.
    HANDLER MUST CAUSE LEVEL TRIGGERED INTERRUPT REQUESTS TO BE DEASSERTED.
    When done, ensure a2 contains a mask of unhandled (still pending)
    enabled ints at this level, and fall through.
    */
3:
    movi    a0, _chip_interrupt_tmp
    s32i    a1, a0, 0
    mov     a1, a0

#ifndef CONFIG_TASK_SWITCH_FASTER
    movi    a0, _xt_isr_handler
    callx0	a0
#else
    call0   _xt_isr_handler
#endif

    movi    a0, _chip_interrupt_tmp
    l32i    a1, a0, 0

    /* Done handling after XT_RTOS_INT_ENTER. Give control to RTOS. */
.L_xt_user_done:
    call0   XT_RTOS_INT_EXIT                /* does not return directly here */
	
    /*
    Exit point for dispatch. Saved in interrupt stack frame at XT_STK_EXIT
    on entry and used to return to a thread or interrupted interrupt handler.
    */
    .global     _xt_user_exit
    .type       _xt_user_exit,@function
    .align      4
_xt_user_exit:
    l32i    a0,  sp,  XT_STK_PS                     /* retrieve interruptee's PS */
    wsr     a0,  PS
    l32i    a0,  sp,  XT_STK_PC                     /* retrieve interruptee's PC */
    wsr     a0,  EPC_1
    l32i    a0,  sp,  XT_STK_A0                     /* retrieve interruptee's A0 */
    l32i    sp,  sp,  XT_STK_A1                     /* remove interrupt stack frame */
    rsync                                           /* ensure PS and EPC written */
    rfe                                             /* PS.EXCM is cleared */


/*
Currently only shells for high priority interrupt handlers are provided
here. However a template and example can be found in the Tensilica tools
documentation: "Microprocessor Programmer's Guide".
*/

    .begin      literal_prefix .NMIExceptionVector
    .section    .NMIExceptionVector.text, "ax"
    .global     _NMIExceptionVector
    .type       _NMIExceptionVector,@function
    .align      4
    .literal_position
_NMIExceptionVector:
    wsr     a0,  EXCSAVE + XCHAL_NMILEVEL           /* preserve a0 */
    wsr     a1,  EXCSAVE_2
    call0   _xt_nmi                                 /* load interrupt handler */
    /* never returns here - call0 is used as a jump (see note at top) */

    .end        literal_prefix


    .section    .text
    .type       _xt_nmi,@function
    .align      4
_xt_nmi:
    /* USER_EDIT:
    ADD HIGH PRIORITY NON-MASKABLE INTERRUPT (NMI) HANDLER CODE HERE.
    */

    movi    a0,  _chip_nmi_cnt
    l32i    a0,  a0,  0

    bnez    a0,  nmi_reentry
    movi    a0,  _chip_nmi_stk + PRI_N_STACK_SIZE   // get ptr to save area
    j       nmi_common
nmi_reentry:
    movi    a0,  _chip_nmi_stk + PRI_N_STACK_SIZE + HESF_TOTALSIZE + PRI_N_STACK_SIZE2// get ptr to save area
nmi_common:

    // interlock

    //  Save a few registers so we can do some work:
    s32i    a2,  a0,  HESF_AR(2)

    movi    a2,  LABEL(_Pri_,_HandlerAddress)       // dispatcher address var.
    s32i    a1,  a0,  HESF_AR(1)
    l32i    a2,  a2,  0                             // get dispatcher address
    s32i    a3,  a0,  HESF_AR(3)
    xsr     a2,  EXCSAVE_LEVEL                           // get saved a0, restore dispatcher address

    s32i    a4,  a0,  HESF_AR(4)
    s32i    a2,  a0,  HESF_AR(0)

    //  Save/restore all exception state
    //  (IMPORTANT:  this code assumes no general exceptions occur
    //   during the execution of this dispatcher until this state
    //   is completely saved and from the point it is restored.)
    //
    //  Exceptions that may normally occur within the C handler
    //  include window exceptions (affecting EPC1), alloca exceptions
    //  (affecting EPC1/EXCCAUSE and its handling uses EXCSAVE1),
    //  and possibly others depending on the particular C handler
    //  (possibly needing save/restore of EXCVADDR; and EXCVADDR
    //   is also possibly corrupted by any access thru an auto-refill
    //   way on a processor with a full MMU).
    //
    rsr     a3,  EPC1
    rsr     a4,  EXCCAUSE
    s32i    a3,  a0,  HESF_EPC1
    s32i    a4,  a0,  HESF_EXCCAUSE
    rsr     a3,  EXCVADDR
    s32i    a3,  a0,  HESF_EXCVADDR
    rsr     a4,  EXCSAVE1
    s32i    a4,  a0,  HESF_EXCSAVE1
    rsr     a3,  EPC3
    movi    a4,  nmi_rfi
    beq     a3,  a4,  nmi_reentried
    s32i    a3,  a0,  HESF_EPC3
    rsr     a4,  EPS3
    s32i    a4,  a0,  HESF_EPS3

nmi_reentried:
    s32i    a5,  a0,  HESF_AR(5)
    s32i    a6,  a0,  HESF_AR(6)
    s32i    a7,  a0,  HESF_AR(7)

1:  s32i    a8,  a0,  HESF_AR(8)
    s32i    a9,  a0,  HESF_AR(9)
    s32i    a10, a0,  HESF_AR(10)
    s32i    a11, a0,  HESF_AR(11)
    s32i    a12, a0,  HESF_AR(12)
    s32i    a13, a0,  HESF_AR(13)
    s32i    a14, a0,  HESF_AR(14)
    s32i    a15, a0,  HESF_AR(15)

    mov     a1,  a0
    movi    a0,  0                                  // mark start of call frames in stack

    //  Critical state saved, a bit more to do to allow window exceptions...

    //  We now have a C-coherent stack and window state.
    //  Still have to fix PS while making sure interrupts stay disabled
    //  at the appropriate level (ie. level 2 and below are disabled in this case).

    //  Load PS for C code, clear EXCM (NOTE: this step is different for XEA1):
    movi    a2,  0x00020 + _INTERRUPT_LEVEL         // WOE=0, CALLINC=0, UM=1, INTLEVEL=N, EXCM=0, RING=0
    wsr     a2,  PS                                 // update PS to enable window exceptions, etc as per above
    rsync

    //  Okay, window exceptions can now happen (although we have to call
    //  deep before any will happen because we've reset WINDOWSTART).

    //  Save other state that might get clobbered by C code:

//////////////////  COMMON DISPATCH CODE BEGIN

    rsr     a14, SAR
    s32i    a14, a1,  HESF_SAR

    //save NMI Count
    movi    a2,  _chip_nmi_cnt
    l32i    a3,  a2,  0
    addi    a3,  a3,  1
    s32i    a3,  a2,  0

    call0   wDev_ProcessFiq                         // call interrupt's C handler

    //Restore NMI level
    movi    a2,  _chip_nmi_cnt
    l32i    a3,  a2,  0
    addi    a3,  a3,  -1
    s32i    a3,  a2,  0
    
    beqi    a3,  1,   nmi_reentry2
    movi    a1,  _chip_nmi_stk + PRI_N_STACK_SIZE    // get ptr to save area
    j       nmi_common2
nmi_reentry2:
    movi    a1,  _chip_nmi_stk + PRI_N_STACK_SIZE + HESF_TOTALSIZE + PRI_N_STACK_SIZE2// get ptr to save area
nmi_common2:

    l32i    a15, a1,  HESF_SAR
    wsr     a15, SAR

    //  Load PS for interrupt exit, set EXCM:
    movi    a2,  0x00030 + _INTERRUPT_LEVEL         // WOE=0, CALLINC=0, UM=1, INTLEVEL=N, EXCM=1, RING=0
    wsr     a2,  PS                                 // update PS to disable window exceptions, etc as per above
    rsync

    //  NOTE:  here for XEA1, restore INTENABLE etc...

    l32i    a4,  a1,  HESF_AR(4)                    // restore general registers
    l32i    a5,  a1,  HESF_AR(5)
    l32i    a6,  a1,  HESF_AR(6)
    l32i    a7,  a1,  HESF_AR(7)
    l32i    a8,  a1,  HESF_AR(8)
    l32i    a9,  a1,  HESF_AR(9)
    l32i    a10, a1,  HESF_AR(10)
    l32i    a11, a1,  HESF_AR(11)
    l32i    a12, a1,  HESF_AR(12)
    l32i    a13, a1,  HESF_AR(13)
    l32i    a14, a1,  HESF_AR(14)
    l32i    a15, a1,  HESF_AR(15)

    //  Restore exception state:
    l32i    a2,  a1,  HESF_EPC1
    l32i    a3,  a1,  HESF_EXCCAUSE
    wsr     a2,  EPC1
    wsr     a3,  EXCCAUSE
    l32i    a2,  a1,  HESF_EXCVADDR
    wsr     a2,  EXCVADDR

    l32i    a3,  a1,  HESF_EXCSAVE1
    wsr     a3,  EXCSAVE1
    l32i    a2,  a1,  HESF_EPC3
    wsr     a2,  EPC3
    l32i    a3,  a1,  HESF_EPS3
    wsr     a3,  EPS3

    l32i    a0,  a1,  HESF_AR(0)

    /* Re-Open NMI */

    rsr     a3,  SAR
    movi    a2,  0x3ff
    slli    a2,  a2,  20
    wsr     a3,  SAR
    rsync
    movi    a3,  1
    s32i    a3,  a2,  0 

    l32i    a2,  a1,  HESF_AR(2)
    l32i    a3,  a1,  HESF_AR(3)
    l32i    a1,  a1,  HESF_AR(1)

nmi_rfi:
    rfi XCHAL_NMILEVEL


    .global     _xt_ext_panic
    .type       _xt_ext_panic, @function
    .align      4
    .literal_position
_xt_ext_panic:
    /*
     * First close intrrupt, only use code, can't use data
     */
    rsil    a0,  PS_INTLEVEL(5)
    wsr     a0,  EXCSAVE_2

    /*
     * CPU must switch from kernel mode to user mode
     */
    movi    a0,  PS_INTLEVEL(5) | PS_UM
    wsr     a0,  PS

    mov     a0,  sp

    movi    sp,  _chip_nmi_cnt
    l32i    sp,  sp,  0

    beqz    sp,  _panic_add_nmi_stk
    /* Allocate exception frame and save minimal context. */
    movi    sp,  _chip_interrupt_tmp
    j       _panic_dump_reg

_panic_add_nmi_stk:
    movi    sp,  LoadStoreErrorHandlerStack

_panic_dump_reg:
    addi    sp,  sp,  -PANIC_STK_FRMSZ
    s32i    a0,  sp,  XT_STK_A1

    rsr     a0,  EXCSAVE_2                   /* save interruptee's PS */
    s32i    a0,  sp,  XT_STK_PS
    rsr     a0,  EPC_1                       /* save interruptee's PC */
    s32i    a0,  sp,  XT_STK_PC

    s32i    a12, sp,  XT_STK_A12             /* _xt_context_save requires A12- */
    s32i    a13, sp,  XT_STK_A13             /* A13 to have already been saved */
    s32i    a14, sp,  XT_STK_A14             
    s32i    a15, sp,  XT_STK_A15             
    call0   _xt_context_save

    /* Save exc cause and vaddr into exception frame */
    rsr     a0,  EXCCAUSE
    s32i    a0,  sp,  XT_STK_EXCCAUSE

    /* _xt_context_save seems to save the current a0, but we need the interuptees a0. Fix this. */
    rsr     a0,  EXCSAVE_1                   /* save interruptee's a0 */

    s32i    a0,  sp,  XT_STK_A0

    call0   Cache_Read_Enable_New

    //Call panic handler
    mov     a2,  sp
    movi    a3,  0
    movi    a0,  panicHandler
    callx0  a0

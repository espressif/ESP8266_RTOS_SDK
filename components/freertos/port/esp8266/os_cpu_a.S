/*
*********************************************************************************************************
* Xtensa Port
*
* Target  : All Xtensa configurable and Diamond preconfigured processors; windowed and call0 ABI.
* Port By : Ross Morley, Tensilica Inc.  : ross@tensilica.com, ross@computer.org
* Web URL : http://www.tensilica.com
*
*********************************************************************************************************
*/
    #include <xtensa/coreasm.h>
    #include <xtensa/config/specreg.h>

    #include "freertos/xtensa_rtos.h"

    .extern _xt_tick_divisor
    .extern _xt_context_save
    .extern _xt_context_restore
    .extern pxCurrentTCB
    .extern CCOMPARE_AddVal

    .section    .text

    .macro  xtos_lock    ax
    rsil    \ax,    XCHAL_EXCM_LEVEL
    .endm

    .macro  xtos_unlock    ax
    wsr     \ax,    PS
    .endm
    
/*
**********************************************************************************************************
*                                            _xt_int_enter 
*                                       void _xt_int_enter(void)
*
* Implements the Xtensa RTOS porting layer's XT_RTOS_INT_ENTER function for uC/OS-II.
* Saves the rest of the interrupt context (not already saved) and implements OSIntEnter().
* May only be called from assembly code by the 'call0' instruction, with interrupts disabled.
* See the detailed description of the XT_RTOS_ENTER macro in xtensa_rtos.h.
*
**********************************************************************************************************
*/
    .globl  _xt_int_enter
    .type   _xt_int_enter,@function
    .align  4
    .literal_position
_xt_int_enter:

    /* Save a12-13 in the stack frame as required by _xt_context_save. */
    s32i    a12, sp, XT_STK_A12
    s32i    a13, sp, XT_STK_A13

    /* Save return address in a safe place (free a0). */
    mov     a12, a0

    /* Save the rest of the interrupted context (preserves A12-13). */
    call0   _xt_context_save

    movi	a0,  pxCurrentTCB
	l32i    a0,  a0,  0
	s32i	sp,	 a0,  0

    /* Retrieve the return address and return to interrupt handler. */
    mov     a0,  a12
    ret

/*
**********************************************************************************************************
*                                            _xt_int_exit 
*                                       void _xt_int_exit(void)
*
* Implements the Xtensa RTOS porting layer's XT_RTOS_INT_EXIT function for uC/OS-II.
* Calls OSIntExit() to perform task context switching, restores the (possibly) new task's 
* context, and returns to the exit dispatcher saved in the task's stack frame at XT_STK_EXIT.
* May only be called from assembly code by the 'call0' instruction. Does not return to caller.
* See the detailed description of the XT_RTOS_ENTER macro in xtensa_rtos.h.
*
**********************************************************************************************************
*/
    .globl  _xt_int_exit
    .type   _xt_int_exit,@function
    .align  4
_xt_int_exit:
    /*
    Call0 ABI callee-saved regs a12-15 need to be saved before possible preemption.
    However a12-13 were already saved by _xt_int_enter().
	Save A14, A15 in sp to a14, a15 in cpu
    */
    s32i    a14, sp,  XT_STK_A14
    s32i    a15, sp,  XT_STK_A15

    /*
	Save A14, A15 in sp to a14, a15 in cpu
    */
    movi	sp,  pxCurrentTCB
	l32i    sp,  sp,  0
	l32i	sp,	 sp,  0

    /*
    We come here only if there was no context switch, that is if this 
    is a nested interrupt or the interrupted task was not preempted.
    We are still on the same stack so there's no need to load the SP.
    */
    movi	a14, pxCurrentTCB
	l32i    a14, a14, 0
    addi    a15, sp,  XT_STK_FRMSZ
    s32i	a15, a14, 0

    /* Restore full context from interrupt stack frame and return to exit dispatcher. */
    call0   _xt_context_restore

    /* In Call0 ABI, restore callee-saved regs (A12, A13 already restored). */
    l32i    a14, sp,  XT_STK_A14   
    l32i    a15, sp,  XT_STK_A15

    l32i    a0,  sp,  XT_STK_EXIT
    ret


/*
**********************************************************************************************************
*                                           _xt_tick_timer_init
*                                      void _xt_tick_timer_init(void)
*
* Initialize timer and timer interrrupt handler (_xt_tick_divisor_init() has already been been called).
* Callable from C (obeys ABI conventions on entry).
*
**********************************************************************************************************
*/
    .section    .text._xt_tick_timer_init, "ax"
    .globl      _xt_tick_timer_init
    .type       _xt_tick_timer_init,@function
    .align      4
_xt_tick_timer_init:

    ENTRY(16)

    /* Set up the periodic tick timer (assume enough time to complete init). */
    #ifdef XT_CLOCK_FREQ
    movi    a3,  XT_TICK_DIVISOR
    #else
    movi    a2,  _xt_tick_divisor
    l32i    a3,  a2,  0
    #endif
    rsr     a2,  CCOUNT              /* current cycle count */
    add     a2,  a2,  a3              /* time of first timer interrupt */
    wsr     a2,  XT_CCOMPARE         /* set the comparator */

    /* Enable the timer interrupt at the device level. */
    movi    a2,  0                   /* protect critical section */
    xsr     a2,  INTENABLE
    movi    a3,  XT_TIMER_INTEN
    or      a2,  a2,  a3
    wsr     a2,  INTENABLE           /* set new INTENABLE, no need to rsync */

    RET(16)

    .section    .text._xt_set_xt_ccompare_val, "ax"
    .globl      _xt_set_xt_ccompare_val
    .type       _xt_set_xt_ccompare_val,@function
    .align      4
_xt_set_xt_ccompare_val:
    ENTRY(16)

    /* Set up the periodic tick timer (assume enough time to complete init). */
    movi    a3,  CCOMPARE_AddVal
	l32i    a2,  a3,  0 
    wsr     a2,  XT_CCOMPARE         /* set the comparator */
	esync
    RET(16)

    .section    .text.ResetCcountVal, "ax"
    .globl      ResetCcountVal
    .type       ResetCcountVal,@function
    .align      4
ResetCcountVal:
    ENTRY(16)
    wsr     a2,  ccount
    RET(16)

/*
**********************************************************************************************************
* isr_unmask
**********************************************************************************************************
*/

    .section    .text._xt_isr_unmask, "ax"
    .globl      _xt_isr_unmask
    .type       _xt_isr_unmask,@function
    .align      4
_xt_isr_unmask:
    ENTRY(16)

    /* Enable the timer interrupt at the device level. */
	xtos_lock	 a7
	rsr     a3,  INTENABLE
	or      a5,  a3,  a2
	wsr		a5,  INTENABLE
	xtos_unlock	 a7
	mov		a2,	 a3
    RET(16)


    .section    .text._xt_isr_mask, "ax"
    .globl      _xt_isr_mask
    .type       _xt_isr_mask,@function
    .align      4
_xt_isr_mask:
    ENTRY(16)

    /* Enable the timer interrupt at the device level. */
	xtos_lock    a7
	rsr     a3,  INTENABLE
	or      a5,  a3, a2
	xor     a5,  a5, a2
	wsr     a5,  INTENABLE
	xtos_unlock	a7
	mov     a2,	 a3
    RET(16)


    .section    .text._xt_enter_first_task, "ax"
    .globl      _xt_enter_first_task
    .type       _xt_enter_first_task,   @function
    .align      4
_xt_enter_first_task:
    movi    sp,  pxCurrentTCB
    l32i    sp,  sp,  0
    l32i    sp,	 sp,  0

    movi    a14, pxCurrentTCB
    l32i    a14, a14, 0
    addi    a15, sp,  XT_STK_FRMSZ
    s32i    a15, a14, 0

    movi    a0, _xt_context_restore
    callx0  a0

    l32i    a14, sp,  XT_STK_A14
    l32i    a15, sp,  XT_STK_A15

    l32i    a0,  sp,  XT_STK_EXIT
    ret

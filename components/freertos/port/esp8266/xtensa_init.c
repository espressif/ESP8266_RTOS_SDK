/*******************************************************************************
Copyright (c) 2006-2009 by Tensilica Inc.  ALL RIGHTS RESERVED.
These coded instructions, statements, and computer programs are the
copyrighted works and confidential proprietary information of Tensilica Inc.
They may not be modified, copied, reproduced, distributed, or disclosed to
third parties in any manner, medium, or form, in whole or in part, without
the prior written consent of Tensilica Inc.
--------------------------------------------------------------------------------

        XTENSA INITIALIZATION ROUTINES CODED IN C

This header is a place to put miscellaneous Xtensa RTOS-generic initialization 
functions that are implemented in C.

This header contains definitions and macros for use primarily by Xtensa
RTOS assembly coded source files. It includes and uses the Xtensa hardware
abstraction layer (HAL) to deal with config specifics. It may also be
included in C source files.

*******************************************************************************/

#ifndef XTENSA_INIT_H
#define XTENSA_INIT_H

#ifdef XT_BOARD
#include    <xtensa/xtbsp.h>
#endif

#include    "freertos/xtensa_rtos.h"


#ifdef XT_RTOS_TIMER_INT
#ifndef XT_CLOCK_FREQ

unsigned _xt_tick_divisor = 0;  /* cached number of cycles per tick */

/*
Compute and initialize at run-time the tick divisor (the number of 
processor clock cycles in an RTOS tick, used to set the tick timer).
Called when the processor clock frequency is not known at compile-time.
*/
void _xt_tick_divisor_init(void)
{
    #ifdef XT_BOARD
    _xt_tick_divisor = xtbsp_clock_freq_hz() / XT_TICK_PER_SEC;
    #else
    #error "No way to obtain processor clock frequency"
    #endif  /* XT_BOARD */
}

#endif /* XT_CLOCK_FREQ */
#endif /* XT_RTOS_TIMER_INT */

#endif /* XTENSA_INIT_H */

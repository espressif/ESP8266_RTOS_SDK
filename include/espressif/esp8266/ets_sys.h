/*
 * copyright (c) 2008 - 2011 Espressif System
 *
 * Define user specified Event signals and Task priorities here
 *
 */

#ifndef __ETS_SYS_H__
#define __ETS_SYS_H__

/* interrupt related */
#define ETS_GPIO_INUM       4
#define ETS_UART_INUM       5
#define ETS_MAX_INUM        6
#define ETS_SOFT_INUM       7		//software isr.
#define ETS_WDT_INUM        8  /* use edge*/
#define ETS_FRC_TIMER1_INUM 9  /* use edge*/

#endif /* _ETS_SYS_H */

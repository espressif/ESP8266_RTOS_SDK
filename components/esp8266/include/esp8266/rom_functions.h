#ifndef _ROM_FUNCTIONS_H
#define _ROM_FUNCTIONS_H

#include <stdint.h>

uint32_t Wait_SPI_Idle();

void uart_div_modify(uint32_t uart_no, uint32_t baud_div);

void ets_delay_us(uint32_t us);
int ets_printf(const char *fmt, ...)
#ifdef __GNUC__
__attribute__ ((format (printf, 1, 2)));
#endif
;

void system_soft_wdt_feed();

void Cache_Read_Enable_New();

#endif

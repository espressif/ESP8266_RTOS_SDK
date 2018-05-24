#ifndef _ROM_FUNCTIONS_H
#define _ROM_FUNCTIONS_H

#include <stdint.h>
#include <stdarg.h>

uint32_t Wait_SPI_Idle();

void uart_div_modify(uint32_t uart_no, uint32_t baud_div);

void ets_delay_us(uint32_t us);
int ets_vprintf(void (*putc)(char), const char* fmt, va_list ap);

void system_soft_wdt_feed();

void Cache_Read_Enable_New();

#endif

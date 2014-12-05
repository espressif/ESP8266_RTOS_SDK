/*
 *  Copyright (C) 2013 -2014  Espressif System
 *
 */

#ifndef __ESP_SYSTEM_H__
#define __ESP_SYSTEM_H__

#include "c_types.h"

enum rst_reason {
	DEFAULT_RST_FLAG	= 0,
	WDT_RST_FLAG	= 1,
	EXP_RST_FLAG    = 2
};

struct rst_info{
	uint32 flag;
	uint32 exccause;
	uint32 epc1;
	uint32 epc2;
	uint32 epc3;
	uint32 excvaddr;
	uint32 depc;
};

void system_restore(void);
void system_restart(void);
void system_deep_sleep(uint32 time_in_us);

uint32 system_get_time(void);

void system_print_meminfo(void);
uint32 system_get_free_heap_size(void);
uint32 system_get_chip_id(void);

uint32 system_rtc_clock_cali_proc(void);
uint32 system_get_rtc_time(void);

bool system_rtc_mem_read(uint8 src, void *dst, uint16 n);
bool system_rtc_mem_write(uint8 dst, const void *src, uint16 n);

void system_uart_swap(void);

#endif

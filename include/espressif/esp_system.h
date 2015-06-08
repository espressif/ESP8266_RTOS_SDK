/*
 *  Copyright (C) 2013 -2014  Espressif System
 *
 */

#ifndef __ESP_SYSTEM_H__
#define __ESP_SYSTEM_H__

#include "c_types.h"

enum rst_reason {
	DEFAULT_RST_FLAG	= 0,
	WDT_RST_FLAG,
	EXCEPTION_RST_FLAG,
	SOFT_WDT_RST_FLAG,
	SOFT_RESTART_FLAG,
	DEEP_SLEEP_AWAKE_FLAG
};

struct rst_info{
	uint32 reason;
	uint32 exccause;
	uint32 epc1;
	uint32 epc2;
	uint32 epc3;
	uint32 excvaddr;
	uint32 depc;
	uint32 rtn_addr;
};

struct rst_info* system_get_rst_info(void);

const char* system_get_sdk_version(void);

void system_restore(void);
void system_restart(void);

void system_deep_sleep(uint32 time_in_us);
bool system_deep_sleep_set_option(uint8 option);

uint32 system_get_time(void);

void system_print_meminfo(void);

uint32 system_get_free_heap_size(void);
uint32 system_get_chip_id(void);

uint32 system_rtc_clock_cali_proc(void);
uint32 system_get_rtc_time(void);

bool system_rtc_mem_read(uint8 src, void *dst, uint16 n);
bool system_rtc_mem_write(uint8 dst, const void *src, uint16 n);

void system_uart_swap(void);
void system_uart_de_swap(void);

uint16 system_adc_read(void);
uint16 system_get_vdd33(void);

enum flash_size_map {
    FLASH_SIZE_4M_MAP_256_256 = 0,
    FLASH_SIZE_2M,
    FLASH_SIZE_8M_MAP_512_512,
    FLASH_SIZE_16M_MAP_512_512,
    FLASH_SIZE_32M_MAP_512_512,
    FLASH_SIZE_16M_MAP_1024_1024,
    FLASH_SIZE_32M_MAP_1024_1024
};

enum flash_size_map system_get_flash_size_map(void);

bool system_param_save_with_protect(uint16 start_sec, void *param, uint16 len);
bool system_param_load(uint16 start_sec, uint16 offset, void *param, uint16 len);

#endif

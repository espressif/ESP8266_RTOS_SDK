/*
 *  Copyright (C) 2013 -2014  Espressif System
 *
 */

#ifndef __ESP_SYSTEM_H__
#define __ESP_SYSTEM_H__

#include "c_types.h"

enum rst_reason {
	REASON_DEFAULT_RST		= 0,
	REASON_WDT_RST,
	REASON_EXCEPTION_RST,
	REASON_SOFT_WDT_RST,
	REASON_SOFT_RESTART,
	REASON_DEEP_SLEEP_AWAKE
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

#define SYS_BOOT_ENHANCE_MODE	0
#define SYS_BOOT_NORMAL_MODE	1

#define SYS_BOOT_NORMAL_BIN		0
#define SYS_BOOT_TEST_BIN		1

uint8 system_get_boot_version(void);
uint32 system_get_userbin_addr(void);
uint8 system_get_boot_mode(void);
bool system_restart_enhance(uint8 bin_type, uint32 bin_addr);

uint8 system_upgrade_userbin_check(void);
void system_upgrade_reboot(void);
uint8 system_upgrade_flag_check();
void system_upgrade_flag_set(uint8 flag);

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

void system_phy_set_max_tpw(uint8 max_tpw);
void system_phy_set_tpw_via_vdd33(uint16 vdd33);
void system_phy_set_rfoption(uint8 option);

#endif

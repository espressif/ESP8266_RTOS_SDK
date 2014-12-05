/* vi: set sw=4 ts=4: */
/*
 * Utility routines.
 *
 * Copyright (C) 2007 Denys Vlasenko
 *
 * Licensed under GPLv2, see file LICENSE in this source tree.
 */

#include "udhcp/common.h"

unsigned long long ICACHE_FLASH_ATTR monotonic_ns(void)
{
	struct timeval tv;
//	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000000ULL + tv.tv_usec * 1000;
}
unsigned long long ICACHE_FLASH_ATTR monotonic_us(void)
{
	struct timeval tv;
//	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000ULL + tv.tv_usec;
}
unsigned long long ICACHE_FLASH_ATTR monotonic_ms(void)
{
	struct timeval tv;
//	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000ULL + tv.tv_usec / 1000;
}

unsigned ICACHE_FLASH_ATTR monotonic_sec(void)
{
	unsigned monotonic_sec = 0;
	monotonic_sec = system_get_time();
	if (monotonic_sec < 0)
		monotonic_sec = rand()-monotonic_sec;

	return monotonic_sec;
}

uint32_t ICACHE_FLASH_ATTR time(void *arg)
{
	return system_get_time();
}

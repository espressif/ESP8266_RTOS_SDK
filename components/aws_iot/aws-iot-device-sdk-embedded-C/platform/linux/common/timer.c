/*
 * Copyright 2010-2015 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

/**
 * @file timer.c
 * @brief Linux implementation of the timer interface.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>

#include "timer_platform.h"

bool has_timer_expired(Timer *timer) {
	struct timeval now, res;
	gettimeofday(&now, NULL);
	timersub(&timer->end_time, &now, &res);
	return res.tv_sec < 0 || (res.tv_sec == 0 && res.tv_usec <= 0);
}

void countdown_ms(Timer *timer, uint32_t timeout) {
	struct timeval now;
#ifdef __cplusplus
	struct timeval interval = {timeout / 1000, static_cast<int>((timeout % 1000) * 1000)};
#else
	struct timeval interval = {timeout / 1000, (int)((timeout % 1000) * 1000)};
#endif
	gettimeofday(&now, NULL);
	timeradd(&now, &interval, &timer->end_time);
}

uint32_t left_ms(Timer *timer) {
	struct timeval now, res;
	uint32_t result_ms = 0;
	gettimeofday(&now, NULL);
	timersub(&timer->end_time, &now, &res);
	if(res.tv_sec >= 0) {
		result_ms = (uint32_t) (res.tv_sec * 1000 + res.tv_usec / 1000);
	}
	return result_ms;
}

void countdown_sec(Timer *timer, uint32_t timeout) {
	struct timeval now;
	struct timeval interval = {timeout, 0};
	gettimeofday(&now, NULL);
	timeradd(&now, &interval, &timer->end_time);
}

void init_timer(Timer *timer) {
	timer->end_time = (struct timeval) {0, 0};
}

#ifdef __cplusplus
}
#endif

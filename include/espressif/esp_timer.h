/*
 *  Copyright (C) 2013 -2014  Espressif System
 *
 */

#ifndef __ESP_TIMER_H__
#define __ESP_TIMER_H__

/* timer related */
typedef void os_timer_func_t(void *timer_arg);

typedef struct _os_timer_t {
	struct _os_timer_t *timer_next;
    void               *freerots_handle;
    uint32             timer_expire;
    uint32             timer_period;
    os_timer_func_t    *timer_func;
    bool               timer_repeat_flag;
    void               *timer_arg;
} os_timer_t;

#endif

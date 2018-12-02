#include "csro_common.h"

static TimerHandle_t sys_timer = NULL;

void sys_timer_callback( TimerHandle_t xTimer )
{
    date_time.time_now++;
    date_time.time_run++;
    localtime_r(&date_time.time_now, &date_time.timeinfo);
}

void csro_datetime_init(void)
{
    sys_timer = xTimerCreate("sys_timer", 1000/portTICK_RATE_MS, pdTRUE, (void *)0, sys_timer_callback);
    if (sys_timer != NULL) {
        xTimerStart(sys_timer, 0);
    }
}
void csro_datetime_print(void)
{
    bzero(date_time.strtime, 64);
    strftime(date_time.strtime, 64, "%Y-%m-%d %H:%M:%S %a", &date_time.timeinfo);
    debug("current time is: %s\r\n", date_time.strtime);
}
void csro_datetime_set(char *timestr)
{

    uint32_t tim[6];
    sscanf(timestr, "%d-%d-%d %d:%d:%d", &tim[0], &tim[1], &tim[2], &tim[3], &tim[4], &tim[5]);
    if ((tim[0]<2018) || (tim[1]>12) || (tim[2]>31) || (tim[3]>23) || (tim[4]>59) || (tim[5]>59)) {
        return;
    }
    date_time.timeinfo.tm_year = tim[0] - 1900;
    date_time.timeinfo.tm_mon = tim[1] - 1;
    date_time.timeinfo.tm_mday = tim[2];
    date_time.timeinfo.tm_hour = tim[3];
    date_time.timeinfo.tm_min = tim[4];
    date_time.timeinfo.tm_sec = tim[5];
    date_time.time_now = mktime(&date_time.timeinfo);
}
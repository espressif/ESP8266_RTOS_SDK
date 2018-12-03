#ifndef CSRO_COMMON_H_
#define CSRO_COMMON_H_

#include "csro_config.h"

// extern csro_system_info sys_info;
// extern csro_wifi_param wifi_param;
// extern csro_date_time date_time;
// extern csro_mqtt mqtt;

void csro_system_init(void);
void csro_mqtt_task(void *pvParameters);
void csro_smartconfig_task(void *pvParameters);
void csro_system_set_status(csro_system_status status);

void csro_datetime_init(void);
void csro_datetime_print(void);
void csro_datetime_set(char *timeinfo);

#endif
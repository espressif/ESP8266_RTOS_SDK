#ifndef CSRO_COMMON_H_
#define CSRO_COMMON_H_

#include "csro_config.h"

extern csro_system_info    sys_info;
extern csro_wifi_param     wifi_param;

void csro_system_init(void);

void csro_mqtt_task(void *pvParameters);

void csro_smartconfig_task(void *pvParameters);

void csro_system_set_status(csro_system_status status);

#endif
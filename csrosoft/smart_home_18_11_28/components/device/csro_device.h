#ifndef CSRO_DEVICE_H_
#define CSRO_DEVICE_H_

#include "../csro/csro_common.h"

void csro_device_init(void);
void csro_device_prepare_basic_message(void);
void csro_device_handle_message(MessageData* data);

void csro_air_system_prepare_basic_message(void);
void csro_air_system_handle_message(MessageData* data);
void csro_air_system_init(void);

#endif

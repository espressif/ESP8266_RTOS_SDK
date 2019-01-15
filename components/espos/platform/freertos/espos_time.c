// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "espos_time.h"

espos_time_t espos_get_tick_per_ms(void)
{
    return portTICK_PERIOD_MS;
}

/**
 * @brief get current system ticks
 */
espos_tick_t espos_get_tick_count(void)
{
    return (espos_tick_t)xTaskGetTickCount();
}

/**
 * @brief transform milliseconds to system ticks
 */
espos_tick_t espos_ms_to_ticks(espos_time_t ms)
{
    return (ms / (portTICK_PERIOD_MS));
}

/**
 * @brief transform system ticks to milliseconds
 */
espos_time_t espos_ticks_to_ms(espos_tick_t ticks)
{
    return (ticks * (portTICK_PERIOD_MS));
}

void espos_add_tick_count(size_t t)
{
    vTaskStepTick((portTickType)t);
}

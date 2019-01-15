// Copyright 2018-2019 Espressif Systems (Shanghai) PTE LTD
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

#ifndef _ESP_NEWLIB_H
#define _ESP_NEWLIB_H

#include <reent.h>

/*
 * @brief Initialize global and thread's private reent object data. We add this instead of
 *        newlib's initialization function to avoid some unnecessary cost and unused function.
 * 
 * @param r reent pointer
 * 
 * @return none
 */
void esp_reent_init(struct _reent* r);

/*
 * @brief Initialize newlib's platform object data
 * 
 * @param none
 * 
 * @return 0 if successful or -1 if failed
 */
int esp_newlib_init(void);

#endif

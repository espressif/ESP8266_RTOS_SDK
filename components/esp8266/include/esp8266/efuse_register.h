// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __EFUSE_REGISTER_H__
#define __EFUSE_REGISTER_H__

#define DR_REG_EFUSE_BASE   0x3FF00050

#define EFUSE_DATA0_REG     (DR_REG_EFUSE_BASE + 0x000)
#define EFUSE_IS_ESP8285    (1 << 4)

#define EFUSE_DATA1_REG     (DR_REG_EFUSE_BASE + 0x004)

#define EFUSE_VERSION       0xF
#define EFUSE_VERSION_M     ((EFUSE_VERSION_V) << (EFUSE_VERSION_S))
#define EFUSE_VERSION_V     0xF
#define EFUSE_VERSION_S     24

#define EFUSE_VERSION_0     0
#define EFUSE_VERSION_1     1
#define EFUSE_VERSION_2     2

#define EFUSE_DATA2_REG     (DR_REG_EFUSE_BASE + 0x008)

#define EFUSE_USE_TYPE2     (1 << 15)   /*< 0: type1, 1: type2 */
#define EFUSE_IS_24PINS     (1 << 14)   /*< 0: 32 pins, 1: 24 pins */
#define EFUSE_4_GPIO_PAD    (1 << 13)   /*< 0: 2 GPIO pad, GPIO0 & GPIO2, 1: 4 GPIO pad, extra GPIO4 & GPIO5 */
#define EFUSE_IS_48BITS_MAC (1 << 12)   /*< 0: 24 bits mac, 1: 32 bits mac */

#define EFUSE_DATA3_REG     (DR_REG_EFUSE_BASE + 0x00C)

#define EFUSE_EMB_FLASH_SIZE    0x3
#define EFUSE_EMB_FLASH_SIZE_M  ((EFUSE_FLASH_SIZE_V) << (EFUSE_FLASH_SIZE_S))
#define EFUSE_EMB_FLASH_SIZE_V  0x3
#define EFUSE_EMB_FLASH_SIZE_S  26

#define EFUSE_EMB_FLASH_SIZE_2MB    0
#define EFUSE_EMB_FLASH_SIZE_4MB    1

#endif /*__EFUSE_REGISTER_H__ */

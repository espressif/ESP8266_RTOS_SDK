// Copyright 2018-2019 Espressif Systems (Shanghai) PTE LTD
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

#pragma once

#include <stdint.h>
#include <stddef.h>

/**
 * @brief ESP8266's IBus(instruction bus) only can read data of word type by 4 bytes align,
 *        so if users' code read a bytes or half word from IBus, an load/store exception will occure.
 *
 *        The exception function will read the word by align and transform it to be the right
 *        data that users' code want to read.
 *
 *        CPU read data from IRAM or flash by IBus. And "const" type value or string will locate at flash by default.
 *        So users' code read "const" type data whose type is byte or half word should lead to exception,
 *        and the exception operation is so slow.
 *
 *        For example:
 *
 *              static const char s_output[] = "123456789";
 *
 *              char get_byte(int index)
 *              {
 *                  return s_output(index);
 *              }
 *
 *        By above way, reading data may be not by 4 bytes align and word type.
 *
 *        For this reason, we add the following function to read byte or half word without leading to exception. Firstly the function reads
 *        a word and then transform it to be the right data that users' code want to read. Users can modify above code to be following:
 *
 *              static const char s_output[] = "123456789";
 *
 *              char get_byte(int index)
 *              {
 *                  return ESP_IBUS_GET_U8_DATA(index, s_output);
 *              }
 */

#ifdef USING_IBUS_FASTER_GET

#include "esp_attr.h"

#ifndef DISABLE_IBUS_INLINE_FUNC
#define IBUS_INLINE inline
#else
#define IBUS_INLINE
#endif

typedef union _ibus_data {
    uint32_t        u32_data[1];
    uint16_t        u16_data[2];
    uint8_t         u8_data[4];
} ibus_data_t;

#define __ESP_IBUS_GET_DATA(_type)                                                              \
    static IBUS_INLINE _type __esp_ibus_get_## _type ## _data(size_t index, const void *pbuf)   \
    {                                                                                           \
        ibus_data_t data;                                                                       \
        const uint8_t num = index / (sizeof(uint32_t) / sizeof(_type));                         \
        const uint8_t off = index % (sizeof(uint32_t) / sizeof(_type));                         \
        const uint32_t *ptable = (const uint32_t *)pbuf;                                        \
                                                                                                \
        data.u32_data[0] = ptable[num];                                                         \
                                                                                                \
        if (sizeof(_type) == sizeof(uint8_t))                                                   \
            return data.u8_data[off];                                                           \
        else if (sizeof(_type) == sizeof(uint16_t))                                             \
            return data.u16_data[off];                                                          \
    }

__ESP_IBUS_GET_DATA(uint8_t)
__ESP_IBUS_GET_DATA(uint16_t)

#define ESP_IBUS_ATTR                               WORD_ALIGNED_ATTR

#define ESP_IBUS_GET_U8_DATA(_index, _pbuf)         __esp_ibus_get_uint8_t_data(_index, _pbuf) 
#define ESP_IBUS_GET_U16_DATA(_index, _pbuf)        __esp_ibus_get_uint16_t_data(_index, _pbuf)
#else
#define ESP_IBUS_ATTR

#define ESP_IBUS_GET_U8_DATA(_index, _pbuf)         ((const uint8_t *)_pbuf)[_index]
#define ESP_IBUS_GET_U16_DATA(_index, _pbuf)        ((const uint16_t *)_pbuf)[_index]
#endif



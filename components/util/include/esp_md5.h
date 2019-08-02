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

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief          MD5 context structure
 */
typedef struct MD5Context{
    uint32_t total[2];          /*!< number of bytes processed  */
    uint32_t state[4];          /*!< intermediate digest state  */
    uint8_t  buffer[64];        /*!< data block being processed */
} esp_md5_context_t;

/**
 * @brief          MD5 context setup
 * @param ctx      context to be initialized
 * @return         0 if successful
 */
int esp_md5_init(esp_md5_context_t *ctx);

/**
 * @brief          MD5 process buffer
 * @param ctx      MD5 context
 * @param input    buffer holding the data
 * @param ilen     length of the input data
 * @return         0 if successful
 */
int esp_md5_update(esp_md5_context_t *ctx, const uint8_t *input, size_t ilen);

/**
 * @brief          MD5 final digest
 * @param ctx      MD5 context
 * @param output   MD5 checksum result
 * @return         0 if successful
 */
int esp_md5_final(esp_md5_context_t *ctx, uint8_t output[16]);

#ifdef __cplusplus
}
#endif


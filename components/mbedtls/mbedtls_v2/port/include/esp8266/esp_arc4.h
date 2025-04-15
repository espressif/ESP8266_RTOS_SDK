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
 * @brief                       ARC4 context structure
 */
typedef struct
{
    int x;                      /*!< permutation index */
    int y;                      /*!< permutation index */
    unsigned char m[256];       /*!< permutation table */
}esp_arc4_context;

/**
 * @brief          ARC4 key schedule
 *
 * @param ctx      ARC4 context to be setup
 * @param key      the secret key
 * @param keylen   length of the key, in bytes
 */
void esp_arc4_setup(esp_arc4_context *ctx, const uint8_t *key, uint32_t keylen);

/**
 * @brief          ARC4 cipher function
 *
 * @param ctx      ARC4 context
 * @param length   length of the input data
 * @param input    buffer holding the input data
 * @param output   buffer for the output data
 *
 * @return         0 if successful
 */
int esp_arc4_encrypt(esp_arc4_context *ctx, size_t length, const uint8_t *input, uint8_t *output);

/**
 * @brief          ARC4 cipher function
 *
 * @param ctx      ARC4 context
 * @param length   length of the input data
 * @param input    buffer holding the input data
 * @param output   buffer for the output data
 *
 * @return         0 if successful
 * @Note           When you encrypt or decrypt, must call esp_arc4_setup function to set key. 
 *                 Encrypt and decrypt will change the ctx value
 */
int esp_arc4_decrypt(esp_arc4_context *ctx, size_t length, const uint8_t *input, uint8_t *output );

#ifdef __cplusplus
}
#endif


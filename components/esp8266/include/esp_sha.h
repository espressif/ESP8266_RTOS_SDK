// Copyright 2019-2020 Espressif Systems (Shanghai) PTE LTD
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

typedef struct {
    uint32_t        state[5];
    uint32_t        total[2];
    uint8_t         buffer[64];
} esp_sha1_t;

typedef struct {
    uint32_t        total[2];
    uint32_t        state[8];
    uint8_t         buffer[64];

    int             is224;
} esp_sha256_t;

typedef struct {
    uint64_t        total[2];
    uint64_t        state[8];
    uint8_t         buffer[128];

    int             is384;
} esp_sha512_t;

typedef esp_sha256_t esp_sha224_t;

typedef esp_sha512_t esp_sha384_t;

/**
 * @brief initialize the SHA1 contex
 * 
 * @param ctx SHA1 contex pointer
 * 
 * @return 0 if success or fail
 */
int esp_sha1_init(esp_sha1_t *ctx);

/**
 * @brief initialize the SHA224 contex
 * 
 * @param ctx SHA224 contex pointer
 * 
 * @return 0 if success or fail
 */
int esp_sha224_init(esp_sha224_t *ctx);

/**
 * @brief initialize the SHA256 contex
 * 
 * @param ctx SHA256 contex pointer
 * 
 * @return 0 if success or fail
 */
int esp_sha256_init(esp_sha256_t *ctx);

/**
 * @brief initialize the SHA384 contex
 * 
 * @param ctx SHA384 contex pointer
 * 
 * @return 0 if success or fail
 */
int esp_sha384_init(esp_sha384_t *ctx);

/**
 * @brief initialize the SHA512 contex
 * 
 * @param ctx SHA512 contex pointer
 * 
 * @return 0 if success or fail
 */
int esp_sha512_init(esp_sha512_t *ctx);

/**
 * @brief calculate input data for SHA1
 * 
 * @param ctx SHA1 contex pointer
 * @param src input data buffer pointer
 * @param size input data bytes
 * 
 * @return 0 if success or fail
 */
int esp_sha1_update(esp_sha1_t *ctx, const void *src, size_t size);

/**
 * @brief calculate input data for SHA224
 * 
 * @param ctx SHA224 contex pointer
 * @param src input data buffer pointer
 * @param size input data bytes
 * 
 * @return 0 if success or fail
 */
int esp_sha224_update(esp_sha224_t *ctx, const void *src, size_t size);

/**
 * @brief calculate input data for SHA256
 * 
 * @param ctx SHA256 contex pointer
 * @param src input data buffer pointer
 * @param size input data bytes
 * 
 * @return 0 if success or fail
 */
int esp_sha256_update(esp_sha256_t *ctx, const void *src, size_t size);

/**
 * @brief calculate input data for SHA384
 * 
 * @param ctx SHA384 contex pointer
 * @param src input data buffer pointer
 * @param size input data bytes
 * 
 * @return 0 if success or fail
 */
int esp_sha384_update(esp_sha384_t *ctx, const void *src, size_t size);

/**
 * @brief calculate input data for SHA512
 * 
 * @param ctx SHA512 contex pointer
 * @param src input data buffer pointer
 * @param size input data bytes
 * 
 * @return 0 if success or fail
 */
int esp_sha512_update(esp_sha512_t *ctx, const void *src, size_t size);

/**
 * @brief output SHA1 calculation result
 * 
 * @param ctx SHA1 contex pointer
 * @param dest output data buffer pointer
 * 
 * @return 0 if success or fail
 */
int esp_sha1_finish(esp_sha1_t *ctx, void *dest);

/**
 * @brief output SHA224 calculation result
 * 
 * @param ctx SHA224 contex pointer
 * @param dest output data buffer pointer
 * 
 * @return 0 if success or fail
 */
int esp_sha224_finish(esp_sha224_t *ctx, void *dest);

/**
 * @brief output SHA256 calculation result
 * 
 * @param ctx SHA256 contex pointer
 * @param dest output data buffer pointer
 * 
 * @return 0 if success or fail
 */
int esp_sha256_finish(esp_sha256_t *ctx, void *dest);

/**
 * @brief output SHA384 calculation result
 * 
 * @param ctx SHA384 contex pointer
 * @param dest output data buffer pointer
 * 
 * @return 0 if success or fail
 */
int esp_sha384_finish(esp_sha384_t *ctx, void *dest);

/**
 * @brief output SHA512 calculation result
 * 
 * @param ctx SHA512 contex pointer
 * @param dest output data buffer pointer
 * 
 * @return 0 if success or fail
 */
int esp_sha512_finish(esp_sha512_t *ctx, void *dest);

#ifdef __cplusplus
}
#endif

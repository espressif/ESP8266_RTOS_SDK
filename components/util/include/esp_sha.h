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

typedef int (*sha_cal_t)(void *ctx, const void *src);

typedef enum {
    SHA1 = 0,
    SHA224 = 1,
    SHA256 = 2,
    SHA384 = 3,
    SHA512 = 4,

    SHA_INVALID = -1,
} esp_sha_type_t;

typedef struct {
    esp_sha_type_t  type;               /*!< The sha type */
    uint8_t         buffer[64];         /*!< The data block being processed. */
    uint32_t        total[2];           /*!< The number of Bytes processed.  */
    uint32_t        state[8];           /*!< The intermediate digest state.  */
    sha_cal_t       sha_cal;            /*!< The sha calculation. */
} esp_sha_t;

typedef struct {
    esp_sha_type_t  type;               /*!< The sha type */
    uint8_t         buffer[128];        /*!< The data block being processed. */
    uint64_t        total[2];           /*!< The number of Bytes processed.  */
    uint64_t        state[8];           /*!< The intermediate digest state.  */
    sha_cal_t       sha_cal;            /*!< The sha calculation. */
} esp_sha512_t;

typedef esp_sha_t       esp_sha1_t;
typedef esp_sha_t       esp_sha224_t;
typedef esp_sha_t       esp_sha256_t;
typedef esp_sha512_t    esp_sha384_t;

/**
 * @brief initialize the SHA(1/224/256) contex
 *
 * @param ctx SHA contex pointer
 * @param type SHA type
 * @param state_ctx SHA calculation factor
 * @param size calculation factor size by "uint32_t"
 * @param sha_cal calculation function for real SHA
 *
 * @return 0 if success or fail
 */
int __esp_sha_init(esp_sha_t *ctx, esp_sha_type_t type, const uint32_t *state_ctx, size_t size, sha_cal_t sha_cal);

/**
 * @brief initialize the SHA(384/512) contex
 *
 * @param ctx SHA contex pointer
 * @param type SHA type
 * @param state_ctx SHA calculation factor
 * @param size calculation factor size by "uint64_t"
 *
 * @return 0 if success or fail
 */
int __esp_sha512_init(esp_sha512_t *ctx, esp_sha_type_t type, const uint64_t *state_ctx, size_t size);

/**
 * @brief initialize the SHA1 contex
 * 
 * @param ctx SHA1 contex pointer
 * 
 * @return 0 if success or fail
 */
static inline int esp_sha1_init(esp_sha1_t *ctx)
{
    extern const uint32_t __g_esp_sha1_state_ctx[];
    extern int __esp_sha1_process(void *ctx, const void *data);

    return __esp_sha_init(ctx, SHA1, __g_esp_sha1_state_ctx, 5, __esp_sha1_process);
}

/**
 * @brief initialize the SHA224 contex
 * 
 * @param ctx SHA224 contex pointer
 * 
 * @return 0 if success or fail
 */
static inline int esp_sha224_init(esp_sha224_t *ctx)
{
    extern const uint32_t __g_esp_sha224_state_ctx[];
    extern int __esp_sha256_process(void *ctx, const void *data);

    return __esp_sha_init(ctx, SHA224, __g_esp_sha224_state_ctx, 8, __esp_sha256_process);
}

/**
 * @brief initialize the SHA256 contex
 * 
 * @param ctx SHA256 contex pointer
 * 
 * @return 0 if success or fail
 */
static inline int esp_sha256_init(esp_sha256_t *ctx)
{
    extern const uint32_t __g_esp_sha256_state_ctx[];
    extern int __esp_sha256_process(void *ctx, const void *data);

    return __esp_sha_init(ctx, SHA256, __g_esp_sha256_state_ctx, 8, __esp_sha256_process);
}

/**
 * @brief initialize the SHA384 contex
 * 
 * @param ctx SHA384 contex pointer
 * 
 * @return 0 if success or fail
 */
static inline int esp_sha384_init(esp_sha384_t *ctx)
{
    extern const uint64_t __g_esp_sha384_state_ctx[];

    return __esp_sha512_init(ctx, SHA384, __g_esp_sha384_state_ctx, 8);
}

/**
 * @brief initialize the SHA512 contex
 * 
 * @param ctx SHA512 contex pointer
 * 
 * @return 0 if success or fail
 */
static inline int esp_sha512_init(esp_sha512_t *ctx)
{
    extern const uint64_t __g_esp_sha512_state_ctx[];

    return __esp_sha512_init(ctx, SHA512, __g_esp_sha512_state_ctx, 8);
}

/**
 * @brief calculate input data for SHA
 * 
 * @param ctx SHA contex pointer
 * @param src input data buffer pointer
 * @param size input data bytes
 * 
 * @return 0 if success or fail
 */
int __esp_sha_update(esp_sha_t *ctx, const void *src, size_t size);

/**
 * @brief calculate input data for SHA1
 * 
 * @param ctx SHA1 contex pointer
 * @param src input data buffer pointer
 * @param size input data bytes
 * 
 * @return 0 if success or fail
 */
static inline int esp_sha1_update(esp_sha1_t *ctx, const void *src, size_t size)
{
    return __esp_sha_update(ctx, src, size);
}

/**
 * @brief calculate input data for SHA224
 * 
 * @param ctx SHA224 contex pointer
 * @param src input data buffer pointer
 * @param size input data bytes
 * 
 * @return 0 if success or fail
 */
static inline int esp_sha224_update(esp_sha224_t *ctx, const void *src, size_t size)
{
    return __esp_sha_update(ctx, src, size);
}

/**
 * @brief calculate input data for SHA256
 * 
 * @param ctx SHA256 contex pointer
 * @param src input data buffer pointer
 * @param size input data bytes
 * 
 * @return 0 if success or fail
 */
static inline int esp_sha256_update(esp_sha256_t *ctx, const void *src, size_t size)
{
    return __esp_sha_update(ctx, src, size);
}

/**
 * @brief calculate input data for SHA384
 * 
 * @param ctx SHA384 contex pointer
 * @param src input data buffer pointer
 * @param size input data bytes
 * 
 * @return 0 if success or fail
 */
static inline int esp_sha384_update(esp_sha384_t *ctx, const void *src, size_t size)
{
    return __esp_sha_update((esp_sha_t *)ctx, src, size);
}

/**
 * @brief calculate input data for SHA512
 * 
 * @param ctx SHA512 contex pointer
 * @param src input data buffer pointer
 * @param size input data bytes
 * 
 * @return 0 if success or fail
 */
static inline int esp_sha512_update(esp_sha512_t *ctx, const void *src, size_t size)
{
    return __esp_sha_update((esp_sha_t *)ctx, src, size);
}

/**
 * @brief output SHA(1/224/256/384/512) calculation result
 * 
 * @param ctx SHA contex pointer
 * @param dest output data buffer pointer
 * 
 * @return 0 if success or fail
 */
int __esp_sha_finish(esp_sha_t *ctx, void *dest);

/**
 * @brief output SHA1 calculation result
 * 
 * @param ctx SHA1 contex pointer
 * @param dest output data buffer pointer
 * 
 * @return 0 if success or fail
 */
static inline int esp_sha1_finish(esp_sha1_t *ctx, void *dest)
{
    return __esp_sha_finish(ctx, dest);
}

/**
 * @brief output SHA224 calculation result
 * 
 * @param ctx SHA224 contex pointer
 * @param dest output data buffer pointer
 * 
 * @return 0 if success or fail
 */
static inline int esp_sha224_finish(esp_sha224_t *ctx, void *dest)
{
    return __esp_sha_finish(ctx, dest);
}

/**
 * @brief output SHA256 calculation result
 * 
 * @param ctx SHA256 contex pointer
 * @param dest output data buffer pointer
 * 
 * @return 0 if success or fail
 */
static inline int esp_sha256_finish(esp_sha256_t *ctx, void *dest)
{
    return __esp_sha_finish(ctx, dest);
}

/**
 * @brief output SHA384 calculation result
 * 
 * @param ctx SHA384 contex pointer
 * @param dest output data buffer pointer
 * 
 * @return 0 if success or fail
 */
static inline int esp_sha384_finish(esp_sha384_t *ctx, void *dest)
{
    return __esp_sha_finish((esp_sha_t *)ctx, dest);
}

/**
 * @brief output SHA512 calculation result
 * 
 * @param ctx SHA512 contex pointer
 * @param dest output data buffer pointer
 * 
 * @return 0 if success or fail
 */
static inline int esp_sha512_finish(esp_sha512_t *ctx, void *dest)
{
    return __esp_sha_finish((esp_sha_t *)ctx, dest);
}

#ifdef __cplusplus
}
#endif

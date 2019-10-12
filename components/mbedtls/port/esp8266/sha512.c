/**
 * \brief AES block cipher, ESP8266 accelerated version
 * Based on mbedTLS FIPS-197 compliant version.
 *
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  Additions Copyright (C) 2016-2017, Espressif Systems (Shanghai) PTE Ltd
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */

#include <string.h>
#include "mbedtls/sha512.h"

#if defined(MBEDTLS_SHA512_ALT)

void mbedtls_sha512_init(mbedtls_sha512_context *ctx)
{
    memset(ctx, 0, sizeof(mbedtls_sha512_context));
}

void mbedtls_sha512_free(mbedtls_sha512_context *ctx)
{
    memset(ctx, 0, sizeof(mbedtls_sha512_context));
}

void mbedtls_sha512_clone(mbedtls_sha512_context *dst, const mbedtls_sha512_context *src)
{
    memcpy(dst, src, sizeof(mbedtls_sha512_context));
}

int mbedtls_sha512_starts_ret(mbedtls_sha512_context *ctx, int is384)
{
    int ret;

    if (is384)
        ret = esp_sha384_init(ctx);
    else
        ret = esp_sha512_init(ctx);

    return ret;
}

int mbedtls_sha512_update_ret(mbedtls_sha512_context *ctx, const unsigned char *input, size_t ilen)
{
    return esp_sha512_update(ctx, input, ilen);
}

int mbedtls_sha512_finish_ret(mbedtls_sha512_context *ctx, unsigned char output[64])
{
    return esp_sha512_finish(ctx, output);
}

int mbedtls_internal_sha512_process(mbedtls_sha512_context *ctx, const unsigned char data[128])
{
    return esp_sha512_update(ctx, data, 128);
}

// int mbedtls_sha512_ret(const unsigned char *input, size_t ilen, unsigned char output[64], int is384)
// {
    
// }

#endif /* MBEDTLS_AES_ALT */

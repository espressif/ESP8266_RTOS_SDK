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
#include "mbedtls/aes.h"

#if defined(MBEDTLS_AES_ALT)

void mbedtls_aes_init(mbedtls_aes_context *ctx)
{
    memset(ctx, 0, sizeof(mbedtls_aes_context));
}

void mbedtls_aes_free(mbedtls_aes_context *ctx)
{
    memset(ctx, 0, sizeof(mbedtls_aes_context));
}

#if defined(MBEDTLS_CIPHER_MODE_XTS)
void mbedtls_aes_xts_init(mbedtls_aes_xts_context *ctx)
{
    memset(ctx, 0, sizeof(mbedtls_aes_xts_context));
}

void mbedtls_aes_xts_free(mbedtls_aes_xts_context *ctx)
{
    memset(ctx, 0, sizeof(mbedtls_aes_xts_context));
}

int mbedtls_aes_xts_setkey_enc(mbedtls_aes_xts_context *ctx,
                               const unsigned char *key,
                               unsigned int keybits)
{
    return esp_aes_xts_set_encrypt_key(ctx, key, keybits);
}

int mbedtls_aes_xts_setkey_dec(mbedtls_aes_xts_context *ctx,
                               const unsigned char *key,
                               unsigned int keybits)
{
    return esp_aes_xts_set_decrypt_key(ctx, key, keybits);   
}

int mbedtls_aes_crypt_xts(mbedtls_aes_xts_context *ctx,
                          int mode,
                          size_t length,
                          const unsigned char data_unit[16],
                          const unsigned char *input,
                          unsigned char *output)
{
    return esp_aes_crypt_xts(ctx, MBEDTLS_AES_ENCRYPT == mode, length, data_unit, input, output);
}
#endif

int mbedtls_aes_setkey_enc(mbedtls_aes_context *ctx, const unsigned char *key,
                    unsigned int keybits)
{
    return esp_aes_set_encrypt_key(ctx, key, keybits);
}

int mbedtls_aes_setkey_dec(mbedtls_aes_context *ctx, const unsigned char *key,
                    unsigned int keybits)
{
    return esp_aes_set_decrypt_key(ctx, key, keybits);
}

int mbedtls_aes_crypt_ecb(mbedtls_aes_context *ctx,
                    int mode,
                    const unsigned char input[16],
                    unsigned char output[16])
{
    if (MBEDTLS_AES_DECRYPT == mode)
        return esp_aes_decrypt_ecb(ctx, input, output);
    else
        return esp_aes_encrypt_ecb(ctx, input, output);
}

#if defined(MBEDTLS_CIPHER_MODE_CBC)
int mbedtls_aes_crypt_cbc(mbedtls_aes_context *ctx,
                    int mode,
                    size_t length,
                    unsigned char iv[16],
                    const unsigned char *input,
                    unsigned char *output)
{
    if (MBEDTLS_AES_DECRYPT == mode)
        return esp_aes_decrypt_cbc(ctx, input, length, output, length, iv);
    else
        return esp_aes_encrypt_cbc(ctx, input, length, output, length, iv);        
}
#endif

#if defined(MBEDTLS_CIPHER_MODE_CFB)
int mbedtls_aes_crypt_cfb128(mbedtls_aes_context *ctx,
                       int mode,
                       size_t length,
                       size_t *iv_off,
                       unsigned char iv[16],
                       const unsigned char *input,
                       unsigned char *output)
{
    if (MBEDTLS_AES_DECRYPT == mode)
        return esp_aes_decrypt_cfb128(ctx, input, length, output, length, iv, iv_off);
    else
        return esp_aes_encrypt_cfb128(ctx, input, length, output, length, iv, iv_off);
}

int mbedtls_aes_crypt_cfb8(mbedtls_aes_context *ctx,
                    int mode,
                    size_t length,
                    unsigned char iv[16],
                    const unsigned char *input,
                    unsigned char *output)
{
    if (MBEDTLS_AES_DECRYPT == mode)
        return esp_aes_decrypt_cfb8(ctx, input, length, output, length, iv);
    else
        return esp_aes_encrypt_cfb8(ctx, input, length, output, length, iv);
}
#endif

#if defined(MBEDTLS_CIPHER_MODE_OFB)
int mbedtls_aes_crypt_ofb(mbedtls_aes_context *ctx,
                       size_t length,
                       size_t *iv_off,
                       unsigned char iv[16],
                       const unsigned char *input,
                       unsigned char *output)
{
    return esp_aes_crypt_ofb(ctx, length, iv_off, iv, input, output);
}
#endif

#if defined(MBEDTLS_CIPHER_MODE_CTR)
int mbedtls_aes_crypt_ctr(mbedtls_aes_context *ctx,
                       size_t length,
                       size_t *nc_off,
                       unsigned char nonce_counter[16],
                       unsigned char stream_block[16],
                       const unsigned char *input,
                       unsigned char *output)
{
    return esp_aes_encrypt_ctr(ctx, nc_off, nonce_counter, stream_block, input,
                               length, output, length);
}      
#endif

int mbedtls_internal_aes_encrypt(mbedtls_aes_context *ctx,
                                 const unsigned char input[16],
                                 unsigned char output[16])
{
    return esp_aes_encrypt(ctx, input, 16, output, 16);
}

int mbedtls_internal_aes_decrypt(mbedtls_aes_context *ctx,
                                 const unsigned char input[16],
                                 unsigned char output[16])
{
    return esp_aes_decrypt(ctx, input, 16, output, 16);
}

#endif /* MBEDTLS_AES_ALT */

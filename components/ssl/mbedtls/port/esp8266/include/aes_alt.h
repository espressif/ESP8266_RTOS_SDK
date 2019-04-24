/**
 * \file aes_alt.h
 *
 * \brief AES block cipher
 *
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
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
 *
 */
#ifndef AES_ALT_H
#define AES_ALT_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(MBEDTLS_AES_ALT)
#include "esp_aes.h"

typedef esp_aes_t mbedtls_aes_context;

#define mbedtls_aes_init(_ctx)                              { }
#define mbedtls_aes_free(_ctx)                              { }
#define mbedtls_aes_setkey_enc(_ctx, _key, _keybits)        esp_aes_set_encrypt_key(_ctx, _key, _keybits)
#define mbedtls_aes_setkey_dec(_ctx, _key, _keybits)        esp_aes_set_decrypt_key(_ctx, _key, _keybits)

#define mbedtls_aes_crypt_ecb(_ctx, _mode, _input, _output)                                         \
({                                                                                                  \
    int ret;                                                                                        \
                                                                                                    \
    if (_mode == MBEDTLS_AES_DECRYPT)                                                               \
        ret = esp_aes_decrypt_ecb(_ctx, _input, _output);                                           \
    else if (_mode == MBEDTLS_AES_ENCRYPT)                                                          \
        ret = esp_aes_encrypt_ecb(_ctx, _input, _output);                                           \
    else                                                                                            \
        ret = -1;                                                                                   \
                                                                                                    \
    ret;                                                                                            \
})

#if defined(MBEDTLS_CIPHER_MODE_CBC)
#define mbedtls_aes_crypt_cbc(_ctx, _mode, _length, _iv, _input, _output)                           \
({                                                                                                  \
    int ret;                                                                                        \
                                                                                                    \
    if (_mode == MBEDTLS_AES_DECRYPT)                                                               \
        ret = esp_aes_decrypt_cbc(_ctx, _input, _length, _output, _length, _iv);                    \
    else if (_mode == MBEDTLS_AES_ENCRYPT)                                                          \
        ret = esp_aes_encrypt_cbc(_ctx, _input, _length, _output, _length, _iv);                    \
    else                                                                                            \
        ret = -1;                                                                                   \
                                                                                                    \
    ret;                                                                                            \
})
#endif

#if defined(MBEDTLS_CIPHER_MODE_CFB)
#define mbedtls_aes_crypt_cfb128(_ctx, _mode, _length, _iv_off, _iv, _input, _output)               \
({                                                                                                  \
    int ret;                                                                                        \
                                                                                                    \
    if (_mode == MBEDTLS_AES_DECRYPT)                                                               \
        ret = esp_aes_decrypt_cfb128(_ctx, _input, _length, _output, _length, _iv, _iv_off);        \
    else if (_mode == MBEDTLS_AES_ENCRYPT)                                                          \
        ret = esp_aes_encrypt_cfb128(_ctx, _input, _length, _output, _length, _iv, _iv_off);        \
    else                                                                                            \
        ret = -1;                                                                                   \
                                                                                                    \
    ret;                                                                                            \
})

#define mbedtls_aes_crypt_cfb8(_ctx, _mode, _length, _iv, _input, _output)                          \
({                                                                                                  \
    int ret;                                                                                        \
                                                                                                    \
    if (_mode == MBEDTLS_AES_DECRYPT)                                                               \
        ret = esp_aes_decrypt_cfb8(_ctx, _input, _length, _output, _length, _iv);                   \
    else if (_mode == MBEDTLS_AES_ENCRYPT)                                                          \
        ret = esp_aes_encrypt_cfb8(_ctx, _input, _length, _output, _length, _iv);                   \
    else                                                                                            \
        ret = -1;                                                                                   \
                                                                                                    \
    ret;                                                                                            \
})
#endif

#if defined(MBEDTLS_CIPHER_MODE_CTR)
#define mbedtls_aes_crypt_ctr(_ctx, _length, _nc_off, _nonce_counter,                               \
                              _stream_block, _input, _output)                                       \
                                                                                                    \
        esp_aes_encrypt_ctr(_ctx, _nc_off, _nonce_counter, _stream_block, _input,                   \
                            _length, _output, _length)
#endif

#define mbedtls_internal_aes_encrypt(_ctx, _input, _output)     esp_aes_encrypt(_ctx, _input, 16, _output, 16)
#define mbedtls_internal_aes_decrypt(_ctx, _input, _output)     esp_aes_decrypt(_ctx, _input, 16, _output, 16)
#endif /* MBEDTLS_AES_ALT */

#ifdef __cplusplus
}
#endif

#endif

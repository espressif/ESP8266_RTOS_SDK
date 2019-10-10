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

typedef struct esp_aes {
    int32_t     nr;                         /*!< The number of AES key bits */
    uint32_t    *rk;                        /*!< The AES AES key */
    uint32_t    buf[68];                    /*!< The AES calculation cache */
} esp_aes_t;

typedef struct esp_aes_xts {
    esp_aes_t   crypt;                      /*!< The AES context to use for AES block encryption or decryption. */
    esp_aes_t   tweak;                      /*!< The AES context used for tweak computation. */
} esp_aes_xts_t;

/**
 * @brief Set AES encrypt key
 *
 * @param aes AES contex pointer
 * @param p_key AES key data buffer
 * @param keybits number of AES key bits
 *
 * @return 0 if success or fail
 */
int esp_aes_set_encrypt_key(esp_aes_t *aes, const void *p_key, size_t keybits);

/**
 * @brief Set AES decrypt key
 *
 * @param aes AES contex pointer
 * @param p_key AES key data buffer
 * @param keybits number of AES key bits
 *
 * @return 0 if success or fail
 */
int esp_aes_set_decrypt_key(esp_aes_t *aes, const void *key, size_t keybits);

/**
 * @brief AES normal encrypt calculation
 *
 * @param aes AES contex pointer
 * @param p_src input data buffer 
 * @param slen input data length by bytes
 * @param p_dst output data buffer
 * @param dlen output data length by bytes
 *
 * @return 0 if success or fail
 */
int esp_aes_encrypt(esp_aes_t *aes, const void *p_src, size_t slen, void *p_dst, size_t dlen);

/**
 * @brief AES normal decrypt calculation
 *
 * @param aes AES contex pointer
 * @param p_src input data buffer 
 * @param slen input data length by bytes
 * @param p_dst output data buffer
 * @param dlen output data length by bytes
 *
 * @return 0 if success or fail
 */
int esp_aes_decrypt(esp_aes_t *aes, const void *p_src, size_t slen, void *p_dst, size_t dlen);

/**
 * @brief AES-ECB encrypt calculation
 *
 * @param aes AES contex pointer
 * @param p_src input data buffer 
 * @param p_dst output data buffer
 *
 * @return 0 if success or fail
 */
static inline int esp_aes_encrypt_ecb(esp_aes_t *aes, const void *p_src, void *p_dst)
{
    return esp_aes_encrypt(aes, p_src, 16, p_dst, 16);
}

/**
 * @brief AES-ECB decrypt calculation
 *
 * @param aes AES contex pointer
 * @param p_src input data buffer 
 * @param p_dst output data buffer
 *
 * @return 0 if success or fail
 */
static inline int esp_aes_decrypt_ecb(esp_aes_t *aes, const void *p_src, void *p_dst)
{
    return esp_aes_decrypt(aes, p_src, 16, p_dst, 16);
}

/**
 * @brief AES-CBC encrypt calculation
 *
 * @param aes AES contex pointer
 * @param p_src input data buffer 
 * @param slen input data length by bytes
 * @param p_dst output data buffer
 * @param dlen output data length by bytes
 * @param p_iv initialization vector buffer
 *
 * @return 0 if success or fail
 */
int esp_aes_encrypt_cbc(esp_aes_t *aes, const void *p_src, size_t slen, void *p_dst, size_t dlen, void *p_iv);

/**
 * @brief AES-CBC decrypt calculation
 *
 * @param aes AES contex pointer
 * @param p_src input data buffer 
 * @param slen input data length by bytes
 * @param p_dst output data buffer
 * @param dlen output data length by bytes
 * @param p_iv initialization vector buffer
 *
 * @return 0 if success or fail
 */
int esp_aes_decrypt_cbc(esp_aes_t *aes, const void *p_src, size_t slen, void *p_dst, size_t dlen, void *p_iv);

/**
 * @brief AES-CFB128 encrypt calculation
 *
 * @param aes AES contex pointer
 * @param p_src input data buffer 
 * @param slen input data length by bytes
 * @param p_dst output data buffer
 * @param dlen output data length by bytes
 * @param p_iv initialization vector buffer
 * @param iv_off initialization vector offset
 *
 * @return 0 if success or fail
 */
int esp_aes_encrypt_cfb128(esp_aes_t *aes, const void *p_src, size_t slen, void *p_dst, size_t dlen, void *p_iv, size_t *iv_off);

/**
 * @brief AES-CFB128 decrypt calculation
 *
 * @param aes AES contex pointer
 * @param p_src input data buffer 
 * @param slen input data length by bytes
 * @param p_dst output data buffer
 * @param dlen output data length by bytes
 * @param p_iv initialization vector buffer
 * @param iv_off initialization vector offset
 *
 * @return 0 if success or fail
 */
int esp_aes_decrypt_cfb128(esp_aes_t *aes, const void *p_src, size_t slen, void *p_dst, size_t dlen, void *p_iv, size_t *iv_off);

/**
 * @brief AES-CFB8 encrypt calculation
 *
 * @param aes AES contex pointer
 * @param p_src input data buffer 
 * @param slen input data length by bytes
 * @param p_dst output data buffer
 * @param dlen output data length by bytes
 * @param p_iv initialization vector buffer
 *
 * @return 0 if success or fail
 */
int esp_aes_encrypt_cfb8(esp_aes_t *aes, const void *p_src, size_t slen, void *p_dst, size_t dlen, void *p_iv);

/**
 * @brief AES-CFB8 decrypt calculation
 *
 * @param aes AES contex pointer
 * @param p_src input data buffer 
 * @param slen input data length by bytes
 * @param p_dst output data buffer
 * @param dlen output data length by bytes
 * @param p_iv initialization vector buffer
 *
 * @return 0 if success or fail
 */
int esp_aes_decrypt_cfb8(esp_aes_t *aes, const void *p_src, size_t slen, void *p_dst, size_t dlen, void *p_iv);

/**
 * @brief AES-CTR encrypt calculation
 *
 * @param aes AES contex pointer
 * @param nc_off offset in the current stream block
 * @param p_nonce_counter 128-bit nonce and counter buffer
 * @param p_stream_block current stream block buffer
 * @param p_src input data buffer 
 * @param slen input data length by bytes
 * @param p_dst output data buffer
 * @param dlen output data length by bytes
 *
 * @return 0 if success or fail
 */
int esp_aes_encrypt_ctr(esp_aes_t *aes, size_t *nc_off, void *p_nonce_counter, void *p_stream_block, const void *p_src, size_t slen, void *p_dst, size_t dlen);

/**
 * @brief AES-CTR decrypt calculation
 *
 * @param aes AES contex pointer
 * @param nc_off offset in the current stream block
 * @param p_nonce_counter 128-bit nonce and counter buffer
 * @param p_stream_block current stream block buffer
 * @param p_src input data buffer 
 * @param slen input data length by bytes
 * @param p_dst output data buffer
 * @param dlen output data length by bytes
 *
 * @return 0 if success or fail
 */
static inline int esp_aes_decrypt_ctr(esp_aes_t *aes, size_t *nc_off, void *p_nonce_counter, void *p_stream_block, const void *p_src, size_t slen, void *p_dst, size_t dlen)
{
    return esp_aes_encrypt_ctr(aes, nc_off, p_nonce_counter, p_stream_block, p_src, slen, p_dst, dlen);   
}

/**
 * @brief Set AES XTS encrypt key
 *
 * @param aes AES XTS contex pointer
 * @param p_key AES XTS key data buffer
 * @param keybits number of AES XTS key bits
 *
 * @return 0 if success or fail
 */
int esp_aes_xts_set_encrypt_key(esp_aes_xts_t *aes, const void *p_key, size_t keybits);

/**
 * @brief Set AES XTS decrypt key
 *
 * @param aes AES XTS contex pointer
 * @param p_key AES XTS key data buffer
 * @param keybits number of AES XTS key bits
 *
 * @return 0 if success or fail
 */
int esp_aes_xts_set_decrypt_key(esp_aes_xts_t *aes, const void *p_key, size_t keybits);

/**
 * @brief AES XTS encrypt/decrypt calculation
 *
 * @param aes AES contex pointer
 * @param encrypt 1 : encrypt, 0 : decrypt
 * @param length data unit data length by bytes
 * @param p_data_unit data unit buffer
 * @param p_src input data buffer
 * @param p_dst output data buffer
 *
 * @return 0 if success or fail
 */
int esp_aes_crypt_xts(esp_aes_xts_t *aes, int encrypt, size_t length, const void *p_data_unit, const void *p_src, void *p_dst);

/**
 * @brief AES OFB encrypt/decrypt calculation
 *
 * @param aes AES contex pointer
 * @param length data length by bytes
 * @param iv_off IV offset
 * @param p_iv IV data buffer
 * @param p_src input data buffer
 * @param p_dst output data buffer
 *
 * @return 0 if success or fail
 */
int esp_aes_crypt_ofb(esp_aes_t *ctx, size_t length, size_t *iv_off, void *p_iv, const void *p_src, void *p_dst);

#ifdef __cplusplus
}
#endif

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

#if defined(MBEDTLS_CIPHER_MODE_XTS)
typedef esp_aes_xts_t mbedtls_aes_xts_context;
#endif

#ifdef __cplusplus
}
#endif

#endif

#endif /* MBEDTLS_AES_ALT */

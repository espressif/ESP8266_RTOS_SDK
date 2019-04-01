/*
 *  SHA-1 implementation with extra ESP8266 support added.
 *  Uses mbedTLS software implementation for failover when concurrent
 *  SHA operations are in use.
 *
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  Additions Copyright (C) 2016, Espressif Systems (Shanghai) PTE LTD
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
#ifndef _SHA1_ALT_H_
#define _SHA1_ALT_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(MBEDTLS_SHA1_ALT)

#include "esp_sha.h"

typedef esp_sha1_t mbedtls_sha1_context;

#define mbedtls_sha1_init(_ctx)                     { }

#define mbedtls_sha1_free(_ctx)                     { }

#define mbedtls_sha1_clone(_d, _s)                  { *(_d) = *(_s); }

#define mbedtls_sha1_starts_ret(_ctx)               esp_sha1_init(_ctx)

#define mbedtls_sha1_update_ret(_ctx, _s, _l)       esp_sha1_update(_ctx, _s, _l)

#define mbedtls_sha1_finish_ret(_ctx, _d)           esp_sha1_finish(_ctx, _d)

#define mbedtls_internal_sha1_process(_ctx, _s)     esp_sha1_update(_ctx, _s, 64)

#endif /* MBEDTLS_SHA1_ALT */

#ifdef __cplusplus
}
#endif

#endif

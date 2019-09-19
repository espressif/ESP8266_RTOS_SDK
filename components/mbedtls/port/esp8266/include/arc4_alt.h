/*
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
#ifndef ARC4_ALT_H
#define ARC4_ALT_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(MBEDTLS_ARC4_ALT)
#include "esp_arc4.h"

typedef esp_arc4_context mbedtls_arc4_context;

#define mbedtls_arc4_init(_ctx)                     { }

#define mbedtls_arc4_free(_ctx)                     { }

#define mbedtls_arc4_setup(_ctx, _s, _l)            esp_arc4_setup(_ctx, _s, _l)

#define mbedtls_arc4_crypt(_ctx, _l, _i, _o)        esp_arc4_encrypt(_ctx, _l, _i, _o)

#endif /* MBEDTLS_ARC4_ALT */

#ifdef __cplusplus
}
#endif

#endif


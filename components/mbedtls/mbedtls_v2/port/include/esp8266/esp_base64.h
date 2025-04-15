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

/**
 * @brief encode a base64-formatted buffer
 * 
 * @param p_src  input encoded data buffer pointer
 * @param slen   input data length by bytes
 * @param p_dst  output buffer pointer
 * @param dlen   output buffer length by bytes
 * 
 * @return the result
 *           0 : Success
 *    -ENOBUFS : output buffer it not enough
 */
int esp_base64_encode(const void *p_src, uint32_t slen, void *p_dst, uint32_t dlen);

/**
 * @brief decode a base64-formatted buffer
 * 
 * @param p_src  input encoded data buffer pointer
 * @param slen   input data length by bytes
 * @param p_dst  output buffer pointer
 * @param dlen   output buffer length by bytes
 * 
 * @return the result
 *           0 : Success
 *     -EINVAL : input parameter invalid
 *    -ENOBUFS : output buffer it not enough
 */
int esp_base64_decode(const void *p_src, uint32_t slen, void *p_dst, uint32_t dlen);

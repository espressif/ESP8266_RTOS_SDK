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

#include "sdkconfig.h"
#include "esp_base64.h"
#include "ibus_data.h"
#include "util_assert.h"
#include <sys/errno.h>

typedef union _cache {
    uint32_t u32d;
    uint8_t  u8d[4];
} cache_t;

/**
 * We use 4-byte align look-up table to speed up loading data
 */

static const uint32_t s_base64_enc_lut[16] = {
    0x44434241, 0x48474645, 0x4c4b4a49, 0x504f4e4d,
    0x54535251, 0x58575655, 0x62615a59, 0x66656463,
    0x6a696867, 0x6e6d6c6b, 0x7271706f, 0x76757473,
    0x7a797877, 0x33323130, 0x37363534, 0x2f2b3938
};

static const uint32_t s_base64_dec_lut[32] = {
    0x7f7f7f7f, 0x7f7f7f7f, 0x7f7f7f7f, 0x7f7f7f7f,
    0x7f7f7f7f, 0x7f7f7f7f, 0x7f7f7f7f, 0x7f7f7f7f,
    0x7f7f7f7f, 0x7f7f7f7f, 0x3e7f7f7f, 0x3f7f7f7f,
    0x37363534, 0x3b3a3938, 0x7f7f3d3c, 0x7f7f407f,
    0x0201007f, 0x06050403, 0x0a090807, 0x0e0d0c0b,
    0x1211100f, 0x16151413, 0x7f191817, 0x7f7f7f7f,
    0x1c1b1a7f, 0x201f1e1d, 0x24232221, 0x28272625,
    0x2c2b2a29, 0x302f2e2d, 0x7f333231, 0x7f7f7f7f
};

/*
 * Encode a buffer into base64 format
 */
int esp_base64_encode(const void *p_src, uint32_t slen, void *p_dst, uint32_t dlen)
{
    uint32_t i, n;
    uint32_t C1, C2, C3;
    uint8_t *p;
    uint8_t *dst = (uint8_t *)p_dst;
    const uint8_t *src = (const uint8_t *)p_src;

    assert(p_dst);
    assert(dlen);
    assert(p_src);
    assert(slen);

    n = slen / 3 + (slen % 3 != 0);
    n *= 4;

    if (dlen < (n + 1))
        return -ENOBUFS;

    n = (slen / 3) * 3;

    for (i = 0, p = dst; i < n; i += 3) {
        C1 = *src++;
        C2 = *src++;
        C3 = *src++;

        *p++ = ESP_IBUS_GET_U8_DATA((C1 >> 2) & 0x3F, s_base64_enc_lut);
        *p++ = ESP_IBUS_GET_U8_DATA((((C1 &  3) << 4) + (C2 >> 4)) & 0x3F, s_base64_enc_lut);
        *p++ = ESP_IBUS_GET_U8_DATA((((C2 & 15) << 2) + (C3 >> 6)) & 0x3F, s_base64_enc_lut);
        *p++ = ESP_IBUS_GET_U8_DATA(C3 & 0x3F, s_base64_enc_lut);
    }

    if (i < slen) {
        C1 = *src++;
        C2 = ((i + 1) < slen) ? *src++ : 0;

        *p++ = ESP_IBUS_GET_U8_DATA((C1 >> 2) & 0x3F, s_base64_enc_lut);
        *p++ = ESP_IBUS_GET_U8_DATA((((C1 & 3) << 4) + (C2 >> 4)) & 0x3F, s_base64_enc_lut);

        if ((i + 1) < slen)
            *p++ = ESP_IBUS_GET_U8_DATA(((C2 & 15) << 2) & 0x3F, s_base64_enc_lut);
        else
            *p++ = '=';

        *p++ = '=';
    }

    *p = 0;

    return p - dst;
}

/*
 * Decode a base64-formatted buffer
 */
int esp_base64_decode(const void *p_src, uint32_t slen, void *p_dst, uint32_t dlen)
{
    uint32_t i, n;
    uint32_t j, x;
    uint8_t *p;
    uint8_t *dst = (uint8_t *)p_dst;
    const uint8_t *src = (const uint8_t *)p_src;

    assert(p_dst);
    assert(dlen);
    assert(p_src);
    assert(slen);

    /* First pass: check for validity and get output length */
    for (i = n = j = 0; i < slen; i++) {
        /* Skip spaces before checking for EOL */
        x = 0;
        while (i < slen && src[i] == ' ') {
            ++i;
            ++x;
        }

        /* Spaces at end of buffer are OK */
        if (i == slen)
            break;

        if ((slen - i ) >= 2 && src[i] == '\r' && src[i + 1] == '\n')
            continue;

        if (src[i] == '\n')
            continue;

        /* Space inside a line is an error */
        if (x != 0)
            return -EINVAL;

        if (src[i] == '=' && ++j > 2)
            return -EINVAL;

        if (src[i] > 127 || ESP_IBUS_GET_U8_DATA(src[i], s_base64_dec_lut) == 127)
            return -EINVAL;

        if (ESP_IBUS_GET_U8_DATA(src[i], s_base64_dec_lut) < 64 && j != 0)
            return -EINVAL;

        n++;
    }

    if( n == 0 )
        return -EINVAL;

    n = (6 * (n >> 3)) + ((6 * (n & 0x7) + 7) >> 3);
    n -= j;

    if (dlen < n )
        return -ENOBUFS;

    for (j = 3, n = x = 0, p = dst; i > 0; i--, src++) {
        if (*src == '\r' || *src == '\n' || *src == ' ')
            continue;

        j -= (ESP_IBUS_GET_U8_DATA(*src, s_base64_dec_lut) == 64);
        x  = (x << 6) | (ESP_IBUS_GET_U8_DATA(*src, s_base64_dec_lut) & 0x3F);

        if (++n == 4) {
            n = 0;
            if (j > 0)
                *p++ = (uint8_t)(x >> 16);
            if (j > 1)
                *p++ = (uint8_t)(x >>  8);
            if (j > 2)
                *p++ = (uint8_t)(x >>  0);
        }
    }

    return p - dst;
}

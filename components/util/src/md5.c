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

#include <sys/errno.h>
#include <string.h>
#include "esp_md5.h"
#include "ibus_data.h"
#include "util_assert.h"

static const uint32_t esp_md5_init_value[4] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476};

static const uint8_t esp_md5_padding[64] =
{
 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

int esp_md5_process(esp_md5_context_t *ctx, const uint8_t data[64])
{
    uint32_t X[64], Y[4];
    util_assert(ctx);

    memcpy(X, data, 64);

#define S(x,n) ((x << n) | ((x & 0xFFFFFFFF) >> (32 - n)))

#define P(a,b,c,d,k,s,t)                                \
{                                                       \
    a += F(b,c,d) + X[k] + t; a = S(a,s) + b;           \
}

    memcpy(Y, ctx->state, sizeof(Y));

#define A Y[0]
#define B Y[1]
#define C Y[2]
#define D Y[3]

#define F(x,y,z) (z ^ (x & (y ^ z)))

    P(A, B, C, D,  0,  7, 0xD76AA478);
    P(D, A, B, C,  1, 12, 0xE8C7B756);
    P(C, D, A, B,  2, 17, 0x242070DB);
    P(B, C, D, A,  3, 22, 0xC1BDCEEE);
    P(A, B, C, D,  4,  7, 0xF57C0FAF);
    P(D, A, B, C,  5, 12, 0x4787C62A);
    P(C, D, A, B,  6, 17, 0xA8304613);
    P(B, C, D, A,  7, 22, 0xFD469501);
    P(A, B, C, D,  8,  7, 0x698098D8);
    P(D, A, B, C,  9, 12, 0x8B44F7AF);
    P(C, D, A, B, 10, 17, 0xFFFF5BB1);
    P(B, C, D, A, 11, 22, 0x895CD7BE);
    P(A, B, C, D, 12,  7, 0x6B901122);
    P(D, A, B, C, 13, 12, 0xFD987193);
    P(C, D, A, B, 14, 17, 0xA679438E);
    P(B, C, D, A, 15, 22, 0x49B40821);

#undef F

#define F(x,y,z) (y ^ (z & (x ^ y)))

    P(A, B, C, D,  1,  5, 0xF61E2562);
    P(D, A, B, C,  6,  9, 0xC040B340);
    P(C, D, A, B, 11, 14, 0x265E5A51);
    P(B, C, D, A,  0, 20, 0xE9B6C7AA);
    P(A, B, C, D,  5,  5, 0xD62F105D);
    P(D, A, B, C, 10,  9, 0x02441453);
    P(C, D, A, B, 15, 14, 0xD8A1E681);
    P(B, C, D, A,  4, 20, 0xE7D3FBC8);
    P(A, B, C, D,  9,  5, 0x21E1CDE6);
    P(D, A, B, C, 14,  9, 0xC33707D6);
    P(C, D, A, B,  3, 14, 0xF4D50D87);
    P(B, C, D, A,  8, 20, 0x455A14ED);
    P(A, B, C, D, 13,  5, 0xA9E3E905);
    P(D, A, B, C,  2,  9, 0xFCEFA3F8);
    P(C, D, A, B,  7, 14, 0x676F02D9);
    P(B, C, D, A, 12, 20, 0x8D2A4C8A);

#undef F

#define F(x,y,z) (x ^ y ^ z)

    P(A, B, C, D,  5,  4, 0xFFFA3942);
    P(D, A, B, C,  8, 11, 0x8771F681);
    P(C, D, A, B, 11, 16, 0x6D9D6122);
    P(B, C, D, A, 14, 23, 0xFDE5380C);
    P(A, B, C, D,  1,  4, 0xA4BEEA44);
    P(D, A, B, C,  4, 11, 0x4BDECFA9);
    P(C, D, A, B,  7, 16, 0xF6BB4B60);
    P(B, C, D, A, 10, 23, 0xBEBFBC70);
    P(A, B, C, D, 13,  4, 0x289B7EC6);
    P(D, A, B, C,  0, 11, 0xEAA127FA);
    P(C, D, A, B,  3, 16, 0xD4EF3085);
    P(B, C, D, A,  6, 23, 0x04881D05);
    P(A, B, C, D,  9,  4, 0xD9D4D039);
    P(D, A, B, C, 12, 11, 0xE6DB99E5);
    P(C, D, A, B, 15, 16, 0x1FA27CF8);
    P(B, C, D, A,  2, 23, 0xC4AC5665);

#undef F

#define F(x,y,z) (y ^ (x | ~z))

    P(A, B, C, D,  0,  6, 0xF4292244);
    P(D, A, B, C,  7, 10, 0x432AFF97);
    P(C, D, A, B, 14, 15, 0xAB9423A7);
    P(B, C, D, A,  5, 21, 0xFC93A039);
    P(A, B, C, D, 12,  6, 0x655B59C3);
    P(D, A, B, C,  3, 10, 0x8F0CCC92);
    P(C, D, A, B, 10, 15, 0xFFEFF47D);
    P(B, C, D, A,  1, 21, 0x85845DD1);
    P(A, B, C, D,  8,  6, 0x6FA87E4F);
    P(D, A, B, C, 15, 10, 0xFE2CE6E0);
    P(C, D, A, B,  6, 15, 0xA3014314);
    P(B, C, D, A, 13, 21, 0x4E0811A1);
    P(A, B, C, D,  4,  6, 0xF7537E82);
    P(D, A, B, C, 11, 10, 0xBD3AF235);
    P(C, D, A, B,  2, 15, 0x2AD7D2BB);
    P(B, C, D, A,  9, 21, 0xEB86D391);

#undef F

    ctx->state[0] += A;
    ctx->state[1] += B;
    ctx->state[2] += C;
    ctx->state[3] += D;

    return 0;
}

/*
 * MD5 context setup
 */
int esp_md5_init(esp_md5_context_t *ctx)
{
    util_assert(ctx);
    memset(ctx->total, 0, sizeof(ctx->total));
    memcpy(ctx->state, esp_md5_init_value, sizeof(ctx->state));

    return 0;
}

/*
 * MD5 process buffer
 */
int esp_md5_update(esp_md5_context_t *ctx, const uint8_t *input, size_t ilen)
{
    int ret;
    size_t fill;
    uint32_t left;

    util_assert(ctx);
    util_assert(input);
    util_assert(ilen > 0);

    left = (ctx->total[0] >> 3)& 0x3F;
    fill = 64 - left;

    ctx->total[0] += (uint32_t) ilen << 3;
    if(ctx->total[0] < (uint32_t) ilen << 3)
        ctx->total[1]++;

    if(left && ilen >= fill)
    {
        memcpy((void *) (ctx->buffer + left), input, fill);
        if((ret = esp_md5_process(ctx, ctx->buffer)) != 0)
            return ret;

        input += fill;
        ilen  -= fill;
        left = 0;
    }

    while(ilen >= 64)
    {
        if((ret = esp_md5_process(ctx, input)) != 0)
            return ret;

        input += 64;
        ilen  -= 64;
    }

    if(ilen > 0)
    {
        memcpy((void *) (ctx->buffer + left), input, ilen);
    }

    return 0;
}

/*
 * MD5 final digest
 */
int esp_md5_final(esp_md5_context_t *ctx, uint8_t output[16])
{
    int ret;
    uint32_t last, padn;
    uint8_t msglen[8];

    util_assert(ctx);

    last = (ctx->total[0] >> 3)  & 0x3F;
    padn = (last < 56) ? (56 - last) : (120 - last);

    memcpy(msglen, ctx->total, 8);
    if((ret = esp_md5_update(ctx, esp_md5_padding, padn)) != 0)
        return ret ;

    if((ret = esp_md5_update(ctx, msglen, 8)) != 0)
        return ret ;

    memcpy(output, ctx->state, 16);
    return 0;
}


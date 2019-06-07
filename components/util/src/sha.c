// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <string.h>
#include "util_assert.h"
#include <sys/errno.h>
#include "esp_sha.h"
#include "esp_log.h"

#define UL64(x) x##ULL

#define F0(x, y, z) ((x & y) | (z & (x | y)))
#define F1(x, y, z) (z ^ (x & (y ^ z)))

#define SHR(x, n)  ((x & 0xFFFFFFFF) >> n)
#define ROTR(x, n) (SHR(x,n) | (x << (32 - n)))

#define S0(x) (ROTR(x, 7) ^ ROTR(x,18) ^  SHR(x, 3))
#define S1(x) (ROTR(x,17) ^ ROTR(x,19) ^  SHR(x,10))

#define S2(x) (ROTR(x, 2) ^ ROTR(x,13) ^ ROTR(x,22))
#define S3(x) (ROTR(x, 6) ^ ROTR(x,11) ^ ROTR(x,25))

#define TAG "SHA"

static const uint32_t sha_padding[] = {
 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

const uint32_t __g_esp_sha1_state_ctx[] = {
    0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0
};

const uint32_t __g_esp_sha224_state_ctx[] = {
    0xC1059ED8, 0x367CD507, 0x3070DD17, 0xF70E5939,
    0xFFC00B31, 0x68581511, 0x64F98FA7, 0xBEFA4FA4
};

const uint32_t __g_esp_sha256_state_ctx[] = {
    0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A,
    0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19
};

const uint64_t __g_esp_sha384_state_ctx[] = {
    0xCBBB9D5DC1059ED8, 0x629A292A367CD507, 0x9159015A3070DD17,
    0x152FECD8F70E5939, 0x67332667FFC00B31, 0x8EB44A8768581511,
    0xDB0C2E0D64F98FA7, 0x47B5481DBEFA4FA4
};

const uint64_t __g_esp_sha512_state_ctx[] = {
    0x6A09E667F3BCC908, 0xBB67AE8584CAA73B, 0x3C6EF372FE94F82B,
    0xA54FF53A5F1D36F1, 0x510E527FADE682D1, 0x9B05688C2B3E6C1F,
    0x1F83D9ABFB41BD6B, 0x5BE0CD19137E2179
};

static void esp_sha_put_be(void *dest, const void *src, size_t size, size_t steps)
{
    uint8_t *d_buf = (uint8_t *)dest;
    const uint8_t *s_buf = (const uint8_t *)src;

    for (int i = 0; i < size; i += steps) {
        for (int j = 0; j < steps; j++) {
            d_buf[i + j] = s_buf[i + (steps - j - 1)];
        }
    }
}

int __esp_sha1_process(void *in_ctx, const void *src)
{
    const uint8_t *data = (const uint8_t *)src;
    esp_sha_t *ctx = (esp_sha_t *)in_ctx;

    uint32_t temp, W[16], A[5];

    esp_sha_put_be(W, data, 64, sizeof(uint32_t));

#undef S
#undef R
#undef P
#undef F
#undef K

#define S(x,n) ((x << n) | ((x & 0xFFFFFFFF) >> (32 - n)))

#define R(t)                                            \
(                                                       \
    temp = W[( t -  3 ) & 0x0F] ^ W[( t - 8 ) & 0x0F] ^ \
           W[( t - 14 ) & 0x0F] ^ W[  t       & 0x0F],  \
    ( W[t & 0x0F] = S(temp,1) )                         \
)

#define P(a,b,c,d,e,x)                                  \
{                                                       \
    e += S(a,5) + F(b,c,d) + K + x; b = S(b,30);        \
}

    for (int i = 0; i < 5; i++)
        A[i] = ctx->state[i];

#define F(x,y,z) (z ^ (x & (y ^ z)))
#define K 0x5A827999

    P( A[0], A[1], A[2], A[3], A[4], W[0]  );
    P( A[4], A[0], A[1], A[2], A[3], W[1]  );
    P( A[3], A[4], A[0], A[1], A[2], W[2]  );
    P( A[2], A[3], A[4], A[0], A[1], W[3]  );
    P( A[1], A[2], A[3], A[4], A[0], W[4]  );
    P( A[0], A[1], A[2], A[3], A[4], W[5]  );
    P( A[4], A[0], A[1], A[2], A[3], W[6]  );
    P( A[3], A[4], A[0], A[1], A[2], W[7]  );
    P( A[2], A[3], A[4], A[0], A[1], W[8]  );
    P( A[1], A[2], A[3], A[4], A[0], W[9]  );
    P( A[0], A[1], A[2], A[3], A[4], W[10] );
    P( A[4], A[0], A[1], A[2], A[3], W[11] );
    P( A[3], A[4], A[0], A[1], A[2], W[12] );
    P( A[2], A[3], A[4], A[0], A[1], W[13] );
    P( A[1], A[2], A[3], A[4], A[0], W[14] );
    P( A[0], A[1], A[2], A[3], A[4], W[15] );
    P( A[4], A[0], A[1], A[2], A[3], R(16) );
    P( A[3], A[4], A[0], A[1], A[2], R(17) );
    P( A[2], A[3], A[4], A[0], A[1], R(18) );
    P( A[1], A[2], A[3], A[4], A[0], R(19) );

#undef K
#undef F

#define F(x,y,z) (x ^ y ^ z)
#define K 0x6ED9EBA1

    P( A[0], A[1], A[2], A[3], A[4], R(20) );
    P( A[4], A[0], A[1], A[2], A[3], R(21) );
    P( A[3], A[4], A[0], A[1], A[2], R(22) );
    P( A[2], A[3], A[4], A[0], A[1], R(23) );
    P( A[1], A[2], A[3], A[4], A[0], R(24) );
    P( A[0], A[1], A[2], A[3], A[4], R(25) );
    P( A[4], A[0], A[1], A[2], A[3], R(26) );
    P( A[3], A[4], A[0], A[1], A[2], R(27) );
    P( A[2], A[3], A[4], A[0], A[1], R(28) );
    P( A[1], A[2], A[3], A[4], A[0], R(29) );
    P( A[0], A[1], A[2], A[3], A[4], R(30) );
    P( A[4], A[0], A[1], A[2], A[3], R(31) );
    P( A[3], A[4], A[0], A[1], A[2], R(32) );
    P( A[2], A[3], A[4], A[0], A[1], R(33) );
    P( A[1], A[2], A[3], A[4], A[0], R(34) );
    P( A[0], A[1], A[2], A[3], A[4], R(35) );
    P( A[4], A[0], A[1], A[2], A[3], R(36) );
    P( A[3], A[4], A[0], A[1], A[2], R(37) );
    P( A[2], A[3], A[4], A[0], A[1], R(38) );
    P( A[1], A[2], A[3], A[4], A[0], R(39) );

#undef K
#undef F

#define F(x,y,z) ((x & y) | (z & (x | y)))
#define K 0x8F1BBCDC

    P( A[0], A[1], A[2], A[3], A[4], R(40) );
    P( A[4], A[0], A[1], A[2], A[3], R(41) );
    P( A[3], A[4], A[0], A[1], A[2], R(42) );
    P( A[2], A[3], A[4], A[0], A[1], R(43) );
    P( A[1], A[2], A[3], A[4], A[0], R(44) );
    P( A[0], A[1], A[2], A[3], A[4], R(45) );
    P( A[4], A[0], A[1], A[2], A[3], R(46) );
    P( A[3], A[4], A[0], A[1], A[2], R(47) );
    P( A[2], A[3], A[4], A[0], A[1], R(48) );
    P( A[1], A[2], A[3], A[4], A[0], R(49) );
    P( A[0], A[1], A[2], A[3], A[4], R(50) );
    P( A[4], A[0], A[1], A[2], A[3], R(51) );
    P( A[3], A[4], A[0], A[1], A[2], R(52) );
    P( A[2], A[3], A[4], A[0], A[1], R(53) );
    P( A[1], A[2], A[3], A[4], A[0], R(54) );
    P( A[0], A[1], A[2], A[3], A[4], R(55) );
    P( A[4], A[0], A[1], A[2], A[3], R(56) );
    P( A[3], A[4], A[0], A[1], A[2], R(57) );
    P( A[2], A[3], A[4], A[0], A[1], R(58) );
    P( A[1], A[2], A[3], A[4], A[0], R(59) );

#undef K
#undef F

#define F(x,y,z) (x ^ y ^ z)
#define K 0xCA62C1D6

    P( A[0], A[1], A[2], A[3], A[4], R(60) );
    P( A[4], A[0], A[1], A[2], A[3], R(61) );
    P( A[3], A[4], A[0], A[1], A[2], R(62) );
    P( A[2], A[3], A[4], A[0], A[1], R(63) );
    P( A[1], A[2], A[3], A[4], A[0], R(64) );
    P( A[0], A[1], A[2], A[3], A[4], R(65) );
    P( A[4], A[0], A[1], A[2], A[3], R(66) );
    P( A[3], A[4], A[0], A[1], A[2], R(67) );
    P( A[2], A[3], A[4], A[0], A[1], R(68) );
    P( A[1], A[2], A[3], A[4], A[0], R(69) );
    P( A[0], A[1], A[2], A[3], A[4], R(70) );
    P( A[4], A[0], A[1], A[2], A[3], R(71) );
    P( A[3], A[4], A[0], A[1], A[2], R(72) );
    P( A[2], A[3], A[4], A[0], A[1], R(73) );
    P( A[1], A[2], A[3], A[4], A[0], R(74) );
    P( A[0], A[1], A[2], A[3], A[4], R(75) );
    P( A[4], A[0], A[1], A[2], A[3], R(76) );
    P( A[3], A[4], A[0], A[1], A[2], R(77) );
    P( A[2], A[3], A[4], A[0], A[1], R(78) );
    P( A[1], A[2], A[3], A[4], A[0], R(79) );

#undef K
#undef F
#undef R
#undef P

    for (int i = 0; i < 5; i++)
        ctx->state[i] += A[i];

    return 0;    
}

int __esp_sha256_process(void *in_ctx, const void *src)
{
    const uint8_t *data = (const uint8_t *)src;
    esp_sha_t *ctx = (esp_sha_t *)in_ctx;
    uint32_t temp1, temp2, W[64];
    uint32_t A[8];

#undef R
#undef P

#define R(t)                                            \
(                                                       \
    W[t] = S1(W[t -  2]) + W[t -  7] +                  \
           S0(W[t - 15]) + W[t - 16]                    \
)

#define P(a, b, c, d, e, f, g, h, x, K)                 \
{                                                       \
    temp1 = h + S3(e) + F1(e,f,g) + K + x;              \
    temp2 = S2(a) + F0(a,b,c);                          \
    d += temp1; h = temp1 + temp2;                      \
}

    static const uint32_t K[] = {
        0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5,
        0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
        0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3,
        0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
        0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC,
        0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
        0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7,
        0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
        0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13,
        0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
        0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3,
        0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
        0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5,
        0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
        0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208,
        0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2,
    };

    for (int i = 0; i < 8; i++)
        A[i] = ctx->state[i];

    for (int i = 0; i < 64; i++) {
        if (i < 16)
            esp_sha_put_be(&W[i], data + 4 * i, 4, sizeof(uint32_t));
        else
            R(i);

        P(A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], W[i], K[i]);

        temp1 = A[7];
        A[7] = A[6];
        A[6] = A[5];
        A[5] = A[4];
        A[4] = A[3];
        A[3] = A[2];
        A[2] = A[1];
        A[1] = A[0];
        A[0] = temp1;
    }

    for (int i = 0; i < 8; i++)
        ctx->state[i] += A[i];

    return 0;

#undef R
#undef P
}

int __esp_sha512_process(void *in_ctx, const void *src)
{
    int i;
    uint64_t temp1, temp2, W[80];
    uint64_t A[8];
    const uint8_t *data = (const uint8_t *)src;
    esp_sha512_t *ctx = (esp_sha512_t *)in_ctx;

    static const uint64_t K[80] =
    {
        UL64(0x428A2F98D728AE22),  UL64(0x7137449123EF65CD),
        UL64(0xB5C0FBCFEC4D3B2F),  UL64(0xE9B5DBA58189DBBC),
        UL64(0x3956C25BF348B538),  UL64(0x59F111F1B605D019),
        UL64(0x923F82A4AF194F9B),  UL64(0xAB1C5ED5DA6D8118),
        UL64(0xD807AA98A3030242),  UL64(0x12835B0145706FBE),
        UL64(0x243185BE4EE4B28C),  UL64(0x550C7DC3D5FFB4E2),
        UL64(0x72BE5D74F27B896F),  UL64(0x80DEB1FE3B1696B1),
        UL64(0x9BDC06A725C71235),  UL64(0xC19BF174CF692694),
        UL64(0xE49B69C19EF14AD2),  UL64(0xEFBE4786384F25E3),
        UL64(0x0FC19DC68B8CD5B5),  UL64(0x240CA1CC77AC9C65),
        UL64(0x2DE92C6F592B0275),  UL64(0x4A7484AA6EA6E483),
        UL64(0x5CB0A9DCBD41FBD4),  UL64(0x76F988DA831153B5),
        UL64(0x983E5152EE66DFAB),  UL64(0xA831C66D2DB43210),
        UL64(0xB00327C898FB213F),  UL64(0xBF597FC7BEEF0EE4),
        UL64(0xC6E00BF33DA88FC2),  UL64(0xD5A79147930AA725),
        UL64(0x06CA6351E003826F),  UL64(0x142929670A0E6E70),
        UL64(0x27B70A8546D22FFC),  UL64(0x2E1B21385C26C926),
        UL64(0x4D2C6DFC5AC42AED),  UL64(0x53380D139D95B3DF),
        UL64(0x650A73548BAF63DE),  UL64(0x766A0ABB3C77B2A8),
        UL64(0x81C2C92E47EDAEE6),  UL64(0x92722C851482353B),
        UL64(0xA2BFE8A14CF10364),  UL64(0xA81A664BBC423001),
        UL64(0xC24B8B70D0F89791),  UL64(0xC76C51A30654BE30),
        UL64(0xD192E819D6EF5218),  UL64(0xD69906245565A910),
        UL64(0xF40E35855771202A),  UL64(0x106AA07032BBD1B8),
        UL64(0x19A4C116B8D2D0C8),  UL64(0x1E376C085141AB53),
        UL64(0x2748774CDF8EEB99),  UL64(0x34B0BCB5E19B48A8),
        UL64(0x391C0CB3C5C95A63),  UL64(0x4ED8AA4AE3418ACB),
        UL64(0x5B9CCA4F7763E373),  UL64(0x682E6FF3D6B2B8A3),
        UL64(0x748F82EE5DEFB2FC),  UL64(0x78A5636F43172F60),
        UL64(0x84C87814A1F0AB72),  UL64(0x8CC702081A6439EC),
        UL64(0x90BEFFFA23631E28),  UL64(0xA4506CEBDE82BDE9),
        UL64(0xBEF9A3F7B2C67915),  UL64(0xC67178F2E372532B),
        UL64(0xCA273ECEEA26619C),  UL64(0xD186B8C721C0C207),
        UL64(0xEADA7DD6CDE0EB1E),  UL64(0xF57D4F7FEE6ED178),
        UL64(0x06F067AA72176FBA),  UL64(0x0A637DC5A2C898A6),
        UL64(0x113F9804BEF90DAE),  UL64(0x1B710B35131C471B),
        UL64(0x28DB77F523047D84),  UL64(0x32CAAB7B40C72493),
        UL64(0x3C9EBE0A15C9BEBC),  UL64(0x431D67C49C100D4C),
        UL64(0x4CC5D4BECB3E42B6),  UL64(0x597F299CFC657E2A),
        UL64(0x5FCB6FAB3AD6FAEC),  UL64(0x6C44198C4A475817)
    };

#undef SHR
#undef ROTR
#undef S0
#undef S1
#undef S2
#undef S3
#undef F0
#undef F1
#undef P

#define  SHR(x,n) (x >> n)
#define ROTR(x,n) (SHR(x,n) | (x << (64 - n)))

#define S0(x) (ROTR(x, 1) ^ ROTR(x, 8) ^  SHR(x, 7))
#define S1(x) (ROTR(x,19) ^ ROTR(x,61) ^  SHR(x, 6))

#define S2(x) (ROTR(x,28) ^ ROTR(x,34) ^ ROTR(x,39))
#define S3(x) (ROTR(x,14) ^ ROTR(x,18) ^ ROTR(x,41))

#define F0(x,y,z) ((x & y) | (z & (x | y)))
#define F1(x,y,z) (z ^ (x & (y ^ z)))

#define P(a,b,c,d,e,f,g,h,x,K)                  \
{                                               \
    temp1 = h + S3(e) + F1(e,f,g) + K + x;      \
    temp2 = S2(a) + F0(a,b,c);                  \
    d += temp1; h = temp1 + temp2;              \
}

    for (i = 0; i < 16; i++) {
        esp_sha_put_be(&W[i], data + (i << 3), sizeof(uint64_t), sizeof(uint64_t));
    }

    for (; i < 80; i++) {
        W[i] = S1(W[i -  2]) + W[i -  7] +
               S0(W[i - 15]) + W[i - 16];
    }

    for (int j = 0; j < 8; j++)
        A[j] = ctx->state[j];

    i = 0;
    do {
        P( A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], W[i], K[i] ); i++;
        P( A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], W[i], K[i] ); i++;
        P( A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], W[i], K[i] ); i++;
        P( A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], W[i], K[i] ); i++;
        P( A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], W[i], K[i] ); i++;
        P( A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], W[i], K[i] ); i++;
        P( A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], W[i], K[i] ); i++;
        P( A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], W[i], K[i] ); i++;
    } while (i < 80);

    for (int j = 0; j < 8; j++)
        ctx->state[j] += A[j];

    return 0;

#undef SHR
#undef ROTR
#undef S0
#undef S1
#undef S2
#undef S3
#undef F0
#undef F1
#undef P
}

/**
 * @brief initialize the SHA1/SHA224/SHA256 contex
 */
int __esp_sha_init(esp_sha_t *ctx, esp_sha_type_t type, const uint32_t *state_ctx, size_t size, sha_cal_t sha_cal)
{
    util_assert(ctx);

    ctx->total[0] = 0;
    ctx->total[1] = 0;

    for (int i = 0; i < size; i ++)
        ctx->state[i] = state_ctx[i];

    ctx->type = type;
    ctx->sha_cal = sha_cal;

    return 0;
}

/**
 * @brief initialize the SHA512 contex
 */
int __esp_sha512_init(esp_sha512_t *ctx, esp_sha_type_t type, const uint64_t *state_ctx, size_t size)
{
    util_assert(ctx);

    ctx->total[0] = 0;
    ctx->total[1] = 0;

    for (int i = 0; i < size; i ++)
        ctx->state[i] = state_ctx[i];

    ctx->type = type;
    ctx->sha_cal = __esp_sha512_process;

    return 0;
}

/**
 * @brief input data which is calculated for SHA
 */
int __esp_sha_update(esp_sha_t *ctx, const void *src, size_t size)
{
    int ret;
    size_t fill;
    uint32_t left;
    uint32_t step;
    sha_cal_t sha_cal;
    size_t ilen = size;
    const uint8_t *input = (const uint8_t *)src;

    util_assert(ctx);
    util_assert(src);

    if (ilen == 0)
        return 0;

    if (SHA1 == ctx->type || SHA224 == ctx->type  || SHA256 == ctx->type) {
        left = ctx->total[0] & 0x3F;

        ctx->total[0] += (uint32_t)ilen;
        if (ctx->total[0] < (uint32_t)ilen)
            ctx->total[1]++;

        sha_cal = ctx->sha_cal;
        step = 64;
    } else {
        esp_sha512_t *ctx512 = (esp_sha512_t *)ctx;

        left = (uint32_t)(ctx512->total[0] & 0x7F);

        ctx512->total[0] += ilen;
        if (ctx512->total[0] < ilen)
            ctx512->total[1]++;

        sha_cal = ctx512->sha_cal;
        step = 128;
    }

    fill = step - left;

    if (left && ilen >= fill) {
        memcpy(ctx->buffer + left, input, fill);

        if ((ret = sha_cal(ctx, ctx->buffer)) != 0)
            return ret;

        input += fill;
        ilen  -= fill;
        left = 0;
    }

    while (ilen >= step) {
        ret = sha_cal(ctx, input);
        if (ret)
            return ret;

        input += step;
        ilen  -= step;
    }

    if (ilen > 0)
        memcpy(ctx->buffer + left, input, ilen);

    return 0;
}

/**
 * @brief input data which is calculated for SHA
 */
int __esp_sha_finish(esp_sha_t *ctx, void *dest)
{
    int ret;
    size_t bytes = 0;
    uint32_t last, padn;
    uint64_t high, low;
    uint8_t *output = dest;
    size_t step;
    void *state;
    uint8_t msglen[16];

    util_assert(ctx);
    util_assert(dest);

    if (SHA1 == ctx->type)
        bytes = 20;
    else if (SHA224 == ctx->type)
        bytes = 28;
    else if (SHA256 == ctx->type)
        bytes = 32;
    else if (SHA384 == ctx->type)
        bytes = 48;
    else if (SHA512 == ctx->type)
        bytes = 64;

    if (SHA1 == ctx->type || SHA224 == ctx->type  || SHA256 == ctx->type) {
        high = (ctx->total[0] >> 29)
               | (ctx->total[1] << 3);

        low  = (ctx->total[0] << 3);

        last = ctx->total[0] & 0x3F;
        padn = (last < 56) ? (56 - last) : (120 - last);

        step = 4;
        state = ctx->state;
    } else {
        esp_sha512_t *ctx512 = (esp_sha512_t *)ctx;

        high = (ctx512->total[0] >> 61)
               | (ctx512->total[1] <<  3);

        low  = (ctx512->total[0] <<  3);

        last = (size_t)(ctx512->total[0] & 0x7F);
        padn = (last < 112) ? (112 - last) : (240 - last);

        step = 8;
        state = ctx512->state;
    }

    esp_sha_put_be(msglen, &high, step, step);
    esp_sha_put_be(msglen + step, &low, step, step);

    ret = __esp_sha_update(ctx, sha_padding, padn);
    if (ret)
        return ret;

    ret = __esp_sha_update(ctx, msglen, step * 2);
    if (ret)
        return ret;

    esp_sha_put_be(output, state, bytes, step);

    return 0;
}


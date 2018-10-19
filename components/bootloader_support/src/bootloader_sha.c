// Copyright 2017 Espressif Systems (Shanghai) PTE LTD
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

#ifdef CONFIG_TARGET_PLATFORM_ESP32

#include "bootloader_sha.h"
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <sys/param.h>

#ifndef BOOTLOADER_BUILD
// App version is a wrapper around mbedTLS SHA API
#include <mbedtls/sha256.h>

bootloader_sha256_handle_t bootloader_sha256_start()
{
    mbedtls_sha256_context *ctx = (mbedtls_sha256_context *)malloc(sizeof(mbedtls_sha256_context));
    if (!ctx) {
        return NULL;
    }
    mbedtls_sha256_init(ctx);
    assert(mbedtls_sha256_starts_ret(ctx, false) == 0);
    return ctx;
}

void bootloader_sha256_data(bootloader_sha256_handle_t handle, const void *data, size_t data_len)
{
    assert(handle != NULL);
    mbedtls_sha256_context *ctx = (mbedtls_sha256_context *)handle;
    assert(mbedtls_sha256_update_ret(ctx, data, data_len) == 0);
}

void bootloader_sha256_finish(bootloader_sha256_handle_t handle, uint8_t *digest)
{
    assert(handle != NULL);
    mbedtls_sha256_context *ctx = (mbedtls_sha256_context *)handle;
    if (digest != NULL) {
        assert(mbedtls_sha256_finish_ret(ctx, digest) == 0);
    }
    mbedtls_sha256_free(ctx);
    free(handle);
}

#else // Bootloader version

#include "rom/sha.h"
#include "soc/dport_reg.h"
#include "soc/hwcrypto_reg.h"

#include "rom/ets_sys.h" // TO REMOVE

static uint32_t words_hashed;

// Words per SHA256 block
static const size_t BLOCK_WORDS = (64/sizeof(uint32_t));
// Words in final SHA256 digest
static const size_t DIGEST_WORDS = (32/sizeof(uint32_t));

bootloader_sha256_handle_t bootloader_sha256_start()
{
    // Enable SHA hardware
    ets_sha_enable();
    words_hashed = 0;
    return (bootloader_sha256_handle_t)&words_hashed; // Meaningless non-NULL value
}

void bootloader_sha256_data(bootloader_sha256_handle_t handle, const void *data, size_t data_len)
{
    assert(handle != NULL);
    assert(data_len % 4 == 0);

    const uint32_t *w = (const uint32_t *)data;
    size_t word_len = data_len / 4;
    uint32_t *sha_text_reg = (uint32_t *)(SHA_TEXT_BASE);

    //ets_printf("word_len %d so far %d\n", word_len, words_hashed);
    while (word_len > 0) {
        size_t block_count = words_hashed % BLOCK_WORDS;
        size_t copy_words = (BLOCK_WORDS - block_count);

        copy_words = MIN(word_len, copy_words);

        // Wait for SHA engine idle
        while(REG_READ(SHA_256_BUSY_REG) != 0) { }

        // Copy to memory block
        //ets_printf("block_count %d copy_words %d\n", block_count, copy_words);
        for (int i = 0; i < copy_words; i++) {
            sha_text_reg[block_count + i] = __builtin_bswap32(w[i]);
        }
        asm volatile ("memw");

        // Update counters
        words_hashed += copy_words;
        block_count += copy_words;
        word_len -= copy_words;
        w += copy_words;

        // If we loaded a full block, run the SHA engine
        if (block_count == BLOCK_WORDS) {
            //ets_printf("running engine @ count %d\n", words_hashed);
            if (words_hashed == BLOCK_WORDS) {
                REG_WRITE(SHA_256_START_REG, 1);
            } else {
                REG_WRITE(SHA_256_CONTINUE_REG, 1);
            }
            block_count = 0;
        }
    }
}

void bootloader_sha256_finish(bootloader_sha256_handle_t handle, uint8_t *digest)
{
    assert(handle != NULL);

    if (digest == NULL) {
        return; // We'd free resources here, but there are none to free
    }

    uint32_t data_words = words_hashed;

    // Pad to a 55 byte long block loaded in the engine
    // (leaving 1 byte 0x80 plus variable padding plus 8 bytes of length,
    // to fill a 64 byte block.)
    int block_bytes = (words_hashed % BLOCK_WORDS) * 4;
    int pad_bytes = 55 - block_bytes;
    if (pad_bytes < 0) {
        pad_bytes += 64;
    }
    static const uint8_t padding[64] = { 0x80, 0, };

    pad_bytes += 5; // 1 byte for 0x80 plus first 4 bytes of the 64-bit length
    assert(pad_bytes % 4 == 0); // should be, as (block_bytes % 4 == 0)

    bootloader_sha256_data(handle, padding, pad_bytes);

    assert(words_hashed % BLOCK_WORDS == 60/4); // 32-bits left in block

    // Calculate 32-bit length for final 32 bits of data
    uint32_t bit_count = __builtin_bswap32( data_words * 32 );
    bootloader_sha256_data(handle, &bit_count, sizeof(bit_count));

    assert(words_hashed % BLOCK_WORDS == 0);

    while(REG_READ(SHA_256_BUSY_REG) == 1) { }
    REG_WRITE(SHA_256_LOAD_REG, 1);
    while(REG_READ(SHA_256_BUSY_REG) == 1) { }

    uint32_t *digest_words = (uint32_t *)digest;
    uint32_t *sha_text_reg = (uint32_t *)(SHA_TEXT_BASE);
    for (int i = 0; i < DIGEST_WORDS; i++) {
        digest_words[i] = __builtin_bswap32(sha_text_reg[i]);
    }
    asm volatile ("memw");
}

#endif

#elif defined(CONFIG_TARGET_PLATFORM_ESP8266)

#ifndef BOOTLOADER_BUILD

#include "bootloader_sha.h"
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <sys/param.h>

#ifdef CONFIG_SSL_USING_MBEDTLS
// App version is a wrapper around mbedTLS SHA API
#include <mbedtls/sha256.h>

bootloader_sha256_handle_t bootloader_sha256_start()
{
    mbedtls_sha256_context *ctx = (mbedtls_sha256_context *)malloc(sizeof(mbedtls_sha256_context));
    if (!ctx) {
        return NULL;
    }
    mbedtls_sha256_init(ctx);
    assert(mbedtls_sha256_starts_ret(ctx, false) == 0);
    return ctx;
}

void bootloader_sha256_data(bootloader_sha256_handle_t handle, const void *data, size_t data_len)
{
    assert(handle != NULL);
    mbedtls_sha256_context *ctx = (mbedtls_sha256_context *)handle;
    assert(mbedtls_sha256_update_ret(ctx, data, data_len) == 0);
}

void bootloader_sha256_finish(bootloader_sha256_handle_t handle, uint8_t *digest)
{
    assert(handle != NULL);
    mbedtls_sha256_context *ctx = (mbedtls_sha256_context *)handle;
    if (digest != NULL) {
        assert(mbedtls_sha256_finish_ret(ctx, digest) == 0);
    }
    mbedtls_sha256_free(ctx);
    free(handle);
}

#endif

#else

#include "bootloader_sha.h"
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <sys/param.h>

typedef void* bootloader_sha256_handle_t;

// Code from mbedTLS sha256

#define F0(x,y,z) ((x & y) | (z & (x | y)))
#define F1(x,y,z) (z ^ (x & (y ^ z)))

#define SHR(x,n)  ((x & 0xFFFFFFFF) >> n)
#define ROTR(x,n) (SHR(x,n) | (x << (32 - n)))

#define S0(x) (ROTR(x, 7) ^ ROTR(x,18) ^  SHR(x, 3))
#define S1(x) (ROTR(x,17) ^ ROTR(x,19) ^  SHR(x,10))

#define S2(x) (ROTR(x, 2) ^ ROTR(x,13) ^ ROTR(x,22))
#define S3(x) (ROTR(x, 6) ^ ROTR(x,11) ^ ROTR(x,25))

#define GET_UINT32_BE(n,b,i)                            \
do {                                                    \
    (n) = ( (uint32_t) (b)[(i)    ] << 24 )             \
        | ( (uint32_t) (b)[(i) + 1] << 16 )             \
        | ( (uint32_t) (b)[(i) + 2] <<  8 )             \
        | ( (uint32_t) (b)[(i) + 3]       );            \
} while( 0 )

#define PUT_UINT32_BE(n,b,i)                            \
do {                                                    \
    (b)[(i)    ] = (unsigned char) ( (n) >> 24 );       \
    (b)[(i) + 1] = (unsigned char) ( (n) >> 16 );       \
    (b)[(i) + 2] = (unsigned char) ( (n) >>  8 );       \
    (b)[(i) + 3] = (unsigned char) ( (n)       );       \
} while( 0 )

#define R(t)                                    \
(                                               \
    W[t] = S1(W[t -  2]) + W[t -  7] +          \
           S0(W[t - 15]) + W[t - 16]            \
)

#define P(a,b,c,d,e,f,g,h,x,K)                  \
{                                               \
    temp1 = h + S3(e) + F1(e,f,g) + K + x;      \
    temp2 = S2(a) + F0(a,b,c);                  \
    d += temp1; h = temp1 + temp2;              \
}

typedef struct {
    uint32_t    total[2];
    uint32_t    state[8];
    uint8_t     buffer[64];
} mbedtls_sha256_context;

static const uint32_t K[] =
{
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

static mbedtls_sha256_context s_sha256;

static int internal_sha256_process(mbedtls_sha256_context *ctx, const uint8_t data[64])
{
    uint32_t temp1, temp2, W[64];
    uint32_t A[8];
    unsigned int i;

    for( i = 0; i < 8; i++ )
        A[i] = ctx->state[i];

    for( i = 0; i < 64; i++ )
    {
        if( i < 16 )
            GET_UINT32_BE( W[i], data, 4 * i );
        else
            R( i );

        P( A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], W[i], K[i] );

        temp1 = A[7]; A[7] = A[6]; A[6] = A[5]; A[5] = A[4]; A[4] = A[3];
        A[3] = A[2]; A[2] = A[1]; A[1] = A[0]; A[0] = temp1;
    }

    for( i = 0; i < 8; i++ )
        ctx->state[i] += A[i];

    return( 0 );
}

bootloader_sha256_handle_t bootloader_sha256_start()
{
    mbedtls_sha256_context *ctx = &s_sha256;

    memset(ctx, 0, sizeof(mbedtls_sha256_context));

    ctx->total[0] = 0;
    ctx->total[1] = 0;

    ctx->state[0] = 0x6A09E667;
    ctx->state[1] = 0xBB67AE85;
    ctx->state[2] = 0x3C6EF372;
    ctx->state[3] = 0xA54FF53A;
    ctx->state[4] = 0x510E527F;
    ctx->state[5] = 0x9B05688C;
    ctx->state[6] = 0x1F83D9AB;
    ctx->state[7] = 0x5BE0CD19;

    return ctx;
}

void bootloader_sha256_data(bootloader_sha256_handle_t handle, const void *data, size_t data_len)
{
    mbedtls_sha256_context *ctx = (mbedtls_sha256_context *)handle;
    size_t ilen = data_len;
    const uint8_t *input = (const uint8_t *)data;

    int ret;
    size_t fill;
    uint32_t left;

    left = ctx->total[0] & 0x3F;
    fill = 64 - left;

    ctx->total[0] += (uint32_t) ilen;
    ctx->total[0] &= 0xFFFFFFFF;

    if( ctx->total[0] < (uint32_t) ilen )
        ctx->total[1]++;

    if( left && ilen >= fill )
    {
        memcpy((void *)(ctx->buffer + left), input, fill);

        if( ( ret = internal_sha256_process( ctx, ctx->buffer ) ) != 0 )
            return ;

        input += fill;
        ilen  -= fill;
        left = 0;
    }

    while( ilen >= 64 )
    {
        if( ( ret = internal_sha256_process( ctx, input ) ) != 0 )
            return ;

        input += 64;
        ilen  -= 64;
    }

    if( ilen > 0 )
        memcpy( (void *) (ctx->buffer + left), input, ilen );
}

void bootloader_sha256_finish(bootloader_sha256_handle_t handle, uint8_t *digest)
{
    uint32_t last, padn;
    uint32_t high, low;
    uint8_t msglen[8];
    uint8_t *output = digest;
    mbedtls_sha256_context *ctx = (mbedtls_sha256_context *)handle;

    static const unsigned char sha256_padding[64] = {
        0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    high = ( ctx->total[0] >> 29 )
         | ( ctx->total[1] <<  3 );
    low  = ( ctx->total[0] <<  3 );

    PUT_UINT32_BE( high, msglen, 0 );
    PUT_UINT32_BE( low,  msglen, 4 );

    last = ctx->total[0] & 0x3F;
    padn = ( last < 56 ) ? ( 56 - last ) : ( 120 - last );

    bootloader_sha256_data(ctx, sha256_padding, padn);

    bootloader_sha256_data(ctx, msglen, 8);

    PUT_UINT32_BE( ctx->state[0], output,  0 );
    PUT_UINT32_BE( ctx->state[1], output,  4 );
    PUT_UINT32_BE( ctx->state[2], output,  8 );
    PUT_UINT32_BE( ctx->state[3], output, 12 );
    PUT_UINT32_BE( ctx->state[4], output, 16 );
    PUT_UINT32_BE( ctx->state[5], output, 20 );
    PUT_UINT32_BE( ctx->state[6], output, 24 );

    PUT_UINT32_BE( ctx->state[7], output, 28 );

    return ;
}

#endif

#endif

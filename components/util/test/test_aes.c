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

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "esp_aes.h"
#include "unity.h"

#define TEST_AES_COUNT 1
#define TEST_AES_DEBUG 0
#define TEST_AES_DEBUG_TIME 0

#if TEST_AES_DEBUG_TIME
#undef TEST_ASSERT_TRUE
#define TEST_ASSERT_TRUE(__e) ((__e) ? (void)0 : (void)0)
#endif

static const uint32_t s_aes_ecb_result[3][4] = {
    // keybits = 128
    {
        0x34eff23c, 0xb90aab82, 0x5781d455, 0x743e4cef,
    },

    // keybits = 192
    {
        0x8b47f409, 0x2752feb6, 0x8001f471, 0x76f0bb1d,
    },

    // keybits = 256
    {
        0x3d33ac44, 0x709b21d1, 0x7372cd80, 0x6bad3758,
    }
};

static const uint32_t s_aes_cbc_result[3][8] = {
    // keybits = 128
    {
        0xda3f4f4f, 0x9b96f894, 0xbfbbf671, 0x2da753f9, 0xad8a1a91, 0x958360a3, 0x857d1ae4, 0xda4c89e1,
    },

    // keybits = 192
    {
        0xe92a9715, 0xbe5983de, 0xefce73cf, 0x21ef8004, 0x31499c08, 0xb7753f3d, 0x54afc778, 0xc2a8209f,
    },

    // keybits = 256
    {
        0x2288383b, 0x8e541787, 0x3eb2c691, 0x76ae0dcd, 0x2a6f677a, 0xa1b0118d, 0x150ca144, 0x41def443,
    }
};

static const uint32_t s_aes_cfb8_result[3][8] = {
    // keybits = 128
    {
        0x6b5dabee, 0x41102894, 0x13e333be, 0x4fd60d9c, 0x2974dde7, 0xcaa73794, 0x1dc3d2d8, 0x3a210c38,
    },

    // keybits = 192
    {
        0xf3e04de7, 0xeca9fd81, 0xebf7ee9e, 0x7b6e7091, 0xa8bde865, 0xc150d7d9, 0xddaf6657, 0x77d6e3f0,
    },

    // keybits = 256
    {
        0xc510918d, 0xac9004da, 0xc23fea19, 0x6914df10, 0xa34d6dd5, 0x9ecc2cc7, 0x9941cd88, 0x1d472bb7,
    }
};

static const uint32_t s_aes_cfb128_result[3][8] = {
    // keybits = 128
    {
        0xfe2d65ee, 0x2d86806b, 0xea5d2e84, 0x10ebab93, 0x32f9354f, 0x0dcbc189, 0x087d75d8, 0xc456acd3,
    },

    // keybits = 192
    {
        0xfaaf66e7, 0x9c2cbe18, 0xa1f67fe2, 0x5f92ac9e, 0x1432de12, 0x4e115c08, 0x36d6a739, 0x5d8c1bce,
    },

    // keybits = 256
    {
        0x14e2318d, 0x185ac4df, 0xe6efc826, 0x65b796fb, 0x39f9a2e7, 0xffd706f4, 0x7ac79119, 0x47b99a97,
    }
};

static const uint32_t s_aes_ctr_result[3][8] = {
    // keybits = 128
    {
        0xfe2d65ee, 0x2d86806b, 0xea5d2e84, 0x10ebab93, 0x26a701de, 0x0e9d716f, 0x2d87c066, 0x2ff1f3af,
    },

    // keybits = 192
    {
        0xfaaf66e7, 0x9c2cbe18, 0xa1f67fe2, 0x5f92ac9e, 0xf7e0935e, 0xf0d3b9c1, 0x0f442d3b, 0xf7368de3,
    },

    // keybits = 256
    {
        0x14e2318d, 0x185ac4df, 0xe6efc826, 0x65b796fb, 0x9fef3b32, 0x742db24c, 0x08c56021, 0x5f5c8bf4,
    }
};

TEST_CASE("Test AES-ECB", "[AES]")
{
    uint32_t buf[4];
    uint32_t output[4];
    uint8_t key[32];
    esp_aes_t ctx;
    uint32_t encode_time = 0, decode_time = 0;
    extern uint32_t esp_get_time(void);

    memset(key, 0x11, sizeof(key));

    for (int cnt = 0; cnt < TEST_AES_COUNT; cnt++) {
        int index = 0;

        for (int i = 128; i <= 256; i += 64) {
            memset(buf, 11, sizeof(buf));
            memset(output, 0, sizeof(output));

            uint32_t tmp = esp_get_time();

            TEST_ASSERT_TRUE(esp_aes_set_encrypt_key(&ctx, key, i) == 0);

            TEST_ASSERT_TRUE(esp_aes_encrypt_ecb(&ctx, buf, output) == 0);

            encode_time += esp_get_time() - tmp;

#if TEST_AES_DEBUG
            for (int j = 0; j < 4; j++)
                printf("0x%08x(0x%08x), ", output[j], s_aes_ecb_result[index][j]);
            printf("\n");
#endif

            TEST_ASSERT_TRUE(memcmp(output, s_aes_ecb_result[index++], sizeof(output)) == 0);

            tmp = esp_get_time();

            TEST_ASSERT_TRUE(esp_aes_set_decrypt_key(&ctx, key, i) == 0);

            TEST_ASSERT_TRUE(esp_aes_decrypt_ecb(&ctx, output, buf) == 0);

            decode_time += esp_get_time() - tmp;

            memset(output, 11, sizeof(output));

#if TEST_AES_DEBUG
            for (int j = 0; j < 4; j++)
                printf("0x%08x(0x%08x), ", buf[j], output[j]);
            printf("\n");
#endif

            TEST_ASSERT_TRUE(memcmp(output, buf, sizeof(output)) == 0);
        }
    }

#if TEST_AES_DEBUG_TIME
    printf("AES-ECB test cost time totally encode %u us and decode %u us, once cost is about encode %u us and decode %u us\n",
        encode_time, decode_time, encode_time / TEST_AES_COUNT, decode_time / TEST_AES_COUNT);
#endif
}

//    align: AES-ECB test cost time totally encode 370305 us and decode 905644 us, once cost is about encode 361 us and decode 884 us
// no align: AES-ECB test cost time totally encode 513219 us and decode 1729937 us, once cost is about encode 501 us and decode 1689 us

TEST_CASE("Test AES-CBC", "[AES]")
{
    uint32_t buf[8];
    uint32_t output[8];
    uint8_t key[32];
    uint8_t iv[16];
    esp_aes_t ctx;
    uint32_t encode_time = 0, decode_time = 0;
    extern uint32_t esp_get_time(void);

    memset(key, 0x11, sizeof(key));

    for (int cnt = 0; cnt < TEST_AES_COUNT; cnt++) {
        int index = 0;

        for (int i = 128; i <= 256; i += 64) {
            memset(buf, 11, sizeof(buf));
            memset(output, 0, sizeof(output));
            memset(iv, 0x11, sizeof(iv));

            uint32_t tmp = esp_get_time();

            TEST_ASSERT_TRUE(esp_aes_set_encrypt_key(&ctx, key, i) == 0);

            TEST_ASSERT_TRUE(esp_aes_encrypt_cbc(&ctx, buf, sizeof(buf), output, sizeof(output), iv) == 0);

            encode_time += esp_get_time() - tmp;

#if TEST_AES_DEBUG
            for (int j = 0; j < 8; j++)
                printf("0x%08x(0x%08x), ", output[j], s_aes_cbc_result[index][j]);
            printf("\n");
#endif

            TEST_ASSERT_TRUE(memcmp(output, s_aes_cbc_result[index++], sizeof(output)) == 0);

            memset(iv, 0x11, sizeof(iv));

            tmp = esp_get_time();

            TEST_ASSERT_TRUE(esp_aes_set_decrypt_key(&ctx, key, i) == 0);

            TEST_ASSERT_TRUE(esp_aes_decrypt_cbc(&ctx, output, sizeof(output), buf, sizeof(buf), iv) == 0);

            decode_time += esp_get_time() - tmp;

            memset(output, 11, sizeof(output));

#if TEST_AES_DEBUG
            for (int j = 0; j < 8; j++)
                printf("0x%08x(0x%08x), ", buf[j], output[j]);
            printf("\n");
#endif

            TEST_ASSERT_TRUE(memcmp(output, buf, sizeof(output)) == 0);
        }
    }

#if TEST_AES_DEBUG_TIME
    printf("AES-CBC test cost time totally encode %u us and decode %u us, once cost is about encode %u us and decode %u us\n",
        encode_time, decode_time, encode_time / TEST_AES_COUNT, decode_time / TEST_AES_COUNT);
#endif
}

//    align: AES-CBC test cost time totally encode 554920 us and decode 1098010 us, once cost is about encode 541 us and decode 1072 us
// no align: AES-CBC test cost time totally encode 730527 us and decode 2052009 us, once cost is about encode 713 us and decode 2003 us


TEST_CASE("Test AES-CFB8", "[AES]")
{
    uint32_t buf[8];
    uint32_t output[8];
    uint8_t key[32];
    uint8_t iv[16];
    esp_aes_t ctx;
    uint32_t encode_time = 0, decode_time = 0;
    extern uint32_t esp_get_time(void);

    memset(key, 0x11, sizeof(key));

    for (int cnt = 0; cnt < TEST_AES_COUNT; cnt++) {
        int index = 0;

        for (int i = 128; i <= 256; i += 64) {
            memset(buf, 11, sizeof(buf));
            memset(output, 0, sizeof(output));
            memset(iv, 0x11, sizeof(iv));

            uint32_t tmp = esp_get_time();

            TEST_ASSERT_TRUE(esp_aes_set_encrypt_key(&ctx, key, i) == 0);

            TEST_ASSERT_TRUE(esp_aes_encrypt_cfb8(&ctx, buf, sizeof(buf), output, sizeof(output), iv) == 0);

            encode_time += esp_get_time() - tmp;

#if TEST_AES_DEBUG
            for (int j = 0; j < 8; j++)
                printf("0x%08x(0x%08x), ", output[j], s_aes_cfb8_result[index][j]);
            printf("\n");
#endif

            TEST_ASSERT_TRUE(memcmp(output, s_aes_cfb8_result[index++], sizeof(output)) == 0);

            memset(iv, 0x11, sizeof(iv));

            tmp = esp_get_time();

            TEST_ASSERT_TRUE(esp_aes_set_encrypt_key(&ctx, key, i) == 0);

            TEST_ASSERT_TRUE(esp_aes_decrypt_cfb8(&ctx, output, sizeof(output), buf, sizeof(buf), iv) == 0);

            decode_time += esp_get_time() - tmp;

            memset(output, 11, sizeof(output));

#if TEST_AES_DEBUG
            for (int j = 0; j < 8; j++)
                printf("0x%08x(0x%08x), ", buf[j], output[j]);
            printf("\n");
#endif
            TEST_ASSERT_TRUE(memcmp(output, buf, sizeof(output)) == 0);
        }
    }

#if TEST_AES_DEBUG_TIME
    printf("AES-CFB8 test cost time totally encode %u us and decode %u us, once cost is about encode %u us and decode %u us\n",
        encode_time, decode_time, encode_time / TEST_AES_COUNT, decode_time / TEST_AES_COUNT);
#endif
}

//    align: AES-CFB8 test cost time totally encode 3951207 us and decode 3918394 us, once cost is about encode 3858 us and decode 3826 us
// no align: AES-CFB8 test cost time totally encode 5510528 us and decode 5481662 us, once cost is about encode 5381 us and decode 5353 us


TEST_CASE("Test AES-CFB128", "[AES]")
{
    uint32_t buf[8];
    uint32_t output[8];
    uint8_t key[32];
    uint8_t iv[16];
    size_t iv_off;
    esp_aes_t ctx;
    uint32_t encode_time = 0, decode_time = 0;
    extern uint32_t esp_get_time(void);

    memset(key, 0x11, sizeof(key));

    for (int cnt = 0; cnt < TEST_AES_COUNT; cnt++) {
        int index = 0;

        for (int i = 128; i <= 256; i += 64) {
            memset(buf, 11, sizeof(buf));
            memset(output, 0, sizeof(output));
            memset(iv, 0x11, sizeof(iv));
            iv_off = 0;

            uint32_t tmp = esp_get_time();

            TEST_ASSERT_TRUE(esp_aes_set_encrypt_key(&ctx, key, i) == 0);

            TEST_ASSERT_TRUE(esp_aes_encrypt_cfb128(&ctx, buf, sizeof(buf), output, sizeof(output), iv, &iv_off) == 0);

            encode_time += esp_get_time() - tmp;

#if TEST_AES_DEBUG
            for (int j = 0; j < 8; j++)
                printf("0x%08x(0x%08x), ", output[j], s_aes_cfb128_result[index][j]);
            printf("\n");
#endif

            TEST_ASSERT_TRUE(memcmp(output, s_aes_cfb128_result[index++], sizeof(output)) == 0);

            memset(iv, 0x11, sizeof(iv));
            iv_off = 0;

            tmp = esp_get_time();

            TEST_ASSERT_TRUE(esp_aes_set_encrypt_key(&ctx, key, i) == 0);

            TEST_ASSERT_TRUE(esp_aes_decrypt_cfb128(&ctx, output, sizeof(output), buf, sizeof(buf), iv, &iv_off) == 0);

            decode_time += esp_get_time() - tmp;

            memset(output, 11, sizeof(output));

#if TEST_AES_DEBUG
            for (int j = 0; j < 8; j++)
                printf("0x%08x(0x%08x), ", buf[j], output[j]);
            printf("\n");
#endif

            TEST_ASSERT_TRUE(memcmp(output, buf, sizeof(buf)) == 0);
        }
    }

#if TEST_AES_DEBUG_TIME
    printf("AES-CFB128 test cost time totally encode %u us and decode %u us, once cost is about encode %u us and decode %u us\n",
        encode_time, decode_time, encode_time / TEST_AES_COUNT, decode_time / TEST_AES_COUNT);
#endif
}

//    align: AES-CFB128 test cost time totally encode 468238 us and decode 423293 us, once cost is about encode 457 us and decode 413 us
// no align: AES-CFB128 test cost time totally encode 675645 us and decode 667750 us, once cost is about encode 659 us and decode 652 us


TEST_CASE("Test AES-CTR", "[AES]")
{
    uint32_t buf[8];
    uint32_t output[8];
    uint8_t key[32];
    uint8_t nonce_counter[16];
    uint8_t stream_block[16];
    size_t nc_off;
    esp_aes_t ctx;
    uint32_t encode_time = 0, decode_time = 0;
    extern uint32_t esp_get_time(void);

    memset(key, 0x11, sizeof(key));

    for (int cnt = 0; cnt < TEST_AES_COUNT; cnt++) {
        int index = 0;

        for (int i = 128; i <= 256; i += 64) {
            memset(buf, 11, sizeof(buf));
            memset(output, 0, sizeof(output));
            memset(nonce_counter, 0x11, sizeof(nonce_counter));
            nc_off = 0;

            uint32_t tmp = esp_get_time();

            TEST_ASSERT_TRUE(esp_aes_set_encrypt_key(&ctx, key, i) == 0);

            TEST_ASSERT_TRUE(esp_aes_encrypt_ctr(&ctx, &nc_off, nonce_counter, stream_block, buf, sizeof(buf), output, sizeof(output)) == 0);

            encode_time += esp_get_time() - tmp;

#if TEST_AES_DEBUG
            for (int j = 0; j < 8; j++)
                printf("0x%08x(0x%08x), ", output[j], s_aes_cfb128_result[index][j]);
            printf("\n");
#endif

            TEST_ASSERT_TRUE(memcmp(output, s_aes_ctr_result[index++], sizeof(output)) == 0);

            memset(nonce_counter, 0x11, sizeof(nonce_counter));
            nc_off = 0;

            tmp = esp_get_time();

            TEST_ASSERT_TRUE(esp_aes_set_encrypt_key(&ctx, key, i) == 0);

            TEST_ASSERT_TRUE(esp_aes_decrypt_ctr(&ctx, &nc_off, nonce_counter, stream_block, output, sizeof(output), buf, sizeof(buf)) == 0);

            decode_time += esp_get_time() - tmp;

            memset(output, 11, sizeof(output));

#if TEST_AES_DEBUG
            for (int j = 0; j < 8; j++)
                printf("0x%08x(0x%08x), ", buf[j], output[j]);
            printf("\n");
#endif

            TEST_ASSERT_TRUE(memcmp(output, buf, sizeof(buf)) == 0);
        }
    }

#if TEST_AES_DEBUG_TIME
    printf("AES-CRT test cost time totally encode %u us and decode %u us, once cost is about encode %u us and decode %u us\n",
        encode_time, decode_time, encode_time / TEST_AES_COUNT, decode_time / TEST_AES_COUNT);
#endif
}

//    align: AES-CRT test cost time totally encode 344845 us and decode 344421 us, once cost is about encode 336 us and decode 336 us
// no align: AES-CRT test cost time totally encode 639256 us and decode 635343 us, once cost is about encode 624 us and decode 620 us

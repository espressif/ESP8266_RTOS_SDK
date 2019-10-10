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

static const uint8_t aes_test_xts_key[][32] = {
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    },
    
    {
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
        0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22
    },
    
    {
        0xff, 0xfe, 0xfd, 0xfc, 0xfb, 0xfa, 0xf9, 0xf8,
        0xf7, 0xf6, 0xf5, 0xf4, 0xf3, 0xf2, 0xf1, 0xf0,
        0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
        0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22
    },
};

static const uint8_t aes_test_xts_pt32[][32] =
{
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    },
    
    {
        0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
        0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
        0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
        0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44
    },
    
    {
        0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
        0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
        0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
        0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44
    },
};

static const uint8_t aes_test_xts_ct32[][32] =
{
    {
        0x91, 0x7c, 0xf6, 0x9e, 0xbd, 0x68, 0xb2, 0xec,
        0x9b, 0x9f, 0xe9, 0xa3, 0xea, 0xdd, 0xa6, 0x92,
        0xcd, 0x43, 0xd2, 0xf5, 0x95, 0x98, 0xed, 0x85,
        0x8c, 0x02, 0xc2, 0x65, 0x2f, 0xbf, 0x92, 0x2e
    },
    
    {
        0xc4, 0x54, 0x18, 0x5e, 0x6a, 0x16, 0x93, 0x6e,
        0x39, 0x33, 0x40, 0x38, 0xac, 0xef, 0x83, 0x8b,
        0xfb, 0x18, 0x6f, 0xff, 0x74, 0x80, 0xad, 0xc4,
        0x28, 0x93, 0x82, 0xec, 0xd6, 0xd3, 0x94, 0xf0
    },
    
    {
        0xaf, 0x85, 0x33, 0x6b, 0x59, 0x7a, 0xfc, 0x1a,
        0x90, 0x0b, 0x2e, 0xb2, 0x1e, 0xc9, 0x49, 0xd2,
        0x92, 0xdf, 0x4c, 0x04, 0x7e, 0x0b, 0x21, 0x53,
        0x21, 0x86, 0xa5, 0x97, 0x1a, 0x22, 0x7a, 0x89
    },
};

static const uint8_t aes_test_xts_data_unit[][16] =
{
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    },
   
    {
        0x33, 0x33, 0x33, 0x33, 0x33, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    },
   
    {
        0x33, 0x33, 0x33, 0x33, 0x33, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    },
};

static const unsigned char aes_test_ofb_key[3][32] =
{
    {
        0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
        0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C
    },

    {
        0x8E, 0x73, 0xB0, 0xF7, 0xDA, 0x0E, 0x64, 0x52,
        0xC8, 0x10, 0xF3, 0x2B, 0x80, 0x90, 0x79, 0xE5,
        0x62, 0xF8, 0xEA, 0xD2, 0x52, 0x2C, 0x6B, 0x7B
    },

    {
        0x60, 0x3D, 0xEB, 0x10, 0x15, 0xCA, 0x71, 0xBE,
        0x2B, 0x73, 0xAE, 0xF0, 0x85, 0x7D, 0x77, 0x81,
        0x1F, 0x35, 0x2C, 0x07, 0x3B, 0x61, 0x08, 0xD7,
        0x2D, 0x98, 0x10, 0xA3, 0x09, 0x14, 0xDF, 0xF4
    }
};

static const unsigned char aes_test_ofb_iv[16] =
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

static const unsigned char aes_test_ofb_pt[64] =
{
    0x6B, 0xC1, 0xBE, 0xE2, 0x2E, 0x40, 0x9F, 0x96,
    0xE9, 0x3D, 0x7E, 0x11, 0x73, 0x93, 0x17, 0x2A,
    0xAE, 0x2D, 0x8A, 0x57, 0x1E, 0x03, 0xAC, 0x9C,
    0x9E, 0xB7, 0x6F, 0xAC, 0x45, 0xAF, 0x8E, 0x51,
    0x30, 0xC8, 0x1C, 0x46, 0xA3, 0x5C, 0xE4, 0x11,
    0xE5, 0xFB, 0xC1, 0x19, 0x1A, 0x0A, 0x52, 0xEF,
    0xF6, 0x9F, 0x24, 0x45, 0xDF, 0x4F, 0x9B, 0x17,
    0xAD, 0x2B, 0x41, 0x7B, 0xE6, 0x6C, 0x37, 0x10
};

static const unsigned char aes_test_ofb_ct[3][64] =
{
    {
        0x3B, 0x3F, 0xD9, 0x2E, 0xB7, 0x2D, 0xAD, 0x20,
        0x33, 0x34, 0x49, 0xF8, 0xE8, 0x3C, 0xFB, 0x4A,
        0x77, 0x89, 0x50, 0x8d, 0x16, 0x91, 0x8f, 0x03,
        0xf5, 0x3c, 0x52, 0xda, 0xc5, 0x4e, 0xd8, 0x25,
        0x97, 0x40, 0x05, 0x1e, 0x9c, 0x5f, 0xec, 0xf6,
        0x43, 0x44, 0xf7, 0xa8, 0x22, 0x60, 0xed, 0xcc,
        0x30, 0x4c, 0x65, 0x28, 0xf6, 0x59, 0xc7, 0x78,
        0x66, 0xa5, 0x10, 0xd9, 0xc1, 0xd6, 0xae, 0x5e
    },

    {
        0xCD, 0xC8, 0x0D, 0x6F, 0xDD, 0xF1, 0x8C, 0xAB,
        0x34, 0xC2, 0x59, 0x09, 0xC9, 0x9A, 0x41, 0x74,
        0xfc, 0xc2, 0x8b, 0x8d, 0x4c, 0x63, 0x83, 0x7c,
        0x09, 0xe8, 0x17, 0x00, 0xc1, 0x10, 0x04, 0x01,
        0x8d, 0x9a, 0x9a, 0xea, 0xc0, 0xf6, 0x59, 0x6f,
        0x55, 0x9c, 0x6d, 0x4d, 0xaf, 0x59, 0xa5, 0xf2,
        0x6d, 0x9f, 0x20, 0x08, 0x57, 0xca, 0x6c, 0x3e,
        0x9c, 0xac, 0x52, 0x4b, 0xd9, 0xac, 0xc9, 0x2a
    },

    {
        0xDC, 0x7E, 0x84, 0xBF, 0xDA, 0x79, 0x16, 0x4B,
        0x7E, 0xCD, 0x84, 0x86, 0x98, 0x5D, 0x38, 0x60,
        0x4f, 0xeb, 0xdc, 0x67, 0x40, 0xd2, 0x0b, 0x3a,
        0xc8, 0x8f, 0x6a, 0xd8, 0x2a, 0x4f, 0xb0, 0x8d,
        0x71, 0xab, 0x47, 0xa0, 0x86, 0xe8, 0x6e, 0xed,
        0xf3, 0x9d, 0x1c, 0x5b, 0xba, 0x97, 0xc4, 0x08,
        0x01, 0x26, 0x14, 0x1d, 0x67, 0xf3, 0x7b, 0xe8,
        0x53, 0x8f, 0x5a, 0x8b, 0xe7, 0x40, 0xe4, 0x84
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



TEST_CASE("Test AES-XTS", "[AES]")
{
    uint32_t encode_time = 0, decode_time = 0;
    const int num_tests = sizeof(aes_test_xts_key) / sizeof(*aes_test_xts_key);
    extern uint32_t esp_get_time(void);

    for (int cnt = 0; cnt < TEST_AES_COUNT; cnt++) {
        for (int i = 0; i < num_tests; i++) {
            const int len = sizeof(*aes_test_xts_ct32);
            uint32_t buf[8];
            uint8_t key[32];
            const uint8_t *data_unit = aes_test_xts_data_unit[i];
            esp_aes_xts_t ctx_xts;

            memcpy(key, aes_test_xts_key[i], 32);

            TEST_ASSERT_TRUE(esp_aes_xts_set_encrypt_key(&ctx_xts, key, 256) == 0);
            memcpy(buf, aes_test_xts_pt32[i], len);

            uint32_t tmp = esp_get_time();
            TEST_ASSERT_TRUE(esp_aes_crypt_xts(&ctx_xts, 1, len, data_unit, buf, buf) == 0);
            encode_time += esp_get_time() - tmp;
            
            TEST_ASSERT_TRUE(memcmp(buf, aes_test_xts_ct32[i], len) == 0);

            TEST_ASSERT_TRUE(esp_aes_xts_set_decrypt_key(&ctx_xts, key, 256) == 0);
            memcpy(buf, aes_test_xts_ct32[i], len);

            tmp = esp_get_time();
            TEST_ASSERT_TRUE(esp_aes_crypt_xts(&ctx_xts, 0, len, data_unit, buf, buf) == 0);
            decode_time += esp_get_time() - tmp;
            
            TEST_ASSERT_TRUE(memcmp(buf, aes_test_xts_pt32[i], len) == 0);
        }
    }

#if TEST_AES_DEBUG_TIME
    printf("AES-XTS test cost time totally encode %u us and decode %u us, once cost is about encode %u us and decode %u us\n",
        encode_time, decode_time, encode_time / TEST_AES_COUNT, decode_time / TEST_AES_COUNT);
#endif
}

//    align: AES-XTS test cost time totally encode 723197 us and decode 533441 us, once cost is about encode 706 us and decode 520 us
// no align: AES-XTS test cost time totally encode 675249 us and decode 645998 us, once cost is about encode 659 us and decode 630 us



TEST_CASE("Test AES-OFB", "[AES]")
{
    uint32_t encode_time = 0, decode_time = 0;
    const int num_tests = sizeof(aes_test_ofb_ct) / sizeof(*aes_test_ofb_ct);
    extern uint32_t esp_get_time(void);

    for (int cnt = 0; cnt < TEST_AES_COUNT; cnt++) {
        for (int i = 0; i < num_tests; i++) {
            uint8_t buf[64];
            uint8_t key[32];
            uint8_t iv[16];
            esp_aes_t ctx;
            size_t keybits;
            size_t offset;

            offset = 0;
            keybits = i * 64 + 128;
            memcpy(iv,  aes_test_ofb_iv, 16);
            memcpy(key, aes_test_ofb_key[i], keybits / 8);

            TEST_ASSERT_TRUE(esp_aes_set_encrypt_key(&ctx, key, keybits) == 0);
            memcpy(buf, aes_test_ofb_ct[i], 64);

            uint32_t tmp = esp_get_time();
            TEST_ASSERT_TRUE(esp_aes_crypt_ofb(&ctx, 64, &offset, iv, buf, buf) == 0);
            encode_time += esp_get_time() - tmp;

            TEST_ASSERT_TRUE(memcmp(buf, aes_test_ofb_pt, 64) == 0);

            offset = 0;
            keybits = i * 64 + 128;
            memcpy(iv,  aes_test_ofb_iv, 16);
            memcpy(key, aes_test_ofb_key[i], keybits / 8);

            TEST_ASSERT_TRUE(esp_aes_set_encrypt_key(&ctx, key, keybits) == 0);
            memcpy(buf, aes_test_ofb_pt, 64);

            tmp = esp_get_time();
            TEST_ASSERT_TRUE(esp_aes_crypt_ofb(&ctx, 64, &offset, iv, buf, buf) == 0);
            decode_time += esp_get_time() - tmp;

            TEST_ASSERT_TRUE(memcmp(buf, aes_test_ofb_ct[i], 64) == 0);
        }
    }

#if TEST_AES_DEBUG_TIME
    printf("AES-OFB test cost time totally encode %u us and decode %u us, once cost is about encode %u us and decode %u us\n",
        encode_time, decode_time, encode_time / TEST_AES_COUNT, decode_time / TEST_AES_COUNT);
#endif
}

//    align: AES-OFB test cost time totally encode 465340 us and decode 455726 us, once cost is about encode 454 us and decode 445 us
// no align: AES-OFB test cost time totally encode 743898 us and decode 736479 us, once cost is about encode 726 us and decode 719 us

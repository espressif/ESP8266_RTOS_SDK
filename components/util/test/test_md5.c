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
#include "esp_md5.h"
#include "unity.h"

#define TEST_MD5_COUNT 256
#define TEST_MD5_DEBUG 1

static const uint8_t md5_test_buf[7][81] =
{
    {""},
    {"a"},
    {"abc"},
    {"message digest"},
    {"abcdefghijklmnopqrstuvwxyz"},
    {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"},
    {"12345678901234567890123456789012345678901234567890123456789012"
    "345678901234567890"}
};

static const size_t md5_test_buflen[7] =
{
    0, 1, 3, 14, 26, 62, 80
};

static const uint8_t md5_test_sum[7][16] =
{
    { 0xD4, 0x1D, 0x8C, 0xD9, 0x8F, 0x00, 0xB2, 0x04,
      0xE9, 0x80, 0x09, 0x98, 0xEC, 0xF8, 0x42, 0x7E },
    { 0x0C, 0xC1, 0x75, 0xB9, 0xC0, 0xF1, 0xB6, 0xA8,
      0x31, 0xC3, 0x99, 0xE2, 0x69, 0x77, 0x26, 0x61 },
    { 0x90, 0x01, 0x50, 0x98, 0x3C, 0xD2, 0x4F, 0xB0,
      0xD6, 0x96, 0x3F, 0x7D, 0x28, 0xE1, 0x7F, 0x72 },
    { 0xF9, 0x6B, 0x69, 0x7D, 0x7C, 0xB7, 0x93, 0x8D,
      0x52, 0x5A, 0x2F, 0x31, 0xAA, 0xF1, 0x61, 0xD0 },
    { 0xC3, 0xFC, 0xD3, 0xD7, 0x61, 0x92, 0xE4, 0x00,
      0x7D, 0xFB, 0x49, 0x6C, 0xCA, 0x67, 0xE1, 0x3B },
    { 0xD1, 0x74, 0xAB, 0x98, 0xD2, 0x77, 0xD9, 0xF5,
      0xA5, 0x61, 0x1C, 0x2C, 0x9F, 0x41, 0x9D, 0x9F },
    { 0x57, 0xED, 0xF4, 0xA2, 0x2B, 0xE3, 0xC9, 0x55,
      0xAC, 0x49, 0xDA, 0x2E, 0x21, 0x07, 0xB6, 0x7A }
};

TEST_CASE("Test MD5", "[md5]")
{
    uint32_t encode_time = 0;
    uint8_t md5sum[16] = {0};
    extern uint32_t esp_get_time(void);

    for (int i = 0; i < TEST_MD5_COUNT; i++) {
        for (int j = 0; j < 7; j++)
        {
            uint32_t tmp = esp_get_time();
            
            esp_md5_context_t ctx;
            TEST_ASSERT_TRUE(esp_md5_init(&ctx) == 0);
            TEST_ASSERT_TRUE(esp_md5_update(&ctx, md5_test_buf[j], md5_test_buflen[j]) == 0);
            TEST_ASSERT_TRUE(esp_md5_final(&ctx, md5sum) == 0);

            encode_time += esp_get_time() - tmp;
            TEST_ASSERT_TRUE(memcmp(md5sum, md5_test_sum[j], 16) == 0);
        }
    }

#if TEST_MD5_DEBUG
    printf("MD5 test cost time totally encode %u us , once cost is about encode %u us\n",
        encode_time, encode_time / (TEST_MD5_COUNT * 7));
#endif
}

//  MD5 test cost time totally encode 88614 us , once cost is about encode 49 us


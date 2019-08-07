
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
#include "esp_arc4.h"
#include "unity.h"

#define TEST_ARC4_COUNT 512
#define TEST_ARC4_DEBUG 1

static const uint8_t arc4_test_key[3][8] =
{
    { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF },
    { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

static const uint8_t arc4_test_pt[3][8] =
{
    { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

static const uint8_t arc4_test_ct[3][8] =
{
    { 0x75, 0xB7, 0x87, 0x80, 0x99, 0xE0, 0xC5, 0x96 },
    { 0x74, 0x94, 0xC2, 0xE7, 0x10, 0x4B, 0x08, 0x79 },
    { 0xDE, 0x18, 0x89, 0x41, 0xA3, 0x37, 0x5D, 0x3A }
};

TEST_CASE("Test ARC4", "[ARC4]")
{
    int i, n = 0;
    uint32_t encode_time = 0;
    uint8_t ibuf[8];
    uint8_t obuf[8];
    extern uint32_t esp_get_time(void);

    for (n = 0; n < TEST_ARC4_COUNT; n++) {
        for(i = 0; i < 3; i++)
        {
            memcpy(ibuf, arc4_test_pt[i], 8);

            uint32_t tmp = esp_get_time();
            esp_arc4_context ctx;
            esp_arc4_setup(&ctx, arc4_test_key[i], 8);
            TEST_ASSERT_TRUE(esp_arc4_encrypt(&ctx, 8, ibuf, obuf) == 0);
            encode_time += esp_get_time() - tmp;

            TEST_ASSERT_TRUE(memcmp(obuf, arc4_test_ct[i], 8) == 0);
        }
    }
#if TEST_ARC4_DEBUG
    printf("ARC4 test cost time totally encode %u us , once cost is about encode %u us\n",
        encode_time, encode_time / (TEST_ARC4_COUNT * 3));
#endif
}

// ARC4 test cost time totally encode 794288 us , once cost is about encode 517 us


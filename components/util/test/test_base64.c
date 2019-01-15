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
#include "esp_base64.h"
#include "unity.h"

#define TEST_BASE64_COUNT 4096
#define TEST_BASE64_DEBUG 1

static unsigned char base64_test_dec[] =
{
    0x24, 0x48, 0x6E, 0x56, 0x87, 0x62, 0x5A, 0xBD,
    0xBF, 0x17, 0xD9, 0xA2, 0xC4, 0x17, 0x1A, 0x01,
    0x94, 0xED, 0x8F, 0x1E, 0x11, 0xB3, 0xD7, 0x09,
    0x0C, 0xB6, 0xE9, 0x10, 0x6F, 0x22, 0xEE, 0x13,
    0xCA, 0xB3, 0x07, 0x05, 0x76, 0xC9, 0xFA, 0x31,
    0x6C, 0x08, 0x34, 0xFF, 0x8D, 0xC2, 0x6C, 0x38,
    0x00, 0x43, 0xE9, 0x54, 0x97, 0xAF, 0x50, 0x4B,
    0xD1, 0x41, 0xBA, 0x95, 0x31, 0x5A, 0x0B, 0x97
};

static unsigned char base64_test_enc[] =
    "JEhuVodiWr2/F9mixBcaAZTtjx4Rs9cJDLbpEG8i7hPK"
    "swcFdsn6MWwINP+Nwmw4AEPpVJevUEvRQbqVMVoLlw==";

TEST_CASE("Test base64 codec", "[base64]")
{
    int ret;
    uint32_t encode_time = 0, decode_time = 0;
    unsigned char buffer[128];

    extern uint32_t esp_get_time(void);

    for (int i = 0; i < TEST_BASE64_COUNT; i++) {
        uint32_t tmp = esp_get_time();

        ret = esp_base64_encode(base64_test_dec, sizeof(base64_test_dec), buffer, sizeof(buffer));
        TEST_ASSERT_TRUE(ret > 0);

        encode_time += esp_get_time() - tmp;

        TEST_ASSERT_TRUE(memcmp(base64_test_enc, buffer, ret) == 0);

        tmp = esp_get_time();

        ret = esp_base64_decode(base64_test_enc, sizeof(base64_test_enc) - 1, buffer, sizeof(buffer));
        TEST_ASSERT_TRUE(ret > 0);

        decode_time += esp_get_time() - tmp;

        TEST_ASSERT_TRUE(memcmp(base64_test_dec, buffer, ret) == 0);
    }

#if TEST_BASE64_DEBUG
    printf("base64 test cost time totally encode %u us and decode %u us, once cost is about encode %u us and decode %u us\n",
        encode_time, decode_time, encode_time / TEST_BASE64_COUNT, decode_time / TEST_BASE64_COUNT);
#endif
}

// align: base64 test cost time totally encode 100921 us and decode 549040 us,
//        once cost is about encode 29 us and decode 133 us

// no align : base64 test cost time totally encode 640726 us and decode 1601834 us,
//            once cost is about encode 156 us and decode 391 us

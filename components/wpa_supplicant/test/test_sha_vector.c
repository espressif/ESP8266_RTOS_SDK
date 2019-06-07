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
#include "esp_log.h"
#include "unity.h"
#include "crypto/crypto.h"

#define TEST_COUNT 1024
#define TEST_DBG   1

TEST_CASE("Test SHA1 Vector", "[SHA1 Vector]")
{
    uint32_t test_time = 0;
    const uint8_t result[20] = {
        0x01, 0xec, 0x29, 0xec, 0x5c, 0x60, 0xa4, 0xef,
        0xb1, 0x66, 0xa3, 0x84, 0x7a, 0x83, 0xce, 0x55,
        0x17, 0x18, 0x4a, 0x7f
    };

    extern uint32_t esp_get_time(void);

    for (int i = 0; i < TEST_COUNT; i++) {
        uint32_t tmp;
        char ssid[10] = "AFAST_IK";
        char count[10] = "kkkk";
        char *addr[2] = {ssid, count};
        size_t len[2] = {9, 4};
        uint8_t output[20];

        tmp = esp_get_time();
        sha1_vector(2, (const uint8_t **)addr, len, output);
        test_time += esp_get_time() - tmp;

        TEST_ASSERT_TRUE(memcmp(output, result, 20) == 0);
    }

#if TEST_DBG
    printf("SHA1 vector test cost time totally %u us, once cost is about %u us\n", test_time, test_time / TEST_COUNT);
#endif
}


// new SHA1: SHA1 vector test cost time totally 77949 us, once cost is about 76 us
// old SHA1: SHA1 vector test cost time totally 95417 us, once cost is about 93 us

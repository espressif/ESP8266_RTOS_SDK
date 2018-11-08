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
#include "crc.h"
#include "unity.h"

#define TEST_CRC_COUNT 4096
#define TEST_CRC_DEBUG 1
#define TEST_DATA_BYTES 128

static uint8_t s_crc_src[TEST_DATA_BYTES];

TEST_CASE("Test CRC8", "[CRC8]")
{
    uint8_t crc = 0;
    uint32_t crc_time = 0;

    extern uint32_t esp_get_time(void);

    memset(s_crc_src, 11, TEST_DATA_BYTES);

    for (int i = 0; i < TEST_CRC_COUNT; i++) {
        uint32_t tmp = esp_get_time();

        crc = esp_crc8(s_crc_src, sizeof(s_crc_src));

        crc_time += esp_get_time() - tmp;

        TEST_ASSERT_EQUAL(0xd7, crc);
    }

#if TEST_CRC_DEBUG
    printf("CRC8 test cost time totally %u us, once cost is about %u us\n",
                crc_time, crc_time / TEST_CRC_COUNT);
#endif    
}

// no align: CRC8 test cost time totally 955287 us, once cost is about 233 us
// align   : CRC8 test cost time totally 186694 us, once cost is about 45 us

TEST_CASE("Test CRC16", "[CRC16]")
{
    uint16_t crc = 0;
    uint32_t crc_time = 0;

    extern uint32_t esp_get_time(void);

    memset(s_crc_src, 11, TEST_DATA_BYTES);

    for (int i = 0; i < TEST_CRC_COUNT; i++) {
        uint32_t tmp = esp_get_time();
    
        crc = crc16_le(0, s_crc_src, sizeof(s_crc_src));

        crc_time += esp_get_time() - tmp;

        TEST_ASSERT_EQUAL(0x495d, crc);
    }

#if TEST_CRC_DEBUG
    printf("CRC16 test cost time totally %u us, once cost is about %u us\n",
                crc_time, crc_time / TEST_CRC_COUNT);
#endif    
}

// no align: CRC16 test cost time totally 1114398 us, once cost is about 272 us
// align   : CRC16 test cost time totally 200268 us, once cost is about 48 us

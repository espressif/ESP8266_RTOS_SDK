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
#include "esp_sha.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "unity.h"

//#define DEBUG_SHA_RESULT

#define TAG "SHA_TEST"

TEST_CASE("Test SHA1", "[SHA]")
{
    int ret;
    uint8_t *buf;
    esp_sha1_t sha_ctx;

    const uint32_t sha_result[] = {
        0xc3b837ce, 0x0c8a528c, 0x032939b5, 0x50a88285, 0x6433458d
    };

    buf = heap_caps_malloc(1024, MALLOC_CAP_8BIT);
    TEST_ASSERT(buf != NULL);

    memset(buf, 11, 1024);

    ret = esp_sha1_init(&sha_ctx);
    TEST_ASSERT(ret == 0);

    ret = esp_sha1_update(&sha_ctx, buf, 1024);
    TEST_ASSERT(ret == 0);

    memset(buf, 0, 1024);
    ret = esp_sha1_finish(&sha_ctx, buf);
    TEST_ASSERT(ret == 0);

#ifdef DEBUG_SHA_RESULT
    const uint32_t *pbuf = (const uint32_t *)buf;
    printf("\n");
    for (int j = 0; j < sizeof(sha_result) / sizeof(sha_result[0]); j++) {
        printf("0x%x(0x%x) ", pbuf[j], sha_result[j]);
    }
    printf("\n");
#endif

    TEST_ASSERT(memcmp(buf, sha_result, sizeof(sha_result)) == 0);

    heap_caps_free(buf);
}

TEST_CASE("Test SHA224", "[SHA]")
{
    int ret;
    uint8_t *buf;
    esp_sha224_t sha_ctx;

    const uint32_t sha_result[] = {
        0xeb5dd981, 0x0ea2508e, 0xfe5708b8, 0xc15f30b5, 0x833f2144, 0xbbf4de16, 0x50d112b7
    };

    buf = heap_caps_malloc(1024, MALLOC_CAP_8BIT);
    TEST_ASSERT(buf != NULL);

    memset(buf, 11, 1024);

    ret = esp_sha224_init(&sha_ctx);
    TEST_ASSERT(ret == 0);

    ret = esp_sha224_update(&sha_ctx, buf, 1024);
    TEST_ASSERT(ret == 0);

    memset(buf, 0, 1024);
    ret = esp_sha224_finish(&sha_ctx, buf);
    TEST_ASSERT(ret == 0);

#ifdef DEBUG_SHA_RESULT
    const uint32_t *pbuf = (const uint32_t *)buf;
    printf("\n");
    for (int j = 0; j < sizeof(sha_result) / sizeof(sha_result[0]); j++) {
        printf("0x%x(0x%x) ", pbuf[j], sha_result[j]);
    }
    printf("\n");
#endif

    TEST_ASSERT(memcmp(buf, sha_result, sizeof(sha_result)) == 0);

    heap_caps_free(buf);
}

TEST_CASE("Test SHA256", "[SHA]")
{
    int ret;
    uint8_t *buf;
    esp_sha256_t sha_ctx;

    const uint32_t sha_result[] = {
        0xfc875b5a, 0x318e3c5a, 0xac2b3233, 0x4df7b366, 0x4c4c9261, 0x0e70af8d, 0x69a7e57c, 0x179cd56e
    };

    buf = heap_caps_malloc(1024, MALLOC_CAP_8BIT);
    TEST_ASSERT(buf != NULL);

    memset(buf, 11, 1024);

    ret = esp_sha256_init(&sha_ctx);
    TEST_ASSERT(ret == 0);

    ret = esp_sha256_update(&sha_ctx, buf, 1024);
    TEST_ASSERT(ret == 0);

    memset(buf, 0, 1024);
    ret = esp_sha256_finish(&sha_ctx, buf);
    TEST_ASSERT(ret == 0);

#ifdef DEBUG_SHA_RESULT
    const uint32_t *pbuf = (const uint32_t *)buf;
    printf("\n");
    for (int j = 0; j < sizeof(sha_result) / sizeof(sha_result[0]); j++) {
        printf("0x%x(0x%x) ", pbuf[j], sha_result[j]);
    }
    printf("\n");
#endif

    TEST_ASSERT(memcmp(buf, sha_result, sizeof(sha_result)) == 0);

    heap_caps_free(buf);
}

TEST_CASE("Test SHA384", "[SHA]")
{
    int ret;
    uint8_t *buf;
    esp_sha384_t sha_ctx;

    const uint32_t sha_result[] = {
        0xd1d31575, 0x494afdef, 0x1d042951, 0x77a02c7b, 0x546db656, 0xdf31c571,
        0x1c3f87c1, 0x0d5cd544, 0x73628b2a, 0xecf051e7, 0xb72e6478, 0x83cee28b
    };

    buf = heap_caps_malloc(1024, MALLOC_CAP_8BIT);
    TEST_ASSERT(buf != NULL);

    memset(buf, 11, 1024);

    ret = esp_sha384_init(&sha_ctx);
    TEST_ASSERT(ret == 0);

    ret = esp_sha384_update(&sha_ctx, buf, 1024);
    TEST_ASSERT(ret == 0);

    memset(buf, 0, 1024);
    ret = esp_sha384_finish(&sha_ctx, buf);
    TEST_ASSERT(ret == 0);

#if DEBUG_SHA_RESULT
    const uint32_t *pbuf = (const uint32_t *)buf;
    printf("\n");
    for (int j = 0; j < sizeof(sha_result) / sizeof(sha_result[0]); j++) {
        printf("0x%x(0x%x) ", pbuf[j], sha_result[j]);
    }
    printf("\n");
#endif

    TEST_ASSERT(memcmp(buf, sha_result, sizeof(sha_result)) == 0);

    heap_caps_free(buf);
}

TEST_CASE("Test SHA512", "[SHA]")
{
    int ret;
    uint8_t *buf;
    esp_sha512_t sha_ctx;

    const uint32_t sha_result[] = {
        0x153be81b, 0x37abc24e, 0x3b6f1a5b, 0x42c713f9, 0x51c9a8e1, 0x7303f29b,
        0x2c979121, 0x1c4e632d, 0xad470c5a, 0xe7643b5e, 0x63447f10, 0x05d613e6,
        0xa3c6b5cc, 0x99e52218, 0x665b659f, 0x1bfc639b
    };

    buf = heap_caps_malloc(1024, MALLOC_CAP_8BIT);
    TEST_ASSERT(buf != NULL);

    memset(buf, 11, 1024);

    ret = esp_sha512_init(&sha_ctx);
    TEST_ASSERT(ret == 0);

    ret = esp_sha512_update(&sha_ctx, buf, 1024);
    TEST_ASSERT(ret == 0);

    memset(buf, 0, 1024);
    ret = esp_sha512_finish(&sha_ctx, buf);
    TEST_ASSERT(ret == 0);

#if DEBUG_SHA_RESULT
    const uint32_t *pbuf = (const uint32_t *)buf;
    printf("\n");
    for (int j = 0; j < sizeof(sha_result) / sizeof(sha_result[0]); j++) {
        printf("0x%x(0x%x) ", pbuf[j], sha_result[j]);
    }
    printf("\n");
#endif

    TEST_ASSERT(memcmp(buf, sha_result, sizeof(sha_result)) == 0);

    heap_caps_free(buf);
}

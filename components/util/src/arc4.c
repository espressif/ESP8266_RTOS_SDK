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

#include <sys/errno.h>
#include <string.h>
#include "esp_arc4.h"
#include "ibus_data.h"
#include "util_assert.h"

/*
 * ARC4 key schedule
 */
void esp_arc4_setup(esp_arc4_context *ctx, const uint8_t *key, uint32_t keylen)
{
    int i, j, a;
    uint32_t k;
    uint8_t *m;

    util_assert(ctx);
    util_assert(key);

    ctx->x = 0;
    ctx->y = 0;
    m = ctx->m;

    for(i = 0; i < 256; i++)
        m[i] = (uint8_t) i;

    j = k = 0;

    for(i = 0; i < 256; i++, k++)
    {
        if(k >= keylen) k = 0;

        a = m[i];
        j = (j + a + key[k]) & 0xFF;
        m[i] = m[j];
        m[j] = (uint8_t) a;
    }
}

/*
 * ARC4 cipher function
 */
int esp_arc4_encrypt(esp_arc4_context *ctx, size_t length, const uint8_t *input, uint8_t *output)
{
    int x, y, a, b;
    size_t i;
    uint8_t *m;

    util_assert(ctx);
    util_assert(input);
    util_assert(output);
    
    x = ctx->x;
    y = ctx->y;
    m = ctx->m;

    for(i = 0; i < length; i++)
    {
        x = (x + 1) & 0xFF; a = m[x];
        y = (y + a) & 0xFF; b = m[y];

        m[x] = (uint8_t) b;
        m[y] = (uint8_t) a;

        output[i] = (uint8_t)(input[i] ^ m[(uint8_t)(a + b)]);
    }

    ctx->x = x;
    ctx->y = y;

    return 0;
}

int esp_arc4_decrypt(esp_arc4_context *ctx, size_t length, const uint8_t *input, uint8_t *output)
{
    return esp_arc4_encrypt(ctx, length, input, output);
}


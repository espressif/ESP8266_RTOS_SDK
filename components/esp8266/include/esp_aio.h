// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef _ESP_AIO_H
#define _ESP_AIO_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * declare
 */
struct esp_aio;

/*
 * aio callback function type
 */
typedef int (*esp_aio_cb_t)(struct esp_aio *aio);

/*
 * aio data object
 */
typedef struct esp_aio {
    int             fd;     // file description

    const char      *pbuf;  // send/recv data pointer
    size_t          len;    // buffer length by bytes

    esp_aio_cb_t    cb;     // callback function
    void            *arg;   // user callback function private data

    int             ret;    // asynchronous operation result
} esp_aio_t;

/*
 * aio data object
 */
typedef struct esp_aio_data {
    const char      *pbuf;  // send/recv data pointer
    size_t          len;    // buffer length by bytes

    int             status; // lowlevel data status
} esp_aio_data_t;

#ifdef __cplusplus
}
#endif

#endif /* _ESP_AIO_H */

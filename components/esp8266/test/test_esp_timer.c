// Copyright 2018-2019 Espressif Systems (Shanghai) PTE LTD
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

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>

#include <unity.h>
#include <esp_spi_flash.h>
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

#define ESP_NUM_MAX 5
#define ESP_TIMER_PERIOD 50 * 1000

static void test_timer_cb(void *p)
{
    SemaphoreHandle_t sem = (SemaphoreHandle_t)p;

    xSemaphoreGive(sem);
}

TEST_CASE("Test esp_timer create and delete once mode", "[log]")
{
    SemaphoreHandle_t sem;
    esp_timer_handle_t timer[ESP_NUM_MAX];

    sem = xSemaphoreCreateCounting(ESP_NUM_MAX, 0);
    TEST_ASSERT_NOT_NULL(sem);

    esp_timer_create_args_t timer_args = {
        .callback = test_timer_cb,
        .arg = sem,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "test_timer",
    };

    for (int i = 0; i < ESP_NUM_MAX; i++) {
        timer[i] = NULL;
    }

    for (int i = 0; i < ESP_NUM_MAX; i++) {
        TEST_ESP_OK(esp_timer_create(&timer_args, &timer[i]));
        TEST_ESP_OK(esp_timer_start_once(timer[i], ESP_TIMER_PERIOD));
    }

    for (int i = 0; i < ESP_NUM_MAX; i++) {
        TEST_ASSERT_EQUAL_HEX32(pdPASS, xSemaphoreTake(sem, portMAX_DELAY));
    }

    TEST_ASSERT_EQUAL_HEX32(pdFALSE, xSemaphoreTake(sem, 1000 / portMAX_DELAY));

    for (int i = 0; i < ESP_NUM_MAX; i++) {
        TEST_ESP_OK(esp_timer_delete(timer[i]));
        timer[i] = NULL;
    }

    vSemaphoreDelete(sem);
}

TEST_CASE("Test esp_timer create and delete once mode", "[log]")
{
    SemaphoreHandle_t sem;
    esp_timer_handle_t timer[ESP_NUM_MAX];

    sem = xSemaphoreCreateCounting(ESP_NUM_MAX, 0);
    TEST_ASSERT_NOT_NULL(sem);

    esp_timer_create_args_t timer_args = {
        .callback = test_timer_cb,
        .arg = sem,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "test_timer",
    };

    for (int i = 0; i < ESP_NUM_MAX; i++) {
        timer[i] = NULL;
    }

    for (int i = 0; i < ESP_NUM_MAX; i++) {
        TEST_ESP_OK(esp_timer_create(&timer_args, &timer[i]));
        TEST_ESP_OK(esp_timer_start_periodic(timer[i], ESP_TIMER_PERIOD));
    }

    for (int i = 0; i < ESP_NUM_MAX * 4; i++) {
        TEST_ASSERT_EQUAL_HEX32(pdPASS, xSemaphoreTake(sem, portMAX_DELAY));
    }

    for (int i = 0; i < ESP_NUM_MAX; i++) {
        TEST_ESP_OK(esp_timer_delete(timer[i]));
        timer[i] = NULL;
    }

    vSemaphoreDelete(sem);
}

// Copyright 2018-2025 Espressif Systems (Shanghai) PTE LTD
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

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "spi_ram_fifo.h"
#include "spi_ram.h"

#define SPI_FIFO_SIZE 64

static const char *TAG = "spi_ram_fifo";

typedef struct {
    uint32_t read_pos;
    uint32_t write_pos;
    uint32_t fill_len;
    uint32_t overflow_num;
    uint32_t underrun_num;
    spi_ram_num_t ram_num;
    uint32_t start_addr;
    uint32_t total_size;
    SemaphoreHandle_t mux;
    SemaphoreHandle_t read_sem;
    SemaphoreHandle_t write_sem;
} spi_ram_fifo_obj_t;

#define SPI_RAM_FIFO_CHECK(a, str, ret_val) \
    if (!(a)) { \
        ESP_LOGE(TAG,"%s(%d): %s", __FUNCTION__, __LINE__, str); \
        return (ret_val); \
    }

esp_err_t spi_ram_fifo_write(spi_ram_fifo_handle_t handle, uint8_t *data, int len, uint32_t timeout_ticks)
{
    spi_ram_fifo_obj_t *obj = (spi_ram_fifo_obj_t *)handle;
    int n;
    SPI_RAM_FIFO_CHECK(handle, "spi ram fifo not created yet", ESP_FAIL);
    SPI_RAM_FIFO_CHECK(data, "param null", ESP_ERR_INVALID_ARG);

    while (len > 0) {
        n = len;

        if (n > SPI_FIFO_SIZE) {
            n = SPI_FIFO_SIZE;        // don't read more than SPI_FIFO_SIZE
        }

        if (n > (obj->total_size - obj->write_pos)) {
            n = obj->total_size - obj->write_pos; // don't read past end of buffer
        }

        xSemaphoreTake(obj->mux, portMAX_DELAY);

        if ((obj->total_size - obj->fill_len) < n) {
            // Not enough free room in FIFO. Wait till there's some read and try again.
            obj->overflow_num++;
            xSemaphoreGive(obj->mux);
            if (pdFALSE == xSemaphoreTake(obj->write_sem, timeout_ticks)) {
                xSemaphoreGive(obj->mux);
                return ESP_ERR_TIMEOUT;
            }
        } else {
            //Write the data.
            spi_ram_write(obj->ram_num, obj->start_addr + obj->write_pos, data, n);
            data += n;
            len -= n;
            obj->fill_len += n;
            obj->write_pos += n;

            if (obj->write_pos >= obj->total_size) {
                obj->write_pos = 0;
            }

            xSemaphoreGive(obj->mux);
            xSemaphoreGive(obj->read_sem); // Tell reader thread there's some data in the fifo.
        }
    }

    return ESP_OK;
}

esp_err_t spi_ram_fifo_read(spi_ram_fifo_handle_t handle, uint8_t *data, int len, uint32_t timeout_ticks)
{
    spi_ram_fifo_obj_t *obj = (spi_ram_fifo_obj_t *)handle;
    int n;
    SPI_RAM_FIFO_CHECK(handle, "spi ram fifo not created yet", ESP_FAIL);
    SPI_RAM_FIFO_CHECK(data, "param null", ESP_ERR_INVALID_ARG);

    while (len > 0) {
        n = len;

        if (n > SPI_FIFO_SIZE) {
            n = SPI_FIFO_SIZE;            // don't read more than SPI_FIFO_SIZE
        }

        if (n > (obj->total_size - obj->read_pos)) {
            n = obj->total_size - obj->read_pos; // don't read past end of buffer
        }

        xSemaphoreTake(obj->mux, portMAX_DELAY);

        if (obj->fill_len < n) {
            // Not enough data in FIFO. Wait till there's some written and try again.
            obj->underrun_num++;
            xSemaphoreGive(obj->mux);
            if (pdFALSE == xSemaphoreTake(obj->read_sem, timeout_ticks)) {
                xSemaphoreGive(obj->mux);
                return ESP_ERR_TIMEOUT;
            }
        } else {
            // Read the data.
            spi_ram_read(obj->ram_num, obj->start_addr + obj->read_pos, data, n);
            data += n;
            len -= n;
            obj->fill_len -= n;
            obj->read_pos += n;

            if (obj->read_pos >= obj->total_size) {
                obj->read_pos = 0;
            }

            xSemaphoreGive(obj->mux);
            xSemaphoreGive(obj->write_sem); // Indicate writer thread there's some free space in the fifo
        }
    }

    return ESP_OK;
}

esp_err_t spi_ram_fifo_get_fill(spi_ram_fifo_handle_t handle, uint32_t *len)
{
    spi_ram_fifo_obj_t *obj = (spi_ram_fifo_obj_t *)handle;
    SPI_RAM_FIFO_CHECK(handle, "spi ram fifo not created yet", ESP_FAIL);
    SPI_RAM_FIFO_CHECK(len, "param null", ESP_ERR_INVALID_ARG);
    xSemaphoreTake(obj->mux, portMAX_DELAY);
    *len = obj->fill_len;
    xSemaphoreGive(obj->mux);
    return ESP_OK;
}

esp_err_t spi_ram_fifo_get_free(spi_ram_fifo_handle_t handle, uint32_t *len)
{
    spi_ram_fifo_obj_t *obj = (spi_ram_fifo_obj_t *)handle;
    SPI_RAM_FIFO_CHECK(handle, "spi ram fifo not created yet", ESP_FAIL);
    SPI_RAM_FIFO_CHECK(len, "param null", ESP_ERR_INVALID_ARG);
    xSemaphoreTake(obj->mux, portMAX_DELAY);
    *len = obj->total_size - obj->fill_len;
    xSemaphoreGive(obj->mux);
    return ESP_OK;
}

esp_err_t spi_ram_fifo_get_total(spi_ram_fifo_handle_t handle, uint32_t *len)
{
    spi_ram_fifo_obj_t *obj = (spi_ram_fifo_obj_t *)handle;
    SPI_RAM_FIFO_CHECK(handle, "spi ram fifo not created yet", ESP_FAIL);
    SPI_RAM_FIFO_CHECK(len, "param null", ESP_ERR_INVALID_ARG);
    xSemaphoreTake(obj->mux, portMAX_DELAY);
    *len = obj->total_size;
    xSemaphoreGive(obj->mux);
    return ESP_OK;
}

esp_err_t spi_ram_fifo_get_overflow(spi_ram_fifo_handle_t handle, uint32_t *num)
{
    spi_ram_fifo_obj_t *obj = (spi_ram_fifo_obj_t *)handle;
    SPI_RAM_FIFO_CHECK(handle, "spi ram fifo not created yet", ESP_FAIL);
    SPI_RAM_FIFO_CHECK(num, "param null", ESP_ERR_INVALID_ARG);
    xSemaphoreTake(obj->mux, portMAX_DELAY);
    *num = obj->overflow_num;
    xSemaphoreGive(obj->mux);
    return ESP_OK;
}

esp_err_t spi_ram_fifo_get_underrun(spi_ram_fifo_handle_t handle, uint32_t *num)
{
    spi_ram_fifo_obj_t *obj = (spi_ram_fifo_obj_t *)handle;
    SPI_RAM_FIFO_CHECK(handle, "spi ram fifo not created yet", ESP_FAIL);
    SPI_RAM_FIFO_CHECK(num, "param null", ESP_ERR_INVALID_ARG);
    xSemaphoreTake(obj->mux, portMAX_DELAY);
    *num = obj->underrun_num;
    xSemaphoreGive(obj->mux);
    return ESP_OK;
}

esp_err_t spi_ram_fifo_delete(spi_ram_fifo_handle_t handle)
{
    spi_ram_fifo_obj_t *obj = (spi_ram_fifo_obj_t *)handle;
    SPI_RAM_FIFO_CHECK(handle, "spi ram fifo not created yet", ESP_FAIL);

    if (obj->mux) {
        xSemaphoreTake(obj->mux, portMAX_DELAY);
    }

    vSemaphoreDelete(obj->write_sem);
    vSemaphoreDelete(obj->read_sem);
    vSemaphoreDelete(obj->mux);
    heap_caps_free(obj);
    return ESP_OK;
}

spi_ram_fifo_handle_t spi_ram_fifo_create(spi_ram_fifo_config_t *config)
{
    spi_ram_fifo_obj_t *obj = NULL;

    if (NULL == config) {
        ESP_LOGE(TAG, "config null");
        return NULL;
    }

    obj = (spi_ram_fifo_obj_t *)heap_caps_zalloc(sizeof(spi_ram_fifo_obj_t), MALLOC_CAP_8BIT);

    if (NULL == obj) {
        ESP_LOGE(TAG, "obj malloc fail");
        return NULL;
    }

    obj->mux = xSemaphoreCreateMutex();
    obj->read_sem = xSemaphoreCreateBinary();
    obj->write_sem = xSemaphoreCreateBinary();

    if (NULL == obj->mux || NULL == obj->read_sem || NULL == obj->read_sem) {
        ESP_LOGE(TAG, "obj create fail");
        spi_ram_fifo_delete((spi_ram_fifo_handle_t)obj);
        return NULL;
    }

    obj->ram_num = config->ram_num;
    obj->start_addr = config->start_addr;
    obj->total_size = config->total_size;

    return (spi_ram_fifo_handle_t)obj;
}
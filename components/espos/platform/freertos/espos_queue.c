// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
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

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "espos_queue.h"
#include "espos_scheduler.h"

#define portYIELD_FROM_ISR() taskYIELD()

/**
 * @brief create a queue
 */
esp_err_t espos_queue_create (
    espos_queue_t *queue,
    espos_size_t msg_len,
    espos_size_t queue_len
)
{
    esp_err_t ret;
    QueueHandle_t os_queue;

    if (!queue || !msg_len || !queue_len) {
        return -EINVAL;
    }

    if (espos_in_isr() == true) {
        return -EINTR;
    }

    os_queue = xQueueCreate(queue_len, msg_len);
    if (os_queue) {
        ret = 0;
        *queue = (espos_queue_t)os_queue;
    } else {
        ret = -ENOMEM;
    }

    return ret;
}

/**
 * @brief set a queue name
 *
 * @param queue queue handle
 * @param name queue's name
 *
 * @return the result
 *             0 : successful
 *       -EINVAL : input parameter error
 */
esp_err_t espos_queue_set_name(espos_queue_t queue, const char *name)
{
    return 0;
}

/**
 * @brief send a message to the queue
 */
esp_err_t espos_queue_send_generic (
    espos_queue_t queue,
    void *msg,
    espos_tick_t wait_ticks,
    espos_pos_t pos,
    espos_opt_t opt
)
{
    esp_err_t ret;
    QueueHandle_t os_queue = (QueueHandle_t)queue;

    if (!queue || !msg || pos >= ESPOS_QUEUE_POS_MAX
            || opt >= ESPOS_QUEUE_SEND_OPT_MAX) {
        return -EINVAL;
    }

    if (espos_in_isr() == true) {
        BaseType_t xHigherPrioritTaskWoken = pdFALSE;

        if (pos == ESPOS_QUEUE_SEND_FRONT) {
            ret = xQueueSendToFrontFromISR(os_queue, msg, &xHigherPrioritTaskWoken);
        } else if (pos == ESPOS_QUEUE_SEND_BACK) {
            ret = xQueueSendToBackFromISR(os_queue, msg, &xHigherPrioritTaskWoken);
        } else {
            ret = pdFAIL;
        }

        if (pdPASS == ret && pdTRUE == xHigherPrioritTaskWoken) {
            portYIELD_FROM_ISR();
        }

        if (ret == pdPASS) {
            ret = 0;
        } else {
            ret = -ETIMEDOUT;
        }
    } else {
        if (pos == ESPOS_QUEUE_SEND_FRONT) {
            ret = xQueueSendToFront(os_queue, msg, wait_ticks);
        } else if (pos == ESPOS_QUEUE_SEND_BACK) {
            ret = xQueueSendToBack(os_queue, msg, wait_ticks);
        } else {
            ret = pdFAIL;
        }

        if (ret == pdPASS) {
            ret = 0;
        } else {
            ret = -ETIMEDOUT;
        }
    }

    return ret;
}

/**
 * @brief receive a message of the queue
 */
esp_err_t espos_queue_recv_generic (
    espos_queue_t queue,
    void *msg,
    espos_tick_t wait_ticks,
    espos_opt_t opt
)
{
    esp_err_t ret;
    QueueHandle_t os_queue = (QueueHandle_t)queue;

    if (!os_queue || !msg || opt >= ESPOS_QUEUE_RECV_OPT_MAX) {
        return -EINVAL;
    }

    if (espos_in_isr() == true) {
        BaseType_t xHigherPrioritTaskWoken = pdFALSE;

        ret = xQueueReceiveFromISR(os_queue, msg, &xHigherPrioritTaskWoken);
        if (pdTRUE == ret && pdTRUE == xHigherPrioritTaskWoken) {
            portYIELD_FROM_ISR();
        }

        if (ret == pdTRUE) {
            ret = 0;
        } else {
            ret = -ETIMEDOUT;
        }
    } else {
        ret = xQueueReceive(os_queue, msg, wait_ticks);
        if (ret == pdTRUE) {
            ret = 0;
        } else {
            ret = -ETIMEDOUT;
        }
    }

    return ret;

}

/**
 * @brief get current message number of the queue
 */
espos_size_t espos_queue_msg_waiting(espos_queue_t queue)
{
    UBaseType_t num;
    QueueHandle_t os_queue = (QueueHandle_t)queue;

    if (!os_queue) {
        return 0;
    }

    num = uxQueueMessagesWaiting(os_queue);

    return num;
}

/**
 * @brief reset the queue
 */
esp_err_t espos_queue_flush (
    espos_queue_t queue
)
{
    BaseType_t ret;
    QueueHandle_t os_queue = (QueueHandle_t)queue;

    if (!os_queue) {
        return -EINVAL;
    }

    ret = xQueueReset(os_queue);
    if (ret == pdTRUE) {
        ret = 0;
    } else {
        ret = -EINVAL;
    }

    return 0;
}

/**
 * @brief delete the queue
 */
esp_err_t espos_queue_del (
    espos_queue_t queue
)
{
    QueueHandle_t os_queue = (QueueHandle_t)queue;

    if (!os_queue) {
        return -EINVAL;
    }

    if (espos_in_isr() == true) {
        return -EINTR;
    }

    vQueueDelete(os_queue);

    return 0;
}


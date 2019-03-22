#include "mqtt_outbox.h"
#include <stdlib.h>
#include <string.h>
#include "rom/queue.h"
#include "esp_log.h"

#ifndef CONFIG_MQTT_CUSTOM_OUTBOX


static const char *TAG = "OUTBOX";

typedef struct outbox_item {
    char *buffer;
    int len;
    int msg_id;
    int msg_type;
    int tick;
    int retry_count;
    bool pending;
    STAILQ_ENTRY(outbox_item) next;
} outbox_item_t;

STAILQ_HEAD(outbox_list_t, outbox_item);


outbox_handle_t outbox_init()
{
    outbox_handle_t outbox = calloc(1, sizeof(struct outbox_list_t));
    ESP_MEM_CHECK(TAG, outbox, return NULL);
    STAILQ_INIT(outbox);
    return outbox;
}

outbox_item_handle_t outbox_enqueue(outbox_handle_t outbox, uint8_t *data, int len, int msg_id, int msg_type, int tick)
{
    outbox_item_handle_t item = calloc(1, sizeof(outbox_item_t));
    ESP_MEM_CHECK(TAG, item, return NULL);
    item->msg_id = msg_id;
    item->msg_type = msg_type;
    item->tick = tick;
    item->len = len;
    item->buffer = malloc(len);
    ESP_MEM_CHECK(TAG, item->buffer, {
        free(item);
        return NULL;
    });
    memcpy(item->buffer, data, len);
    STAILQ_INSERT_TAIL(outbox, item, next);
    ESP_LOGD(TAG, "ENQUEUE msgid=%d, msg_type=%d, len=%d, size=%d", msg_id, msg_type, len, outbox_get_size(outbox));
    return item;
}

outbox_item_handle_t outbox_get(outbox_handle_t outbox, int msg_id)
{
    outbox_item_handle_t item;
    STAILQ_FOREACH(item, outbox, next) {
        if (item->msg_id == msg_id) {
            return item;
        }
    }
    return NULL;
}

outbox_item_handle_t outbox_dequeue(outbox_handle_t outbox)
{
    outbox_item_handle_t item;
    STAILQ_FOREACH(item, outbox, next) {
        if (!item->pending) {
            return item;
        }
    }
    return NULL;
}
esp_err_t outbox_delete(outbox_handle_t outbox, int msg_id, int msg_type)
{
    outbox_item_handle_t item, tmp;
    STAILQ_FOREACH_SAFE(item, outbox, next, tmp) {
        if (item->msg_id == msg_id && item->msg_type == msg_type) {
            STAILQ_REMOVE(outbox, item, outbox_item, next);
            free(item->buffer);
            free(item);
            ESP_LOGD(TAG, "DELETED msgid=%d, msg_type=%d, remain size=%d", msg_id, msg_type, outbox_get_size(outbox));
            return ESP_OK;
        }

    }
    return ESP_FAIL;
}
esp_err_t outbox_delete_msgid(outbox_handle_t outbox, int msg_id)
{
    outbox_item_handle_t item, tmp;
    STAILQ_FOREACH_SAFE(item, outbox, next, tmp) {
        if (item->msg_id == msg_id) {
            STAILQ_REMOVE(outbox, item, outbox_item, next);
            free(item->buffer);
            free(item);
        }

    }
    return ESP_OK;
}
esp_err_t outbox_set_pending(outbox_handle_t outbox, int msg_id)
{
    outbox_item_handle_t item = outbox_get(outbox, msg_id);
    if (item) {
        item->pending = true;
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t outbox_delete_msgtype(outbox_handle_t outbox, int msg_type)
{
    outbox_item_handle_t item, tmp;
    STAILQ_FOREACH_SAFE(item, outbox, next, tmp) {
        if (item->msg_type == msg_type) {
            STAILQ_REMOVE(outbox, item, outbox_item, next);
            free(item->buffer);
            free(item);
        }

    }
    return ESP_OK;
}

esp_err_t outbox_delete_expired(outbox_handle_t outbox, int current_tick, int timeout)
{
    outbox_item_handle_t item, tmp;
    STAILQ_FOREACH_SAFE(item, outbox, next, tmp) {
        if (current_tick - item->tick > timeout) {
            STAILQ_REMOVE(outbox, item, outbox_item, next);
            free(item->buffer);
            free(item);
        }

    }
    return ESP_OK;
}

int outbox_get_size(outbox_handle_t outbox)
{
    int siz = 0;
    outbox_item_handle_t item;
    STAILQ_FOREACH(item, outbox, next) {
        siz += item->len;
    }
    return siz;
}

esp_err_t outbox_cleanup(outbox_handle_t outbox, int max_size)
{
    while(outbox_get_size(outbox) > max_size) {
        outbox_item_handle_t item = outbox_dequeue(outbox);
        if (item == NULL) {
            return ESP_FAIL;
        }
        STAILQ_REMOVE(outbox, item, outbox_item, next);
        free(item->buffer);
        free(item);
    }
    return ESP_OK;
}

void outbox_destroy(outbox_handle_t outbox)
{
    outbox_cleanup(outbox, 0);
    free(outbox);
}

#endif /* CONFIG_MQTT_CUSTOM_OUTBOX */
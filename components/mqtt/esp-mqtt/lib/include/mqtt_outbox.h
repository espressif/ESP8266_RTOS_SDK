/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 * Tuan PM <tuanpm at live dot com>
 */
#ifndef _MQTT_OUTOBX_H_
#define _MQTT_OUTOBX_H_
#include "platform.h"

#ifdef  __cplusplus
extern "C" {
#endif

struct outbox_item;

typedef struct outbox_list_t * outbox_handle_t;
typedef struct outbox_item * outbox_item_handle_t;
typedef struct outbox_message * outbox_message_handle_t;

typedef struct outbox_message {
  uint8_t *data;
  int len;
  int msg_id;
  int msg_qos;
  int msg_type;
  uint8_t *remaining_data;
  int remaining_len;
} outbox_message_t;

typedef enum pending_state {
    QUEUED,
    TRANSMITTED,
    CONFIRMED
} pending_state_t;

outbox_handle_t outbox_init();
outbox_item_handle_t outbox_enqueue(outbox_handle_t outbox, outbox_message_handle_t message, int tick);
outbox_item_handle_t outbox_dequeue(outbox_handle_t outbox, pending_state_t pending);
outbox_item_handle_t outbox_get(outbox_handle_t outbox, int msg_id);
uint8_t* outbox_item_get_data(outbox_item_handle_t item,  size_t *len, uint16_t *msg_id, int *msg_type, int *qos);
esp_err_t outbox_delete(outbox_handle_t outbox, int msg_id, int msg_type);
esp_err_t outbox_delete_msgid(outbox_handle_t outbox, int msg_id);
esp_err_t outbox_delete_msgtype(outbox_handle_t outbox, int msg_type);
esp_err_t outbox_delete_expired(outbox_handle_t outbox, int current_tick, int timeout);

esp_err_t outbox_set_pending(outbox_handle_t outbox, int msg_id, pending_state_t pending);
int outbox_get_size(outbox_handle_t outbox);
esp_err_t outbox_cleanup(outbox_handle_t outbox, int max_size);
void outbox_destroy(outbox_handle_t outbox);

#ifdef  __cplusplus
}
#endif
#endif

#include <stdio.h>
#include "platform.h"

#include "mqtt_client.h"
#include "mqtt_msg.h"
#include "esp_transport.h"
#include "esp_transport_tcp.h"
#include "esp_transport_ssl.h"
#include "esp_transport_ws.h"
#include "platform.h"
#include "mqtt_outbox.h"

/* using uri parser */
#include "http_parser.h"

#ifdef MQTT_DISABLE_API_LOCKS
# define MQTT_API_LOCK(c)
# define MQTT_API_UNLOCK(c)
# define MQTT_API_LOCK_FROM_OTHER_TASK(c)
# define MQTT_API_UNLOCK_FROM_OTHER_TASK(c)
#else
# define MQTT_API_LOCK(c)          xSemaphoreTake(c->api_lock, portMAX_DELAY)
# define MQTT_API_UNLOCK(c)        xSemaphoreGive(c->api_lock)
# define MQTT_API_LOCK_FROM_OTHER_TASK(c)    { if (c->task_handle != xTaskGetCurrentTaskHandle()) { xSemaphoreTake(c->api_lock, portMAX_DELAY); } }
# define MQTT_API_UNLOCK_FROM_OTHER_TASK(c)  { if (c->task_handle != xTaskGetCurrentTaskHandle()) { xSemaphoreGive(c->api_lock); } }
#endif /* MQTT_USE_API_LOCKS */

static const char *TAG = "MQTT_CLIENT";

typedef struct mqtt_state
{
    mqtt_connect_info_t *connect_info;
    uint8_t *in_buffer;
    uint8_t *out_buffer;
    int in_buffer_length;
    int out_buffer_length;
    uint32_t message_length;
    uint32_t message_length_read;
    mqtt_message_t *outbound_message;
    mqtt_connection_t mqtt_connection;
    uint16_t pending_msg_id;
    int pending_msg_type;
    int pending_publish_qos;
    int pending_msg_count;
} mqtt_state_t;

typedef struct {
    mqtt_event_callback_t event_handle;
    int task_stack;
    int task_prio;
    char *uri;
    char *host;
    char *path;
    char *scheme;
    int port;
    bool auto_reconnect;
    void *user_context;
    int network_timeout_ms;
    int refresh_connection_after_ms;
} mqtt_config_storage_t;

typedef enum {
    MQTT_STATE_ERROR = -1,
    MQTT_STATE_UNKNOWN = 0,
    MQTT_STATE_INIT,
    MQTT_STATE_CONNECTED,
    MQTT_STATE_WAIT_TIMEOUT,
} mqtt_client_state_t;

struct esp_mqtt_client {
    esp_transport_list_handle_t transport_list;
    esp_transport_handle_t transport;
    mqtt_config_storage_t *config;
    mqtt_state_t  mqtt_state;
    mqtt_connect_info_t connect_info;
    mqtt_client_state_t state;
    uint64_t refresh_connection_tick;
    uint64_t keepalive_tick;
    uint64_t reconnect_tick;
    int wait_timeout_ms;
    int auto_reconnect;
    esp_mqtt_event_t event;
    bool run;
    bool wait_for_ping_resp;
    outbox_handle_t outbox;
    EventGroupHandle_t status_bits;
    SemaphoreHandle_t  api_lock;
    TaskHandle_t       task_handle;
};

const static int STOPPED_BIT = BIT0;
const static int RECONNECT_BIT = BIT1;

static esp_err_t esp_mqtt_dispatch_event(esp_mqtt_client_handle_t client);
static esp_err_t esp_mqtt_dispatch_event_with_msgid(esp_mqtt_client_handle_t client);
static esp_err_t esp_mqtt_destroy_config(esp_mqtt_client_handle_t client);
static esp_err_t esp_mqtt_connect(esp_mqtt_client_handle_t client, int timeout_ms);
static esp_err_t esp_mqtt_abort_connection(esp_mqtt_client_handle_t client);
static esp_err_t esp_mqtt_client_ping(esp_mqtt_client_handle_t client);
static char *create_string(const char *ptr, int len);

esp_err_t esp_mqtt_set_config(esp_mqtt_client_handle_t client, const esp_mqtt_client_config_t *config)
{
    MQTT_API_LOCK(client);
    //Copy user configurations to client context
    esp_err_t err = ESP_OK;
    mqtt_config_storage_t *cfg;
    if (client->config) {
        cfg = client->config;
    } else {
        cfg = calloc(1, sizeof(mqtt_config_storage_t));
        ESP_MEM_CHECK(TAG, cfg, {
            MQTT_API_UNLOCK(client);
            return ESP_ERR_NO_MEM;
        });
        client->config = cfg;
    }
    if (config->task_prio) {
        cfg->task_prio = config->task_prio;
    }

    if (cfg->task_prio <= 0) {
        cfg->task_prio = MQTT_TASK_PRIORITY;
    }
    if (config->task_stack) {
        cfg->task_stack = config->task_stack;
    }
    if (cfg->task_stack == 0) {
        cfg->task_stack = MQTT_TASK_STACK;
    }
    if (config->port) {
        cfg->port = config->port;
    }

    err = ESP_ERR_NO_MEM;
    if (config->host) {
        free(cfg->host);
        cfg->host = strdup(config->host);
        ESP_MEM_CHECK(TAG, cfg->host, goto _mqtt_set_config_failed);
    }

    if (config->username) {
        free(client->connect_info.username);
        client->connect_info.username = strdup(config->username);
        ESP_MEM_CHECK(TAG, client->connect_info.username, goto _mqtt_set_config_failed);
    }

    if (config->password) {
        free(client->connect_info.password);
        client->connect_info.password = strdup(config->password);
        ESP_MEM_CHECK(TAG, client->connect_info.password, goto _mqtt_set_config_failed);
    }

    if (config->client_id) {
        free(client->connect_info.client_id);
        client->connect_info.client_id = strdup(config->client_id);
        ESP_MEM_CHECK(TAG, client->connect_info.client_id, goto _mqtt_set_config_failed);
    } else if (client->connect_info.client_id == NULL) {
        client->connect_info.client_id = platform_create_id_string();
    }
    ESP_MEM_CHECK(TAG, client->connect_info.client_id, goto _mqtt_set_config_failed);
    ESP_LOGD(TAG, "MQTT client_id=%s", client->connect_info.client_id);

    if (config->uri) {
        free(cfg->uri);
        cfg->uri = strdup(config->uri);
        ESP_MEM_CHECK(TAG, cfg->uri, goto _mqtt_set_config_failed);
    }

    if (config->lwt_topic) {
        free(client->connect_info.will_topic);
        client->connect_info.will_topic = strdup(config->lwt_topic);
        ESP_MEM_CHECK(TAG, client->connect_info.will_topic, goto _mqtt_set_config_failed);
    }

    if (config->lwt_msg_len && config->lwt_msg) {
        free(client->connect_info.will_message);
        client->connect_info.will_message = malloc(config->lwt_msg_len);
        ESP_MEM_CHECK(TAG, client->connect_info.will_message, goto _mqtt_set_config_failed);
        memcpy(client->connect_info.will_message, config->lwt_msg, config->lwt_msg_len);
        client->connect_info.will_length = config->lwt_msg_len;
    } else if (config->lwt_msg) {
        free(client->connect_info.will_message);
        client->connect_info.will_message = strdup(config->lwt_msg);
        ESP_MEM_CHECK(TAG, client->connect_info.will_message, goto _mqtt_set_config_failed);
        client->connect_info.will_length = strlen(config->lwt_msg);
    }
    if (config->lwt_qos) {
        client->connect_info.will_qos = config->lwt_qos;
    }
    if (config->lwt_retain) {
        client->connect_info.will_retain = config->lwt_retain;
    }

    if (config->disable_clean_session == client->connect_info.clean_session) {
        client->connect_info.clean_session = !config->disable_clean_session;
    }
    if (config->keepalive) {
        client->connect_info.keepalive = config->keepalive;
    }
    if (client->connect_info.keepalive == 0) {
        client->connect_info.keepalive = MQTT_KEEPALIVE_TICK;
    }
    cfg->network_timeout_ms = MQTT_NETWORK_TIMEOUT_MS;
    if (config->user_context) {
        cfg->user_context = config->user_context;
    }

    if (config->event_handle) {
        cfg->event_handle = config->event_handle;
    }

    if (config->refresh_connection_after_ms) {
        cfg->refresh_connection_after_ms = config->refresh_connection_after_ms;
    }

    cfg->auto_reconnect = true;
    if (config->disable_auto_reconnect == cfg->auto_reconnect) {
        cfg->auto_reconnect = !config->disable_auto_reconnect;
    }
    MQTT_API_UNLOCK(client);
    return ESP_OK;
_mqtt_set_config_failed:
    esp_mqtt_destroy_config(client);
    MQTT_API_UNLOCK(client);
    return err;
}

static esp_err_t esp_mqtt_destroy_config(esp_mqtt_client_handle_t client)
{
    mqtt_config_storage_t *cfg = client->config;
    free(cfg->host);
    free(cfg->uri);
    free(cfg->path);
    free(cfg->scheme);
    memset(cfg, 0, sizeof(mqtt_config_storage_t));
    free(client->connect_info.will_topic);
    free(client->connect_info.will_message);
    free(client->connect_info.client_id);
    free(client->connect_info.username);
    free(client->connect_info.password);
    memset(&client->connect_info, 0, sizeof(mqtt_connect_info_t));
    free(client->config);
    return ESP_OK;
}

static esp_err_t esp_mqtt_connect(esp_mqtt_client_handle_t client, int timeout_ms)
{
    int write_len, read_len, connect_rsp_code;
    client->wait_for_ping_resp = false;
    mqtt_msg_init(&client->mqtt_state.mqtt_connection,
                  client->mqtt_state.out_buffer,
                  client->mqtt_state.out_buffer_length);
    client->mqtt_state.outbound_message = mqtt_msg_connect(&client->mqtt_state.mqtt_connection,
                                          client->mqtt_state.connect_info);
    client->mqtt_state.pending_msg_type = mqtt_get_type(client->mqtt_state.outbound_message->data);
    client->mqtt_state.pending_msg_id = mqtt_get_id(client->mqtt_state.outbound_message->data,
                                        client->mqtt_state.outbound_message->length);
    ESP_LOGI(TAG, "Sending MQTT CONNECT message, type: %d, id: %04X",
             client->mqtt_state.pending_msg_type,
             client->mqtt_state.pending_msg_id);

    write_len = esp_transport_write(client->transport,
                                (char *)client->mqtt_state.outbound_message->data,
                                client->mqtt_state.outbound_message->length,
                                client->config->network_timeout_ms);
    if (write_len < 0) {
        ESP_LOGE(TAG, "Writing failed, errno= %d", errno);
        return ESP_FAIL;
    }
    read_len = esp_transport_read(client->transport,
                              (char *)client->mqtt_state.in_buffer,
                              client->mqtt_state.in_buffer_length,
                              client->config->network_timeout_ms);
    if (read_len < 0) {
        ESP_LOGE(TAG, "Error network response");
        return ESP_FAIL;
    }

    if (mqtt_get_type(client->mqtt_state.in_buffer) != MQTT_MSG_TYPE_CONNACK) {
        ESP_LOGE(TAG, "Invalid MSG_TYPE response: %d, read_len: %d", mqtt_get_type(client->mqtt_state.in_buffer), read_len);
        return ESP_FAIL;
    }
    connect_rsp_code = mqtt_get_connect_return_code(client->mqtt_state.in_buffer);
    switch (connect_rsp_code) {
        case CONNECTION_ACCEPTED:
            ESP_LOGD(TAG, "Connected");
            return ESP_OK;
        case CONNECTION_REFUSE_PROTOCOL:
            ESP_LOGW(TAG, "Connection refused, bad protocol");
            return ESP_FAIL;
        case CONNECTION_REFUSE_SERVER_UNAVAILABLE:
            ESP_LOGW(TAG, "Connection refused, server unavailable");
            return ESP_FAIL;
        case CONNECTION_REFUSE_BAD_USERNAME:
            ESP_LOGW(TAG, "Connection refused, bad username or password");
            return ESP_FAIL;
        case CONNECTION_REFUSE_NOT_AUTHORIZED:
            ESP_LOGW(TAG, "Connection refused, not authorized");
            return ESP_FAIL;
        default:
            ESP_LOGW(TAG, "Connection refused, Unknow reason");
            return ESP_FAIL;
    }
    return ESP_OK;
}

static esp_err_t esp_mqtt_abort_connection(esp_mqtt_client_handle_t client)
{
    esp_transport_close(client->transport);
    client->wait_timeout_ms = MQTT_RECONNECT_TIMEOUT_MS;
    client->reconnect_tick = platform_tick_get_ms();
    client->state = MQTT_STATE_WAIT_TIMEOUT;
    ESP_LOGD(TAG, "Reconnect after %d ms", client->wait_timeout_ms);
    client->event.event_id = MQTT_EVENT_DISCONNECTED;
    client->wait_for_ping_resp = false;
    esp_mqtt_dispatch_event_with_msgid(client);
    return ESP_OK;
}

esp_mqtt_client_handle_t esp_mqtt_client_init(const esp_mqtt_client_config_t *config)
{
    esp_mqtt_client_handle_t client = calloc(1, sizeof(struct esp_mqtt_client));
    ESP_MEM_CHECK(TAG, client, return NULL);
    client->api_lock = xSemaphoreCreateMutex();
    if (!client->api_lock) {
        free(client);
        return NULL;
    }
    esp_mqtt_set_config(client, config);
    MQTT_API_LOCK(client);
    client->transport_list = esp_transport_list_init();
    ESP_MEM_CHECK(TAG, client->transport_list, goto _mqtt_init_failed);

    esp_transport_handle_t tcp = esp_transport_tcp_init();
    ESP_MEM_CHECK(TAG, tcp, goto _mqtt_init_failed);
    esp_transport_set_default_port(tcp, MQTT_TCP_DEFAULT_PORT);
    esp_transport_list_add(client->transport_list, tcp, "mqtt");
    if (config->transport == MQTT_TRANSPORT_OVER_TCP) {
        client->config->scheme = create_string("mqtt", 4);
        ESP_MEM_CHECK(TAG, client->config->scheme, goto _mqtt_init_failed);
    }

#if MQTT_ENABLE_WS
    esp_transport_handle_t ws = esp_transport_ws_init(tcp);
    ESP_MEM_CHECK(TAG, ws, goto _mqtt_init_failed);
    esp_transport_set_default_port(ws, MQTT_WS_DEFAULT_PORT);
    esp_transport_list_add(client->transport_list, ws, "ws");
    if (config->transport == MQTT_TRANSPORT_OVER_WS) {
        client->config->scheme = create_string("ws", 2);
        ESP_MEM_CHECK(TAG, client->config->scheme, goto _mqtt_init_failed);
    }
#endif

#if MQTT_ENABLE_SSL
    esp_transport_handle_t ssl = esp_transport_ssl_init();
    ESP_MEM_CHECK(TAG, ssl, goto _mqtt_init_failed);
    esp_transport_set_default_port(ssl, MQTT_SSL_DEFAULT_PORT);
    if (config->cert_pem) {
        esp_transport_ssl_set_cert_data(ssl, config->cert_pem, strlen(config->cert_pem));
    }
    if (config->client_cert_pem) {
        esp_transport_ssl_set_client_cert_data(ssl, config->client_cert_pem, strlen(config->client_cert_pem));
    }
    if (config->client_key_pem) {
        esp_transport_ssl_set_client_key_data(ssl, config->client_key_pem, strlen(config->client_key_pem));
    }
    esp_transport_list_add(client->transport_list, ssl, "mqtts");
    if (config->transport == MQTT_TRANSPORT_OVER_SSL) {
        client->config->scheme = create_string("mqtts", 5);
        ESP_MEM_CHECK(TAG, client->config->scheme, goto _mqtt_init_failed);
    }
#endif

#if MQTT_ENABLE_WSS
    esp_transport_handle_t wss = esp_transport_ws_init(ssl);
    ESP_MEM_CHECK(TAG, wss, goto _mqtt_init_failed);
    esp_transport_set_default_port(wss, MQTT_WSS_DEFAULT_PORT);
    esp_transport_list_add(client->transport_list, wss, "wss");
    if (config->transport == MQTT_TRANSPORT_OVER_WSS) {
        client->config->scheme = create_string("wss", 3);
        ESP_MEM_CHECK(TAG, client->config->scheme, goto _mqtt_init_failed);
    }
#endif
    if (client->config->uri) {
        if (esp_mqtt_client_set_uri(client, client->config->uri) != ESP_OK) {
            goto _mqtt_init_failed;
        }
    }

    client->keepalive_tick = platform_tick_get_ms();
    client->reconnect_tick = platform_tick_get_ms();
    client->refresh_connection_tick = platform_tick_get_ms();
    client->wait_for_ping_resp = false;
    int buffer_size = config->buffer_size;
    if (buffer_size <= 0) {
        buffer_size = MQTT_BUFFER_SIZE_BYTE;
    }

    client->mqtt_state.in_buffer = (uint8_t *)malloc(buffer_size);
    ESP_MEM_CHECK(TAG, client->mqtt_state.in_buffer, goto _mqtt_init_failed);
    client->mqtt_state.in_buffer_length = buffer_size;
    client->mqtt_state.out_buffer = (uint8_t *)malloc(buffer_size);
    ESP_MEM_CHECK(TAG, client->mqtt_state.out_buffer, goto _mqtt_init_failed);

    client->mqtt_state.out_buffer_length = buffer_size;
    client->mqtt_state.connect_info = &client->connect_info;
    client->outbox = outbox_init();
    ESP_MEM_CHECK(TAG, client->outbox, goto _mqtt_init_failed);
    client->status_bits = xEventGroupCreate();
    ESP_MEM_CHECK(TAG, client->status_bits, goto _mqtt_init_failed);
    MQTT_API_UNLOCK(client);
    return client;
_mqtt_init_failed:
    esp_mqtt_client_destroy(client);
    MQTT_API_UNLOCK(client);
    return NULL;
}

esp_err_t esp_mqtt_client_destroy(esp_mqtt_client_handle_t client)
{
    esp_mqtt_client_stop(client);
    esp_mqtt_destroy_config(client);
    esp_transport_list_destroy(client->transport_list);
    outbox_destroy(client->outbox);
    vEventGroupDelete(client->status_bits);
    free(client->mqtt_state.in_buffer);
    free(client->mqtt_state.out_buffer);
    vSemaphoreDelete(client->api_lock);
    free(client);
    return ESP_OK;
}

static char *create_string(const char *ptr, int len)
{
    char *ret;
    if (len <= 0) {
        return NULL;
    }
    ret = calloc(1, len + 1);
    ESP_MEM_CHECK(TAG, ret, return NULL);
    memcpy(ret, ptr, len);
    return ret;
}

esp_err_t esp_mqtt_client_set_uri(esp_mqtt_client_handle_t client, const char *uri)
{
    struct http_parser_url puri;
    http_parser_url_init(&puri);
    int parser_status = http_parser_parse_url(uri, strlen(uri), 0, &puri);
    if (parser_status != 0) {
        ESP_LOGE(TAG, "Error parse uri = %s", uri);
        return ESP_FAIL;
    }

    // set uri overrides actual scheme, host, path if configured previously
    free(client->config->scheme);
    free(client->config->host);
    free(client->config->path);

    client->config->scheme = create_string(uri + puri.field_data[UF_SCHEMA].off, puri.field_data[UF_SCHEMA].len);
    client->config->host = create_string(uri + puri.field_data[UF_HOST].off, puri.field_data[UF_HOST].len);
    client->config->path = create_string(uri + puri.field_data[UF_PATH].off, puri.field_data[UF_PATH].len);

    if (client->config->path) {
        esp_transport_handle_t trans = esp_transport_list_get_transport(client->transport_list, "ws");
        if (trans) {
            esp_transport_ws_set_path(trans, client->config->path);
        }
        trans = esp_transport_list_get_transport(client->transport_list, "wss");
        if (trans) {
            esp_transport_ws_set_path(trans, client->config->path);
        }
    }

    if (puri.field_data[UF_PORT].len) {
        client->config->port = strtol((const char*)(uri + puri.field_data[UF_PORT].off), NULL, 10);
    }

    char *user_info = create_string(uri + puri.field_data[UF_USERINFO].off, puri.field_data[UF_USERINFO].len);
    if (user_info) {
        char *pass = strchr(user_info, ':');
        if (pass) {
            pass[0] = 0; //terminal username
            pass ++;
            client->connect_info.password = strdup(pass);
        }
        client->connect_info.username = strdup(user_info);

        free(user_info);
    }

    return ESP_OK;
}

static esp_err_t mqtt_write_data(esp_mqtt_client_handle_t client)
{
    int write_len = esp_transport_write(client->transport,
                                    (char *)client->mqtt_state.outbound_message->data,
                                    client->mqtt_state.outbound_message->length,
                                    client->config->network_timeout_ms);
    // client->mqtt_state.pending_msg_type = mqtt_get_type(client->mqtt_state.outbound_message->data);
    if (write_len <= 0) {
        ESP_LOGE(TAG, "Error write data or timeout, written len = %d", write_len);
        return ESP_FAIL;
    }
    /* we've just sent a mqtt control packet, update keepalive counter
     * [MQTT-3.1.2-23]
     */
    client->keepalive_tick = platform_tick_get_ms();
    return ESP_OK;
}

static esp_err_t esp_mqtt_dispatch_event_with_msgid(esp_mqtt_client_handle_t client)
{
    client->event.msg_id = mqtt_get_id(client->mqtt_state.in_buffer, client->mqtt_state.in_buffer_length);
    return esp_mqtt_dispatch_event(client);
}

static esp_err_t esp_mqtt_dispatch_event(esp_mqtt_client_handle_t client)
{
    client->event.user_context = client->config->user_context;
    client->event.client = client;

    if (client->config->event_handle) {
        return client->config->event_handle(&client->event);
    }
    return ESP_FAIL;
}

static void deliver_publish(esp_mqtt_client_handle_t client, uint8_t *message, int length)
{
    const char *mqtt_topic = NULL, *mqtt_data = NULL;
    uint32_t mqtt_topic_length, mqtt_data_length;
    uint32_t mqtt_len = 0, mqtt_offset = 0, total_mqtt_len = 0;
    int len_read= length;
    int max_to_read = client->mqtt_state.in_buffer_length;
    int buffer_offset = 0;
    esp_transport_handle_t transport = client->transport;

    do
    {
        if (total_mqtt_len == 0) {
            /* any further reading only the underlying payload */
            transport = esp_transport_get_payload_transport_handle(transport);
            mqtt_data_length = mqtt_topic_length = length;
            if (NULL == (mqtt_topic = mqtt_get_publish_topic(message, &mqtt_topic_length)) ||
                NULL == (mqtt_data = mqtt_get_publish_data(message, &mqtt_data_length)) ) {
                // mqtt header is not complete, continue reading
                memmove(client->mqtt_state.in_buffer, message, length);
                buffer_offset = length;
                message = client->mqtt_state.in_buffer;
                max_to_read = client->mqtt_state.in_buffer_length - length;
                mqtt_len = 0;
            } else {
                total_mqtt_len = client->mqtt_state.message_length - client->mqtt_state.message_length_read + mqtt_data_length;
                mqtt_len = mqtt_data_length;
                mqtt_data_length = client->mqtt_state.message_length - ((uint8_t*)mqtt_data- message);
                /* read msg id only once */
                client->event.msg_id = mqtt_get_id(message, length);
            }
        } else {
            mqtt_len = len_read;
            mqtt_data = (const char*)client->mqtt_state.in_buffer;
            mqtt_topic = NULL;
            mqtt_topic_length = 0;
        }

        if (total_mqtt_len != 0) {
            ESP_LOGD(TAG, "Get data len= %d, topic len=%d", mqtt_len, mqtt_topic_length);
            client->event.event_id = MQTT_EVENT_DATA;
            client->event.data = (char *)mqtt_data;
            client->event.data_len = mqtt_len;
            client->event.total_data_len = mqtt_data_length;
            client->event.current_data_offset = mqtt_offset;
            client->event.topic = (char *)mqtt_topic;
            client->event.topic_len = mqtt_topic_length;
            esp_mqtt_dispatch_event(client);
        }

        mqtt_offset += mqtt_len;
        if (client->mqtt_state.message_length_read >= client->mqtt_state.message_length) {
            break;
        }

        len_read = esp_transport_read(transport,
                                  (char *)client->mqtt_state.in_buffer + buffer_offset,
                                  client->mqtt_state.message_length - client->mqtt_state.message_length_read > max_to_read ?
                                  max_to_read : client->mqtt_state.message_length - client->mqtt_state.message_length_read,
                                  client->config->network_timeout_ms);
        length = len_read + buffer_offset;
        buffer_offset = 0;
        max_to_read = client->mqtt_state.in_buffer_length;
        if (len_read <= 0) {
            ESP_LOGE(TAG, "Read error or timeout: len_read=%d, errno=%d", len_read, errno);
            break;
        }
        client->mqtt_state.message_length_read += len_read;
    } while (1);

}

static bool is_valid_mqtt_msg(esp_mqtt_client_handle_t client, int msg_type, int msg_id)
{
    ESP_LOGD(TAG, "pending_id=%d, pending_msg_count = %d", client->mqtt_state.pending_msg_id, client->mqtt_state.pending_msg_count);
    if (client->mqtt_state.pending_msg_count == 0) {
        return false;
    }
    if (outbox_delete(client->outbox, msg_id, msg_type) == ESP_OK) {
        client->mqtt_state.pending_msg_count --;
        return true;
    }
    if (client->mqtt_state.pending_msg_type == msg_type && client->mqtt_state.pending_msg_id == msg_id) {
        client->mqtt_state.pending_msg_count --;
        return true;
    }

    return false;
}

static void mqtt_enqueue_oversized(esp_mqtt_client_handle_t client, uint8_t *remaining_data, int remaining_len)
{
    ESP_LOGD(TAG, "mqtt_enqueue_oversized id: %d, type=%d successful",
             client->mqtt_state.pending_msg_id, client->mqtt_state.pending_msg_type);
    //lock mutex
    outbox_message_t msg = { 0 };
    if (client->mqtt_state.pending_msg_count > 0) {
        client->mqtt_state.pending_msg_count --;
    }
    msg.data = client->mqtt_state.outbound_message->data;
    msg.len =  client->mqtt_state.outbound_message->length;
    msg.msg_id = client->mqtt_state.pending_msg_id;
    msg.msg_type = client->mqtt_state.pending_msg_type;
    msg.msg_qos = client->mqtt_state.pending_publish_qos;
    msg.remaining_data = remaining_data;
    msg.remaining_len = remaining_len;
    //Copy to queue buffer
    outbox_enqueue(client->outbox, &msg, platform_tick_get_ms());

    //unlock
}

static void mqtt_enqueue(esp_mqtt_client_handle_t client)
{
    ESP_LOGD(TAG, "mqtt_enqueue id: %d, type=%d successful",
             client->mqtt_state.pending_msg_id, client->mqtt_state.pending_msg_type);
    //lock mutex
    if (client->mqtt_state.pending_msg_count > 0) {
        outbox_message_t msg = { 0 };
        msg.data = client->mqtt_state.outbound_message->data;
        msg.len =  client->mqtt_state.outbound_message->length;
        msg.msg_id = client->mqtt_state.pending_msg_id;
        msg.msg_type = client->mqtt_state.pending_msg_type;
        msg.msg_qos = client->mqtt_state.pending_publish_qos;
        //Copy to queue buffer
        outbox_enqueue(client->outbox, &msg, platform_tick_get_ms());
    }
    //unlock
}

static esp_err_t mqtt_process_receive(esp_mqtt_client_handle_t client)
{
    int read_len;
    uint8_t msg_type;
    uint8_t msg_qos;
    uint16_t msg_id;
    uint32_t transport_message_offset = 0 ;

    read_len = esp_transport_read(client->transport, (char *)client->mqtt_state.in_buffer, client->mqtt_state.in_buffer_length, 0);

    if (read_len < 0) {
        ESP_LOGE(TAG, "Read error or end of stream");
        return ESP_FAIL;
    }

    if (read_len == 0) {
        return ESP_OK;
    }

    // In case of fragmented packet (when receiving a large publish message), the deliver_publish function will read the rest of the message with more transport read, which means the MQTT message length will be greater than the initial transport read length. That explains that the stopping condition is not the equality here
    while ( transport_message_offset < read_len ) {
        // If the message was valid, get the type, quality of service and id of the message
        msg_type = mqtt_get_type(&client->mqtt_state.in_buffer[transport_message_offset]);
        msg_qos = mqtt_get_qos(&client->mqtt_state.in_buffer[transport_message_offset]);
        msg_id = mqtt_get_id(&client->mqtt_state.in_buffer[transport_message_offset], read_len - transport_message_offset);
        client->mqtt_state.message_length_read = read_len - transport_message_offset;
        client->mqtt_state.message_length = mqtt_get_total_length(&client->mqtt_state.in_buffer[transport_message_offset], client->mqtt_state.message_length_read);

        ESP_LOGD(TAG, "msg_type=%d, msg_id=%d", msg_type, msg_id);

        switch (msg_type)
        {
            case MQTT_MSG_TYPE_SUBACK:
                if (is_valid_mqtt_msg(client, MQTT_MSG_TYPE_SUBSCRIBE, msg_id)) {
                    ESP_LOGD(TAG, "Subscribe successful");
                    client->event.event_id = MQTT_EVENT_SUBSCRIBED;
                    esp_mqtt_dispatch_event_with_msgid(client);
                }
                break;
            case MQTT_MSG_TYPE_UNSUBACK:
                if (is_valid_mqtt_msg(client, MQTT_MSG_TYPE_UNSUBSCRIBE, msg_id)) {
                    ESP_LOGD(TAG, "UnSubscribe successful");
                    client->event.event_id = MQTT_EVENT_UNSUBSCRIBED;
                    esp_mqtt_dispatch_event_with_msgid(client);
                }
                break;
            case MQTT_MSG_TYPE_PUBLISH:
                if (msg_qos == 1) {
                    client->mqtt_state.outbound_message = mqtt_msg_puback(&client->mqtt_state.mqtt_connection, msg_id);
                }
                else if (msg_qos == 2) {
                    client->mqtt_state.outbound_message = mqtt_msg_pubrec(&client->mqtt_state.mqtt_connection, msg_id);
                }

                if (msg_qos == 1 || msg_qos == 2) {
                    ESP_LOGD(TAG, "Queue response QoS: %d", msg_qos);

                    if (mqtt_write_data(client) != ESP_OK) {
                        ESP_LOGE(TAG, "Error write qos msg repsonse, qos = %d", msg_qos);
                        // TODO: Shoule reconnect?
                        // return ESP_FAIL;
                    }
                }
                // Deliver the publish message
                ESP_LOGD(TAG, "deliver_publish, message_length_read=%d, message_length=%d", client->mqtt_state.message_length_read, client->mqtt_state.message_length);
                deliver_publish(client, &client->mqtt_state.in_buffer[transport_message_offset], client->mqtt_state.message_length_read);
                break;
            case MQTT_MSG_TYPE_PUBACK:
                if (is_valid_mqtt_msg(client, MQTT_MSG_TYPE_PUBLISH, msg_id)) {
                    ESP_LOGD(TAG, "received MQTT_MSG_TYPE_PUBACK, finish QoS1 publish");
                    outbox_set_pending(client->outbox, msg_id, CONFIRMED);
                    client->event.event_id = MQTT_EVENT_PUBLISHED;
                    esp_mqtt_dispatch_event_with_msgid(client);
                }
                break;
            case MQTT_MSG_TYPE_PUBREC:
                ESP_LOGD(TAG, "received MQTT_MSG_TYPE_PUBREC");
                client->mqtt_state.outbound_message = mqtt_msg_pubrel(&client->mqtt_state.mqtt_connection, msg_id);
                outbox_set_pending(client->outbox, msg_id, CONFIRMED);
                mqtt_write_data(client);
                break;
            case MQTT_MSG_TYPE_PUBREL:
                ESP_LOGD(TAG, "received MQTT_MSG_TYPE_PUBREL");
                client->mqtt_state.outbound_message = mqtt_msg_pubcomp(&client->mqtt_state.mqtt_connection, msg_id);
                mqtt_write_data(client);
                break;
            case MQTT_MSG_TYPE_PUBCOMP:
                ESP_LOGD(TAG, "received MQTT_MSG_TYPE_PUBCOMP");
                if (is_valid_mqtt_msg(client, MQTT_MSG_TYPE_PUBLISH, msg_id)) {
                    ESP_LOGD(TAG, "Receive MQTT_MSG_TYPE_PUBCOMP, finish QoS2 publish");
                    client->event.event_id = MQTT_EVENT_PUBLISHED;
                    esp_mqtt_dispatch_event_with_msgid(client);
                }
                break;
            case MQTT_MSG_TYPE_PINGRESP:
                ESP_LOGD(TAG, "MQTT_MSG_TYPE_PINGRESP");
                client->wait_for_ping_resp = false;
                break;
        }

        transport_message_offset += client->mqtt_state.message_length;
    }

    return ESP_OK;
}

static esp_err_t mqtt_resend_queued(esp_mqtt_client_handle_t client, outbox_item_handle_t item)
{
    // decode queued data
    client->mqtt_state.outbound_message->data = outbox_item_get_data(item, &client->mqtt_state.outbound_message->length, &client->mqtt_state.pending_msg_id,
                                                                        &client->mqtt_state.pending_msg_type, &client->mqtt_state.pending_publish_qos);
    // set duplicate flag for QoS-2 message
    if (client->mqtt_state.pending_msg_type == MQTT_MSG_TYPE_PUBLISH &&client->mqtt_state.pending_publish_qos==2) {
        mqtt_set_dup(client->mqtt_state.outbound_message->data);
    }

    // try to resend the data
    if (mqtt_write_data(client) != ESP_OK) {
        ESP_LOGE(TAG, "Error to public data ");
        esp_mqtt_abort_connection(client);
        return ESP_FAIL;
    }
    return ESP_OK;
}

static void esp_mqtt_task(void *pv)
{
    esp_mqtt_client_handle_t client = (esp_mqtt_client_handle_t) pv;
    uint32_t last_retransmit = 0;
    client->run = true;

    //get transport by scheme
    client->transport = esp_transport_list_get_transport(client->transport_list, client->config->scheme);

    if (client->transport == NULL) {
        ESP_LOGE(TAG, "There are no transports valid, stop mqtt client, config scheme = %s", client->config->scheme);
        client->run = false;
    }
    //default port
    if (client->config->port == 0) {
        client->config->port = esp_transport_get_default_port(client->transport);
    }

    client->state = MQTT_STATE_INIT;
    xEventGroupClearBits(client->status_bits, STOPPED_BIT);
    while (client->run) {
        MQTT_API_LOCK(client);
        switch ((int)client->state) {
            case MQTT_STATE_INIT:
                xEventGroupClearBits(client->status_bits, RECONNECT_BIT);
                client->event.event_id = MQTT_EVENT_BEFORE_CONNECT;
                esp_mqtt_dispatch_event_with_msgid(client);

                if (client->transport == NULL) {
                    ESP_LOGE(TAG, "There are no transport");
                    client->run = false;
                }

                if (esp_transport_connect(client->transport,
                                      client->config->host,
                                      client->config->port,
                                      client->config->network_timeout_ms) < 0) {
                    ESP_LOGE(TAG, "Error transport connect");
                    esp_mqtt_abort_connection(client);
                    break;
                }
                ESP_LOGD(TAG, "Transport connected to %s://%s:%d", client->config->scheme, client->config->host, client->config->port);
                if (esp_mqtt_connect(client, client->config->network_timeout_ms) != ESP_OK) {
                    ESP_LOGI(TAG, "Error MQTT Connected");
                    esp_mqtt_abort_connection(client);
                    break;
                }
                client->event.event_id = MQTT_EVENT_CONNECTED;
                client->event.session_present = mqtt_get_connect_session_present(client->mqtt_state.in_buffer);
                client->state = MQTT_STATE_CONNECTED;
                esp_mqtt_dispatch_event_with_msgid(client);
                client->refresh_connection_tick = platform_tick_get_ms();

                break;
            case MQTT_STATE_CONNECTED:
                // receive and process data
                if (mqtt_process_receive(client) == ESP_FAIL) {
                    esp_mqtt_abort_connection(client);
                    break;
                }

                // resend all non-transmitted messages first
                outbox_item_handle_t item = outbox_dequeue(client->outbox, QUEUED);
                if (item) {
                    if (mqtt_resend_queued(client, item) == ESP_OK) {
                        outbox_set_pending(client->outbox, client->mqtt_state.pending_msg_id, TRANSMITTED);
                    }
                // resend other "transmitted" messages after 1s
                } else if (platform_tick_get_ms() - last_retransmit > 1000) {
                    last_retransmit = platform_tick_get_ms();
                    item = outbox_dequeue(client->outbox, TRANSMITTED);
                    if (item) {
                        mqtt_resend_queued(client, item);
                    }
                }

                if (platform_tick_get_ms() - client->keepalive_tick > client->connect_info.keepalive * 1000 / 2) {
                    //No ping resp from last ping => Disconnected
                	if(client->wait_for_ping_resp){
                    	ESP_LOGE(TAG, "No PING_RESP, disconnected");
                    	esp_mqtt_abort_connection(client);
                    	client->wait_for_ping_resp = false;
                    	break;
                    }
                	if (esp_mqtt_client_ping(client) == ESP_FAIL) {
                        ESP_LOGE(TAG, "Can't send ping, disconnected");
                        esp_mqtt_abort_connection(client);
                        break;
                    } else {
                    	client->wait_for_ping_resp = true;
                    }
                	ESP_LOGD(TAG, "PING sent");
                }

                if (client->config->refresh_connection_after_ms &&
                    platform_tick_get_ms() - client->refresh_connection_tick > client->config->refresh_connection_after_ms) {
                    ESP_LOGD(TAG, "Refreshing the connection...");
                    esp_mqtt_abort_connection(client);
                    client->state = MQTT_STATE_INIT;
                }

                //Delete mesaage after 30 senconds
                outbox_delete_expired(client->outbox, platform_tick_get_ms(), OUTBOX_EXPIRED_TIMEOUT_MS);
                //
                outbox_cleanup(client->outbox, OUTBOX_MAX_SIZE);
                break;
            case MQTT_STATE_WAIT_TIMEOUT:

                if (!client->config->auto_reconnect) {
                    client->run = false;
                    break;
                }
                if (platform_tick_get_ms() - client->reconnect_tick > client->wait_timeout_ms) {
                    client->state = MQTT_STATE_INIT;
                    client->reconnect_tick = platform_tick_get_ms();
                    ESP_LOGD(TAG, "Reconnecting...");
                    break;
                }
                MQTT_API_UNLOCK(client);
                xEventGroupWaitBits(client->status_bits, RECONNECT_BIT, false, true,
                                    client->wait_timeout_ms / 2 / portTICK_RATE_MS);
                // continue the while loop insted of break, as the mutex is unlocked
                continue;
        }
        MQTT_API_UNLOCK(client);
        if (MQTT_STATE_CONNECTED == client->state) {
            if (esp_transport_poll_read(client->transport, MQTT_POLL_READ_TIMEOUT_MS) < 0) {
                ESP_LOGE(TAG, "Poll read error: %d, aborting connection", errno);
                esp_mqtt_abort_connection(client);
            }
        }

    }
    esp_transport_close(client->transport);
    xEventGroupSetBits(client->status_bits, STOPPED_BIT);

    vTaskDelete(NULL);
}

esp_err_t esp_mqtt_client_start(esp_mqtt_client_handle_t client)
{
    if (client->state >= MQTT_STATE_INIT) {
        ESP_LOGE(TAG, "Client has started");
        return ESP_FAIL;
    }
#if MQTT_CORE_SELECTION_ENABLED
        ESP_LOGD(TAG, "Core selection enabled on %u", MQTT_TASK_CORE);
        if (xTaskCreatePinnedToCore(esp_mqtt_task, "mqtt_task", client->config->task_stack, client, client->config->task_prio, &client->task_handle, MQTT_TASK_CORE) != pdTRUE) {
            ESP_LOGE(TAG, "Error create mqtt task");
            return ESP_FAIL;
        }
#else
        ESP_LOGD(TAG, "Core selection disabled");
        if (xTaskCreate(esp_mqtt_task, "mqtt_task", client->config->task_stack, client, client->config->task_prio, &client->task_handle) != pdTRUE) {
            ESP_LOGE(TAG, "Error create mqtt task");
            return ESP_FAIL;
        }
#endif
    return ESP_OK;
}

esp_err_t esp_mqtt_client_reconnect(esp_mqtt_client_handle_t client)
{
    ESP_LOGI(TAG, "Client force reconnect requested");
    if (client->state != MQTT_STATE_WAIT_TIMEOUT) {
        ESP_LOGD(TAG, "The client is not waiting for reconnection. Ignore the request");
        return ESP_FAIL;
    }
    client->wait_timeout_ms = 0;
    xEventGroupSetBits(client->status_bits, RECONNECT_BIT);
    return ESP_OK;
}

esp_err_t esp_mqtt_client_stop(esp_mqtt_client_handle_t client)
{
    if (client->run) {
        client->run = false;
        xEventGroupWaitBits(client->status_bits, STOPPED_BIT, false, true, portMAX_DELAY);
        client->state = MQTT_STATE_UNKNOWN;
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "Client asked to stop, but was not started");
        return ESP_FAIL;
    }
}

static esp_err_t esp_mqtt_client_ping(esp_mqtt_client_handle_t client)
{
    client->mqtt_state.outbound_message = mqtt_msg_pingreq(&client->mqtt_state.mqtt_connection);

    if (mqtt_write_data(client) != ESP_OK) {
        ESP_LOGE(TAG, "Error sending ping");
        return ESP_FAIL;
    }
    ESP_LOGD(TAG, "Sent PING successful");
    return ESP_OK;
}

int esp_mqtt_client_subscribe(esp_mqtt_client_handle_t client, const char *topic, int qos)
{
    if (client->state != MQTT_STATE_CONNECTED) {
        ESP_LOGE(TAG, "Client has not connected");
        return -1;
    }
    MQTT_API_LOCK_FROM_OTHER_TASK(client);
    client->mqtt_state.outbound_message = mqtt_msg_subscribe(&client->mqtt_state.mqtt_connection,
                                          topic, qos,
                                          &client->mqtt_state.pending_msg_id);

    client->mqtt_state.pending_msg_type = mqtt_get_type(client->mqtt_state.outbound_message->data);
    client->mqtt_state.pending_msg_count ++;
    mqtt_enqueue(client); //move pending msg to outbox (if have)

    if (mqtt_write_data(client) != ESP_OK) {
        ESP_LOGE(TAG, "Error to subscribe topic=%s, qos=%d", topic, qos);
        MQTT_API_UNLOCK_FROM_OTHER_TASK(client);
        return -1;
    }

    ESP_LOGD(TAG, "Sent subscribe topic=%s, id: %d, type=%d successful", topic, client->mqtt_state.pending_msg_id, client->mqtt_state.pending_msg_type);
    MQTT_API_UNLOCK_FROM_OTHER_TASK(client);
    return client->mqtt_state.pending_msg_id;
}

int esp_mqtt_client_unsubscribe(esp_mqtt_client_handle_t client, const char *topic)
{
    if (client->state != MQTT_STATE_CONNECTED) {
        ESP_LOGE(TAG, "Client has not connected");
        return -1;
    }
    MQTT_API_LOCK_FROM_OTHER_TASK(client);
    client->mqtt_state.outbound_message = mqtt_msg_unsubscribe(&client->mqtt_state.mqtt_connection,
                                          topic,
                                          &client->mqtt_state.pending_msg_id);
    ESP_LOGD(TAG, "unsubscribe, topic\"%s\", id: %d", topic, client->mqtt_state.pending_msg_id);

    client->mqtt_state.pending_msg_type = mqtt_get_type(client->mqtt_state.outbound_message->data);
    client->mqtt_state.pending_msg_count ++;
    mqtt_enqueue(client);

    if (mqtt_write_data(client) != ESP_OK) {
        ESP_LOGE(TAG, "Error to unsubscribe topic=%s", topic);
        MQTT_API_UNLOCK_FROM_OTHER_TASK(client);
        return -1;
    }

    ESP_LOGD(TAG, "Sent Unsubscribe topic=%s, id: %d, successful", topic, client->mqtt_state.pending_msg_id);
    MQTT_API_UNLOCK_FROM_OTHER_TASK(client);
    return client->mqtt_state.pending_msg_id;
}

int esp_mqtt_client_publish(esp_mqtt_client_handle_t client, const char *topic, const char *data, int len, int qos, int retain)
{
    uint16_t pending_msg_id = 0;

    if (len <= 0) {
        len = strlen(data);
    }

    MQTT_API_LOCK_FROM_OTHER_TASK(client);
    mqtt_message_t *publish_msg = mqtt_msg_publish(&client->mqtt_state.mqtt_connection,
                                  topic, data, len,
                                  qos, retain,
                                  &pending_msg_id);

    /* We have to set as pending all the qos>0 messages) */
    if (qos > 0) {
        client->mqtt_state.outbound_message = publish_msg;
        client->mqtt_state.pending_msg_type = mqtt_get_type(client->mqtt_state.outbound_message->data);
        client->mqtt_state.pending_msg_id = pending_msg_id;
        client->mqtt_state.pending_publish_qos = qos;
        client->mqtt_state.pending_msg_count ++;
        // by default store as QUEUED (not transmitted yet)
        mqtt_enqueue(client);
    } else {
        client->mqtt_state.outbound_message = publish_msg;
    }

    /* Skip sending if not connected (rely on resending) */
    if (client->state != MQTT_STATE_CONNECTED) {
        ESP_LOGD(TAG, "Publish: client is not connected");
        goto cannot_publish;
    }

    /* Provide support for sending fragmented message if it doesn't fit buffer */
    int remaining_len = len;
    const char *current_data = data;
    bool sending = true;

    while (sending)  {

        if (mqtt_write_data(client) != ESP_OK) {
            esp_mqtt_abort_connection(client);
            goto cannot_publish;
        }

        int data_sent = client->mqtt_state.outbound_message->length - client->mqtt_state.outbound_message->fragmented_msg_data_offset;
        remaining_len -= data_sent;
        current_data +=  data_sent;

        if (remaining_len > 0) {
            mqtt_connection_t* connection = &client->mqtt_state.mqtt_connection;
            ESP_LOGD(TAG, "Sending fragmented message, remains to send %d bytes of %d", remaining_len, len);
            if (connection->message.fragmented_msg_data_offset) {
                // asked to enqueue oversized message (first time only)
                connection->message.fragmented_msg_data_offset = 0;
                connection->message.fragmented_msg_total_length = 0;
                if (qos > 0) {
                    // internally enqueue all big messages, as they dont fit 'pending msg' structure
                    mqtt_enqueue_oversized(client, (uint8_t*)current_data, remaining_len);
                }
            }

            if (remaining_len > connection->buffer_length) {
                // Continue with sending
                memcpy(connection->buffer, current_data, connection->buffer_length);
                connection->message.length = connection->buffer_length;
                sending = true;
            } else {
                memcpy(connection->buffer, current_data, remaining_len);
                connection->message.length = remaining_len;
                sending = true;
            }
            connection->message.data = connection->buffer;
            client->mqtt_state.outbound_message = &connection->message;
        } else {
            // Message was sent correctly
            sending = false;
        }
    }

    if (qos > 0) {
        outbox_set_pending(client->outbox, pending_msg_id, TRANSMITTED);
    }
    MQTT_API_UNLOCK_FROM_OTHER_TASK(client);
    return pending_msg_id;

cannot_publish:
    if (qos == 0) {
        ESP_LOGW(TAG, "Publish: Loosing qos0 data when client not connected");
    }
    MQTT_API_UNLOCK_FROM_OTHER_TASK(client);
    return 0;
}



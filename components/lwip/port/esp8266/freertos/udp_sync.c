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

#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <sys/errno.h>
#include "lwip/udp.h"
#include "lwip/priv/api_msg.h"
#include "lwip/priv/tcp_priv.h"

#include "esp_log.h"

#if ESP_UDP && LWIP_NETIF_TX_SINGLE_PBUF

#define UDP_SYNC_MAX MEMP_NUM_NETCONN
#define UDP_SYNC_RETRY_MAX CONFIG_ESP_UDP_SYNC_RETRY_MAX

/*
 * All function has no mutex, so they must put into one task(LWIP main task).
 */

#if LWIP_TCPIP_CORE_LOCKING
#define TCPIP_APIMSG_ACK(m)   NETCONN_SET_SAFE_ERR((m)->conn, (m)->err)
#else /* LWIP_TCPIP_CORE_LOCKING */
#define TCPIP_APIMSG_ACK(m)   do { NETCONN_SET_SAFE_ERR((m)->conn, (m)->err); sys_sem_signal(LWIP_API_MSG_SEM(m)); } while(0)
#endif /* LWIP_TCPIP_CORE_LOCKING */

typedef struct udp_sync {
    struct api_msg      *msg;

    struct netif        *netif;

    int8_t              ret;

    uint8_t             retry;
} udp_sync_t;

static const char *TAG = "udp_sync";
static size_t s_udp_sync_num;
static udp_sync_t s_udp_sync[UDP_SYNC_MAX];
static struct api_msg *s_cur_msg;

/*
 * @brief initialize UDP sync module
 */
void udp_sync_init(void)
{
    memset(s_udp_sync, 0, sizeof(s_udp_sync));
    s_udp_sync_num = 0;
}

/*
 * @brief register a UDP API message(struct api_msg) to module
 */
void udp_sync_regitser(void *in_msg)
{
    s_cur_msg = in_msg;

    struct api_msg *msg = (struct api_msg *)in_msg;
    int s = msg->conn->socket;

    if (s < 0 || s >= UDP_SYNC_MAX) {
        ESP_LOGE(TAG, "UDP sync register error, socket is %d", s);
    } else if (s_udp_sync[s].msg) {
        ESP_LOGE(TAG, "UDP sync register error, msg is %p", s_udp_sync[s].msg);
    }

    s_udp_sync_num++;
    s_udp_sync[s].ret = ERR_OK;
    s_udp_sync[s].retry = 0;
    s_udp_sync[s].msg = msg;
}

static void _udp_sync_ack_ret(int s, struct api_msg *msg)
{
    /* Only cache when low-level has no buffer to send packet */
    if (s_udp_sync[s].ret != ERR_MEM || s_udp_sync[s].retry >= UDP_SYNC_RETRY_MAX) {

        ESP_LOGD(TAG, "UDP sync ret %d retry %d", s_udp_sync[s].ret, s_udp_sync[s].retry);

        s_udp_sync[s].msg = NULL;
        s_udp_sync[s].retry = 0;
        s_udp_sync[s].ret = ERR_OK;
        s_udp_sync_num--;

        TCPIP_APIMSG_ACK(msg);
    } else {
        s_udp_sync[s].retry++;
        ESP_LOGD(TAG, "UDP sync ack error, errno %d", s_udp_sync[s].ret);
    }
}

/*
 * @brief ack the message
 */
void udp_sync_ack(void *in_msg)
{
    struct api_msg *msg = (struct api_msg *)in_msg;
    int s = msg->conn->socket;

    if (s < 0 || s >= UDP_SYNC_MAX) {
        ESP_LOGE(TAG, "UDP sync ack error, socket is %d", s);
    } else if (!s_udp_sync[s].msg) {
        ESP_LOGE(TAG, "UDP sync ack error, msg is NULL");
    }

    _udp_sync_ack_ret(s, msg);

    s_cur_msg = NULL;
}

/*
 * @brief set the current message send result
 */
void udp_sync_set_ret(void *netif, int ret)
{
    /* Only poll and regitser can set current message */
    if (!s_cur_msg || !sys_current_task_is_tcpip()) {
        /* You may use it to debug */
        //ESP_LOGE(TAG, "UDP sync ack error, current message is %p, task name is %s", s_cur_msg, sys_current_task_name());
        return ;
    }

    struct api_msg *msg = s_cur_msg;
    int s = msg->conn->socket;

    if (s < 0 || s >= UDP_SYNC_MAX) {
        ESP_LOGE(TAG, "UDP sync ack error, socket is %d", s);
    } else if (!s_udp_sync[s].msg) {
        ESP_LOGE(TAG, "UDP sync ack error, msg is NULL");
    }

    s_udp_sync[s].netif = netif;
    s_udp_sync[s].ret = ret;
}

static void udp_sync_send(struct api_msg *msg)
{
    struct pbuf *p = msg->msg.b->p;
    int s = msg->conn->socket;
    struct netif *netif = s_udp_sync[s].netif;

    s_cur_msg = msg;

    netif->linkoutput(netif, p);
    _udp_sync_ack_ret(s, msg);

    s_cur_msg = NULL;
}

/*
 * @brief process the sync
 */
void udp_sync_proc(void)
{
    if (!s_udp_sync_num)
        return ;

    for (int i = 0; i < UDP_SYNC_MAX; i++) {
        if (!s_udp_sync[i].msg)
            continue;

        udp_sync_send(s_udp_sync[i].msg);

        if (s_udp_sync[i].ret == ERR_MEM)
            break;
    }
}

/*
 * @brief NULL function and just as sync message
 */
static void udp_sync_trigger_null(void *p)
{

}

/*
 * @brief trigger a UDP sync process
 */
void udp_sync_trigger(void)
{
    if (!s_udp_sync_num)
        return ;

    tcpip_callback_with_block((tcpip_callback_fn)udp_sync_trigger_null, NULL, 0);
}

#endif /* ESP_UDP */

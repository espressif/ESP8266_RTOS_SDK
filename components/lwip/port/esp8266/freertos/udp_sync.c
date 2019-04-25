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

#include "lwipopts.h"

#if ESP_UDP

#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <sys/errno.h>
#include "lwip/udp.h"
#include "lwip/priv/api_msg.h"
#include "lwip/priv/tcp_priv.h"

//#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include "esp_log.h"

#define UDP_SYNC_SOCK_RETRY_MAX CONFIG_ESP_UDP_SYNC_RETRY_MAX

#define UDP_SYNC_UDP_RETRY_MAX 64

/*
 * All function has no mutex, so they must put into one task(LWIP main task).
 */

#if LWIP_TCPIP_CORE_LOCKING
#define TCPIP_APIMSG_ACK(m)   NETCONN_SET_SAFE_ERR((m)->conn, (m)->err)
#else /* LWIP_TCPIP_CORE_LOCKING */
#define TCPIP_APIMSG_ACK(m)   do { NETCONN_SET_SAFE_ERR((m)->conn, (m)->err); sys_sem_signal(LWIP_API_MSG_SEM(m)); } while(0)
#endif /* LWIP_TCPIP_CORE_LOCKING */

#define UDP_SYNC_NONE 0
#define UDP_SYNC_SOCK 1
#define UDP_SYNC_UDP 2

typedef struct udp_sync_method {
    uint8_t             type;

    void (*free) (void *p);
} udp_sync_method_t;

typedef struct udp_sync_udp {
    udp_sync_method_t   method;

    int8_t              ret;

    uint8_t             retry;

    struct pbuf         *pbuf;

    struct netif        *netif;

    struct udp_pcb      *pcb;
} udp_sync_udp_t;

typedef struct udp_sync_sock {
    udp_sync_method_t   method;

    int8_t              ret;

    uint8_t             retry;

    struct api_msg      *msg;

    struct netif        *netif;
} udp_sync_sock_t;

typedef struct udp_sync_netconn {
    struct tcpip_api_call_data call;

    struct netconn  *conn;
} udp_sync_netconn_t;

static const char *TAG = "udp_sync";
static void *s_cur_msg;
static size_t s_msg_type;
static size_t s_udp_sync_num;

static inline int _udp_need_proc(struct udp_pcb *pcb)
{
    return pcb->cb != NULL;
}

static inline int _udp_do_proc(struct udp_pcb *pcb, int force)
{
    return pcb->cb(pcb->arg, force);
}

static inline void *_udp_priv_data(struct udp_pcb *pcb)
{
    return pcb->arg;
}

static inline void _udp_end_proc(struct udp_pcb *pcb)
{
    pcb->cb = NULL;
    pcb->arg = NULL;
    s_udp_sync_num--;
}

static inline void _udp_add_proc(struct udp_pcb *pcb, udp_cb_fn cb, void *arg)
{
    s_udp_sync_num++;
    pcb->cb = cb;
    pcb->arg = arg;
}

static inline struct udp_pcb *_get_msg_pcb(struct api_msg *msg)
{
    return msg->conn->pcb.udp;
}

static void _udp_sync_do_meth_free(udp_sync_method_t *meth)
{
    meth->free(meth);
}

static int _udp_sync_ack_sock_ret(struct api_msg *msg, int force)
{
    int ret;
    struct udp_pcb *pcb = msg->conn->pcb.udp;
    udp_sync_sock_t *udp_sync_sock = _udp_priv_data(pcb);

    /* Only cache when low-level has no buffer to send packet */
    if (force || !udp_sync_sock || udp_sync_sock->ret != ERR_MEM || udp_sync_sock->retry >= UDP_SYNC_SOCK_RETRY_MAX) {

        if (udp_sync_sock) {
            ESP_LOGD(TAG, "UDP sync sock ret %d retry %d", udp_sync_sock->ret, udp_sync_sock->retry);
        }

        TCPIP_APIMSG_ACK(msg);

        ret = ERR_OK;
    } else {
        udp_sync_sock->retry++;
        ESP_LOGD(TAG, "UDP sync sock ack error, errno %d", udp_sync_sock->ret);

        ret = ERR_INPROGRESS;
    }

    return ret;
}

static void _udp_sync_meth_sock_free(void *p)
{
    heap_caps_free(p);
}

static void _udp_sync_meth_udp_free(void *p)
{
    udp_sync_udp_t *udp_sync_udp = (udp_sync_udp_t *)p;

    pbuf_free(udp_sync_udp->pbuf);
    heap_caps_free(udp_sync_udp);
}

static int sock_udp_sync_sock_cb(void *p, int force)
{
    int ret;
    udp_sync_sock_t *udp_sync_sock = (udp_sync_sock_t *)p;
    struct api_msg *msg = udp_sync_sock->msg;
    struct netif *netif = udp_sync_sock->netif;
    struct pbuf *pbuf = msg->msg.b->p;

    s_cur_msg = msg;
    s_msg_type = UDP_SYNC_SOCK;

    netif->linkoutput(netif, pbuf);
    ret = _udp_sync_ack_sock_ret(msg, force);

    s_msg_type = UDP_SYNC_NONE;
    s_cur_msg = NULL;

    return ret;
}

static int sock_udp_sync_udp_cb(void *p, int force)
{
    int ret;
    udp_sync_udp_t *udp_sync_udp = (udp_sync_udp_t *)p;
    struct netif *netif = udp_sync_udp->netif;
    struct pbuf *pbuf = udp_sync_udp->pbuf;
    struct udp_pcb *pcb = udp_sync_udp->pcb;

    s_cur_msg = pcb;
    s_msg_type = UDP_SYNC_UDP;

    netif->linkoutput(netif, pbuf);

    s_msg_type = UDP_SYNC_NONE;
    s_cur_msg = NULL;

    if (force || udp_sync_udp->ret != ERR_MEM || udp_sync_udp->retry >= UDP_SYNC_UDP_RETRY_MAX) {
        if (udp_sync_udp) {
            ESP_LOGD(TAG, "UDP sync sync ret %d retry %d", udp_sync_udp->ret, udp_sync_udp->retry);
        }
        ret = ERR_OK;
    } else {
        udp_sync_udp->retry++;
        ESP_LOGD(TAG, "UDP sync udp send error, errno %d", udp_sync_udp->ret);
        ret = ERR_INPROGRESS;
    }

    return ret;
}

/*
 * @brief NULL function and just as sync message
 */
static void udp_sync_trigger_null(void *p)
{

}

static err_t udp_sync_do_close_netconn(struct tcpip_api_call_data *call)
{
    udp_sync_netconn_t *sync = (udp_sync_netconn_t *)call;
    struct netconn *conn = sync->conn;
    struct udp_pcb *pcb = conn->pcb.udp;

    udp_sync_close_udp(pcb);

    return ERR_OK;
}

/*
 * @brief ack the message
 */
void udp_sync_regitser_sock(void *in_msg)
{
    s_cur_msg = in_msg;
    s_msg_type = UDP_SYNC_SOCK;

    ESP_LOGD(TAG, "UDP sync regitser sock msg %p", in_msg);
}

/*
 * @brief ack the message
 */
void udp_sync_ack_sock(void *in_msg)
{
    int ret;
    struct api_msg *msg = (struct api_msg *)in_msg;
    struct udp_pcb *pcb = _get_msg_pcb(msg);

    if (UDP_SYNC_NONE == s_msg_type) {
        TCPIP_APIMSG_ACK(msg);
        return ;
    }

    ret = _udp_sync_ack_sock_ret(msg, 0);
    if (ret == ERR_OK && _udp_need_proc(pcb)) {
        udp_sync_method_t *method = (udp_sync_method_t *)_udp_priv_data(pcb);

        _udp_sync_do_meth_free(method);
        _udp_end_proc(pcb);
    }

    s_msg_type = UDP_SYNC_NONE;
    s_cur_msg = NULL;

    ESP_LOGD(TAG, "UDP sync ack msg %p", msg);
}

/*
 * @brief set the current message send result
 */
void udp_sync_set_ret(void *netif, void *in_pbuf, int ret)
{
    struct udp_pcb *pcb;
    struct api_msg *msg;
    udp_sync_sock_t *udp_sync_sock;
    udp_sync_udp_t *udp_sync_udp;
    struct pbuf *pbuf;

    /* Only poll and regitser can set current message */
    if (!s_cur_msg || !sys_current_task_is_tcpip()) {
        /* You may use it to debug */
        //ESP_LOGE(TAG, "UDP sync ack error, current message is %p, task name is %s", s_cur_msg, sys_current_task_name());
        return ;
    }

    switch (s_msg_type) {
        case UDP_SYNC_SOCK:
            msg = s_cur_msg;
            pcb = _get_msg_pcb(msg);
            udp_sync_sock = _udp_priv_data(pcb);

            if (udp_sync_sock) {
                udp_sync_sock->ret = ret;
                ESP_LOGD(TAG, "UDP sync set1 port %d ret %d netif %p", pcb->local_port, ret, netif);
                return ;
            } else {
                if (ERR_OK == ret) {
                    return ;
                }
            }

            udp_sync_sock = heap_caps_malloc(sizeof(udp_sync_sock_t), MALLOC_CAP_8BIT);
            if (!udp_sync_sock) {
                ESP_LOGE(TAG, "UDP sync sock regitser error MEM_ERR");
                return ;
            }

            udp_sync_sock->method.free = _udp_sync_meth_sock_free;
            udp_sync_sock->method.type = UDP_SYNC_SOCK;

            udp_sync_sock->msg = msg;
            udp_sync_sock->retry = 0;
            udp_sync_sock->netif = netif;
            udp_sync_sock->ret = ret;

            _udp_add_proc(pcb, sock_udp_sync_sock_cb, udp_sync_sock);

            ESP_LOGD(TAG, "UDP sync set2 port %d ret %d netif %p", pcb->local_port, ret, netif);
            break;
        case UDP_SYNC_UDP:
            pcb = s_cur_msg;
            udp_sync_udp = _udp_priv_data(pcb);

            if (udp_sync_udp) {
                udp_sync_udp->ret = ret;
                ESP_LOGD(TAG, "UDP sync set3 port %d ret %d netif %p", pcb->local_port, ret, netif);
                return ;
            } else {
                if (ERR_OK == ret)
                    return ;
            }

            udp_sync_udp = heap_caps_malloc(sizeof(udp_sync_udp_t), MALLOC_CAP_8BIT);
            if (!udp_sync_udp) {
                ESP_LOGE(TAG, "UDP sync udp regitser error MEM_ERR");
                return ;
            }

            pbuf = (struct pbuf *)in_pbuf;
            pbuf_ref(pbuf);

            udp_sync_udp->method.free = _udp_sync_meth_udp_free;
            udp_sync_udp->method.type = UDP_SYNC_UDP;

            udp_sync_udp->pbuf = pbuf;
            udp_sync_udp->retry = 0;
            udp_sync_udp->netif = netif;
            udp_sync_udp->pcb = pcb;
            udp_sync_udp->ret = ret;

            _udp_add_proc(pcb, sock_udp_sync_udp_cb, udp_sync_udp);

            ESP_LOGD(TAG, "UDP sync set4 port %d ret %d netif %p", pcb->local_port, ret, netif);
            break;
        default:
            break;
    }
}

void udp_sync_cache_udp(void *pcb)
{
    if (s_cur_msg || !sys_current_task_is_tcpip())
        return ;

    s_msg_type = UDP_SYNC_UDP;
    s_cur_msg = pcb;

    ESP_LOGD(TAG, "UDP sync cache udp %p", pcb);
}

void udp_sync_clear_udp(void)
{
    if (s_msg_type != UDP_SYNC_UDP || !sys_current_task_is_tcpip())
        return ;

    struct udp_pcb *pcb = (struct udp_pcb *)s_cur_msg;

    s_msg_type = UDP_SYNC_NONE;
    s_cur_msg = NULL;

    ESP_LOGD(TAG, "UDP sync clear udp %p", pcb);
}

/*
 * @brief trigger a UDP sync process
 */
void udp_sync_trigger(void)
{
    if (!s_udp_sync_num)
        return ;

    tcpip_callback_with_block(udp_sync_trigger_null, NULL, 0);
}

/**
 * @brief close the udp sync before close the socket
 */ 
void udp_sync_close_udp(void *udp_pcb)
{
    struct udp_pcb *pcb = udp_pcb;

    ESP_LOGD(TAG, "UDP sync close port %d", pcb->local_port);

    if (_udp_need_proc(pcb)) {
        udp_sync_method_t *method = (udp_sync_method_t *)_udp_priv_data(pcb);

        _udp_do_proc(pcb, 1);
        _udp_sync_do_meth_free(method);
        _udp_end_proc(pcb);
        ESP_LOGD(TAG, "UDP sync free proc port %d", pcb->local_port);
    }
}

/**
 * @brief close the udp sync netconn before close the socket
 */ 
void udp_sync_close_netconn(void *netconn)
{
    udp_sync_netconn_t sync;
    struct netconn *conn = netconn;

    if (conn->type != NETCONN_UDP)
        return ;

    sync.conn = netconn;

    tcpip_api_call(udp_sync_do_close_netconn, &sync.call);
}

/*
 * @brief process the sync
 */
void udp_sync_proc(void)
{
    struct udp_pcb *pcb;

    if (!s_udp_sync_num)
        return ;

    for (pcb = udp_pcbs; pcb != NULL; pcb = pcb->next) {
        udp_sync_method_t *method;

        if (!_udp_need_proc(pcb) || _udp_do_proc(pcb, 0) != ERR_OK)
            continue;

        method = (udp_sync_method_t *)_udp_priv_data(pcb);

        _udp_sync_do_meth_free(method);
        _udp_end_proc(pcb);

        ESP_LOGD(TAG, "UDP sync end proc port %d", pcb->local_port);
    }
}

#endif /* ESP_UDP */

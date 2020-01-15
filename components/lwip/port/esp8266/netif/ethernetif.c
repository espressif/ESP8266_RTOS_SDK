// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
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

#include <string.h>

#include "lwip/pbuf.h"
#include "lwip/ethip6.h"
#include "netif/etharp.h"
#include "esp_libc.h"
#include "esp_wifi.h"
#include "esp_aio.h"
#include "tcpip_adapter.h"
#include "freertos/semphr.h"
#include "lwip/tcpip.h"
#include "stdlib.h"

#include "esp8266/eagle_soc.h"

int ieee80211_output_pbuf(esp_aio_t *aio);
int8_t wifi_get_netif(uint8_t fd);
void wifi_station_set_default_hostname(uint8_t* hwaddr);

#define IFNAME0 'e'
#define IFNAME1 'n'

#if ESP_TCP
typedef struct pbuf_send_list {
    struct pbuf_send_list* next;
    struct pbuf* p;
    int aiofd;
    int err_cnt;
} pbuf_send_list_t;

static pbuf_send_list_t* pbuf_list_head = NULL;
static int pbuf_send_list_num = 0;
#endif
static int low_level_send_cb(esp_aio_t* aio);

#if ESP_TCP_TXRX_PBUF_DEBUG
void tcp_print_status(int status, void* buf, uint32_t tmp1, uint32_t tmp2, uint32_t tmp3)
{
    struct pbuf* p = (struct pbuf*)buf;
    if (p->tot_len < 50) {
        return;
    }

    uint32_t i;

    i = *((unsigned char*)p->payload + 12);

    if (i == 0x08) { /*ipv4*/
        i = *((unsigned char*)p->payload + 13);

        if (i == 0) {
            i = *((unsigned char*)p->payload + 23);

            if (i == 0x06) { /*tcp*/
                i = *((unsigned char*)p->payload + 16);
                i <<= 8;
                i += *((unsigned char*)p->payload + 17);

                if (i >= 40) { /*tcp data*/
                    uint32_t len, seq, ack, srcport, destport, flags;
                    wifi_tx_status_t *tx_result = (wifi_tx_status_t *)(&tmp1);
                    len = i;
                    i = *((unsigned char*)p->payload + 38);
                    i <<= 8;
                    i += *((unsigned char*)p->payload + 39);
                    i <<= 8;
                    i += *((unsigned char*)p->payload + 40);
                    i <<= 8;
                    i += *((unsigned char*)p->payload + 41);
                    seq = i;
                    i = *((unsigned char*)p->payload + 42);
                    i <<= 8;
                    i += *((unsigned char*)p->payload + 43);
                    i <<= 8;
                    i += *((unsigned char*)p->payload + 44);
                    i <<= 8;
                    i += *((unsigned char*)p->payload + 45);
                    ack = i;
                    i = *((unsigned char*)p->payload + 34);
                    i <<= 8;
                    i += *((unsigned char*)p->payload + 35);
                    srcport = i;
                    i = *((unsigned char*)p->payload + 36);
                    i <<= 8;
                    i += *((unsigned char*)p->payload + 37);
                    destport = i;
                    flags = *((unsigned char *)p->payload+47);

                    switch (status) {
                        case LWIP_SEND_DATA_TO_WIFI:
                            LWIP_DEBUGF(ESP_TCP_TXRX_PBUF_DEBUG, ("@@ Tx - L:%u, S:%u, A:%u, SP:%u, DP:%u, F:%x\n", len, seq, ack, srcport, destport, flags));
                            break;

                        case LWIP_RESEND_DATA_TO_WIFI_WHEN_WIFI_SEND_FAILED:
                            LWIP_DEBUGF(ESP_TCP_TXRX_PBUF_DEBUG, ("@@ Cache Tx - L:%u, S:%u, A:%u, SP:%u, DP:%u, F:%x\n", len, seq, ack, srcport, destport, flags));
                            break;

                        case LWIP_RECV_DATA_FROM_WIFI:
                            LWIP_DEBUGF(ESP_TCP_TXRX_PBUF_DEBUG, ("@@ WiFi Rx - L:%u, S:%u, A:%u, SP:%u, DP:%u, F:%x\n", len, seq, ack, srcport, destport, flags));
                            break;

                        case LWIP_RETRY_DATA_WHEN_RECV_ACK_TIMEOUT:
                            LWIP_DEBUGF(ESP_TCP_TXRX_PBUF_DEBUG, ("@@ TCP RTY - rtime:%d, rto:%d, L:%u, S:%u, A:%u, SP:%u, DP:%u, F:%x\n", tmp1, tmp2, len, seq, ack, srcport, destport, flags));
                            return;

                        case LWIP_FETCH_DATA_AT_TCPIP_THREAD:
                            LWIP_DEBUGF(ESP_TCP_TXRX_PBUF_DEBUG, ("@@ eth Rx - L:%u, S:%u, A:%u, SP:%u, DP:%u, F:%x\n", len, seq, ack, srcport, destport, flags));
                            break;

                        case WIFI_SEND_DATA_FAILED:
                            LWIP_DEBUGF(ESP_TCP_TXRX_PBUF_DEBUG, ("@@ WiFi Tx Fail - result:%d, src:%d, lrc:%d, rate:%d, L:%u, S:%u, A:%u, SP:%u, DP:%u, F:%x\n",\
                            tx_result->wifi_tx_result, tx_result->wifi_tx_src, tx_result->wifi_tx_lrc, tx_result->wifi_tx_rate, len, seq, ack, srcport, destport, flags));
                            break;

                        default:
                            LWIP_DEBUGF(ESP_TCP_TXRX_PBUF_DEBUG, ("@@ status:%d, L:%u, S:%u, A:%u, SP:%u, DP:%u, F:%x, tmp1:%d, tmp2:%d, tmp3:%d\n",\
                            status, len, seq, ack, srcport, destport, flags, tmp1, tmp2, tmp3));
                            break;
                    }
                }
            }
        }
    }
}
#endif

#if ESP_TCP
static inline bool check_pbuf_to_insert(struct pbuf* p)
{
    uint8_t* buf = (uint8_t*)p->payload;

    /*Check if pbuf is tcp ip*/
    if (buf[12] == 0x08 && buf[13] == 0x00 && buf[23] == 0x06) {
        return true;
    }

    return false;
}

static void insert_to_list(int fd, struct pbuf* p)
{
    pbuf_send_list_t* tmp_pbuf_list1;
    pbuf_send_list_t* tmp_pbuf_list2;

    if (pbuf_send_list_num > (TCP_SND_QUEUELEN * MEMP_NUM_TCP_PCB + MEMP_NUM_TCP_PCB)) {
        return;
    }

    if (!check_pbuf_to_insert(p)) {
        return;
    }

    LWIP_DEBUGF(PBUF_CACHE_DEBUG, ("Insert %p,%d\n", p, pbuf_send_list_num));

    if (pbuf_list_head == NULL) {
        tmp_pbuf_list1 = (pbuf_send_list_t*)malloc(sizeof(pbuf_send_list_t));

        if (!tmp_pbuf_list1) {
            LWIP_DEBUGF(PBUF_CACHE_DEBUG, ("no memory malloc pbuf list error\n"));
            return;
        }

        pbuf_ref(p);
        tmp_pbuf_list1->aiofd = fd;
        tmp_pbuf_list1->p = p;
        tmp_pbuf_list1->next = NULL;
        tmp_pbuf_list1->err_cnt = 0;
        pbuf_list_head = tmp_pbuf_list1;
        pbuf_send_list_num++;
        return;
    }

    tmp_pbuf_list1 = pbuf_list_head;
    tmp_pbuf_list2 = tmp_pbuf_list1;

    while (tmp_pbuf_list1 != NULL) {
        if (tmp_pbuf_list1->p == p) {
            tmp_pbuf_list1->err_cnt ++;
            return;
        }

        tmp_pbuf_list2 = tmp_pbuf_list1;
        tmp_pbuf_list1 = tmp_pbuf_list2->next;
    }

    tmp_pbuf_list1 = (pbuf_send_list_t*)malloc(sizeof(pbuf_send_list_t));

    if (!tmp_pbuf_list1) {
        LWIP_DEBUGF(PBUF_CACHE_DEBUG, ("no memory malloc pbuf list error\n"));
        return;
    }

    pbuf_ref(p);
    tmp_pbuf_list1->aiofd = fd;
    tmp_pbuf_list1->p = p;
    tmp_pbuf_list1->next = NULL;
    tmp_pbuf_list1->err_cnt = 0;
    tmp_pbuf_list2->next = tmp_pbuf_list1;
    pbuf_send_list_num++;
}

void send_from_list()
{
    pbuf_send_list_t* tmp_pbuf_list1;

    while (pbuf_list_head != NULL) {
        if (pbuf_list_head->p->ref == 1) {
            tmp_pbuf_list1 = pbuf_list_head->next;
            LWIP_DEBUGF(PBUF_CACHE_DEBUG, ("Delete %p,%d\n", pbuf_list_head->p, pbuf_send_list_num));
            pbuf_free(pbuf_list_head->p);
            free(pbuf_list_head);
            pbuf_send_list_num--;
            pbuf_list_head = tmp_pbuf_list1;
        } else {
            esp_aio_t aio;
            esp_err_t err;
            aio.fd = (int)pbuf_list_head->aiofd;
            aio.pbuf = pbuf_list_head->p->payload;
            aio.len = pbuf_list_head->p->len;
            aio.cb = low_level_send_cb;
            aio.arg = pbuf_list_head->p;
            aio.ret = 0;

            err = ieee80211_output_pbuf(aio);

#if ESP_TCP_TXRX_PBUF_DEBUG
            tcp_print_status(LWIP_RESEND_DATA_TO_WIFI_WHEN_WIFI_SEND_FAILED, (void*)pbuf_list_head->p, 0 ,0, 0);
#endif
            tmp_pbuf_list1 = pbuf_list_head->next;

            if (err == ERR_MEM) {
                pbuf_list_head->err_cnt++;

                if (pbuf_list_head->err_cnt >= 3) {
                    LWIP_DEBUGF(PBUF_CACHE_DEBUG, ("Delete %p,%d\n", pbuf_list_head->p, pbuf_send_list_num));
                    pbuf_free(pbuf_list_head->p);
                    free(pbuf_list_head);
                    pbuf_send_list_num--;
                    pbuf_list_head = tmp_pbuf_list1;
                }

                return;
            } else if (err == ERR_OK) {
                LWIP_DEBUGF(PBUF_CACHE_DEBUG, ("Delete %p,%d\n", pbuf_list_head->p, pbuf_send_list_num));
                free(pbuf_list_head);
                pbuf_send_list_num--;
                pbuf_list_head = tmp_pbuf_list1;
            } else {
                LWIP_DEBUGF(PBUF_CACHE_DEBUG, ("Delete %p,%d\n", pbuf_list_head->p, pbuf_send_list_num));
                pbuf_free(pbuf_list_head->p);
                free(pbuf_list_head);
                pbuf_send_list_num--;
                pbuf_list_head = tmp_pbuf_list1;
            }
        }
    }
}
#endif

/**
 * In this function, the hardware should be initialized.
 * Called from ethernetif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this ethernetif
 */
static void low_level_init(struct netif* netif)
{
    if (netif == NULL) {
        LWIP_DEBUGF(NETIF_DEBUG, ("low_level_init: netif is NULL\n"));
        return;
    }

    /* set MAC hardware address length */
    netif->hwaddr_len = ETHARP_HWADDR_LEN;

    /* maximum transfer unit */
    netif->mtu = 1500;

    /* device capabilities */
    /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

#if LWIP_IGMP
    netif->flags |= NETIF_FLAG_IGMP;
#endif
    /* Do whatever else is needed to initialize interface. */

#if LWIP_IPV6_AUTOCONFIG
    netif->ip6_autoconfig_enabled = 1;
#endif /* LWIP_IPV6_AUTOCONFIG */
}

/*
 * @brief LWIP low-level AI/O sending callback function, it is to free pbuf
 *
 * @param aio AI/O control block pointer
 *
 * @return 0 meaning successs
 */
static int low_level_send_cb(esp_aio_t* aio)
{
    struct pbuf* pbuf = aio->arg;

#if ESP_TCP_TXRX_PBUF_DEBUG
    wifi_tx_status_t* status = (wifi_tx_status_t*) & (aio->ret);
    if (TX_STATUS_SUCCESS != status->wifi_tx_result) {
        uint8_t* buf = (uint8_t*)pbuf->payload;

        /*Check if pbuf is tcp ip*/
        if (buf[12] == 0x08 && buf[13] == 0x00 && buf[23] == 0x06) {
            tcp_print_status(WIFI_SEND_DATA_FAILED, (void*)pbuf, aio->ret ,0, 0);
        }
    }
#endif

#if ESP_TCP
    wifi_tx_status_t* status = (wifi_tx_status_t*) & (aio->ret);

    if ((TX_STATUS_SUCCESS != status->wifi_tx_result) && check_pbuf_to_insert(pbuf)) {
        uint8_t* buf = (uint8_t*)pbuf->payload;
        struct eth_hdr ethhdr;

        if (*(buf - 17) & 0x01) { //From DS
            memcpy(&ethhdr.dest, buf - 2, ETH_HWADDR_LEN);
            memcpy(&ethhdr.src, buf - 2 - ETH_HWADDR_LEN, ETH_HWADDR_LEN);
        } else if (*(buf - 17) & 0x02) { //To DS
            memcpy(&ethhdr.dest, buf - 2 - ETH_HWADDR_LEN - ETH_HWADDR_LEN, ETH_HWADDR_LEN);
            memcpy(&ethhdr.src, buf - 2, 6);
        } else {
            pbuf_free(pbuf);
            return 0;
        }

        memcpy(buf, &ethhdr, (ETH_HWADDR_LEN + ETH_HWADDR_LEN));
        LWIP_DEBUGF(PBUF_CACHE_DEBUG, ("Send packet fail: result:%d, LRC:%d, SRC:%d, RATE:%d",
                                       status->wifi_tx_result, status->wifi_tx_lrc, status->wifi_tx_src, status->wifi_tx_rate));
        insert_to_list(aio->fd, aio->arg);
    }
#endif

    pbuf_free(pbuf);

#if ESP_UDP
    udp_sync_trigger();
#endif

    return 0;
}

/*
 * @brief transform custom pbuf to LWIP core pbuf, LWIP may use input custom pbuf
 *        to send ARP data directly
 *
 * @param pbuf LWIP pbuf pointer
 *
 * @return LWIP pbuf pointer which it not "PBUF_FLAG_IS_CUSTOM" attribute
 */
static inline struct pbuf* ethernetif_transform_pbuf(struct pbuf* pbuf)
{
    struct pbuf* p;

    if (!(pbuf->flags & PBUF_FLAG_IS_CUSTOM) && IS_DRAM(pbuf->payload)) {
        /*
         * Add ref to pbuf to avoid it to be freed by upper layer.
         */
        pbuf_ref(pbuf);
        return pbuf;
    }

    p = pbuf_alloc(PBUF_RAW, pbuf->len, PBUF_RAM);

    if (!p) {
        return NULL;
    }

    if (IS_IRAM(p->payload)) {
        LWIP_DEBUGF(NETIF_DEBUG, ("low_level_output: data in IRAM\n"));
        pbuf_free(p);
        return NULL;
    }

    memcpy(p->payload, pbuf->payload, pbuf->len);

    /*
     * The input pbuf(named "pbuf") should not be freed, becasue it will be
     * freed by upper layer.
     *
     * The output pbuf(named "p") should not be freed either, becasue it will
     * be freed at callback function "low_level_send_cb".
     */

    return p;
}

/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an int8_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become availale since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */

static int8_t low_level_output(struct netif* netif, struct pbuf* p)
{
    esp_aio_t aio;
    int8_t err = ERR_OK;

    if (netif == NULL) {
        LWIP_DEBUGF(NETIF_DEBUG, ("low_level_output: netif is NULL\n"));
        return ERR_ARG;
    }

    if (!netif_is_up(netif)) {
        LWIP_DEBUGF(NETIF_DEBUG, ("low_level_output: netif is not up\n"));
        return ERR_RTE;
    }

#if ETH_PAD_SIZE
    pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif

    p = ethernetif_transform_pbuf(p);

    if (!p) {
        LWIP_DEBUGF(NETIF_DEBUG, ("low_level_output: lack memory\n"));
        return ERR_OK;
    }

    aio.fd = (int)netif->state;
    aio.pbuf = p->payload;
    aio.len = p->len;
    aio.cb = low_level_send_cb;
    aio.arg = p;
    aio.ret = 0;

#if ESP_TCP_TXRX_PBUF_DEBUG
    tcp_print_status(LWIP_SEND_DATA_TO_WIFI, (void*)p, 0 ,0, 0);
#endif

    /*
     * we use "SOCK_RAW" to create socket, so all input/output datas include full ethernet
     * header, meaning we should not pass target low-level address here.
     */
    err = ieee80211_output_pbuf(&aio);
#if ESP_UDP
    udp_sync_set_ret(netif, p, err);
#endif

    if (err != ERR_OK) {
        if (err == ERR_MEM) {
#if ESP_TCP
            insert_to_list(aio.fd, p);
#endif
            err = ERR_OK;
        }

        pbuf_free(p);
    }

//  signal that packet should be sent();

#if ETH_PAD_SIZE
    pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

#if LWIP_STATS
    LINK_STATS_INC(link.xmit);
#endif
    return err;
}

/**
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface. Then the type of the received packet is determined and
 * the appropriate input function is called.
 *
 * @param netif the lwip network interface structure for this ethernetif
 */
void ethernetif_input(struct netif* netif, struct pbuf* p)
{
    struct eth_hdr* ethhdr;

    if (p == NULL) {
        LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: pbuf is NULL\n"));
        goto _exit;
    }

    if (p->payload == NULL) {
        LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: payload is NULL\n"));
        pbuf_free(p);
        goto _exit;
    }

    if (netif == NULL) {
        LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: netif is NULL\n"));
        pbuf_free(p);
        goto _exit;
    }

    if (!(netif->flags & NETIF_FLAG_LINK_UP)) {
        LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: netif is not up\n"));
        pbuf_free(p);
        p = NULL;
        goto _exit;
    }

    /* points to packet payload, which starts with an Ethernet header */
    ethhdr = p->payload;

    switch (htons(ethhdr->type)) {
        /* IP or ARP packet? */
        case ETHTYPE_IP:
        case ETHTYPE_IPV6:
        case ETHTYPE_ARP:
#if PPPOE_SUPPORT

        /* PPPoE packet? */
        case ETHTYPE_PPPOEDISC:
        case ETHTYPE_PPPOE:
#endif /* PPPOE_SUPPORT */

            /* full packet send to tcpip_thread to process */
            if (netif->input(p, netif) != ERR_OK) {
                LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
                pbuf_free(p);
                p = NULL;
            }

            break;

        default:
            pbuf_free(p);
            p = NULL;
            break;
    }

_exit:
    ;
}

/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other int8_t on error
 */
int8_t ethernetif_init(struct netif* netif)
{
    if (netif == NULL) {
        LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_init: netif is NULL\n"));
    }

#if LWIP_NETIF_HOSTNAME

    netif->hostname = "LWIP";

#endif /* LWIP_NETIF_HOSTNAME */

    /*
     * Initialize the snmp variables and counters inside the struct netif.
     * The last argument should be replaced with your link speed, in units
     * of bits per second.
     */

    netif->name[0] = IFNAME0;
    netif->name[1] = IFNAME1;
    /* We directly use etharp_output() here to save a function call.
     * You can instead declare your own function an call etharp_output()
     * from it if you have to do some checks before sending (e.g. if link
     * is available...) */
    netif->output = etharp_output;
#if LWIP_IPV6
    netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */
    netif->linkoutput = low_level_output;

    /* initialize the hardware */
    low_level_init(netif);

    return ERR_OK;
}

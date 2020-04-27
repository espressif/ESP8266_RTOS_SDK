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

#include <stdlib.h>
#include <string.h>

#include "tcpip_adapter.h"

#if CONFIG_TCPIP_LWIP

#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/dhcp.h"
#if LWIP_IPV4 && LWIP_AUTOIP
#include "lwip/autoip.h"
#include "lwip/prot/autoip.h"
#endif
#include "lwip/ip_addr.h"
#include "lwip/ip6_addr.h"
#include "lwip/nd6.h"
#include "lwip/dns.h"
#include "lwip/errno.h"
#include "lwip/timeouts.h"
#include "lwip/prot/dhcp.h"
#include "lwip/priv/tcpip_priv.h"
#include "netif/etharp.h"
#include "esp_wifi.h"
#include "esp_timer.h"
#include "rom/ets_sys.h"
#include "dhcpserver/dhcpserver.h"
#include "dhcpserver/dhcpserver_options.h"
#include "esp_log.h"
#include "internal/esp_wifi_internal.h"

#include "FreeRTOS.h"
#include "timers.h"

struct tcpip_adapter_pbuf {
    struct pbuf_custom  pbuf;

    void                *eb;
    void                *buffer;
};

struct tcpip_adapter_api_call_data {
    struct tcpip_api_call_data call;

    struct netif        *netif;
};

static struct netif *esp_netif[TCPIP_ADAPTER_IF_MAX];
static tcpip_adapter_ip_info_t esp_ip[TCPIP_ADAPTER_IF_MAX];
static tcpip_adapter_ip_info_t esp_ip_old[TCPIP_ADAPTER_IF_MAX];
#if TCPIP_ADAPTER_IPV6
/*TODO need add ip6*/
static tcpip_adapter_ip6_info_t esp_ip6[TCPIP_ADAPTER_IF_MAX];
#endif
static tcpip_adapter_ip_lost_timer_t esp_ip_lost_timer[TCPIP_ADAPTER_IF_MAX];

static tcpip_adapter_dhcp_status_t dhcps_status = TCPIP_ADAPTER_DHCP_INIT;
static tcpip_adapter_dhcp_status_t dhcpc_status[TCPIP_ADAPTER_IF_MAX] = {TCPIP_ADAPTER_DHCP_INIT};
static esp_err_t tcpip_adapter_reset_ip_info(tcpip_adapter_if_t tcpip_if);
static esp_err_t tcpip_adapter_start_ip_lost_timer(tcpip_adapter_if_t tcpip_if);
static void tcpip_adapter_dhcpc_cb(struct netif *netif);
static void tcpip_adapter_ip_lost_timer(void *arg);
static void tcpip_adapter_dhcpc_done(TimerHandle_t arg);
static bool tcpip_inited = false;

static const char* TAG = "tcpip_adapter";

/* Avoid warning. No header file has include these function */
err_t ethernetif_init(struct netif* netif);
void system_station_got_ip_set();

static int dhcp_fail_time = 0;
static tcpip_adapter_ip_info_t esp_ip[TCPIP_ADAPTER_IF_MAX];
static TimerHandle_t *dhcp_check_timer;

static void tcpip_adapter_dhcps_cb(u8_t client_ip[4])
{
    ESP_LOGI(TAG,"softAP assign IP to station,IP is: %d.%d.%d.%d",
                client_ip[0],client_ip[1],client_ip[2],client_ip[3]);
    system_event_t evt;
    evt.event_id = SYSTEM_EVENT_AP_STAIPASSIGNED;
    memcpy(&evt.event_info.ap_staipassigned.ip, client_ip, sizeof(ip4_addr_t));
    esp_event_send(&evt);
}

static err_t _dhcp_start(struct tcpip_api_call_data *p)
{
    err_t ret;
    struct tcpip_adapter_api_call_data *call = (struct tcpip_adapter_api_call_data *)p;

    ret = dhcp_start(call->netif);

#if ESP_LWIP
    if (ret == ERR_OK)
        dhcp_set_cb(call->netif, tcpip_adapter_dhcpc_cb);
#endif

    return ret;
}

static err_t _dhcp_stop(struct tcpip_api_call_data *p)
{
    struct tcpip_adapter_api_call_data *call = (struct tcpip_adapter_api_call_data *)p;

    dhcp_stop(call->netif);

    return 0;
}

static err_t _dhcp_release(struct tcpip_api_call_data *p)
{
    struct tcpip_adapter_api_call_data *call = (struct tcpip_adapter_api_call_data *)p;

    dhcp_release(call->netif);
    dhcp_stop(call->netif);
    dhcp_cleanup(call->netif);

    return 0;
}

static err_t _dhcp_clean(struct tcpip_api_call_data *p)
{
    struct tcpip_adapter_api_call_data *call = (struct tcpip_adapter_api_call_data *)p;

    dhcp_stop(call->netif);
    dhcp_cleanup(call->netif);

    return 0;
}

static int tcpip_adapter_start_dhcp(struct netif *netif)
{
    struct tcpip_adapter_api_call_data call;

    call.netif = netif;

    return tcpip_api_call(_dhcp_start, (struct tcpip_api_call_data *)&call);
}

static int tcpip_adapter_stop_dhcp(struct netif *netif)
{
    struct tcpip_adapter_api_call_data call;

    call.netif = netif;

    return tcpip_api_call(_dhcp_stop, (struct tcpip_api_call_data *)&call);
}

static int tcpip_adapter_release_dhcp(struct netif *netif)
{
    struct tcpip_adapter_api_call_data call;

    call.netif = netif;

    return tcpip_api_call(_dhcp_release, (struct tcpip_api_call_data *)&call);
}

static int tcpip_adapter_clean_dhcp(struct netif *netif)
{
    struct tcpip_adapter_api_call_data call;

    call.netif = netif;

    return tcpip_api_call(_dhcp_clean, (struct tcpip_api_call_data *)&call);
}

void tcpip_adapter_init(void)
{
    if (tcpip_inited == false) {
        tcpip_inited = true;
        tcpip_init(NULL, NULL);

        memset(esp_ip, 0, sizeof(tcpip_adapter_ip_info_t)*TCPIP_ADAPTER_IF_MAX);
        memset(esp_ip_old, 0, sizeof(tcpip_adapter_ip_info_t)*TCPIP_ADAPTER_IF_MAX);

        IP4_ADDR(&esp_ip[TCPIP_ADAPTER_IF_AP].ip, 192, 168 , 4, 1);
        IP4_ADDR(&esp_ip[TCPIP_ADAPTER_IF_AP].gw, 192, 168 , 4, 1);
        IP4_ADDR(&esp_ip[TCPIP_ADAPTER_IF_AP].netmask, 255, 255 , 255, 0);

        dhcp_check_timer = xTimerCreate("check_dhcp", 500 / portTICK_RATE_MS, true, NULL, tcpip_adapter_dhcpc_done);
        if (!dhcp_check_timer) {
            ESP_LOGI(TAG, "TCPIP adapter timer create error");
        }
    }
}

/*
 * @brief LWIP custom pbuf callback function, it is to free custom pbuf
 *
 * @param p LWIP pbuf pointer
 *
 * @return none
 */
static void tcpip_adapter_free_pbuf(struct pbuf *p)
{
    struct tcpip_adapter_pbuf *pa = (struct tcpip_adapter_pbuf *)p;

    if (pa->eb) {
        esp_wifi_internal_free_rx_buffer(pa->eb);
        pa->eb = NULL;
    } else {
        os_free(pa->buffer);
        pa->buffer = NULL;
    }

    os_free(pa);
}

/*
 * @brief TCPIP adapter AI/O recieve callback function, it is to recieve input data
 *        and pass it to LWIP core
 *
 * @param aio AI/O control block pointer
 *
 * @return 0 if success or others if failed
 */
static int tcpip_adapter_recv_cb(void *index, void *buffer, uint16_t len, void *eb)
{
    struct pbuf *pbuf = NULL;
    struct tcpip_adapter_pbuf *p;
    struct netif *netif = (struct netif *)index;

    extern void ethernetif_input(struct netif *netif, struct pbuf *p);

    p = os_malloc(sizeof(struct tcpip_adapter_pbuf));
    if (!p)
        goto no_mem;

    // PBUF_RAW means payload = (char *)aio->pbuf + offset(=0)
    pbuf = pbuf_alloced_custom(PBUF_RAW, len, PBUF_REF, &p->pbuf, buffer, len);
    if (!pbuf)
        goto pbuf_err;

    p->pbuf.custom_free_function = tcpip_adapter_free_pbuf;
    p->eb = (void *)eb;
    p->buffer = buffer;

    ethernetif_input(netif, pbuf);

    return 0;

pbuf_err:
    os_free(p);
no_mem:
    return -ENOMEM;
}

/*
 * @brief TCPIP adapter AI/O recieve callback function, it is to recieve input data
 *        and pass it to LWIP core
 *
 * @param aio AI/O control block pointer
 *
 * @return 0 if success or others if failed
 */
static int tcpip_adapter_ap_recv_cb(void *buffer, uint16_t len, void *eb)
{
    return tcpip_adapter_recv_cb(esp_netif[ESP_IF_WIFI_AP], buffer, len, eb);
}

/*
 * @brief TCPIP adapter AI/O recieve callback function, it is to recieve input data
 *        and pass it to LWIP core
 *
 * @param aio AI/O control block pointer
 *
 * @return 0 if success or others if failed
 */
static int tcpip_adapter_sta_recv_cb(void *buffer, uint16_t len, void *eb)
{
    return tcpip_adapter_recv_cb(esp_netif[ESP_IF_WIFI_STA], buffer, len, eb);
}

static void tcpip_adapter_dhcpc_done(TimerHandle_t xTimer)
{
    struct dhcp *clientdhcp = netif_dhcp_data(esp_netif[TCPIP_ADAPTER_IF_STA]) ;
    struct netif *netif = esp_netif[TCPIP_ADAPTER_IF_STA];

    if (!netif) {
        ESP_LOGD(TAG, "null netif=%p", netif);
        return;
    }

#if LWIP_IPV4 && LWIP_AUTOIP
    struct autoip *autoip = netif_autoip_data(netif);
#endif

    xTimerStop(dhcp_check_timer, 0);
    if (netif_is_up(esp_netif[TCPIP_ADAPTER_IF_STA])) {
        if (clientdhcp->state == DHCP_STATE_BOUND
#if LWIP_IPV4 && LWIP_AUTOIP
            || (autoip && autoip->state == AUTOIP_STATE_ANNOUNCING)
#endif
        ) {
            /*send event here*/
            tcpip_adapter_dhcpc_cb(esp_netif[TCPIP_ADAPTER_IF_STA]);
            ESP_LOGD(TAG,"ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR "\n", IP2STR(ip_2_ip4(&(esp_netif[0]->ip_addr))),
                IP2STR(ip_2_ip4(&(esp_netif[0]->netmask))), IP2STR(ip_2_ip4(&(esp_netif[0]->gw))));
            dhcp_fail_time = 0;
        } else if (dhcp_fail_time < (CONFIG_IP_LOST_TIMER_INTERVAL * 1000 / 500)) {
            ESP_LOGD(TAG,"dhcpc time(ms): %d\n", dhcp_fail_time * 500);
            dhcp_fail_time ++;
            xTimerReset(dhcp_check_timer, 0);
        } else {
            dhcp_fail_time = 0;
            ESP_LOGD(TAG,"ERROR dhcp get ip error\n");
        }
    } else {
        dhcp_fail_time = 0;
        tcpip_adapter_release_dhcp(esp_netif[TCPIP_ADAPTER_IF_STA]);

        dhcpc_status[TCPIP_ADAPTER_IF_STA] = TCPIP_ADAPTER_DHCP_INIT;

        tcpip_adapter_reset_ip_info(TCPIP_ADAPTER_IF_STA);
    }
}

static esp_err_t tcpip_adapter_update_default_netif(void)
{
    if ((esp_netif[TCPIP_ADAPTER_IF_STA] != NULL) && netif_is_up(esp_netif[TCPIP_ADAPTER_IF_STA])) {
        netif_set_default(esp_netif[TCPIP_ADAPTER_IF_STA]);
    } else if ((esp_netif[TCPIP_ADAPTER_IF_AP] != NULL) && netif_is_up(esp_netif[TCPIP_ADAPTER_IF_AP])) {
        netif_set_default(esp_netif[TCPIP_ADAPTER_IF_AP]);
    }

    return ESP_OK;
}

esp_err_t tcpip_adapter_start(tcpip_adapter_if_t tcpip_if, uint8_t *mac, tcpip_adapter_ip_info_t *ip_info)
{
    esp_err_t ret = -1;
    if (tcpip_if >= TCPIP_ADAPTER_IF_MAX || mac == NULL || ip_info == NULL) {
        return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    }

    if (esp_netif[tcpip_if] != NULL && memcmp(esp_netif[tcpip_if]->hwaddr, mac, NETIF_MAX_HWADDR_LEN)) {
        memcpy(esp_netif[tcpip_if]->hwaddr, mac, NETIF_MAX_HWADDR_LEN);
    }

    if (esp_netif[tcpip_if] == NULL || !netif_is_up(esp_netif[tcpip_if])) {
        if (esp_netif[tcpip_if] == NULL) {
            esp_netif[tcpip_if] = (struct netif*)os_zalloc(sizeof(*esp_netif[tcpip_if]));
        }

        if (esp_netif[tcpip_if] == NULL) {
            ESP_LOGE(TAG, "TCPIP adapter has no memory\n");
            return ESP_ERR_NO_MEM;
        }
        memcpy(esp_netif[tcpip_if]->hwaddr, mac, NETIF_MAX_HWADDR_LEN);
        if (tcpip_if == TCPIP_ADAPTER_IF_STA) {
            ret = esp_wifi_internal_reg_rxcb(ESP_IF_WIFI_STA, tcpip_adapter_sta_recv_cb);
            if (ret < 0) {
                ESP_LOGE(TAG, "TCPIP adapter bind %d error\n", ESP_IF_WIFI_STA);
                return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
            }
        } else if (tcpip_if == TCPIP_ADAPTER_IF_AP) {
            ret = esp_wifi_internal_reg_rxcb(ESP_IF_WIFI_AP, tcpip_adapter_ap_recv_cb);
            if (ret < 0) {
                ESP_LOGE(TAG, "TCPIP adapter bind %d error\n", ESP_IF_WIFI_AP);
                return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
            }
        }
        netif_add(esp_netif[tcpip_if], &ip_info->ip, &ip_info->netmask, &ip_info->gw, (void *)tcpip_if, ethernetif_init, tcpip_input);
#if ESP_GRATUITOUS_ARP
        if (tcpip_if == TCPIP_ADAPTER_IF_STA || tcpip_if == TCPIP_ADAPTER_IF_ETH) {
            netif_set_garp_flag(esp_netif[tcpip_if]);
        }
#endif
    }

    if (tcpip_if == TCPIP_ADAPTER_IF_AP) {
        netif_set_up(esp_netif[tcpip_if]);

        if (dhcps_status == TCPIP_ADAPTER_DHCP_INIT) {
            dhcps_set_new_lease_cb(tcpip_adapter_dhcps_cb);
            
            dhcps_start(esp_netif[tcpip_if], ip_info->ip);

            ESP_LOGD(TAG, "dhcp server start:(ip: " IPSTR ", mask: " IPSTR ", gw: " IPSTR ")",
                   IP2STR(&ip_info->ip), IP2STR(&ip_info->netmask), IP2STR(&ip_info->gw));

            dhcps_status = TCPIP_ADAPTER_DHCP_STARTED;
        }
    }

    tcpip_adapter_update_default_netif();

    return ESP_OK;
}

esp_err_t tcpip_adapter_stop(tcpip_adapter_if_t tcpip_if)
{
    if (tcpip_if >= TCPIP_ADAPTER_IF_MAX) {
        return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    }

    if (esp_netif[tcpip_if] == NULL) {
        return ESP_ERR_TCPIP_ADAPTER_IF_NOT_READY;
    }
    
    esp_wifi_internal_reg_rxcb((wifi_interface_t)tcpip_if, NULL);
    if (!netif_is_up(esp_netif[tcpip_if])) {
        tcpip_adapter_clean_dhcp(esp_netif[tcpip_if]);
        netif_remove(esp_netif[tcpip_if]);
        return ESP_ERR_TCPIP_ADAPTER_IF_NOT_READY;
    }

    if (tcpip_if == TCPIP_ADAPTER_IF_AP) {
        dhcps_stop(esp_netif[tcpip_if]);    // TODO: dhcps checks status by its self
        if (TCPIP_ADAPTER_DHCP_STOPPED != dhcps_status) {
            dhcps_status = TCPIP_ADAPTER_DHCP_INIT;
        }
    } else if (tcpip_if == TCPIP_ADAPTER_IF_STA || tcpip_if == TCPIP_ADAPTER_IF_ETH) {
        tcpip_adapter_release_dhcp(esp_netif[tcpip_if]);

        dhcpc_status[tcpip_if] = TCPIP_ADAPTER_DHCP_INIT;

        tcpip_adapter_reset_ip_info(tcpip_if);
    }

    netif_set_down(esp_netif[tcpip_if]);
    netif_remove(esp_netif[tcpip_if]);
    tcpip_adapter_update_default_netif();
    //os_free(esp_netif[tcpip_if]);
    //esp_netif[tcpip_if] = NULL;
    return ESP_OK;
}

esp_err_t tcpip_adapter_up(tcpip_adapter_if_t tcpip_if)
{
    if (tcpip_if == TCPIP_ADAPTER_IF_STA) {
        if (esp_netif[tcpip_if] == NULL) {
            return ESP_ERR_TCPIP_ADAPTER_IF_NOT_READY;
        }

        /* use last obtained ip, or static ip */
        netif_set_addr(esp_netif[tcpip_if], &esp_ip[tcpip_if].ip, &esp_ip[tcpip_if].netmask, &esp_ip[tcpip_if].gw);
        netif_set_up(esp_netif[tcpip_if]);
    }

    tcpip_adapter_update_default_netif();

    return ESP_OK;
}

static esp_err_t tcpip_adapter_reset_ip_info(tcpip_adapter_if_t tcpip_if)
{
    ip4_addr_set_zero(&esp_ip[tcpip_if].ip);
    ip4_addr_set_zero(&esp_ip[tcpip_if].gw);
    ip4_addr_set_zero(&esp_ip[tcpip_if].netmask);
    return ESP_OK;
}

esp_err_t tcpip_adapter_down(tcpip_adapter_if_t tcpip_if)
{
    if (tcpip_if == TCPIP_ADAPTER_IF_STA) {
        if (esp_netif[tcpip_if] == NULL) {
            return ESP_ERR_TCPIP_ADAPTER_IF_NOT_READY;
        }

        if (dhcpc_status[tcpip_if] == TCPIP_ADAPTER_DHCP_STARTED) {
            tcpip_adapter_stop_dhcp(esp_netif[tcpip_if]);

            dhcpc_status[tcpip_if] = TCPIP_ADAPTER_DHCP_INIT;

            tcpip_adapter_reset_ip_info(tcpip_if);
        }

        netif_set_addr(esp_netif[tcpip_if], IP4_ADDR_ANY4, IP4_ADDR_ANY4, IP4_ADDR_ANY4);
        netif_set_down(esp_netif[tcpip_if]);
        tcpip_adapter_start_ip_lost_timer(tcpip_if);
    }

    tcpip_adapter_update_default_netif();

    return ESP_OK;
}

esp_err_t tcpip_adapter_set_old_ip_info(tcpip_adapter_if_t tcpip_if, tcpip_adapter_ip_info_t *ip_info)
{
    if (tcpip_if >= TCPIP_ADAPTER_IF_MAX || ip_info == NULL) {
        return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    }

    memcpy(&esp_ip_old[tcpip_if], ip_info, sizeof(tcpip_adapter_ip_info_t));

    return ESP_OK;
}

esp_err_t tcpip_adapter_get_old_ip_info(tcpip_adapter_if_t tcpip_if, tcpip_adapter_ip_info_t *ip_info)
{
    if (tcpip_if >= TCPIP_ADAPTER_IF_MAX || ip_info == NULL) {
        return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    }

    memcpy(ip_info, &esp_ip_old[tcpip_if], sizeof(tcpip_adapter_ip_info_t));

    return ESP_OK;
}

esp_err_t tcpip_adapter_get_ip_info(tcpip_adapter_if_t tcpip_if, tcpip_adapter_ip_info_t *ip_info)
{
    struct netif *p_netif;

    if (tcpip_if >= TCPIP_ADAPTER_IF_MAX || ip_info == NULL) {
        return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    }

    p_netif = esp_netif[tcpip_if];

    if (p_netif != NULL && netif_is_up(p_netif)) {
        ip4_addr_set(&ip_info->ip, ip_2_ip4(&p_netif->ip_addr));
        ip4_addr_set(&ip_info->netmask, ip_2_ip4(&p_netif->netmask));
        ip4_addr_set(&ip_info->gw, ip_2_ip4(&p_netif->gw));

        return ESP_OK;
    }

    ip4_addr_copy(ip_info->ip, esp_ip[tcpip_if].ip);
    ip4_addr_copy(ip_info->gw, esp_ip[tcpip_if].gw);
    ip4_addr_copy(ip_info->netmask, esp_ip[tcpip_if].netmask);

    return ESP_OK;
}

esp_err_t tcpip_adapter_set_ip_info(tcpip_adapter_if_t tcpip_if, tcpip_adapter_ip_info_t *ip_info)
{
    struct netif *p_netif;
    tcpip_adapter_dhcp_status_t status;

    if (tcpip_if >= TCPIP_ADAPTER_IF_MAX || ip_info == NULL) {
        return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    }

    if (tcpip_if == TCPIP_ADAPTER_IF_AP) {
        tcpip_adapter_dhcps_get_status(tcpip_if, &status);

        if (status != TCPIP_ADAPTER_DHCP_STOPPED) {
            return ESP_ERR_TCPIP_ADAPTER_DHCP_NOT_STOPPED;
        }
    } else if (tcpip_if == TCPIP_ADAPTER_IF_STA) {
        tcpip_adapter_dhcpc_get_status(tcpip_if, &status);

        if (status != TCPIP_ADAPTER_DHCP_STOPPED) {
            return ESP_ERR_TCPIP_ADAPTER_DHCP_NOT_STOPPED;
        }
#if LWIP_DNS /* don't build if not configured for use in lwipopts.h */
        //dns_clear_servers(true);
#endif
    }

    ip4_addr_copy(esp_ip[tcpip_if].ip, ip_info->ip);
    ip4_addr_copy(esp_ip[tcpip_if].gw, ip_info->gw);
    ip4_addr_copy(esp_ip[tcpip_if].netmask, ip_info->netmask);

    p_netif = esp_netif[tcpip_if];

    if (p_netif != NULL && netif_is_up(p_netif)) {
        netif_set_addr(p_netif, &ip_info->ip, &ip_info->netmask, &ip_info->gw);
        if (tcpip_if == TCPIP_ADAPTER_IF_STA) {
            if (!(ip4_addr_isany_val(ip_info->ip) || ip4_addr_isany_val(ip_info->netmask) || ip4_addr_isany_val(ip_info->gw))) {
                system_event_t evt;
                if (tcpip_if == TCPIP_ADAPTER_IF_STA) {
                    evt.event_id = SYSTEM_EVENT_STA_GOT_IP;
                } else if (tcpip_if == TCPIP_ADAPTER_IF_ETH) {
                    evt.event_id = SYSTEM_EVENT_ETH_GOT_IP;
                }
                evt.event_info.got_ip.ip_changed = false;

                if (memcmp(ip_info, &esp_ip_old[tcpip_if], sizeof(tcpip_adapter_ip_info_t))) {
                    evt.event_info.got_ip.ip_changed = true;
                }

                memcpy(&evt.event_info.got_ip.ip_info, ip_info, sizeof(tcpip_adapter_ip_info_t));
                memcpy(&esp_ip_old[tcpip_if], ip_info, sizeof(tcpip_adapter_ip_info_t));
                esp_event_send(&evt);
                ESP_LOGD(TAG, "if%d tcpip adapter set static ip: ip changed=%d", tcpip_if, evt.event_info.got_ip.ip_changed);
            }
        }
    }

    return ESP_OK;
}

#if TCPIP_ADAPTER_IPV6
static void tcpip_adapter_nd6_cb(struct netif *p_netif, uint8_t ip_idex)
{
    tcpip_adapter_ip6_info_t *ip6_info;

    system_event_t evt;
    //notify event

    evt.event_id = SYSTEM_EVENT_GOT_IP6;

    if (!p_netif) {
        ESP_LOGD(TAG, "null p_netif=%p", p_netif);
        return;
    }

    if (p_netif == esp_netif[TCPIP_ADAPTER_IF_STA]) {
        ip6_info = &esp_ip6[TCPIP_ADAPTER_IF_STA];
        evt.event_info.got_ip6.if_index = TCPIP_ADAPTER_IF_STA;
    } else if (p_netif == esp_netif[TCPIP_ADAPTER_IF_AP]) {
        ip6_info = &esp_ip6[TCPIP_ADAPTER_IF_AP];
        evt.event_info.got_ip6.if_index = TCPIP_ADAPTER_IF_AP;
    } else if (p_netif == esp_netif[TCPIP_ADAPTER_IF_ETH]) {
        ip6_info = &esp_ip6[TCPIP_ADAPTER_IF_ETH];
        evt.event_info.got_ip6.if_index = TCPIP_ADAPTER_IF_ETH;
    } else {
        return;
    }

    ip6_addr_set(&ip6_info->ip, ip_2_ip6(&p_netif->ip6_addr[ip_idex]));

    memcpy(&evt.event_info.got_ip6.ip6_info, ip6_info, sizeof(tcpip_adapter_ip6_info_t));
    esp_event_send(&evt);
}
#endif

#if TCPIP_ADAPTER_IPV6
esp_err_t tcpip_adapter_create_ip6_linklocal(tcpip_adapter_if_t tcpip_if)
{
    struct netif *p_netif;

    if (tcpip_if >= TCPIP_ADAPTER_IF_MAX) {
        return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    }

    p_netif = esp_netif[tcpip_if];
    if (p_netif != NULL && netif_is_up(p_netif)) {
        netif_create_ip6_linklocal_address(p_netif, 1);
        /*TODO need add ipv6 address cb*/
        nd6_set_cb(p_netif, tcpip_adapter_nd6_cb);

        return ESP_OK;
    } else {
        return ESP_FAIL;
    }
}

esp_err_t tcpip_adapter_get_ip6_linklocal(tcpip_adapter_if_t tcpip_if, ip6_addr_t *if_ip6)
{
    struct netif *p_netif;

    if (tcpip_if >= TCPIP_ADAPTER_IF_MAX || if_ip6 == NULL) {
        return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    }

    p_netif = esp_netif[tcpip_if];
    if (p_netif != NULL && netif_is_up(p_netif) && ip6_addr_ispreferred(netif_ip6_addr_state(p_netif, 0))) {
        memcpy(if_ip6, &p_netif->ip6_addr[0], sizeof(ip6_addr_t));
    } else {
        return ESP_FAIL;
    }
    return ESP_OK;
}
#endif

esp_err_t tcpip_adapter_dhcps_option(tcpip_adapter_option_mode_t opt_op, tcpip_adapter_option_id_t opt_id, void *opt_val, uint32_t opt_len)
{
    void *opt_info = dhcps_option_info(opt_id, opt_len);

    if (opt_info == NULL || opt_val == NULL) {
        return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    }

    if (opt_op == TCPIP_ADAPTER_OP_GET) {
        if (dhcps_status == TCPIP_ADAPTER_DHCP_STOPPED) {
            return ESP_ERR_TCPIP_ADAPTER_DHCP_ALREADY_STOPPED;
        }

        switch (opt_id) {
        case IP_ADDRESS_LEASE_TIME: {
            *(uint32_t *)opt_val = *(uint32_t *)opt_info;
            break;
        }
        case REQUESTED_IP_ADDRESS: {
            memcpy(opt_val, opt_info, opt_len);
            break;
        }
        case ROUTER_SOLICITATION_ADDRESS: {
            if ((*(uint8_t *)opt_info) & OFFER_ROUTER) {
                *(uint8_t *)opt_val = 1;
            } else {
                *(uint8_t *)opt_val = 0;
            }
            break;
        }
        case DOMAIN_NAME_SERVER: {
            if ((*(uint8_t *)opt_info) & OFFER_DNS) {
                *(uint8_t *)opt_val = 1;
            } else {
                *(uint8_t *)opt_val = 0;
            }
            break;
        }
        default:
            break;
        }
    } else if (opt_op == TCPIP_ADAPTER_OP_SET) {
        if (dhcps_status == TCPIP_ADAPTER_DHCP_STARTED) {
            return ESP_ERR_TCPIP_ADAPTER_DHCP_ALREADY_STARTED;
        }

        switch (opt_id) {
        case IP_ADDRESS_LEASE_TIME: {
            if (*(uint32_t *)opt_val != 0) {
                *(uint32_t *)opt_info = *(uint32_t *)opt_val;
            } else {
                *(uint32_t *)opt_info = DHCPS_LEASE_TIME_DEF;
            }
            break;
        }
        case REQUESTED_IP_ADDRESS: {
            tcpip_adapter_ip_info_t info;
            uint32_t softap_ip = 0;
            uint32_t start_ip = 0;
            uint32_t end_ip = 0;
            dhcps_lease_t *poll = opt_val;

            if (poll->enable) {
                memset(&info, 0x00, sizeof(tcpip_adapter_ip_info_t));
                tcpip_adapter_get_ip_info(ESP_IF_WIFI_AP, &info);
                softap_ip = htonl(info.ip.addr);
                start_ip = htonl(poll->start_ip.addr);
                end_ip = htonl(poll->end_ip.addr);

                /*config ip information can't contain local ip*/
                if ((start_ip <= softap_ip) && (softap_ip <= end_ip)) {
                    return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
                }

                /*config ip information must be in the same segment as the local ip*/
                softap_ip >>= 8;
                if ((start_ip >> 8 != softap_ip)
                        || (end_ip >> 8 != softap_ip)) {
                    return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
                }

                if (end_ip - start_ip > DHCPS_MAX_LEASE) {
                    return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
                }
            }

            memcpy(opt_info, opt_val, opt_len);
            break;
        }
        case ROUTER_SOLICITATION_ADDRESS: {
            if (*(uint8_t *)opt_val) {
                *(uint8_t *)opt_info |= OFFER_ROUTER;
            } else {
                *(uint8_t *)opt_info &= ((~OFFER_ROUTER)&0xFF);
            }
            break;
        }
        case DOMAIN_NAME_SERVER: {
            if (*(uint8_t *)opt_val) {
                *(uint8_t *)opt_info |= OFFER_DNS;
            } else {
                *(uint8_t *)opt_info &= ((~OFFER_DNS)&0xFF);
            }
            break;
        }
       
        default:
            break;
        }
        dhcps_set_option_info(opt_id, opt_info,opt_len);
    } else {
        return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    }

    return ESP_OK;
}

esp_err_t tcpip_adapter_set_dns_info(tcpip_adapter_if_t tcpip_if, tcpip_adapter_dns_type_t type, tcpip_adapter_dns_info_t *dns)
{
    if (tcpip_if >= TCPIP_ADAPTER_IF_MAX) {
        ESP_LOGD(TAG, "set dns invalid if=%d", tcpip_if);
        return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    }
 
    if (!dns) {
        ESP_LOGD(TAG, "set dns null dns");
        return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    }

    if (type >= TCPIP_ADAPTER_DNS_MAX) {
        ESP_LOGD(TAG, "set dns invalid type=%d", type);
        return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    }

    if (ip4_addr_isany_val(*ip_2_ip4(&(dns->ip)))) {
        ESP_LOGD(TAG, "set dns invalid dns");
        return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    }

    ESP_LOGD(TAG, "set dns if=%d type=%d dns=%x", tcpip_if, type, ip_2_ip4(&(dns->ip))->addr);
    IP_SET_TYPE_VAL(dns->ip, IPADDR_TYPE_V4);

    if (tcpip_if == TCPIP_ADAPTER_IF_STA || tcpip_if == TCPIP_ADAPTER_IF_ETH) {
        dns_setserver(type, &(dns->ip));
    } else {
        if (type != TCPIP_ADAPTER_DNS_MAIN) {
            ESP_LOGD(TAG, "set dns invalid type");
            return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
        } else {
            dhcps_dns_setserver(&(dns->ip));
        }
    }

    return ESP_OK;
}

esp_err_t tcpip_adapter_get_dns_info(tcpip_adapter_if_t tcpip_if, tcpip_adapter_dns_type_t type, tcpip_adapter_dns_info_t *dns)
{ 
    const ip_addr_t *ns;

    if (!dns) {
        ESP_LOGD(TAG, "get dns null dns");
        return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    }

    if (type >= TCPIP_ADAPTER_DNS_MAX) {
        ESP_LOGD(TAG, "get dns invalid type=%d", type);
        return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    }
    
    if (tcpip_if >= TCPIP_ADAPTER_IF_MAX) {
        ESP_LOGD(TAG, "get dns invalid tcpip_if=%d",tcpip_if);
        return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    }

    if (tcpip_if == TCPIP_ADAPTER_IF_STA || tcpip_if == TCPIP_ADAPTER_IF_ETH) {
        ns = dns_getserver(type);
        dns->ip = *ns;
    } else {
        *ip_2_ip4(&(dns->ip)) = dhcps_dns_getserver();
    }

    return ESP_OK;
}

esp_err_t tcpip_adapter_dhcps_get_status(tcpip_adapter_if_t tcpip_if, tcpip_adapter_dhcp_status_t *status)
{
    *status = dhcps_status;

    return ESP_OK;
}

esp_err_t tcpip_adapter_dhcps_start(tcpip_adapter_if_t tcpip_if)
{
    /* only support ap now */
    if (tcpip_if != TCPIP_ADAPTER_IF_AP || tcpip_if >= TCPIP_ADAPTER_IF_MAX) {
        ESP_LOGD(TAG, "dhcp server invalid if=%d", tcpip_if);
        return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    }

    if (dhcps_status != TCPIP_ADAPTER_DHCP_STARTED) {
        struct netif *p_netif = esp_netif[tcpip_if];

        if (p_netif != NULL && netif_is_up(p_netif)) {
            tcpip_adapter_ip_info_t default_ip;
            tcpip_adapter_get_ip_info(ESP_IF_WIFI_AP, &default_ip);
            dhcps_start(p_netif, default_ip.ip);
            dhcps_status = TCPIP_ADAPTER_DHCP_STARTED;
            ESP_LOGD(TAG, "dhcp server start successfully");
            return ESP_OK;
        } else {
            ESP_LOGD(TAG, "dhcp server re init");
            dhcps_status = TCPIP_ADAPTER_DHCP_INIT;
            return ESP_OK;
        }
    }

    ESP_LOGD(TAG, "dhcp server already start");
    return ESP_ERR_TCPIP_ADAPTER_DHCP_ALREADY_STARTED;
}

esp_err_t tcpip_adapter_dhcps_stop(tcpip_adapter_if_t tcpip_if)
{
    /* only support ap now */
    if (tcpip_if != TCPIP_ADAPTER_IF_AP || tcpip_if >= TCPIP_ADAPTER_IF_MAX) {
        ESP_LOGD(TAG, "dhcp server invalid if=%d", tcpip_if);
        return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    }

    if (dhcps_status == TCPIP_ADAPTER_DHCP_STARTED) {
        struct netif *p_netif = esp_netif[tcpip_if];

        if (p_netif != NULL) {
            dhcps_stop(p_netif);
        } else {
            ESP_LOGD(TAG, "dhcp server if not ready");
            return ESP_ERR_TCPIP_ADAPTER_IF_NOT_READY;
        }
    } else if (dhcps_status == TCPIP_ADAPTER_DHCP_STOPPED) {
        ESP_LOGD(TAG, "dhcp server already stoped");
        return ESP_ERR_TCPIP_ADAPTER_DHCP_ALREADY_STOPPED;
    }

    ESP_LOGD(TAG, "dhcp server stop successfully");
    dhcps_status = TCPIP_ADAPTER_DHCP_STOPPED;
    return ESP_OK;
}

esp_err_t tcpip_adapter_dhcpc_option(tcpip_adapter_option_mode_t opt_op, tcpip_adapter_option_id_t opt_id, void *opt_val, uint32_t opt_len)
{
    // TODO: when dhcp request timeout,change the retry count
    return ESP_ERR_NOT_SUPPORTED;
}

static void tcpip_adapter_dhcpc_cb(struct netif *netif)
{
    tcpip_adapter_ip_info_t *ip_info_old = NULL;
    tcpip_adapter_ip_info_t *ip_info = NULL;
    tcpip_adapter_if_t tcpip_if;

    if (!netif) {
        ESP_LOGD(TAG, "null netif=%p", netif);
        return;
    }

    if( netif == esp_netif[TCPIP_ADAPTER_IF_STA] ) {
        tcpip_if = TCPIP_ADAPTER_IF_STA;
    } else { 
        ESP_LOGD(TAG, "err netif=%p", netif);
        return;
    }

    ESP_LOGD(TAG, "if%d dhcpc cb", tcpip_if);
    ip_info = &esp_ip[tcpip_if];
    ip_info_old = &esp_ip_old[tcpip_if];

    if ( !ip_addr_isany_val(netif->ip_addr) ) {
        
        //check whether IP is changed
        if ( !ip4_addr_cmp(ip_2_ip4(&netif->ip_addr), &ip_info->ip) ||
                !ip4_addr_cmp(ip_2_ip4(&netif->netmask), &ip_info->netmask) ||
                !ip4_addr_cmp(ip_2_ip4(&netif->gw), &ip_info->gw) ) {
            system_event_t evt;

            ip4_addr_set(&ip_info->ip, ip_2_ip4(&netif->ip_addr));
            ip4_addr_set(&ip_info->netmask, ip_2_ip4(&netif->netmask));
            ip4_addr_set(&ip_info->gw, ip_2_ip4(&netif->gw));

            //notify event
            evt.event_id = SYSTEM_EVENT_STA_GOT_IP;
            evt.event_info.got_ip.ip_changed = false;

            if (memcmp(ip_info, ip_info_old, sizeof(tcpip_adapter_ip_info_t))) {
                evt.event_info.got_ip.ip_changed = true;
            }

            memcpy(&evt.event_info.got_ip.ip_info, ip_info, sizeof(tcpip_adapter_ip_info_t));
            memcpy(ip_info_old, ip_info, sizeof(tcpip_adapter_ip_info_t));
            ESP_LOGD(TAG, "if%d ip changed=%d", tcpip_if, evt.event_info.got_ip.ip_changed);
            esp_event_send(&evt);
        } else {
            ESP_LOGD(TAG, "if%d ip unchanged", CONFIG_IP_LOST_TIMER_INTERVAL);
        }
    } else {
        if (!ip4_addr_isany_val(ip_info->ip)) {
            tcpip_adapter_start_ip_lost_timer(tcpip_if);
        }
    }

    return;
}

static esp_err_t tcpip_adapter_start_ip_lost_timer(tcpip_adapter_if_t tcpip_if)
{
    tcpip_adapter_ip_info_t *ip_info_old = &esp_ip_old[tcpip_if];
    struct netif *netif = esp_netif[tcpip_if];

    ESP_LOGD(TAG, "if%d start ip lost tmr: enter", tcpip_if);
    if (tcpip_if != TCPIP_ADAPTER_IF_STA) {
        ESP_LOGD(TAG, "if%d start ip lost tmr: only sta support ip lost timer", tcpip_if);
        return ESP_OK;
    }

    if (esp_ip_lost_timer[tcpip_if].timer_running) {
        ESP_LOGD(TAG, "if%d start ip lost tmr: already started", tcpip_if);
        return ESP_OK;
    }

    if ( netif && (CONFIG_IP_LOST_TIMER_INTERVAL > 0) && !ip4_addr_isany_val(ip_info_old->ip)) {
        esp_ip_lost_timer[tcpip_if].timer_running = true;
        sys_timeout(CONFIG_IP_LOST_TIMER_INTERVAL*1000, tcpip_adapter_ip_lost_timer, (void*)tcpip_if);
        ESP_LOGD(TAG, "if%d start ip lost tmr: interval=%d", tcpip_if, CONFIG_IP_LOST_TIMER_INTERVAL);
        return ESP_OK;
    }

    ESP_LOGD(TAG, "if%d start ip lost tmr: no need start because netif=%p interval=%d ip=%x", 
                  tcpip_if, netif, CONFIG_IP_LOST_TIMER_INTERVAL, ip_info_old->ip.addr);

    return ESP_OK;
}

static void tcpip_adapter_ip_lost_timer(void *arg)
{
    tcpip_adapter_if_t tcpip_if = (tcpip_adapter_if_t)arg;

    ESP_LOGD(TAG, "if%d ip lost tmr: enter", tcpip_if);

    if (esp_ip_lost_timer[tcpip_if].timer_running == false) {
        ESP_LOGD(TAG, "ip lost time is not running");
        return;
    }

    esp_ip_lost_timer[tcpip_if].timer_running = false;

    if (tcpip_if == TCPIP_ADAPTER_IF_STA) {
        struct netif *netif = esp_netif[tcpip_if];

        if ( (!netif) || (netif && ip_addr_isany_val(netif->ip_addr))){
            system_event_t evt;

            ESP_LOGD(TAG, "if%d ip lost tmr: raise ip lost event", tcpip_if);
            memset(&esp_ip_old[tcpip_if], 0, sizeof(tcpip_adapter_ip_info_t));
            evt.event_id = SYSTEM_EVENT_STA_LOST_IP;
            esp_event_send(&evt);
        } else {
            ESP_LOGD(TAG, "if%d ip lost tmr: no need raise ip lost event", tcpip_if);
        }
    } else {
        ESP_LOGD(TAG, "if%d ip lost tmr: not station", tcpip_if);
    }
}

esp_err_t tcpip_adapter_dhcpc_get_status(tcpip_adapter_if_t tcpip_if, tcpip_adapter_dhcp_status_t *status)
{
    *status = dhcpc_status[tcpip_if];

    return ESP_OK;
}

esp_err_t tcpip_adapter_dhcpc_start(tcpip_adapter_if_t tcpip_if)
{
    if ((tcpip_if != TCPIP_ADAPTER_IF_STA)  || tcpip_if >= TCPIP_ADAPTER_IF_MAX) {
        ESP_LOGD(TAG, "dhcp client invalid if=%d", tcpip_if);
        return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    }

    if (dhcpc_status[tcpip_if] != TCPIP_ADAPTER_DHCP_STARTED) {
        struct netif *p_netif = esp_netif[tcpip_if];

        tcpip_adapter_reset_ip_info(tcpip_if);
#if LWIP_DNS
        //dns_clear_servers(true);
#endif

        if (p_netif != NULL) {
            if (netif_is_up(p_netif)) {
                ESP_LOGD(TAG, "dhcp client init ip/mask/gw to all-0");
                ip_addr_set_zero(&p_netif->ip_addr);
                ip_addr_set_zero(&p_netif->netmask);
                ip_addr_set_zero(&p_netif->gw);
                tcpip_adapter_start_ip_lost_timer(tcpip_if);
            } else {
                ESP_LOGD(TAG, "dhcp client re init");
                dhcpc_status[tcpip_if] = TCPIP_ADAPTER_DHCP_INIT;
                return ESP_OK;
            }

            if (tcpip_adapter_start_dhcp(p_netif) != ERR_OK) {
                ESP_LOGD(TAG, "dhcp client start failed");
                return ESP_ERR_TCPIP_ADAPTER_DHCPC_START_FAILED;
            }

            dhcp_fail_time = 0;
            xTimerReset(dhcp_check_timer, 0);
            ESP_LOGD(TAG, "dhcp client start successfully");
            dhcpc_status[tcpip_if] = TCPIP_ADAPTER_DHCP_STARTED;
            return ESP_OK;
        } else {
            ESP_LOGD(TAG, "dhcp client re init");
            dhcpc_status[tcpip_if] = TCPIP_ADAPTER_DHCP_INIT;
            return ESP_OK;
        }
    }

    ESP_LOGD(TAG, "dhcp client already started");
    return ESP_ERR_TCPIP_ADAPTER_DHCP_ALREADY_STARTED;
}

esp_err_t tcpip_adapter_dhcpc_stop(tcpip_adapter_if_t tcpip_if)
{
    if (tcpip_if != TCPIP_ADAPTER_IF_STA || tcpip_if >= TCPIP_ADAPTER_IF_MAX) {
        ESP_LOGD(TAG, "dhcp client invalid if=%d", tcpip_if);
        return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    }

    esp_ip_lost_timer[tcpip_if].timer_running = false;

    if (dhcpc_status[tcpip_if] == TCPIP_ADAPTER_DHCP_STARTED) {
        struct netif *p_netif = esp_netif[tcpip_if];

        if (p_netif != NULL) {
            tcpip_adapter_stop_dhcp(p_netif);
            tcpip_adapter_reset_ip_info(tcpip_if);
        } else {
            ESP_LOGD(TAG, "dhcp client if not ready");
            return ESP_ERR_TCPIP_ADAPTER_IF_NOT_READY;
        }
    } else if (dhcpc_status[tcpip_if] == TCPIP_ADAPTER_DHCP_STOPPED) {
        ESP_LOGD(TAG, "dhcp client already stoped");
        return ESP_ERR_TCPIP_ADAPTER_DHCP_ALREADY_STOPPED;
    }

    ESP_LOGD(TAG, "dhcp client stop successfully");
    dhcpc_status[tcpip_if] = TCPIP_ADAPTER_DHCP_STOPPED;
    return ESP_OK;
}

esp_interface_t tcpip_adapter_get_esp_if(void *dev)
{
    struct netif *p_netif = (struct netif *)dev;

    if (p_netif == esp_netif[TCPIP_ADAPTER_IF_STA]) {
        return ESP_IF_WIFI_STA;
    } else if (p_netif == esp_netif[TCPIP_ADAPTER_IF_AP]) {
        return ESP_IF_WIFI_AP;
    }

    return ESP_IF_MAX;
}

esp_err_t tcpip_adapter_get_sta_list(wifi_sta_list_t *wifi_sta_list, tcpip_adapter_sta_list_t *tcpip_sta_list)
{
    int i;

    if ((wifi_sta_list == NULL) || (tcpip_sta_list == NULL)) {
        return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    }

    memset(tcpip_sta_list, 0, sizeof(tcpip_adapter_sta_list_t));
    tcpip_sta_list->num = wifi_sta_list->num;
    for (i = 0; i < wifi_sta_list->num; i++) {
        memcpy(tcpip_sta_list->sta[i].mac, wifi_sta_list->sta[i].mac, 6);
        dhcp_search_ip_on_mac(tcpip_sta_list->sta[i].mac, &tcpip_sta_list->sta[i].ip);
    }

    return ESP_OK;
}

esp_err_t tcpip_adapter_set_hostname(tcpip_adapter_if_t tcpip_if, const char *hostname)
{
#if LWIP_NETIF_HOSTNAME
    struct netif *p_netif;
    static char hostinfo[TCPIP_ADAPTER_IF_MAX][TCPIP_HOSTNAME_MAX_SIZE + 1];

    if (tcpip_if >= TCPIP_ADAPTER_IF_MAX || hostname == NULL) {
        return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    }

    if (strlen(hostname) > TCPIP_HOSTNAME_MAX_SIZE) {
        return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    }

    p_netif = esp_netif[tcpip_if];
    if (p_netif != NULL) {
        memset(hostinfo[tcpip_if], 0, sizeof(hostinfo[tcpip_if]));
        strlcpy(hostinfo[tcpip_if], hostname, sizeof(hostinfo[tcpip_if]));
        p_netif->hostname = hostinfo[tcpip_if];
        return ESP_OK;
    } else {
        return ESP_ERR_TCPIP_ADAPTER_IF_NOT_READY;
    }
#else
    return ESP_ERR_TCPIP_ADAPTER_IF_NOT_READY;
#endif
}

esp_err_t tcpip_adapter_get_hostname(tcpip_adapter_if_t tcpip_if, const char **hostname)
{
#if LWIP_NETIF_HOSTNAME
    struct netif *p_netif = NULL;
    if (tcpip_if >= TCPIP_ADAPTER_IF_MAX || hostname == NULL) {
        return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    }

    p_netif = esp_netif[tcpip_if];
    if (p_netif != NULL) {
        *hostname = p_netif->hostname;
        return ESP_OK;
    } else {
        return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    }
#else
    return ESP_ERR_TCPIP_ADAPTER_IF_NOT_READY;
#endif
}

esp_err_t tcpip_adapter_get_netif(tcpip_adapter_if_t tcpip_if, void ** netif)
{
    if (tcpip_if >= TCPIP_ADAPTER_IF_MAX) {
        return ESP_ERR_TCPIP_ADAPTER_INVALID_PARAMS;
    }

    *netif = esp_netif[tcpip_if];

    if (*netif == NULL) {
        return ESP_ERR_TCPIP_ADAPTER_IF_NOT_READY;
    }
    return ESP_OK;
}

struct netif* ip4_route_src_hook(const ip4_addr_t* dest, const ip4_addr_t* src)
{
    extern struct netif *netif_list;
    struct netif *netif = NULL;

    /* destination IP is broadcast IP? */
    if ((src != NULL) && !ip4_addr_isany(src)) {
        /* iterate through netifs */
        for (netif = netif_list; netif != NULL; netif = netif->next) {
        /* is the netif up, does it have a link and a valid address? */
            if (netif_is_up(netif) && netif_is_link_up(netif) && !ip4_addr_isany_val(*netif_ip4_addr(netif))) {
                /* source IP matches? */
                if (ip4_addr_cmp(src, netif_ip4_addr(netif))) {
                    /* return netif on which to forward IP packet */
                    return netif;
                }
            }
        }
    }
    return netif;
}

bool tcpip_adapter_is_netif_up(tcpip_adapter_if_t tcpip_if)
{
    if (esp_netif[tcpip_if] != NULL && netif_is_up(esp_netif[tcpip_if])) {
        return true;
    } else {
        return false;
    }
}

#endif /* CONFIG_TCPIP_LWIP */

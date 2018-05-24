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

#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/dhcp.h"
#include "lwip/errno.h"
#include "lwip/prot/dhcp.h"
#include "netif/etharp.h"
#include "esp_wifi.h"
#include "esp_timer.h"
#include "esp_misc.h"
#include "tcpip_adapter.h"
#include "dhcpserver/dhcpserver.h"
#include "net/sockio.h"
#include "esp_socket.h"

struct tcpip_adapter_pbuf {
    struct pbuf_custom  pbuf;

    void                *base;

    struct netif        *netif;
};

u32_t LwipTimOutLim = 0; // For light sleep. time out. limit is 3000ms

/* Avoid warning. No header file has include these function */
err_t ethernetif_init(struct netif* netif);
void system_station_got_ip_set();

static os_timer_t* get_ip_timer;
static uint8_t dhcp_fail_time;
static bool dhcps_flag = true;
static bool dhcpc_flag = true;
static struct ip_info esp_ip[TCPIP_ADAPTER_IF_MAX];

void esp_wifi_station_dhcpc_event(uint8_t netif_index)
{
    if (TCPIP_ADAPTER_IF_VALID(netif_index)) {
        TCPIP_ATAPTER_LOG("wifi station dhcpc start\n");
        dhcp_stop(esp_netif[netif_index]);
        dhcp_cleanup(esp_netif[netif_index]);
        dhcp_inform(esp_netif[netif_index]);
    } else {
        TCPIP_ATAPTER_LOG("ERROR bad netif index:%d\n", netif_index);
    }
}

static void tcpip_adapter_dhcpc_done()
{
    struct dhcp *clientdhcp = netif_dhcp_data(esp_netif[TCPIP_ADAPTER_IF_STA]) ;

    os_timer_disarm(get_ip_timer);

    if (clientdhcp->state == DHCP_STATE_BOUND) {
        /*send event here*/
        system_station_got_ip_set();
        printf("ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR "\n", IP2STR(ip_2_ip4(&(esp_netif[0]->ip_addr))),
               IP2STR(ip_2_ip4(&(esp_netif[0]->netmask))), IP2STR(ip_2_ip4(&(esp_netif[0]->gw))));
    } else if (dhcp_fail_time < 100) {
        TCPIP_ATAPTER_LOG("dhcpc time(ms): %d\n", dhcp_fail_time * 200);
        dhcp_fail_time ++;
        os_timer_setfn(get_ip_timer, tcpip_adapter_dhcpc_done, NULL);
        os_timer_arm(get_ip_timer, 200, 1);
    } else {
        extern void wifi_station_dhcpc_event();
        wifi_station_dhcpc_event();
        TCPIP_ATAPTER_LOG("ERROR dhcp get ip error\n");
        free(get_ip_timer);
    }
}

static void tcpip_adapter_station_dhcp_start()
{
    err_t ret;
    get_ip_timer = (os_timer_t*)malloc(sizeof(*get_ip_timer));

    if (get_ip_timer == NULL) {
        TCPIP_ATAPTER_LOG("ERROR NO MEMORY\n");
    }

    TCPIP_ATAPTER_LOG("dhcpc start\n");
    ret = dhcp_start(esp_netif[TCPIP_ADAPTER_IF_STA]);
    dhcp_fail_time = 0;

    if (ret == 0) {
        os_timer_disarm(get_ip_timer);
        os_timer_setfn(get_ip_timer, tcpip_adapter_dhcpc_done, NULL);
        os_timer_arm(get_ip_timer, 100, 1);
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
    int s = (int)pa->netif->state;

    esp_free_pbuf(s, pa->base);
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
static int tcpip_adapter_recv_cb(struct esp_aio *aio)
{
    struct pbuf *pbuf = NULL;
    struct tcpip_adapter_pbuf *p;
    struct netif *netif = (struct netif *)aio->arg;

    extern void ethernetif_input(struct netif *netif, struct pbuf *p);

    p = os_malloc(sizeof(struct tcpip_adapter_pbuf));
    if (!p)
        return -ENOMEM;
    p->pbuf.custom_free_function = tcpip_adapter_free_pbuf;
    p->base = (void *)aio->pbuf;
    p->netif = netif;

    // PBUF_RAW means payload = (char *)aio->pbuf + offset(=0)
    pbuf = pbuf_alloced_custom(PBUF_RAW, aio->len, PBUF_REF, &p->pbuf, (void *)aio->pbuf, aio->len);
    if (!pbuf)
        return -ENOMEM;

    ethernetif_input(netif, pbuf);

    return 0;
}

/*
 * @brief create a "esp_socket" and bind it to target net card
 *
 * @param name net card name pointer
 * @param netif LWIP net interface pointer
 *
 * @return 0 if success or others if failed
 */
static int tcpip_adapter_bind_netcard(const char *name, struct netif *netif)
{
    int s, ret;

    s = esp_socket(AF_PACKET, SOCK_RAW, ETH_P_ALL);
    if (s < 0) {
        TCPIP_ATAPTER_LOG("create socket of (AF_PACKET, SOCK_RAW, ETH_P_ALL) error\n");
        return -1;
    }

    ret = esp_ioctl(s, SIOCGIFINDEX, name);
    if (ret) {
        TCPIP_ATAPTER_LOG("bind socket %d to netcard %s error\n", s, name);
        esp_close(s);
        return -1;
    }

    ret = esp_aio_event(s, ESP_SOCKET_RECV_EVENT, tcpip_adapter_recv_cb, netif);
    if (ret) {
        TCPIP_ATAPTER_LOG("socket %d register receive callback function %p error\n", s, tcpip_adapter_recv_cb);
        esp_close(s);
        return -1;
    }

    return s;
}

void tcpip_adapter_start(uint8_t netif_index, bool authed)
{
    if (!TCPIP_ADAPTER_IF_VALID(netif_index)) {
        TCPIP_ATAPTER_LOG("ERROR bad netif index:%d\n", netif_index);
        return;
    }

    TCPIP_ATAPTER_LOG("start netif[%d]\n", netif_index);

    if (netif_index == TCPIP_ADAPTER_IF_STA) {
        if (authed == 0) {
            if (esp_netif[netif_index] == NULL) {
                int s;
                const char *netcard_name = "sta0";

                esp_netif[netif_index] = (struct netif*)os_zalloc(sizeof(*esp_netif[netif_index]));
                TCPIP_ATAPTER_LOG("Malloc netif:%d\n", netif_index);
                TCPIP_ATAPTER_LOG("Add netif:%d\n", netif_index);

                s = tcpip_adapter_bind_netcard(netcard_name, esp_netif[netif_index]);
                if (s < 0) {
                    TCPIP_ATAPTER_LOG("TCPIP adapter bind net card %s error\n", netcard_name);
                    return ;
                }

                netif_add(esp_netif[netif_index], NULL, NULL, NULL, (void *)s, ethernetif_init, tcpip_input);
            }
        } else {
            if (dhcpc_flag) {
                printf("dhcp client start...\n");
                tcpip_adapter_station_dhcp_start();
            } else {
                if (esp_ip[TCPIP_ADAPTER_IF_STA].ip.addr != 0) {
                    netif_set_addr(esp_netif[netif_index], &esp_ip[TCPIP_ADAPTER_IF_STA].ip,
                                   &esp_ip[TCPIP_ADAPTER_IF_STA].netmask, &esp_ip[TCPIP_ADAPTER_IF_STA].gw);
                    netif_set_up(esp_netif[netif_index]);
                    system_station_got_ip_set();
                    printf("ip: 0.0.0.0,mask: 0.0.0.0,gw: 0.0.0.0\n");
                } else {
                    printf("check your static ip\n");
                }
            }
        }
    } else if (netif_index == TCPIP_ADAPTER_IF_AP) {
        if (dhcps_flag) {
            IP4_ADDR(&esp_ip[TCPIP_ADAPTER_IF_AP].ip, 192, 168 , 4, 1);
            IP4_ADDR(&esp_ip[TCPIP_ADAPTER_IF_AP].gw, 192, 168 , 4, 1);
            IP4_ADDR(&esp_ip[TCPIP_ADAPTER_IF_AP].netmask, 255, 255 , 255, 0);
        }

        if (esp_netif[netif_index] == NULL) {
            int s;
            const char *netcard_name = "ap0";

            TCPIP_ATAPTER_LOG("Malloc netif:%d\n", netif_index);
            esp_netif[netif_index] = (struct netif*)os_zalloc(sizeof(*esp_netif[netif_index]));

            s = tcpip_adapter_bind_netcard(netcard_name, esp_netif[netif_index]);
            if (s < 0) {
                TCPIP_ATAPTER_LOG("TCPIP adapter bind net card %s error\n", netcard_name);
                return ;
            }

            netif_add(esp_netif[netif_index], &esp_ip[TCPIP_ADAPTER_IF_AP].ip,
                      &esp_ip[TCPIP_ADAPTER_IF_AP].netmask, &esp_ip[TCPIP_ADAPTER_IF_AP].gw, (void *)s, ethernetif_init, tcpip_input);
        }

        if (dhcps_flag) {
            dhcps_start(&esp_ip[TCPIP_ADAPTER_IF_AP]);
            printf("dhcp server start:(");
            printf("ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR, IP2STR((ip_2_ip4(&esp_netif[TCPIP_ADAPTER_IF_AP]->ip_addr))),
                   IP2STR((ip_2_ip4(&esp_netif[TCPIP_ADAPTER_IF_AP]->netmask))), IP2STR((ip_2_ip4(&esp_netif[TCPIP_ADAPTER_IF_AP]->gw))));
            printf(")\n");
        }

        netif_set_up(esp_netif[netif_index]);
        netif_set_default(esp_netif[netif_index]);
    }

    uint8_t opmode = wifi_get_opmode();

    if (opmode == STATION_MODE) {
        netif_set_default(esp_netif[netif_index]);
    }
}

void tcpip_adapter_stop(uint8_t netif_index)
{
    if (!TCPIP_ADAPTER_IF_VALID(netif_index)) {
        TCPIP_ATAPTER_LOG("ERROR bad netif index:%d\n", netif_index);
        return;
    }
    if (esp_netif[netif_index] == NULL)
        return;

    if (netif_index == TCPIP_ADAPTER_IF_STA) {
        TCPIP_ATAPTER_LOG("dhcp stop netif index:%d\n", netif_index);
        dhcp_stop(esp_netif[netif_index]);
    }

    if (netif_index == TCPIP_ADAPTER_IF_AP) {
        if(dhcps_flag){
            TCPIP_ATAPTER_LOG("dhcp stop netif index:%d\n", netif_index);
            dhcps_stop();
        }
    }

    TCPIP_ATAPTER_LOG("stop netif[%d]\n", netif_index);
    esp_close((int)esp_netif[netif_index]->state);
    netif_remove(esp_netif[netif_index]);
    os_free(esp_netif[netif_index]);
    esp_netif[netif_index] = NULL;
}

bool wifi_set_ip_info(WIFI_INTERFACE netif_index, struct ip_info* if_ip)
{
    if (!TCPIP_ADAPTER_IF_VALID((uint8_t)netif_index)) {
        TCPIP_ATAPTER_LOG("ERROR bad netif index:%d\n", netif_index);
        return false;
    }

    TCPIP_ATAPTER_LOG("Set netif[%d] ip info\n", netif_index);
    netif_set_addr(esp_netif[netif_index], &if_ip->ip,  &if_ip->netmask, &if_ip->gw);
    return true;
}

bool wifi_get_ip_info(WIFI_INTERFACE netif_index, struct ip_info* if_ip)
{
    if (!TCPIP_ADAPTER_IF_VALID((uint8_t)netif_index)) {
        TCPIP_ATAPTER_LOG("ERROR bad netif index:%d\n", netif_index);
        return false;
    }

    if(if_ip == NULL){
        TCPIP_ATAPTER_LOG("ERROR ip info is NULL\n");
        return false;        
    }

    TCPIP_ATAPTER_LOG("Get netif[%d] ip info\n", netif_index);
    if_ip->ip.addr = ip_addr_get_ip4_u32(&esp_netif[netif_index]->ip_addr);
    if_ip->netmask.addr = ip_addr_get_ip4_u32(&esp_netif[netif_index]->netmask);
    if_ip->gw.addr = ip_addr_get_ip4_u32(&esp_netif[netif_index]->gw);
    return true;
}

bool wifi_create_linklocal_ip(uint8_t netif_index, bool ipv6)
{
    if (!TCPIP_ADAPTER_IF_VALID(netif_index)) {
        TCPIP_ATAPTER_LOG("ERROR bad netif index:%d\n", netif_index);
        return false;
    }

    netif_create_ip6_linklocal_address(esp_netif[netif_index], ipv6);
    return true;
}

#if LWIP_IPV6
bool wifi_get_linklocal_ip(uint8_t netif_index, ip6_addr_t* linklocal)
{
    if (TCPIP_ADAPTER_IF_VALID(netif_index)) {
        memcpy(linklocal, ip_2_ip6(&esp_netif[netif_index]->ip6_addr[0]), sizeof(*linklocal));
    } else {
        TCPIP_ATAPTER_LOG("ERROR bad netif index:%d\n", netif_index);
        return false;
    }

    return true;
}

bool wifi_get_ipinfo_v6(uint8_t netif_index, uint8_t ip_index, ip6_addr_t* ipv6)
{

    if (TCPIP_ADAPTER_IF_VALID(netif_index)) {
        memcpy(ipv6, &esp_netif[netif_index]->ip6_addr[ip_index], sizeof(ip6_addr_t));
    } else {
        TCPIP_ATAPTER_LOG("ERROR bad netif index:%d\n", netif_index);
        return false;
    }

    return true;
}
#endif

bool wifi_softap_dhcps_start(void)
{
    uint8_t opmode = NULL_MODE;
    TCPIP_ATAPTER_LOG("start softap dhcps\n");
    taskENTER_CRITICAL();
    opmode = wifi_get_opmode();

    if ((opmode == STATION_MODE) || (opmode == NULL_MODE)) {
        taskEXIT_CRITICAL();
        TCPIP_ATAPTER_LOG("ERROR you shoud enable wifi softap before start dhcp server\n");
        return false;
    }

    if (dhcps_flag == false) {
        struct ip_info ipinfo;
        wifi_get_ip_info(SOFTAP_IF, &ipinfo);
        TCPIP_ATAPTER_LOG("start softap dhcpserver\n");
        dhcps_start(&ipinfo);
    }

    dhcps_flag = true;
    taskEXIT_CRITICAL();
    return true;
}

enum dhcp_status wifi_softap_dhcps_status()
{
    return dhcps_flag;
}

void tcpip_adapter_sta_leave()
{
    TCPIP_ATAPTER_LOG("station leave\n");

    if (esp_netif[TCPIP_ADAPTER_IF_STA] == NULL) {
        return;
    }

    netif_set_down(esp_netif[TCPIP_ADAPTER_IF_STA]);

    if (dhcpc_flag) {
        dhcp_release(esp_netif[TCPIP_ADAPTER_IF_STA]);
        dhcp_stop(esp_netif[TCPIP_ADAPTER_IF_STA]);
        dhcp_cleanup(esp_netif[TCPIP_ADAPTER_IF_STA]);
    }

    ip_addr_set_zero(&esp_netif[TCPIP_ADAPTER_IF_STA]->ip_addr);
    ip_addr_set_zero(&esp_netif[TCPIP_ADAPTER_IF_STA]->netmask);
    ip_addr_set_zero(&esp_netif[TCPIP_ADAPTER_IF_STA]->gw);
}

bool wifi_softap_dhcps_stop()
{
    uint8_t opmode = NULL_MODE;
    taskENTER_CRITICAL();
    opmode = wifi_get_opmode();

    if ((opmode == STATION_MODE) || (opmode == NULL_MODE)) {
        taskEXIT_CRITICAL();
        TCPIP_ATAPTER_LOG("ERROR you shoud enable wifi softap before start dhcp server\n");
        return false;
    }

    if (dhcps_flag == true) {
        TCPIP_ATAPTER_LOG("dhcps stop\n");
        dhcps_stop();
    }

    dhcps_flag = false;
    taskEXIT_CRITICAL();
    return true;

}

bool wifi_station_dhcpc_start()
{
    uint8_t opmode = NULL_MODE;
    s8 ret;
    taskENTER_CRITICAL();
    opmode = wifi_get_opmode();

    if ((opmode == SOFTAP_MODE) || (opmode == NULL_MODE)) {
        taskEXIT_CRITICAL();
        TCPIP_ATAPTER_LOG("ERROR you shoud enable wifi station mode before start dhcp client\n");
        return false;
    }

    if (dhcpc_flag == false) {
        if (netif_is_up(esp_netif[TCPIP_ADAPTER_IF_STA])) {
            ip_addr_set_zero(&esp_netif[TCPIP_ADAPTER_IF_STA]->ip_addr);
            ip_addr_set_zero(&esp_netif[TCPIP_ADAPTER_IF_STA]->netmask);
            ip_addr_set_zero(&esp_netif[TCPIP_ADAPTER_IF_STA]->gw);
        } else {
            taskEXIT_CRITICAL();
            TCPIP_ATAPTER_LOG("ERROR please init station netif\n");
            return false;
        }

        ret = dhcp_start(esp_netif[TCPIP_ADAPTER_IF_STA]);

        if (ret != ERR_OK) {
            taskEXIT_CRITICAL();
            TCPIP_ATAPTER_LOG("ERROR start dhcp client failed.ret=%d\n", ret);
            return false;
        }
    }

    dhcps_flag = true;
    taskEXIT_CRITICAL();
    TCPIP_ATAPTER_LOG("dhcp client start\n");
    return true;
}

bool wifi_station_dhcpc_stop()
{
    uint8_t opmode = NULL_MODE;
    taskENTER_CRITICAL();
    opmode = wifi_get_opmode();

    if ((opmode == SOFTAP_MODE) || (opmode == NULL_MODE)) {
        taskEXIT_CRITICAL();
        TCPIP_ATAPTER_LOG("ERROR you shoud enable wifi station mode before stop dhcp client\n");
        return false;
    }

    if (dhcpc_flag == true) {
        dhcp_stop(esp_netif[TCPIP_ADAPTER_IF_STA]);
    } else {
        TCPIP_ATAPTER_LOG("WARING dhcp client have not start yet\n");
    }

    dhcpc_flag = false;
    taskEXIT_CRITICAL();
    TCPIP_ATAPTER_LOG("stop dhcp client\n");
    return true;
}

enum dhcp_status wifi_station_dhcpc_status()
{
    return dhcpc_flag;
}

bool wifi_station_dhcpc_set_maxtry(uint8_t num)
{
    return true;
}

bool tcpip_adapter_set_macaddr(uint8_t netif_index, uint8_t* macaddr)
{
    if (esp_netif[netif_index] == NULL || macaddr == NULL) {
        TCPIP_ATAPTER_LOG("set macaddr fail\n");
        return false;
    }

    memcpy(esp_netif[netif_index]->hwaddr, macaddr, 6);
    TCPIP_ATAPTER_LOG("set macaddr ok\n");
    return true;
}

bool tcpip_adapter_get_macaddr(uint8_t netif_index, uint8_t* macaddr)
{
    if (esp_netif[netif_index] == NULL || macaddr == NULL) {
        return false;
    }

    if (esp_netif[netif_index]->hwaddr[0] == 0 && esp_netif[netif_index]->hwaddr[1] == 0
            && esp_netif[netif_index]->hwaddr[2] == 0 && esp_netif[netif_index]->hwaddr[3] == 0
            && esp_netif[netif_index]->hwaddr[4] == 0 && esp_netif[netif_index]->hwaddr[5] == 0)
    return false;
    
    memcpy(macaddr, esp_netif[netif_index]->hwaddr, 6);
    return true;
}


bool wifi_station_set_hostname(char* name)
{
    if (name == NULL) {
        return false;
    }

    uint32 len = strlen(name);

    if (len > 32) {
        return false;
    }

    uint8_t opmode = wifi_get_opmode();

    if (opmode == STATION_MODE || opmode == STATIONAP_MODE) {
        default_hostname = 0;

        if (hostname != NULL) {
            free(hostname);
            hostname = NULL;
        }

        hostname = (char*)malloc(len + 1);

        if (hostname != NULL) {
            strcpy(hostname, name);
            esp_netif[opmode - 1]->hostname = hostname;
        } else {
            return false;
        }
    } else {
        return false;
    }

    return true;
}

struct netif* eagle_lwip_getif(uint8_t netif_index)
{
    if (TCPIP_ADAPTER_IF_VALID(netif_index)) {
        return esp_netif[netif_index];
    } else {
        TCPIP_ATAPTER_LOG("ERROR bad netif index:%d\n", netif_index);
        return NULL;
    }
}

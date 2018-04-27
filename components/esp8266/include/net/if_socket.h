// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
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

#ifndef _IF_ETHER_H
#define _IF_ETHER_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * undefine some global macro
 */
#undef ETH_P_ALL
#undef ETH_P_ARP
#undef ETH_P_IP

#undef AF_PACKET

#undef SOCK_RAW

/*
 * socket domain
 */
#define AF_PACKET 0

/*
 * socket type
 */
#define SOCK_RAW 0

/*
 * use 16(2^4) type for socket
 */ 
#define IF_SOCK_SHIFT           4
#define IF_SOCK_TYPE(d)         (d & 0xf)
#define IF_SOCK_DATA(t, s)      ((1 << (s + IF_SOCK_SHIFT)) + t)

enum if_sock_type {
    IF_SOCK_ETH = 0,
    IF_SOCK_WIFI,

    IF_SOCK_MAX,
};

#define IF_ETH_DATA(s)          IF_SOCK_DATA(IF_SOCK_ETH, s)
#define IF_WIFI_DATA(s)         IF_SOCK_DATA(IF_SOCK_WIFI, s)

/*
 * socket protocol
 */
#define ETH_P_IP                IF_ETH_DATA(0)
#define ETH_P_ARP               IF_ETH_DATA(1)
#define ETH_P_ALL               ETH_P_IP | ETH_P_ARP

#define WIFI_P_MNG              IF_WIFI_DATA(0)
#define WIFI_P_CTL              IF_WIFI_DATA(1)
#define WIFI_P_DAT              IF_WIFI_DATA(2)
#define WIFI_P_ALL              WIFI_P_MNG | WIFI_P_CTL | WIFI_P_DAT

#ifdef __cplusplus
}
#endif

#endif /* _IF_ETHER_H */

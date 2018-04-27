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

#ifndef _IF_PACKET_H
#define _IF_PACKET_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * socket II type address used by low-level module whose address is base on MAC address
 *
 * Note: Now it just support 802.3 and 802.11 protocol, so only "sll_addr" works here
 */
struct sockaddr_ll {
    unsigned short      sll_family;     // it must be AF_PACKET
    unsigned short      sll_protocol;   // physics level protocol
    int                 sll_ifindex;    // interface index not socket ID
    unsigned short      sll_hatype;     // ARP hardware address type
    unsigned char       sll_pkttype;    // packet type
    unsigned char       sll_halen;      // hardware address length
    unsigned char       sll_addr[8];    // address data
};

#ifdef __cplusplus
}
#endif

#endif /* _IF_PACKET_H */

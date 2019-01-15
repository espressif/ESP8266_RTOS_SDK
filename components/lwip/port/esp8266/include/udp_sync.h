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

#ifndef _UDP_SYNC_H
#define _UDP_SYNC_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * @brief initialize UDP sync module
 */
void udp_sync_init(void);

/*
 * @brief register a UDP API message(struct api_msg) to module
 * 
 * @param in_msg message pointer
 */
void udp_sync_regitser_sock(void *in_msg);

/*
 * @brief ack the message
 * 
 * @param in_msg message pointer
 */
void udp_sync_ack_sock(void *in_msg);

/*
 * @brief set the current message send result
 *
 * @param netif LwIP netif pointer
 * @param pbuf low-level netif output pbuf pointer
 * @param ret current message send result
 */
void udp_sync_set_ret(void *netif, void *pbuf, int ret);

void udp_sync_cache_udp(void *pcb);

void udp_sync_clear_udp(void);

/*
 * @brief process the sync
 */
void udp_sync_proc(void);

/*
 * @brief trigger a UDP sync process
 */
void udp_sync_trigger(void);

/**
 * @brief close the udp pcb
 * 
 * @param udp_pcb LwIP raw UDP pcb pointer
 */
void udp_sync_close_udp(void *udp_pcb);

/**
 * @brief close the udp sync netconn before close the socket
 * 
 * @param netconn LwIP raw netconn pointer
 */ 
void udp_sync_close_netconn(void *netconn);

#ifdef __cplusplus
}
#endif

#endif /* _UDP_SYNC_H */

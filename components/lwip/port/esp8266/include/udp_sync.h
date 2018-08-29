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
void udp_sync_regitser(void *in_msg);

/*
 * @brief ack the message
 * 
 * @param in_msg message pointer
 */
void udp_sync_ack(void *in_msg);

/*
 * @brief set the current message send result
 * 
 * @param ret current message send result
 */
void udp_sync_set_ret(void *netif, int ret);

/*
 * @brief process the sync
 * 
 * @param ret current message send result
 */
void udp_sync_proc(void);

/*
 * @brief trigger a UDP sync process
 */
void udp_sync_trigger(void);

#ifdef __cplusplus
}
#endif

#endif /* _UDP_SYNC_H */

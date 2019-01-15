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

#ifndef _ESP_SOCKET_H
#define _ESP_SOCKET_H

#include <stdint.h>

#include "esp_aio.h"
#include "esp_module.h"
#include "net/if_packet.h"
#include "net/if_socket.h"

#ifdef __cplusplus
extern "C" {
#endif

/* used for socket link */
#define ESP_SOCKET_METHOD_BASENAME        "esp_socket"
#define ESP_SOCKET_METHOD_NAME(sec)       __DECLARE_SYMBOL_NAME(ESP_SOCKET_METHOD_BASENAME)
#define ESP_SOCKET_METHOD_DECLARE(sym)    __DECLARE_SYMBOL(esp_socket_method_t, sym, ESP_SOCKET_METHOD_BASENAME)

#ifndef socklen_t
typedef uint32_t socklen_t;
#endif

#define esp_socket_va_start(va, arg)      va = (va_list)arg

/*
 * socket async event
 */
enum esp_socket_event_type {
    ESP_SOCKET_CONNECT_EVENT = 0,
    ESP_SOCKET_ACCEPT_EVENT = ESP_SOCKET_CONNECT_EVENT,
    ESP_SOCKET_RECV_EVENT = 1,
    ESP_SOCKET_ERROR_EVENT = 2,

    ESP_SOCKET_MAX_EVENT
};

/*
 * socket information object
 */
typedef struct esp_socket_info {
    int     domain;
    int     type;
    int     protocol;
} esp_socket_info_t;

/*
 * socket method object
 */
typedef struct esp_socket_method {
    /*
     * socket method name
     */
    const char *name;

    /*
     * @brief open lowlevel socket module
     *
     * @param info socket information
     *
     * @return return lowlevel socket index if successfully, otherwise return NULL
     *         and you can get the real fail reason by "errno"
     */
    void* (*open)(esp_socket_info_t *info);

    /*
     * @brief send a block of data asynchronously and receive result by callback function
     *
     * @param index lowlevel index
     * @param aio asynchronous I/O controlling block
     * @param to target address with lowlevel address data format
     * @param len target address data length
     *
     * @return return 0 if successfully, otherwise return -1 and you can get the real fail
     *         reason by "errno"
     */
    int (*aio_sendto)(void *index, esp_aio_t *aio, const struct sockaddr_ll *to, socklen_t len);

    /*
     * @brief register an event and its callback function to lowlevel socket module
     *
     * @param index lowlevel index
     * @param event asynchronous I/O event
     * @param cb callback function
     * @param arg callback function parameter
     *
     * @return return 0 if successfully, otherwise return -1 and you can get the real fail
     *         reason by "errno"
     */
    int (*aio_event)(void *index, unsigned int event, esp_aio_cb_t cb, void *arg);

    /*
     * @brief free buffer taken from event callback
     * 
     * @param index lowlevel index
     * @param pbuf buffer pointer
     * 
     * @return return 0 if successfully, otherwise return -1 and you can get the real fail
     *         reason by "errno"
     */
    int (*free_pbuf)(void *index, void *pbuf);

    /*
     * @brief send request command to lowlevel socket module and get the result synchronously
     *
     * @param index lowlevel index
     * @param cmd request command
     * @param arg start address to variable parameters
     *
     * @return return 0 if successfully, otherwise return -1 and you can get the real fail
     *         reason by "errno"
     */
    int (*ioctl)(void *index, unsigned int cmd, void *arg);

    /*
     * @brief close lowlevel socket module
     *
     * @param index lowlevel index
     *
     * @return return 0 if successfully, otherwise return -1 and you can get the real fail
     *         reason by "errno"
     */
    int (*close)(void *index);
} esp_socket_method_t;

/*
 * @brief free an aio control block by calling the callback function
 * 
 * @param aio asynchronous I/O controlling block
 * @param aio data handling result
 * 
 * @return none
 */
static inline void esp_aio_free(esp_aio_t *aio, int status)
{
    if (aio->cb) {
        aio->ret = status;
        aio->cb(aio);
    }
}

/*
 * @brief drop an aio control block by disable "pbuf" and "cb"
 *        and then the aio control block has no meaning
 * 
 * @param aio asynchronous I/O controlling block
 * 
 * @return none
 */
static inline void esp_aio_drop(esp_aio_t *aio)
{
    aio->pbuf = NULL;
    aio->cb = NULL;
}

/*
 * @brief create a socket file description
 *
 * @param domain protocal domain and it must be "AF_PACKET" now
 * @param type socket type and it must be "SOCK_RAW" now
 * @param protocol target protocol and it must be "ETH_P_ALL" now
 *
 * @return return 0 if successfully, otherwise return -1 and you can get the real fail
 *         reason by "errno"
 *
 * @note using "AF_PACKET", "SOCK_RAW" and "ETH_P_ALL" means that you receive and send full
 *       ethernet II type message with full message head: " destination mac | source mac | type"
 */
int esp_socket(int domain, int type, int protocol);

/*
 * @brief send a block of data asynchronously and receive result by callback function
 *
 * @param aio asynchronous I/O controlling block
 * @param to target address with lowlevel address data format
 * @param len target address data length
 *
 * @return return 0 if successfully, otherwise return -1 and you can get the real fail
 *         reason by "errno"
 */
int esp_aio_sendto(esp_aio_t *aio, const struct sockaddr_ll *to, socklen_t len);

/*
 * @brief register an event and its callback function to target of file description
 *
 * @param fd file description
 * @param event asynchronous I/O event
 * @param cb callback function
 * @param arg callback function parameter
 *
 * @return return 0 if successfully, otherwise return -1 and you can get the real fail
 *         reason by "errno"
 */
int esp_aio_event(int fd, unsigned int event, esp_aio_cb_t cb, void *arg);

/*
 * @brief lowlevel socket module upload event and its data
 *
 * @param index lowlevel index
 * @param info socket information
 * @param event asynchronous I/O event
 * @param aio asynchronous I/O controlling block
 *
 * @return return 0 if successfully, otherwise return -1 and you can get the real fail
 *         reason by "errno"
 */
int esp_upload_event(void *index, esp_socket_info_t *info, unsigned int event, esp_aio_data_t *aio_data);

/*
 * @brief free buffer taken from event callback
 * 
 * @param fd file description
 * @param pbuf buffer pointer
 * 
 * @return return 0 if successfully, otherwise return -1 and you can get the real fail
 *         reason by "errno"
 */
int esp_free_pbuf(int fd, void *pbuf);

/*
 * @brief send request command to target by file description and get the result synchronously
 *
 * @param fd file description
 * @param cmd request command
 * @param ... realy command parameters and it must be related to realy object of lowlevel
 *
 * @return return 0 if successfully, otherwise return -1 and you can get the real fail
 *         reason by "errno"
 */
int esp_ioctl(int fd, unsigned int cmd, ...);

/*
 * @brief close target of file description
 *
 * @param fd file description
 *
 * @return return 0 if successfully, otherwise return -1 and you can get the real fail
 *         reason by "errno"
 */
int esp_close(int fd);

#ifdef __cplusplus
}
#endif

#endif /* _ESP_SOCKET_H */

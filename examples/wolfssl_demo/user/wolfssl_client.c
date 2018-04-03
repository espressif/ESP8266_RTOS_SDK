/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <stddef.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sys/socket.h"
#include "netdb.h"

#include <wolfssl/ssl.h>

#define WOLFSSL_DEMO_THREAD_NAME        "wolfssl_client"
#define WOLFSSL_DEMO_THREAD_STACK_WORDS 512
#define WOLFSSL_DEMO_THREAD_PRORIOTY    6

#define WOLFSSL_DEMO_TARGET_NAME        "www.baidu.com"
#define WOLFSSL_DEMO_TARGET_PORT        443

#define WOLFSSL_DEMO_SNTP_SERVERS       "pool.ntp.org"

#define WOLFSSL_EXAMPLE_REQUEST         "{\"path\": \"/v1/ping/\", \"method\": \"GET\"}\r\n"

const char send_data[] = WOLFSSL_EXAMPLE_REQUEST;
const int send_bytes = sizeof(send_data);
char recv_data[1024] = {0};

static void wolfssl_client(void* pv)
{
    int ret = 0;

    uint32_t current_timestamp = 0;
    const portTickType xDelay = 500 / portTICK_RATE_MS;
    WOLFSSL_CTX *ctx = NULL;
    WOLFSSL *ssl = NULL;

    int socket = -1;
    struct sockaddr_in sock_addr;
    struct hostent *entry = NULL;

    /*enable sntp for sync the time*/
    sntp_setoperatingmode(0);
    sntp_setservername(0, WOLFSSL_DEMO_SNTP_SERVERS);
    sntp_init();

    do {
        current_timestamp = sntp_get_current_timestamp();
        vTaskDelay(xDelay);
    } while (current_timestamp == 0);

    /*get addr info for hostname*/
    do {
        entry = gethostbyname(WOLFSSL_DEMO_TARGET_NAME);
        vTaskDelay(xDelay);
    } while(entry == NULL);

    ret = wolfSSL_Init();
    if (ret != WOLFSSL_SUCCESS) {
        goto failed1;
    }

    ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());
    if (!ctx) {
        goto failed1;
    }

    socket = socket(AF_INET, SOCK_STREAM, 0);
    if (socket < 0) {
        goto failed2;
    }

    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(WOLFSSL_DEMO_TARGET_PORT);
    memcpy(&sock_addr.sin_addr.s_addr, entry->h_addr_list[0], entry->h_length);

    ret = connect(socket, (struct sockaddr*)&sock_addr, sizeof(sock_addr));
    if (ret) {
        goto failed3;
    }

    ssl = wolfSSL_new(ctx);
    if (!ssl) {
        goto failed3;
    }

    wolfSSL_set_fd(ssl, socket);

    wolfSSL_set_verify(ssl, WOLFSSL_VERIFY_NONE, NULL);

    ret = wolfSSL_connect(ssl);
    if (!ret) {
        goto failed4;
    }

    ret = wolfSSL_write(ssl, send_data, send_bytes);
    if (ret <= 0) {
        goto failed5;
    }

    ret = wolfSSL_read(ssl, recv_data, sizeof(recv_data));

    printf("receive: %s\n",recv_data);

failed5:
    wolfSSL_shutdown(ssl);
failed4:
    wolfSSL_free(ssl);
failed3:
    close(socket);
failed2:
    wolfSSL_CTX_free(ctx);
failed1:
    wolfSSL_Cleanup();
    vTaskDelete(NULL);

    return;
}

void user_conn_init(void)
{
    int ret;

    ret = xTaskCreate(wolfssl_client,
            WOLFSSL_DEMO_THREAD_NAME,
            WOLFSSL_DEMO_THREAD_STACK_WORDS,
            NULL,
            WOLFSSL_DEMO_THREAD_PRORIOTY,
            NULL);

    if (ret != pdPASS)  {
        printf("create thread %s failed\n", WOLFSSL_DEMO_THREAD_NAME);
        return ;
    }
}



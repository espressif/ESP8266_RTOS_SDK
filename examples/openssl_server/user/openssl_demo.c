/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2017 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
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
#include "openssl_demo.h"
#include "openssl/ssl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "espressif/c_types.h"
#include "espressif/esp_misc.h"
#include "lwip/sockets.h"
#include "ssl_server_crt.h"

#define OPENSSL_DEMO_THREAD_NAME "ssl_demo"
#define OPENSSL_DEMO_THREAD_STACK_WORDS 2048
#define OPENSSL_DEMO_THREAD_PRORIOTY 6

/*
Fragment size range 2048~8192
| Private key len | Fragment size recommend |
| RSA2048         | 2048                    |
| RSA3072         | 3072                    |
| RSA4096         | 4096                    |
*/
#define OPENSSL_DEMO_FRAGMENT_SIZE 2048

/* Local server tcp port */
#define OPENSSL_DEMO_LOCAL_TCP_PORT 443

#define OPENSSL_DEMO_REQUEST "{\"path\": \"/v1/ping/\", \"method\": \"GET\"}\r\n"

/* receive length */
#define OPENSSL_DEMO_RECV_BUF_LEN 1024

LOCAL xTaskHandle openssl_handle;

LOCAL char send_data[] = OPENSSL_DEMO_REQUEST;
LOCAL int send_bytes = sizeof(send_data);

LOCAL char recv_buf[OPENSSL_DEMO_RECV_BUF_LEN];

LOCAL void openssl_demo_thread(void* p)
{
    int ret;

    SSL_CTX* ctx;
    SSL* ssl;

    struct sockaddr_in sock_addr;
    int sockfd, new_sockfd;
    int recv_bytes = 0;
    socklen_t addr_len;

    printf("OpenSSL demo thread start...\n");

    printf("create SSL context ......");
    ctx = SSL_CTX_new(TLSv1_1_server_method());

    if (!ctx) {
        printf("failed\n");
        goto failed1;
    }

    printf("OK\n");

    printf("load ca crt ......");
    X509* cacrt = d2i_X509(NULL, ca_crt, ca_crt_len);

    if (cacrt) {
        SSL_CTX_add_client_CA(ctx, cacrt);
        printf("OK\n");
    } else {
        printf("failed\n");
        goto failed2;
    }

    printf("load server crt ......");
    ret = SSL_CTX_use_certificate_ASN1(ctx, server_crt_len, server_crt);

    if (ret) {
        printf("OK\n");
    } else {
        printf("failed\n");
        goto failed2;
    }

    printf("load server private key ......");
    ret = SSL_CTX_use_PrivateKey_ASN1(0, ctx, server_key, server_key_len);

    if (ret) {
        printf("OK\n");
    } else {
        printf("failed\n");
        goto failed2;
    }

    printf("set verify mode verify peer\n");
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

    printf("set SSL context read buffer size ......OK\n");
    SSL_CTX_set_default_read_buffer_len(ctx, OPENSSL_DEMO_FRAGMENT_SIZE);

    printf("create socket ......");
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        printf("failed\n");
        goto failed2;
    }

    printf("OK\n");

    printf("socket bind ......");
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = 0;
    sock_addr.sin_port = htons(OPENSSL_DEMO_LOCAL_TCP_PORT);

    ret = bind(sockfd, (struct sockaddr*)&sock_addr, sizeof(sock_addr));

    if (ret) {
        printf("bind failed\n");
        goto failed3;
    }

    printf("bind OK\n");

    printf("server socket listen ......");
    ret = listen(sockfd, 32);

    if (ret) {
        printf("failed\n");
        goto failed3;
    }

    printf("OK\n");

reconnect:
    printf("SSL server create ......");
    ssl = SSL_new(ctx);

    if (!ssl) {
        printf("failed\n");
        goto failed3;
    }

    printf("OK\n");

    printf("SSL server socket accept client ......");
    new_sockfd = accept(sockfd, (struct sockaddr*)&sock_addr, &addr_len);

    if (new_sockfd < 0) {
        printf("failed");
        goto failed4;
    }

    printf("OK\n");

    SSL_set_fd(ssl, new_sockfd);

    printf("SSL server accept client ......");
    ret = SSL_accept(ssl);

    if (!ret) {
        printf("failed\n");
        goto failed5;
    }

    printf("OK\n");

    do {
        ret = SSL_read(ssl, recv_buf, OPENSSL_DEMO_RECV_BUF_LEN - 1);

        if (ret <= 0) {
            break;
        }

        recv_bytes += ret;
        recv_buf[ret] = '\0';
        printf("%s", recv_buf);
    } while (1);

    SSL_shutdown(ssl);
failed5:
    close(new_sockfd);
    new_sockfd = -1;
failed4:
    SSL_free(ssl);
    ssl = NULL;
    goto reconnect;
failed3:
    close(sockfd);
    sockfd = -1;
failed2:
    SSL_CTX_free(ctx);
    ctx = NULL;
failed1:
    vTaskDelete(NULL);
    printf("task exit\n");

    return ;
}

void user_conn_init(void)
{
    int ret;

    ret = xTaskCreate(openssl_demo_thread,
                      OPENSSL_DEMO_THREAD_NAME,
                      OPENSSL_DEMO_THREAD_STACK_WORDS,
                      NULL,
                      OPENSSL_DEMO_THREAD_PRORIOTY,
                      &openssl_handle);

    if (ret != pdPASS)  {
        printf("create thread %s failed\n", OPENSSL_DEMO_THREAD_NAME);
        return ;
    }
}


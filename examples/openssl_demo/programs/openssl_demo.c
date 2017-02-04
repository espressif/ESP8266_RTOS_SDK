#include <stddef.h>
#include "openssl_demo.h"
#include "openssl/ssl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "espressif/c_types.h"
#include "lwip/sockets.h"

#define OPENSSL_DEMO_THREAD_NAME "ssl_demo"
#define OPENSSL_DEMO_THREAD_STACK_WORDS 2048
#define OPENSSL_DEMO_THREAD_PRORIOTY 6

#define OPENSSL_DEMO_FRAGMENT_SIZE 8192

#define OPENSSL_DEMO_LOCAL_TCP_PORT 1000

#define OPENSSL_DEMO_TARGET_NAME "www.baidu.com"
#define OPENSSL_DEMO_TARGET_TCP_PORT 443

#define OPENSSL_DEMO_REQUEST "{\"path\": \"/v1/ping/\", \"method\": \"GET\"}\r\n"

#define OPENSSL_DEMO_RECV_BUF_LEN 1024

LOCAL xTaskHandle openssl_handle;

LOCAL char send_data[] = OPENSSL_DEMO_REQUEST;
LOCAL int send_bytes = sizeof(send_data);

LOCAL char recv_buf[OPENSSL_DEMO_RECV_BUF_LEN];

LOCAL void openssl_demo_thread(void *p)
{
    int ret;

    SSL_CTX *ctx;
    SSL *ssl;

    int socket;
    struct sockaddr_in sock_addr;

    ip_addr_t target_ip;

    int recv_bytes = 0;

    os_printf("OpenSSL demo thread start...\n");

    do {
        ret = netconn_gethostbyname(OPENSSL_DEMO_TARGET_NAME, &target_ip);
    } while(ret);
    os_printf("get target IP is %d.%d.%d.%d\n", (unsigned char)((target_ip.addr & 0x000000ff) >> 0),
                                                (unsigned char)((target_ip.addr & 0x0000ff00) >> 8),
                                                (unsigned char)((target_ip.addr & 0x00ff0000) >> 16),
                                                (unsigned char)((target_ip.addr & 0xff000000) >> 24));

    os_printf("create SSL context ......");
    ctx = SSL_CTX_new(TLSv1_1_client_method());
    if (!ctx) {
        os_printf("failed\n");
        goto failed1;
    }
    os_printf("OK\n");

    os_printf("set SSL context read buffer size ......");
    SSL_CTX_set_default_read_buffer_len(ctx, OPENSSL_DEMO_FRAGMENT_SIZE);
    ret = 0;
    if (ret) {
        os_printf("failed, return %d\n", ret);
        goto failed2;
    }
    os_printf("OK\n");

    os_printf("create socket ......");
    socket = socket(AF_INET, SOCK_STREAM, 0);
    if (socket < 0) {
        os_printf("failed\n");
        goto failed3;
    }
    os_printf("OK\n");

    os_printf("bind socket ......");
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = 0;
    sock_addr.sin_port = htons(OPENSSL_DEMO_LOCAL_TCP_PORT);
    ret = bind(socket, (struct sockaddr*)&sock_addr, sizeof(sock_addr));
    if (ret) {
        os_printf("failed\n");
        goto failed4;
    }
    os_printf("OK\n");

    os_printf("socket connect to remote ......");
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = target_ip.addr;
    sock_addr.sin_port = htons(OPENSSL_DEMO_TARGET_TCP_PORT);
    ret = connect(socket, (struct sockaddr*)&sock_addr, sizeof(sock_addr));
    if (ret) {
        os_printf("failed\n", OPENSSL_DEMO_TARGET_NAME);
        goto failed5;
    }
    os_printf("OK\n");

    os_printf("create SSL ......");
    ssl = SSL_new(ctx);
    if (!ssl) {
        os_printf("failed\n");
        goto failed6;
    }
    os_printf("OK\n");

    SSL_set_fd(ssl, socket);

    os_printf("SSL connected to %s port %d ......", OPENSSL_DEMO_TARGET_NAME, OPENSSL_DEMO_TARGET_TCP_PORT);
    ret = SSL_connect(ssl);
    if (!ret) {
        os_printf("failed, return [-0x%x]\n", -ret);
        goto failed7;
    }
    os_printf("OK\n");

    os_printf("send request to %s port %d ......", OPENSSL_DEMO_TARGET_NAME, OPENSSL_DEMO_TARGET_TCP_PORT);
    ret = SSL_write(ssl, send_data, send_bytes);
    if (ret <= 0) {
        os_printf("failed, return [-0x%x]\n", -ret);
        goto failed8;
    }
    os_printf("OK\n\n");

    do {
        ret = SSL_read(ssl, recv_buf, OPENSSL_DEMO_RECV_BUF_LEN - 1);
        if (ret <= 0) {
            break;
        }
        recv_bytes += ret;
        os_printf("%s", recv_buf);
    } while (1);
    os_printf("read %d bytes data from %s ......\n", recv_bytes, OPENSSL_DEMO_TARGET_NAME);

failed8:
    SSL_shutdown(ssl);
failed7:
    SSL_free(ssl);
failed6:
failed5:
failed4:
    close(socket);
failed3:
failed2:
    SSL_CTX_free(ctx);
failed1:
    vTaskDelete(NULL);

    os_printf("task exit\n");

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
        os_printf("create thread %s failed\n", OPENSSL_DEMO_THREAD_NAME);
        return ;
    }
}


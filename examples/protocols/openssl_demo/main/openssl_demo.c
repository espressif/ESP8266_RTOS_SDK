/* openSSL demo example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stddef.h>
#include <string.h>
#include <strings.h>

#include "esp_sta.h"
#include "esp_system.h"
#include "esp_wifi.h"

#include "openssl/ssl.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lwip/sockets.h"
#include "lwip/api.h"

#include "user_config.h"

#define OPENSSL_DEMO_THREAD_NAME "ssl_demo"
#define OPENSSL_DEMO_THREAD_STACK_WORDS 2048
#define OPENSSL_DEMO_THREAD_PRORIOTY 6

#define OPENSSL_DEMO_FRAGMENT_SIZE 8192

#define OPENSSL_DEMO_LOCAL_TCP_PORT 1000

#define OPENSSL_DEMO_TARGET_NAME "www.baidu.com"
#define OPENSSL_DEMO_TARGET_TCP_PORT 443

#define OPENSSL_DEMO_REQUEST "{\"path\": \"/v1/ping/\", \"method\": \"GET\"}\r\n"

#define OPENSSL_DEMO_RECV_BUF_LEN 1024

static xTaskHandle openssl_handle;

static char send_data[] = OPENSSL_DEMO_REQUEST;
static int send_bytes = sizeof(send_data);

static char recv_buf[OPENSSL_DEMO_RECV_BUF_LEN];

static void openssl_demo_thread(void* p)
{
    int ret;

    SSL_CTX* ctx;
    SSL* ssl;

    int socket;
    struct sockaddr_in sock_addr;

    ip_addr_t target_ip;

    int recv_bytes = 0;

    printf("OpenSSL demo thread start...\n");

    do {
        ret = netconn_gethostbyname(OPENSSL_DEMO_TARGET_NAME, &target_ip);
    } while (ret);

    printf("get target IP is %d.%d.%d.%d\n", (unsigned char)((target_ip.u_addr.ip4.addr & 0x000000ff) >> 0),
           (unsigned char)((target_ip.u_addr.ip4.addr & 0x0000ff00) >> 8),
           (unsigned char)((target_ip.u_addr.ip4.addr & 0x00ff0000) >> 16),
           (unsigned char)((target_ip.u_addr.ip4.addr & 0xff000000) >> 24));

    printf("create SSL context ......");
    ctx = SSL_CTX_new(TLSv1_1_client_method());

    if (!ctx) {
        printf("failed\n");
        goto failed1;
    }

    printf("OK\n");

    printf("set SSL context read buffer size ......");
    SSL_CTX_set_default_read_buffer_len(ctx, OPENSSL_DEMO_FRAGMENT_SIZE);
    ret = 0;

    if (ret) {
        printf("failed, return %d\n", ret);
        goto failed2;
    }

    printf("OK\n");

    printf("create socket ......");
    socket = socket(AF_INET, SOCK_STREAM, 0);

    if (socket < 0) {
        printf("failed\n");
        goto failed3;
    }

    printf("OK\n");

    printf("bind socket ......");
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = 0;
    sock_addr.sin_port = htons(OPENSSL_DEMO_LOCAL_TCP_PORT);
    ret = bind(socket, (struct sockaddr*)&sock_addr, sizeof(sock_addr));

    if (ret) {
        printf("failed\n");
        goto failed4;
    }

    printf("OK\n");

    printf("socket connect to remote ......");
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = target_ip.u_addr.ip4.addr;
    sock_addr.sin_port = htons(OPENSSL_DEMO_TARGET_TCP_PORT);
    ret = connect(socket, (struct sockaddr*)&sock_addr, sizeof(sock_addr));

    if (ret) {
        printf("failed\n");
        goto failed5;
    }

    printf("OK\n");

    printf("create SSL ......");
    ssl = SSL_new(ctx);

    if (!ssl) {
        printf("failed\n");
        goto failed6;
    }

    printf("OK\n");

    SSL_set_fd(ssl, socket);

    printf("SSL connected to %s port %d ......", OPENSSL_DEMO_TARGET_NAME, OPENSSL_DEMO_TARGET_TCP_PORT);
    ret = SSL_connect(ssl);

    if (!ret) {
        printf("failed, return [-0x%x]\n", -ret);
        goto failed7;
    }

    printf("OK\n");

    printf("send request to %s port %d ......", OPENSSL_DEMO_TARGET_NAME, OPENSSL_DEMO_TARGET_TCP_PORT);
    ret = SSL_write(ssl, send_data, send_bytes);

    if (ret <= 0) {
        printf("failed, return [-0x%x]\n", -ret);
        goto failed8;
    }

    printf("OK\n\n");

    do {
        ret = SSL_read(ssl, recv_buf, OPENSSL_DEMO_RECV_BUF_LEN - 1);

        if (ret <= 0) {
            break;
        }

        recv_bytes += ret;
        printf("%s", recv_buf);
    } while (1);

    printf("read %d bytes data from %s ......\n", recv_bytes, OPENSSL_DEMO_TARGET_NAME);

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

/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32_t user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();
    uint32_t rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        case FLASH_SIZE_64M_MAP_1024_1024:
            rf_cal_sec = 2048 - 5;
            break;

        case FLASH_SIZE_128M_MAP_1024_1024:
            rf_cal_sec = 4096 - 5;
            break;

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

void wifi_event_handler_cb(System_Event_t* event)
{
    if (event == NULL) {
        return;
    }

    switch (event->event_id) {
        case EVENT_STAMODE_GOT_IP:
            printf("sta got ip , creat task %d\n", system_get_free_heap_size());
            user_conn_init();
            break;

        default:
            break;
    }
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
    printf("SDK version:%s %d\n", system_get_sdk_version(), system_get_free_heap_size());
    wifi_set_opmode(STATION_MODE);

    {
        // set AP parameter
        struct station_config config;
        bzero(&config, sizeof(struct station_config));
        sprintf((char*)config.ssid, SSID);
        sprintf((char*)config.password, PASSWORD);
        wifi_station_set_config(&config);
    }

    wifi_set_event_handler_cb(wifi_event_handler_cb);
}

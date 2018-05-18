/* openSSL server example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "sdkconfig.h"

#include "esp_misc.h"
#include "esp_sta.h"
#include "esp_system.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <sys/socket.h>

#if CONFIG_SSL_USING_WOLFSSL
#include "lwip/apps/sntp.h"
#endif

#include "openssl/ssl.h"

#define OPENSSL_SERVER_THREAD_NAME "openssl_server"
#define OPENSSL_SERVER_THREAD_STACK_WORDS 2048
#define OPENSSL_SERVER_THREAD_PRORIOTY 6

extern const uint8_t ca_pem_start[] asm("_binary_ca_pem_start");
extern const uint8_t ca_pem_end[]   asm("_binary_ca_pem_end");
extern const uint8_t server_pem_start[] asm("_binary_server_pem_start");
extern const uint8_t server_pem_end[]   asm("_binary_server_pem_end");
extern const uint8_t server_key_start[] asm("_binary_server_key_start");
extern const uint8_t server_key_end[]   asm("_binary_server_key_end");

/*
Fragment size range 2048~8192
| Private key len | Fragment size recommend |
| RSA2048         | 2048                    |
| RSA3072         | 3072                    |
| RSA4096         | 4096                    |
*/
#define OPENSSL_SERVER_FRAGMENT_SIZE 2048

/* Local server tcp port */
#define OPENSSL_SERVER_LOCAL_TCP_PORT 443

#define OPENSSL_SERVER_REQUEST "{\"path\": \"/v1/ping/\", \"method\": \"GET\"}\r\n"

/* receive length */
#define OPENSSL_SERVER_RECV_BUF_LEN 1024

static xTaskHandle openssl_handle;

static char send_data[] = OPENSSL_SERVER_REQUEST;
static int send_bytes = sizeof(send_data);

static char recv_buf[OPENSSL_SERVER_RECV_BUF_LEN];

#if CONFIG_SSL_USING_WOLFSSL
static void get_time()
{
    struct timeval now;
    int sntp_retry_cnt = 0;
    int sntp_retry_time = 0;

    sntp_setoperatingmode(0);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();

    while (1) {
        for (int32_t i = 0; (i < (SNTP_RECV_TIMEOUT / 100)) && now.tv_sec < 1525952900; i++) {
            vTaskDelay(100 / portTICK_RATE_MS);
            gettimeofday(&now, NULL);
        }

        if (now.tv_sec < 1525952900) {
            sntp_retry_time = SNTP_RECV_TIMEOUT << sntp_retry_cnt;

            if (SNTP_RECV_TIMEOUT << (sntp_retry_cnt + 1) < SNTP_RETRY_TIMEOUT_MAX) {
                sntp_retry_cnt ++;
            }

            printf("SNTP get time failed, retry after %d ms\n", sntp_retry_time);
            vTaskDelay(sntp_retry_time / portTICK_RATE_MS);
        } else {
            printf("SNTP get time success\n");
            break;
        }
    }
}
#endif

static void openssl_server_thread(void* p)
{
    int ret;

    SSL_CTX* ctx;
    SSL* ssl;

    struct sockaddr_in sock_addr;
    int sockfd, new_sockfd;
    int recv_bytes = 0;
    socklen_t addr_len;

    printf("OpenSSL server thread start...\n");

#if CONFIG_SSL_USING_WOLFSSL
    /* CA date verification need system time */
    get_time();
#endif

    printf("create SSL context ......");
    ctx = SSL_CTX_new(TLSv1_2_server_method());

    if (!ctx) {
        printf("failed\n");
        goto failed1;
    }

    printf("OK\n");

    printf("load ca crt ......");
    ret = SSL_CTX_load_verify_buffer(ctx, ca_pem_start, ca_pem_end - ca_pem_start);

    if (ret) {
        printf("OK\n");
    } else {
        printf("failed\n");
        goto failed2;
    }

    printf("load server crt ......");
    ret = SSL_CTX_use_certificate_ASN1(ctx, server_pem_end - server_pem_start, server_pem_start);

    if (ret) {
        printf("OK\n");
    } else {
        printf("failed\n");
        goto failed2;
    }

    printf("load server private key ......");
    ret = SSL_CTX_use_PrivateKey_ASN1(0, ctx, server_key_start, server_key_end - server_key_start);

    if (ret) {
        printf("OK\n");
    } else {
        printf("failed\n");
        goto failed2;
    }

    printf("set verify mode verify peer\n");
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

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
    sock_addr.sin_port = htons(OPENSSL_SERVER_LOCAL_TCP_PORT);

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

    printf("send data to client ......");
    ret = SSL_write(ssl, send_data, send_bytes);

    if (ret <= 0) {
        printf("failed, return [-0x%x]\n", -ret);
        goto failed5;
    }

    printf("OK\n\n");

    do {
        ret = SSL_read(ssl, recv_buf, OPENSSL_SERVER_RECV_BUF_LEN - 1);

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

    ret = xTaskCreate(openssl_server_thread,
                      OPENSSL_SERVER_THREAD_NAME,
                      OPENSSL_SERVER_THREAD_STACK_WORDS,
                      NULL,
                      OPENSSL_SERVER_THREAD_PRORIOTY,
                      &openssl_handle);

    if (ret != pdPASS)  {
        printf("create thread %s failed\n", OPENSSL_SERVER_THREAD_NAME);
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
        sprintf((char*)config.ssid, CONFIG_WIFI_SSID);
        sprintf((char*)config.password, CONFIG_WIFI_PASSWORD);
        wifi_station_set_config(&config);
    }

    wifi_set_event_handler_cb(wifi_event_handler_cb);
}

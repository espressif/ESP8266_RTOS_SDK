/* wolfSSL example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>

#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"

#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include <sys/socket.h>
#include <netdb.h>
#include "lwip/apps/sntp.h"

#include "wolfssl/ssl.h"

/* The examples use simple WiFi configuration that you can set via
   'make menuconfig'.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_WIFI_SSID CONFIG_WIFI_SSID
#define EXAMPLE_WIFI_PASS CONFIG_WIFI_PASSWORD

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;

#if CONFIG_CERT_AUTH
extern const uint8_t server_root_cert_pem_start[] asm("_binary_server_root_cert_pem_start");
extern const uint8_t server_root_cert_pem_end[]   asm("_binary_server_root_cert_pem_end");
#endif

/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "www.howsmyssl.com"
#define WEB_PORT 443
#define WEB_URL "https://www.howsmyssl.com/a/check"

#define REQUEST "GET " WEB_URL " HTTP/1.0\r\n" \
    "Host: "WEB_SERVER"\r\n" \
    "User-Agent: esp-idf/1.0 espressif\r\n" \
    "\r\n"

#define WOLFSSL_DEMO_THREAD_NAME        "wolfssl_client"
#define WOLFSSL_DEMO_THREAD_STACK_WORDS 8192
#define WOLFSSL_DEMO_THREAD_PRORIOTY    6

#define WOLFSSL_DEMO_SNTP_SERVERS       "pool.ntp.org"

static const char *TAG = "example";

const char send_data[] = REQUEST;
const int32_t send_bytes = sizeof(send_data);
char recv_data[1024] = {0};

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_WIFI_SSID,
            .password = EXAMPLE_WIFI_PASS,
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

static void get_time()
{
    struct timeval now;
    int sntp_retry_cnt = 0;
    int sntp_retry_time = 0;

    sntp_setoperatingmode(0);
    sntp_setservername(0, WOLFSSL_DEMO_SNTP_SERVERS);
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

static void wolfssl_client(void* pv)
{
    int32_t ret = 0;

    const portTickType xDelay = 500 / portTICK_RATE_MS;
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL* ssl = NULL;

    int32_t socket = -1;
    struct sockaddr_in sock_addr;
    struct hostent* entry = NULL;

    /* CA date verification need system time */
    get_time();

    while (1) {

        printf("Setting hostname for TLS session...\n");

        /*get addr info for hostname*/
        do {
            entry = gethostbyname(WEB_SERVER);
            vTaskDelay(xDelay);
        } while (entry == NULL);

        printf("Init wolfSSL...\n");
        ret = wolfSSL_Init();

        if (ret != WOLFSSL_SUCCESS) {
            printf("Init wolfSSL failed:%d...\n", ret);
            goto failed1;
        }

        printf("Set wolfSSL ctx ...\n");
        ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());

        if (!ctx) {
            printf("Set wolfSSL ctx failed...\n");
            goto failed1;
        }

        printf("Creat socket ...\n");
        socket = socket(AF_INET, SOCK_STREAM, 0);

        if (socket < 0) {
            printf("Creat socket failed...\n");
            goto failed2;
        }

#if CONFIG_CERT_AUTH
        printf("Loading the CA root certificate...\n");
        ret = wolfSSL_CTX_load_verify_buffer(ctx, server_root_cert_pem_start, server_root_cert_pem_end - server_root_cert_pem_start, WOLFSSL_FILETYPE_PEM);

        if (WOLFSSL_SUCCESS != ret) {
            printf("Loading the CA root certificate failed...\n");
            goto failed3;
        }

        wolfSSL_CTX_set_verify(ctx, WOLFSSL_VERIFY_PEER, NULL);
#else
        wolfSSL_CTX_set_verify(ctx, WOLFSSL_VERIFY_NONE, NULL);
#endif

        memset(&sock_addr, 0, sizeof(sock_addr));
        sock_addr.sin_family = AF_INET;
        sock_addr.sin_port = htons(WEB_PORT);
        sock_addr.sin_addr.s_addr = ((struct in_addr*)(entry->h_addr))->s_addr;

        printf("Connecting to %s:%d...\n", WEB_SERVER, WEB_PORT);
        ret = connect(socket, (struct sockaddr*)&sock_addr, sizeof(sock_addr));

        if (ret) {
            printf("Connecting to %s:%d failed: %d\n", WEB_SERVER, WEB_PORT, ret);
            goto failed3;
        }

        printf("Create wolfSSL...\n");
        ssl = wolfSSL_new(ctx);

        if (!ssl) {
            printf("Create wolfSSL failed...\n");
            goto failed3;
        }

        wolfSSL_set_fd(ssl, socket);

        printf("Performing the SSL/TLS handshake...\n");
        ret = wolfSSL_connect(ssl);

        if (WOLFSSL_SUCCESS != ret) {
            printf("Performing the SSL/TLS handshake failed:%d\n", ret);
            goto failed4;
        }

        printf("Writing HTTPS request...\n");
        ret = wolfSSL_write(ssl, send_data, send_bytes);

        if (ret <= 0) {
            printf("Writing HTTPS request failed:%d\n", ret);
            goto failed5;
        }

        printf("Reading HTTPS response...\n");

        do {
            ret = wolfSSL_read(ssl, recv_data, sizeof(recv_data));


            if (ret <= 0) {
                printf("\nConnection closed\n");
                break;
            }

            /* Print response directly to stdout as it is read */
            for (int i = 0; i < ret; i++) {
                printf("%c", recv_data[i]);
            }
        } while (1);

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

        for (int countdown = 10; countdown >= 0; countdown--) {
            printf("%d...\n", countdown);
            vTaskDelay(1000 / portTICK_RATE_MS);
        }

        printf("Starting again!\n");
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK( nvs_flash_init() );
    initialise_wifi();
    xTaskCreate(wolfssl_client,
                WOLFSSL_DEMO_THREAD_NAME,
                WOLFSSL_DEMO_THREAD_STACK_WORDS,
                NULL,
                WOLFSSL_DEMO_THREAD_PRORIOTY,
                NULL);
}

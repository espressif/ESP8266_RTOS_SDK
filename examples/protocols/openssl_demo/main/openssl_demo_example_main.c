/* openSSL demo example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stddef.h>
#include <string.h>
#include <strings.h>

#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include <sys/socket.h>
#include <netdb.h>

#include "openssl/ssl.h"

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

static const char *TAG = "example";

#define OPENSSL_DEMO_LOCAL_TCP_PORT 9999

#define OPENSSL_DEMO_TARGET_NAME "www.baidu.com"
#define OPENSSL_DEMO_TARGET_TCP_PORT 443

#define OPENSSL_DEMO_REQUEST "GET / HTTP/1.1\r\n\r\n"

#define OPENSSL_DEMO_RECV_BUF_LEN 1024

static char send_data[] = OPENSSL_DEMO_REQUEST;
static int send_bytes = sizeof(send_data);

static char recv_buf[OPENSSL_DEMO_RECV_BUF_LEN];

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    /* For accessing reason codes in case of disconnection */
    system_event_info_t *info = &event->event_info;

    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;

        case SYSTEM_EVENT_STA_GOT_IP:
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
            break;

        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGE(TAG, "Disconnect reason : %d", info->disconnected.reason);
            if (info->disconnected.reason == WIFI_REASON_BASIC_RATE_NOT_SUPPORT) {
                /*Switch to 802.11 bgn mode */
                esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
            }
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
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_WIFI_SSID,
            .password = EXAMPLE_WIFI_PASS,
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void openssl_task(void *p)
{
    int ret;

    SSL_CTX *ctx;
    SSL *ssl;

    int socket;
    struct sockaddr_in sock_addr;
    struct hostent *entry = NULL;

    int recv_bytes = 0;

    ESP_LOGI(TAG, "OpenSSL demo thread start...");

    /* Wait for the callback to set the CONNECTED_BIT in the
       event group.
    */
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                        false, true, portMAX_DELAY);
    ESP_LOGI(TAG, "Connected to AP");

    /*get addr info for hostname*/
    do {
        entry = gethostbyname(OPENSSL_DEMO_TARGET_NAME);
        vTaskDelay(500 / portTICK_RATE_MS);
    } while (entry == NULL);

    ESP_LOGI(TAG, "get target IP is %d.%d.%d.%d", (unsigned char)((((struct in_addr *)(entry->h_addr))->s_addr & 0x000000ff) >> 0),
           (unsigned char)((((struct in_addr *)(entry->h_addr))->s_addr & 0x0000ff00) >> 8),
           (unsigned char)((((struct in_addr *)(entry->h_addr))->s_addr & 0x00ff0000) >> 16),
           (unsigned char)((((struct in_addr *)(entry->h_addr))->s_addr & 0xff000000) >> 24));

    ESP_LOGI(TAG, "create SSL context ......");
    ctx = SSL_CTX_new(TLSv1_1_client_method());

    if (!ctx) {
        ESP_LOGI(TAG, "failed");
        goto failed1;
    }

    ESP_LOGI(TAG, "OK");

    // The client will verify the certificate received from the server during the handshake.
    // This is turned on by default in wolfSSL.
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);

    ESP_LOGI(TAG, "create socket ......");
    socket = socket(AF_INET, SOCK_STREAM, 0);

    if (socket < 0) {
        ESP_LOGI(TAG, "failed");
        goto failed2;
    }

    ESP_LOGI(TAG, "OK");

    ESP_LOGI(TAG, "bind socket ......");
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = 0;
    sock_addr.sin_port = htons(OPENSSL_DEMO_LOCAL_TCP_PORT);
    ret = bind(socket, (struct sockaddr *)&sock_addr, sizeof(sock_addr));

    if (ret) {
        ESP_LOGI(TAG, "failed");
        goto failed3;
    }

    ESP_LOGI(TAG, "OK");

    ESP_LOGI(TAG, "socket connect to remote ......");
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = ((struct in_addr *)(entry->h_addr))->s_addr;
    sock_addr.sin_port = htons(OPENSSL_DEMO_TARGET_TCP_PORT);
    ret = connect(socket, (struct sockaddr *)&sock_addr, sizeof(sock_addr));

    if (ret) {
        ESP_LOGI(TAG, "failed\n");
        goto failed4;
    }

    ESP_LOGI(TAG, "OK");

    ESP_LOGI(TAG, "create SSL ......");
    ssl = SSL_new(ctx);

    if (!ssl) {
        ESP_LOGI(TAG, "failed");
        goto failed5;
    }

    ESP_LOGI(TAG, "OK");

    SSL_set_fd(ssl, socket);

    ESP_LOGI(TAG, "SSL connected to %s port %d ......", OPENSSL_DEMO_TARGET_NAME, OPENSSL_DEMO_TARGET_TCP_PORT);
    ret = SSL_connect(ssl);

    if (!ret) {
        ESP_LOGI(TAG, "failed, return [-0x%x]", -ret);
        goto failed6;
    }

    ESP_LOGI(TAG, "OK");

    ESP_LOGI(TAG,  "send request to %s port %d ......", OPENSSL_DEMO_TARGET_NAME, OPENSSL_DEMO_TARGET_TCP_PORT);
    ret = SSL_write(ssl, send_data, send_bytes);

    if (ret <= 0) {
        ESP_LOGI(TAG, "failed, return [-0x%x]", -ret);
        goto failed7;
    }

    ESP_LOGI(TAG, "OK\n");

    do {
        ret = SSL_read(ssl, recv_buf, OPENSSL_DEMO_RECV_BUF_LEN - 1);

        if (ret <= 0) {
            break;
        }

        recv_bytes += ret;
        ESP_LOGI(TAG, "%s", recv_buf);
    } while (1);

    ESP_LOGI(TAG, "read %d bytes data from %s ......", recv_bytes, OPENSSL_DEMO_TARGET_NAME);

failed7:
    SSL_shutdown(ssl);
failed6:
    SSL_free(ssl);
failed5:
failed4:
failed3:
    close(socket);
failed2:
    SSL_CTX_free(ctx);
failed1:
    vTaskDelete(NULL);

    ESP_LOGI(TAG, "task exit");

    return ;
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    initialise_wifi();
    xTaskCreate(&openssl_task, "openssl_task", 8192, NULL, 5, NULL);
}

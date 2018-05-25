/* mbedtls example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <strings.h>

#include "sdkconfig.h"

#include "esp_misc.h"
#include "esp_sta.h"
#include "esp_system.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <sys/socket.h>
#include <netdb.h>

#include "lwip/apps/sntp.h"

#include "mbedtls/platform.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"

extern const uint8_t server_root_cert_pem_start[] asm("_binary_server_root_cert_pem_start");
extern const uint8_t server_root_cert_pem_end[]   asm("_binary_server_root_cert_pem_end");

#define EXAMPLE_WIFI_SSID CONFIG_WIFI_SSID
#define EXAMPLE_WIFI_PASS CONFIG_WIFI_PASSWORD

/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "www.howsmyssl.com"
#define WEB_PORT "443"
#define WEB_URL "https://www.howsmyssl.com/a/check"

static const char* REQUEST = "GET " WEB_URL " HTTP/1.0\r\n"
                             "Host: "WEB_SERVER"\r\n"
                             "User-Agent: esp-idf/1.0 espressif\r\n"
                             "\r\n";

#define HTTPS_MBEDTLS_THREAD_NAME        "https_mbedtls"
#define HTTPS_MBEDTLS_THREAD_STACK_WORDS 2048
#define HTTPS_MBEDTLS_THREAD_PRORIOTY    6

#define HTTPS_MBEDTLS_SNTP_SERVERS       "pool.ntp.org"

static void https_get_task()
{
    char buf[512];
    int ret, flags, len;

    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ssl;
    mbedtls_x509_crt cacert;
    mbedtls_ssl_config conf;
    mbedtls_net_context server_fd;

    mbedtls_ssl_init(&ssl);
    mbedtls_x509_crt_init(&cacert);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    printf("Seeding the random number generator\n");

    mbedtls_ssl_config_init(&conf);

    mbedtls_entropy_init(&entropy);

    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                     NULL, 0)) != 0) {
        printf("mbedtls_ctr_drbg_seed returned %d\n", ret);
        vTaskDelete(NULL);
        return;
    }

    printf("Loading the CA root certificate...\n");

    ret = mbedtls_x509_crt_parse(&cacert, server_root_cert_pem_start, server_root_cert_pem_end - server_root_cert_pem_start);

    if (ret < 0) {
        printf("mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
        vTaskDelete(NULL);
        return;
    }

    printf("Setting hostname for TLS session...\n");

    /* Hostname set here should match CN in server certificate */
    if ((ret = mbedtls_ssl_set_hostname(&ssl, WEB_SERVER)) != 0) {
        printf("mbedtls_ssl_set_hostname returned -0x%x", -ret);
        vTaskDelete(NULL);
        return;
    }

    printf("Setting up the SSL/TLS structure...\n");

    if ((ret = mbedtls_ssl_config_defaults(&conf,
                                           MBEDTLS_SSL_IS_CLIENT,
                                           MBEDTLS_SSL_TRANSPORT_STREAM,
                                           MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
        printf("mbedtls_ssl_config_defaults returned %d", ret);
        goto exit;
    }

    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
    mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);

    if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0) {
        printf("mbedtls_ssl_setup returned -0x%x\n\n", -ret);
        goto exit;
    }

    while (1) {
        mbedtls_net_init(&server_fd);

        printf("Connecting to %s:%s...\n", WEB_SERVER, WEB_PORT);

        if ((ret = mbedtls_net_connect(&server_fd, WEB_SERVER,
                                       WEB_PORT, MBEDTLS_NET_PROTO_TCP)) != 0) {
            printf("mbedtls_net_connect returned -%x\n", -ret);
            goto exit;
        }

        printf("Connected.\n");

        mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

        printf("Performing the SSL/TLS handshake...\n");

        while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
            if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
                printf("mbedtls_ssl_handshake returned -0x%x\n", -ret);
                goto exit;
            }
        }

        printf("Verifying peer X.509 certificate...\n");

        if ((flags = mbedtls_ssl_get_verify_result(&ssl)) != 0) {
            /* In real life, we probably want to close connection if ret != 0 */
            printf("Failed to verify peer certificate!\n");
            bzero(buf, sizeof(buf));
            mbedtls_x509_crt_verify_info(buf, sizeof(buf), "  ! ", flags);
            printf("verification info: %s\n", buf);
        } else {
            printf("Certificate verified.\n");
        }

        printf("Cipher suite is %s\n", mbedtls_ssl_get_ciphersuite(&ssl));

        printf("Writing HTTP request...\n");

        size_t written_bytes = 0;

        do {
            ret = mbedtls_ssl_write(&ssl,
                                    (const unsigned char*)REQUEST + written_bytes,
                                    strlen(REQUEST) - written_bytes);

            if (ret >= 0) {
                printf("%d bytes written\n", ret);
                written_bytes += ret;
            } else if (ret != MBEDTLS_ERR_SSL_WANT_WRITE && ret != MBEDTLS_ERR_SSL_WANT_READ) {
                printf("mbedtls_ssl_write returned -0x%x\n", -ret);
                goto exit;
            }
        } while (written_bytes < strlen(REQUEST));

        printf("Reading HTTP response...\n");

        do {
            len = sizeof(buf) - 1;
            bzero(buf, sizeof(buf));
            ret = mbedtls_ssl_read(&ssl, (unsigned char*)buf, len);

            if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
                continue;
            }

            if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
                ret = 0;
                break;
            }

            if (ret < 0) {
                printf("mbedtls_ssl_read returned -0x%x\n", -ret);
                break;
            }

            if (ret == 0) {
                printf("\nconnection closed\n");
                break;
            }

            len = ret;

            /* Print response directly to stdout as it is read */
            for (int i = 0; i < len; i++) {
                printf("%c", buf[i]);
            }
        } while (1);

        mbedtls_ssl_close_notify(&ssl);

exit:
        mbedtls_ssl_session_reset(&ssl);
        mbedtls_net_free(&server_fd);

        if (ret != 0) {
            mbedtls_strerror(ret, buf, 100);
            printf("Last error was: -0x%x - %s\n", -ret, buf);
        }

        printf("\n"); // JSON output doesn't have a newline at end

        static int request_count;
        printf("Completed %d requests\n", ++request_count);

        for (int countdown = 10; countdown >= 0; countdown--) {
            printf("%d...\n", countdown);
            vTaskDelay(1000 / portTICK_RATE_MS);
        }

        printf("Starting again!\n");
    }
}

void user_conn_init(void)
{
    int32_t ret;

    ret = xTaskCreate(https_get_task,
                      HTTPS_MBEDTLS_THREAD_NAME,
                      HTTPS_MBEDTLS_THREAD_STACK_WORDS,
                      NULL,
                      HTTPS_MBEDTLS_THREAD_PRORIOTY,
                      NULL);

    if (ret != pdPASS)  {
        printf("create thread %s failed\n", HTTPS_MBEDTLS_THREAD_NAME);
        return ;
    }
}

void wifi_event_handler_cb(System_Event_t* event)
{
    if (event == NULL) {
        return;
    }

    switch (event->event_id) {
        case EVENT_STAMODE_GOT_IP:
            printf("sta got ip\n");
            user_conn_init();
            break;

        default:
            break;
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

void user_init()
{
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

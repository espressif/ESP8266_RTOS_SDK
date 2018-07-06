/* CoAP client Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_wifi.h"
#include "coap.h"
#include "user_config.h"

#define COAP_CLIENT_THREAD_NAME         "coap_client_thread"
#define COAP_CLIENT_THREAD_STACK_WORDS  2048
#define COAP_CLIENT_THREAD_PRIO         8

static xTaskHandle coap_client_handle;

#define COAP_DEFAULT_TIME_SEC 5
#define COAP_DEFAULT_TIME_USEC 0

// for libcirom.a
int _getpid_r(struct _reent *r)
{
    return -1;
 }

int _kill_r(struct _reent *r, int pid, int sig)
{
    return -1;
 }

static void message_handler(struct coap_context_t *ctx, const coap_endpoint_t *local_interface, const coap_address_t *remote,
              coap_pdu_t *sent, coap_pdu_t *received,
                const coap_tid_t id)
{
    unsigned char* data = NULL;
    size_t data_len;
    if (COAP_RESPONSE_CLASS(received->hdr->code) == 2) {
        if (coap_get_data(received, &data_len, &data)) {
            printf("Received: %s\n", data);
        }
    }
}

static void coap_client_example_thread(void *p)
{
    ip_addr_t target_ip;
    int ret = 0;
    coap_context_t*   ctx = NULL;
    coap_address_t    dst_addr, src_addr;
    static coap_uri_t uri;
    fd_set            readfds;
    struct timeval    tv;
    int flags, result;
    coap_pdu_t*       request = NULL;
    const char*       server_uri = COAP_DEFAULT_DEMO_URI;
    uint8_t     get_method = 1;

    while (1) {

        if (coap_split_uri((const uint8_t *)server_uri, strlen(server_uri), &uri) == -1) {
            printf("CoAP server uri error\n");
            break;
        }

        ret = netconn_gethostbyname(COAP_SERVER, &target_ip);

        if (ret != 0) {
            printf("DNS lookup failed\n");
            vTaskDelay(1000 / portTICK_RATE_MS);
            continue;
        }

        coap_address_init(&src_addr);
        src_addr.addr.sin.sin_family      = AF_INET;
        src_addr.addr.sin.sin_port        = htons(0);
        src_addr.addr.sin.sin_addr.s_addr = INADDR_ANY;

        ctx = coap_new_context(&src_addr);
        if (ctx) {
            coap_address_init(&dst_addr);
            dst_addr.addr.sin.sin_family      = AF_INET;
            dst_addr.addr.sin.sin_port        = htons(COAP_DEFAULT_PORT);
            dst_addr.addr.sin.sin_addr.s_addr = target_ip.addr;

            request            = coap_new_pdu();
            if (request){
                request->hdr->type = COAP_MESSAGE_CON;
                request->hdr->id   = coap_new_message_id(ctx);
                request->hdr->code = get_method;
                coap_add_option(request, COAP_OPTION_URI_PATH, uri.path.length, uri.path.s);

                coap_register_response_handler(ctx, message_handler);
                coap_send_confirmed(ctx, ctx->endpoint, &dst_addr, request);

                flags = fcntl(ctx->sockfd, F_GETFL, 0);
                fcntl(ctx->sockfd, F_SETFL, flags|O_NONBLOCK);

                tv.tv_usec = COAP_DEFAULT_TIME_USEC;
                tv.tv_sec = COAP_DEFAULT_TIME_SEC;

                for(;;) {
                    FD_ZERO(&readfds);
                    FD_CLR( ctx->sockfd, &readfds );
                    FD_SET( ctx->sockfd, &readfds );
                    result = select( ctx->sockfd+1, &readfds, 0, 0, &tv );
                    if (result > 0) {
                        if (FD_ISSET( ctx->sockfd, &readfds ))
                            coap_read(ctx);
                    } else if (result < 0) {
                        break;
                    } else {
                        printf("select timeout\n");
                    }
                }
            }
            coap_free_context(ctx);
        }
    }

    vTaskDelete(NULL);
}

void user_conn_init(void)
{
    int ret;
    ret = xTaskCreate(coap_client_example_thread,
                      COAP_CLIENT_THREAD_NAME,
                      COAP_CLIENT_THREAD_STACK_WORDS,
                      NULL,
                      COAP_CLIENT_THREAD_PRIO,
                      &coap_client_handle);

    if (ret != pdPASS)  {
        printf("create coap client thread %s failed\n", COAP_CLIENT_THREAD_NAME);
    }
}

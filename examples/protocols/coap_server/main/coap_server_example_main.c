/* CoAP server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <sys/socket.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"

#include "protocol_examples_common.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "coap.h"

/* Set this to 9 to get verbose logging from within libcoap */
/* If want to change log level num to open log, don't forget to enlarge coap_example_task size*/
#define COAP_LOGGING_LEVEL 0
const static char *TAG = "CoAP_server";
static char espressif_data[100];
static int espressif_data_len = 0;

/*
 * The resource handler
 */
static void
hnd_espressif_get(coap_context_t *ctx, coap_resource_t *resource,
                  coap_session_t *session,
                  coap_pdu_t *request, coap_binary_t *token,
                  coap_string_t *query, coap_pdu_t *response)
{
    coap_add_data_blocked_response(resource, session, request, response, token,
                                   COAP_MEDIATYPE_TEXT_PLAIN, 0,
                                   (size_t)espressif_data_len,
                                   (const u_char *)espressif_data);
}

static void
hnd_espressif_put(coap_context_t *ctx,
                  coap_resource_t *resource,
                  coap_session_t *session,
                  coap_pdu_t *request,
                  coap_binary_t *token,
                  coap_string_t *query,
                  coap_pdu_t *response)
{
    size_t size;
    unsigned char *data;

    coap_resource_notify_observers(resource, NULL);

    if (strcmp (espressif_data, "no data") == 0) {
        response->code = COAP_RESPONSE_CODE(201);
    }
    else {
        response->code = COAP_RESPONSE_CODE(204);
    }

    /* coap_get_data() sets size to 0 on error */
    (void)coap_get_data(request, &size, &data);

    if (size == 0) {      /* re-init */
        snprintf(espressif_data, sizeof(espressif_data), "no data");
        espressif_data_len = strlen(espressif_data);
    } else {
        espressif_data_len = size > sizeof (espressif_data) ? sizeof (espressif_data) : size;
        memcpy (espressif_data, data, espressif_data_len);
    }
}

static void
hnd_espressif_delete(coap_context_t *ctx,
                     coap_resource_t *resource,
                     coap_session_t *session,
                     coap_pdu_t *request,
                     coap_binary_t *token,
                     coap_string_t *query,
                     coap_pdu_t *response)
{
    coap_resource_notify_observers(resource, NULL);
    snprintf(espressif_data, sizeof(espressif_data), "no data");
    espressif_data_len = strlen(espressif_data);
    response->code = COAP_RESPONSE_CODE(202);
}

static int coap_example_join_ipv6(int sock, char *group_name)
{
    struct ipv6_mreq mreq = {0};
    int  netif_index = 0, err = 0;

    netif_index = tcpip_adapter_get_netif_index(TCPIP_ADAPTER_IF_STA);
    if (netif_index < 0) {
        ESP_LOGE(TAG, "Failed to get netif index");
        return ESP_FAIL;
    }
    err = setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_IF, &netif_index, sizeof(uint8_t));
    if (err < 0) {
        ESP_LOGE(TAG, "Failed to set IPV6_MULTICAST_IF. Error %d", errno);
        return ESP_FAIL;
    }
    err = inet6_aton(group_name, &mreq.ipv6mr_multiaddr);
    if (err != 1) {
        ESP_LOGE(TAG, "Configured IPV6 multicast address '%s' is invalid.", group_name);
        return ESP_FAIL;
    }

    if (!IN6_IS_ADDR_MULTICAST(&mreq.ipv6mr_multiaddr)) {
        ESP_LOGW(TAG, "Configured IPV6 multicast address '%s' is not a valid multicast address. This will probably not work.", group_name);
    }
    mreq.ipv6mr_interface = (unsigned int)netif_index;

    err = setsockopt(sock, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &mreq, sizeof(struct ipv6_mreq));
    if (err < 0) {
        ESP_LOGE(TAG, "Failed to set IPV6_ADD_MEMBERSHIP. Error %d", errno);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Configured IPV6 Multicast address %s", group_name);
    return ESP_OK;
}

static int coap_example_join_ipv4(int sock, char *group_name)
{
    struct ip_mreq imreq = { 0 };
    struct in_addr iaddr = { 0 };
    int err = 0;
    // Configure source interface

    tcpip_adapter_ip_info_t ip_info = { 0 };
    err = tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get IP address info. Error 0x%x", err);
        return ESP_FAIL;
    }
    inet_addr_from_ip4addr(&iaddr, &ip_info.ip);

    // Configure multicast address to listen to
    err = inet_aton(group_name, &imreq.imr_multiaddr.s_addr);
    if (err != 1) {
        ESP_LOGE(TAG, "Configured IPV4 multicast address '%s' is invalid.", group_name);
        return ESP_FAIL;
    }

    if (!IP_MULTICAST(ntohl(imreq.imr_multiaddr.s_addr))) {
        ESP_LOGW(TAG, "Configured IPV4 multicast address '%s' is not a valid multicast address. This will probably not work.", group_name);
    }

    // Assign the IPv4 multicast source interface, via its IP
    // (only necessary if this socket is IPV4 only)
    err = setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, &iaddr, sizeof(struct in_addr));
    if (err < 0) {
        ESP_LOGE(TAG, "Failed to set IP_MULTICAST_IF. Error %d", errno);
        return ESP_FAIL;
    }

    err = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &imreq, sizeof(struct ip_mreq));
    if (err < 0) {
        ESP_LOGE(TAG, "Failed to set IP_ADD_MEMBERSHIP. Error %d", errno);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Configured IPV4 Multicast address %s", inet_ntoa(imreq.imr_multiaddr.s_addr));
    return ESP_OK;
}

static void coap_example_thread(void *p)
{
    coap_context_t *ctx = NULL;
    coap_address_t   serv_addr;
    coap_resource_t *resource = NULL;

    snprintf(espressif_data, sizeof(espressif_data), "no data");
    espressif_data_len = strlen(espressif_data);
    coap_set_log_level(COAP_LOGGING_LEVEL);
    while (1) {
        coap_endpoint_t *ep_udp = NULL;
        coap_endpoint_t *ep_tcp = NULL;
        unsigned wait_ms;

        ctx = coap_new_context(NULL);
        if (!ctx) {
           continue;
        }
        /* Prepare the CoAP server socket */
        coap_address_init(&serv_addr);
        serv_addr.addr.sin.sin_family      = AF_INET;
        serv_addr.addr.sin.sin_addr.s_addr = INADDR_ANY;
        serv_addr.addr.sin.sin_port        = htons(COAP_DEFAULT_PORT);
        ep_udp = coap_new_endpoint(ctx, &serv_addr, COAP_PROTO_UDP); //Add IPv4 endpoint
        if (!ep_udp) {
           goto clean_up;
        }
        coap_example_join_ipv4(ep_udp->sock.fd, CONFIG_TARGET_MULTICAST_IPV4);
        ep_tcp = coap_new_endpoint(ctx, &serv_addr, COAP_PROTO_TCP);
        if (!ep_tcp) {
           goto clean_up;
        }

        /* Prepare the CoAP server socket */
        coap_address_init(&serv_addr);
        serv_addr.addr.sin6.sin6_port   = htons(COAP_DEFAULT_PORT);
        serv_addr.addr.sin6.sin6_family = AF_INET6;
        serv_addr.size = sizeof(struct sockaddr_in6);
        bzero(&serv_addr.addr.sin6.sin6_addr.un, sizeof(serv_addr.addr.sin6.sin6_addr.un));
        ep_udp = coap_new_endpoint(ctx, &serv_addr, COAP_PROTO_UDP); //Add IPv6 endpoint
        if (!ep_udp) {
           goto clean_up;
        }
        coap_example_join_ipv6(ep_udp->sock.fd, CONFIG_TARGET_MULTICAST_IPV6);
        ep_tcp = coap_new_endpoint(ctx, &serv_addr, COAP_PROTO_TCP);
        if (!ep_tcp) {
           goto clean_up;
        }

        resource = coap_resource_init(coap_make_str_const("Espressif"), 0);
        if (!resource) {
           goto clean_up;
        }
        coap_register_handler(resource, COAP_REQUEST_GET, hnd_espressif_get);
        coap_register_handler(resource, COAP_REQUEST_PUT, hnd_espressif_put);
        coap_register_handler(resource, COAP_REQUEST_DELETE, hnd_espressif_delete);
        /* We possibly want to Observe the GETs */
        coap_resource_set_get_observable(resource, 1);
        coap_add_resource(ctx, resource);

        wait_ms = COAP_RESOURCE_CHECK_TIME * 1000;

        while (1) {
            int result = coap_run_once(ctx, wait_ms);
            if (result < 0) {
                break;
            } else if (result && (unsigned)result < wait_ms) {
                /* decrement if there is a result wait time returned */
                wait_ms -= result;
            }
            if (result) {
                /* result must have been >= wait_ms, so reset wait_ms */
                wait_ms = COAP_RESOURCE_CHECK_TIME * 1000;
            }
        }
    }
clean_up:
    coap_free_context(ctx);
    coap_cleanup();

    vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_ERROR_CHECK( nvs_flash_init() );
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    xTaskCreate(coap_example_thread, "coap", 1024 * 5, NULL, 5, NULL);
}

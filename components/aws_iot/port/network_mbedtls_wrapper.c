/*
 * Copyright 2010-2015 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * Additions Copyright 2016 Espressif Systems (Shanghai) PTE LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */
#include <sys/param.h>
#include <stdbool.h>
#include <string.h>
#include <timer_platform.h>

#include "aws_iot_config.h"
#include "aws_iot_error.h"
#include "network_interface.h"
#include "network_platform.h"

#include "esp_log.h"
#include "esp_system.h"

#include "esp_tls.h"

static const char *TAG = "aws_iot";

/* This is the value used for ssl read timeout */
#define IOT_SSL_READ_TIMEOUT 10

static void _iot_tls_set_connect_params(Network *pNetwork, const char *pRootCALocation, const char *pDeviceCertLocation,
                                 const char *pDevicePrivateKeyLocation, const char *pDestinationURL,
                                 uint16_t destinationPort, uint32_t timeout_ms, bool ServerVerificationFlag) {
    pNetwork->tlsConnectParams.DestinationPort = destinationPort;
    pNetwork->tlsConnectParams.pDestinationURL = pDestinationURL;
    pNetwork->tlsConnectParams.pDeviceCertLocation = pDeviceCertLocation;
    pNetwork->tlsConnectParams.pDevicePrivateKeyLocation = pDevicePrivateKeyLocation;
    pNetwork->tlsConnectParams.pRootCALocation = pRootCALocation;
    pNetwork->tlsConnectParams.timeout_ms = timeout_ms;
    pNetwork->tlsConnectParams.ServerVerificationFlag = ServerVerificationFlag;
}

IoT_Error_t iot_tls_init(Network *pNetwork, const char *pRootCALocation, const char *pDeviceCertLocation,
                         const char *pDevicePrivateKeyLocation, const char *pDestinationURL,
                         uint16_t destinationPort, uint32_t timeout_ms, bool ServerVerificationFlag) {
    _iot_tls_set_connect_params(pNetwork, pRootCALocation, pDeviceCertLocation, pDevicePrivateKeyLocation,
                                pDestinationURL, destinationPort, timeout_ms, ServerVerificationFlag);

    pNetwork->connect = iot_tls_connect;
    pNetwork->read = iot_tls_read;
    pNetwork->write = iot_tls_write;
    pNetwork->disconnect = iot_tls_disconnect;
    pNetwork->isConnected = iot_tls_is_connected;
    pNetwork->destroy = iot_tls_destroy;

    pNetwork->tlsDataParams.flags = 0;

    return SUCCESS;
}

IoT_Error_t iot_tls_is_connected(Network *pNetwork) {
    /* Use this to add implementation which can check for physical layer disconnect */
    return NETWORK_PHYSICAL_LAYER_CONNECTED;
}

IoT_Error_t iot_tls_connect(Network *pNetwork, TLSConnectParams *params) {
    int ret = SUCCESS;
    TLSDataParams *tlsDataParams = NULL;

    if(NULL == pNetwork) {
        return NULL_VALUE_ERROR;
    }

    if(NULL != params) {
        _iot_tls_set_connect_params(pNetwork, params->pRootCALocation, params->pDeviceCertLocation,
                                    params->pDevicePrivateKeyLocation, params->pDestinationURL,
                                    params->DestinationPort, params->timeout_ms, params->ServerVerificationFlag);
    }

    tlsDataParams = &(pNetwork->tlsDataParams);

    esp_tls_cfg_t cfg = {
        .cacert_pem_buf  = (const unsigned char *)pNetwork->tlsConnectParams.pRootCALocation,
        .cacert_pem_bytes = strlen(pNetwork->tlsConnectParams.pRootCALocation) + 1,
        .clientcert_pem_buf = (const unsigned char *)pNetwork->tlsConnectParams.pDeviceCertLocation,
        .clientcert_pem_bytes = strlen(pNetwork->tlsConnectParams.pDeviceCertLocation) + 1,
        .clientkey_pem_buf = (const unsigned char *)pNetwork->tlsConnectParams.pDevicePrivateKeyLocation,
        .clientkey_pem_bytes = strlen(pNetwork->tlsConnectParams.pDevicePrivateKeyLocation) + 1,
        .timeout_ms = pNetwork->tlsConnectParams.timeout_ms,
        .non_block = true
    };

    /* Use the AWS IoT ALPN extension for MQTT, if port 443 is requested */
    if (pNetwork->tlsConnectParams.DestinationPort == 443) {
        const char *alpnProtocols[] = { "x-amzn-mqtt-ca", NULL };
        cfg.alpn_protos = alpnProtocols;
    }

    rtc_clk_cpu_freq_set(RTC_CPU_FREQ_160M);

    struct esp_tls *tls = esp_tls_conn_new(pNetwork->tlsConnectParams.pDestinationURL, strlen(pNetwork->tlsConnectParams.pDestinationURL), pNetwork->tlsConnectParams.DestinationPort, &cfg);

    if (!tls) {
        ret = SSL_CONNECTION_ERROR;
    }

    tlsDataParams->timeout = pNetwork->tlsConnectParams.timeout_ms;
    tlsDataParams->handle = (esp_network_handle_t)tls;

    rtc_clk_cpu_freq_set(RTC_CPU_FREQ_80M);

    return (IoT_Error_t) ret;
}

IoT_Error_t iot_tls_write(Network *pNetwork, unsigned char *pMsg, size_t len, Timer *timer, size_t *written_len) {
    size_t written_so_far;
    bool isErrorFlag = false;
    int frags, ret = 0;
    TLSDataParams *tlsDataParams = &(pNetwork->tlsDataParams);
    struct esp_tls *tls = (struct esp_tls *)tlsDataParams->handle;
    if (!tls) {
        return NULL_VALUE_ERROR;
    }

    for(written_so_far = 0, frags = 0;
        written_so_far < len && !has_timer_expired(timer); written_so_far += ret, frags++) {
        while(!has_timer_expired(timer) &&
              (ret = esp_tls_conn_write(tls, pMsg + written_so_far, len - written_so_far)) <= 0) {
            if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
                ESP_LOGE(TAG, "failed! mbedtls_ssl_write returned -0x%x", -ret);
                /* All other negative return values indicate connection needs to be reset.
                * Will be caught in ping request so ignored here */
                isErrorFlag = true;
                break;
            }
        }
        if(isErrorFlag) {
            break;
        }
    }

    *written_len = written_so_far;

    if(isErrorFlag) {
        return NETWORK_SSL_WRITE_ERROR;
    } else if(has_timer_expired(timer) && written_so_far != len) {
        return NETWORK_SSL_WRITE_TIMEOUT_ERROR;
    }

    return SUCCESS;
}

IoT_Error_t iot_tls_read(Network *pNetwork, unsigned char *pMsg, size_t len, Timer *timer, size_t *read_len) {
    TLSDataParams *tlsDataParams = &(pNetwork->tlsDataParams);
    struct esp_tls *tls = (struct esp_tls *)tlsDataParams->handle;
    uint32_t read_timeout = tlsDataParams->timeout;
    size_t rxLen = 0;
    int ret;
    struct timeval timeout;
    if (!tls) {
        return NULL_VALUE_ERROR;
    }

    while (len > 0) {
        /* Make sure we never block on read for longer than timer has left,
           but also that we don't block indefinitely (ie read_timeout > 0) */
        read_timeout = MAX(1, MIN(read_timeout, left_ms(timer)));
        timeout.tv_sec = read_timeout / 1000;
        timeout.tv_usec = (read_timeout % 1000) * 1000;

        ret = setsockopt(tls->sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
        ret = esp_tls_conn_read(tls, pMsg, len);

        /* Restore the old timeout */
        tlsDataParams->timeout = read_timeout;

        if (ret > 0) {
            rxLen += ret;
            pMsg += ret;
            len -= ret;
        } else if (ret == 0 || (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE && ret != MBEDTLS_ERR_SSL_TIMEOUT)) {
            return NETWORK_SSL_READ_ERROR;
        }

        // Evaluate timeout after the read to make sure read is done at least once
        if (has_timer_expired(timer)) {
            break;
        }
    }

    if (len == 0) {
        *read_len = rxLen;
        return SUCCESS;
    }

    if (rxLen == 0) {
        return NETWORK_SSL_NOTHING_TO_READ;
    } else {
        return NETWORK_SSL_READ_TIMEOUT_ERROR;
    }
}

IoT_Error_t iot_tls_disconnect(Network *pNetwork) {
    TLSDataParams *tlsDataParams = &(pNetwork->tlsDataParams);
    struct esp_tls *tls = (struct esp_tls *)tlsDataParams->handle;
    if (!tls) {
        return NULL_VALUE_ERROR;
    }

    esp_tls_conn_delete(tls);

    /* All other negative return values indicate connection needs to be reset.
     * No further action required since this is disconnect call */

    return SUCCESS;
}

IoT_Error_t iot_tls_destroy(Network *pNetwork) {
    TLSDataParams *tlsDataParams = &(pNetwork->tlsDataParams);
    struct esp_tls *tls = (struct esp_tls *)tlsDataParams->handle;
    if (!tls) {
        return NULL_VALUE_ERROR;
    }

    esp_tls_conn_delete(tls);

    return SUCCESS;
}

/*******************************************************************************
 * Copyright (c) 2014, 2015 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Allan Stockdill-Mander - initial API and implementation and/or initial documentation
 *    Ian Craggs - convert to FreeRTOS
 *******************************************************************************/

#include <string.h>

#include <netdb.h>
#include <sys/socket.h>

#include "MQTTFreeRTOS.h"

int ThreadStart(Thread* thread, void (*fn)(void*), void* arg)
{
    int rc = 0;
    uint16_t usTaskStackSize = (configMINIMAL_STACK_SIZE * 5);
    UBaseType_t uxTaskPriority = uxTaskPriorityGet(NULL); /* set the priority as the same as the calling task*/

    rc = xTaskCreate(fn,    /* The function that implements the task. */
                     "MQTTTask",         /* Just a text name for the task to aid debugging. */
                     usTaskStackSize,    /* The stack size is defined in FreeRTOSIPConfig.h. */
                     arg,                /* The task parameter, not used in this case. */
                     uxTaskPriority,     /* The priority assigned to the task is defined in FreeRTOSConfig.h. */
                     &thread->task);     /* The task handle is not used. */

    return rc;
}


void MutexInit(Mutex* mutex)
{
    mutex->sem = xSemaphoreCreateMutex();
}

int MutexLock(Mutex* mutex)
{
    return xSemaphoreTake(mutex->sem, portMAX_DELAY);
}

int MutexUnlock(Mutex* mutex)
{
    int x = xSemaphoreGive(mutex->sem);
    taskYIELD();
    return x;
}


void TimerCountdownMS(Timer* timer, unsigned int timeout_ms)
{
    timer->xTicksToWait = timeout_ms / portTICK_PERIOD_MS; /* convert milliseconds to ticks */
    vTaskSetTimeOutState(&timer->xTimeOut); /* Record the time at which this function was entered. */
}


void TimerCountdown(Timer* timer, unsigned int timeout)
{
    TimerCountdownMS(timer, timeout * 1000);
}


int TimerLeftMS(Timer* timer)
{
    xTaskCheckForTimeOut(&timer->xTimeOut, &timer->xTicksToWait); /* updates xTicksToWait to the number left */
    return timer->xTicksToWait * portTICK_PERIOD_MS;
}


char TimerIsExpired(Timer* timer)
{
    return xTaskCheckForTimeOut(&timer->xTimeOut, &timer->xTicksToWait) == pdTRUE;
}


void TimerInit(Timer* timer)
{
    timer->xTicksToWait = 0;
    memset(&timer->xTimeOut, '\0', sizeof(timer->xTimeOut));
}


static int esp_read(Network* n, unsigned char* buffer, unsigned int len, unsigned int timeout_ms)
{
    portTickType xTicksToWait = timeout_ms / portTICK_RATE_MS; /* convert milliseconds to ticks */
    xTimeOutType xTimeOut;
    int recvLen = 0;
    int rc = 0;

    struct timeval timeout;
    fd_set fdset;

    FD_ZERO(&fdset);
    FD_SET(n->my_socket, &fdset);

    timeout.tv_sec = 0;
    timeout.tv_usec = timeout_ms * 1000;

    vTaskSetTimeOutState(&xTimeOut); /* Record the time at which this function was entered. */

    if (select(n->my_socket + 1, &fdset, NULL, NULL, &timeout) > 0) {
        if (FD_ISSET(n->my_socket, &fdset)) {
            do {
                rc = recv(n->my_socket, buffer + recvLen, len - recvLen, MSG_DONTWAIT);

                if (rc > 0) {
                    recvLen += rc;
                } else if (rc < 0) {
                    recvLen = rc;
                    break;
                }
            } while (recvLen < len && xTaskCheckForTimeOut(&xTimeOut, &xTicksToWait) == pdFALSE);
        }
    }

    return recvLen;
}


static int esp_write(Network* n, unsigned char* buffer, unsigned int len, unsigned int timeout_ms)
{
    portTickType xTicksToWait = timeout_ms / portTICK_RATE_MS; /* convert milliseconds to ticks */
    xTimeOutType xTimeOut;
    int sentLen = 0;
    int rc = 0;
    int readysock;

    struct timeval timeout;
    fd_set fdset;

    FD_ZERO(&fdset);
    FD_SET(n->my_socket, &fdset);

    timeout.tv_sec = 0;
    timeout.tv_usec = timeout_ms * 1000;

    vTaskSetTimeOutState(&xTimeOut); /* Record the time at which this function was entered. */

    do {
        readysock = select(n->my_socket + 1, NULL, &fdset, NULL, &timeout);
    } while (readysock <= 0);

    if (FD_ISSET(n->my_socket, &fdset)) {
        do {
            rc = send(n->my_socket, buffer + sentLen, len - sentLen, MSG_DONTWAIT);

            if (rc > 0) {
                sentLen += rc;
            } else if (rc < 0) {
                sentLen = rc;
                break;
            }
        } while (sentLen < len && xTaskCheckForTimeOut(&xTimeOut, &xTicksToWait) == pdFALSE);
    }

    return sentLen;
}


static void esp_disconnect(Network* n)
{
    close(n->my_socket);
}


void NetworkInit(Network* n)
{
    n->my_socket = 0;
    n->mqttread = esp_read;
    n->mqttwrite = esp_write;
    n->disconnect = esp_disconnect;
}


int NetworkConnect(Network* n, char* addr, int port)
{
    struct sockaddr_in sAddr;
    int retVal = -1;
    struct hostent* ipAddress;

    if ((ipAddress = gethostbyname(addr)) == 0) {
        goto exit;
    }

    sAddr.sin_family = AF_INET;
    sAddr.sin_addr.s_addr = ((struct in_addr*)(ipAddress->h_addr))->s_addr;
    sAddr.sin_port = htons(port);

    if ((n->my_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        goto exit;
    }

    if ((retVal = connect(n->my_socket, (struct sockaddr*)&sAddr, sizeof(sAddr))) < 0) {
        close(n->my_socket);
        goto exit;
    }

exit:
    return retVal;
}

#ifdef CONFIG_SSL_USING_MBEDTLS
static int esp_ssl_read(Network* n, unsigned char* buffer, unsigned int len, unsigned int timeout_ms)
{
    portTickType xTicksToWait = timeout_ms / portTICK_RATE_MS; /* convert milliseconds to ticks */
    xTimeOutType xTimeOut;
    int recvLen = 0;
    int rc = 0;
    static unsigned char* read_buffer;

    struct timeval timeout;
    fd_set readset;
    fd_set errset;

    FD_ZERO(&readset);
    FD_ZERO(&errset);
    FD_SET(n->my_socket, &readset);
    FD_SET(n->my_socket, &errset);

    timeout.tv_sec = 0;
    timeout.tv_usec = timeout_ms * 1000;

    vTaskSetTimeOutState(&xTimeOut); /* Record the time at which this function was entered. */

    if (!n->read_count) { /* read mqtt packet for the first time */
        if (select(n->my_socket + 1, &readset, NULL, &errset, &timeout) > 0) {
            if (FD_ISSET(n->my_socket, &errset)) {
                return recvLen;
            } else if (FD_ISSET(n->my_socket, &readset)) {
                read_buffer = buffer;
                len = 2; /* len: msg_type(1 octet) + msg_len(1 octet) */

                do {
                    rc = SSL_read(n->ssl, read_buffer, len);

                    if (rc > 0) {
                        recvLen += rc;
                    } else if (rc < 0) {
                        recvLen = rc;
                        return recvLen;
                    }
                } while (recvLen < len && xTaskCheckForTimeOut(&xTimeOut, &xTicksToWait) == pdFALSE);

                recvLen = 0;
                len = *(read_buffer + 1); /* len: remaining msg */

                if (len > 0) {
                    do {
                        rc = SSL_read(n->ssl, read_buffer + 2, len);

                        if (rc > 0) {
                            recvLen += rc;
                        } else if (rc < 0) {
                            recvLen = rc;
                            return recvLen;
                        }
                    } while (recvLen < len && xTaskCheckForTimeOut(&xTimeOut, &xTicksToWait) == pdFALSE);
                }

                n->read_count++;
                return 1;
            }
        }
    } else if (n->read_count == 1) { /* read same mqtt packet for the second time */
        if (xTaskCheckForTimeOut(&xTimeOut, &xTicksToWait) == pdTRUE) {
            n->read_count = 0;
            read_buffer[0] = 0;
            return 0;
        }

        n->read_count++;
        *buffer = *(read_buffer + 1);
        return 1;
    } else if (n->read_count == 2) { /* read same mqtt packet for the third time */
        n->read_count = 0;

        if (xTaskCheckForTimeOut(&xTimeOut, &xTicksToWait) == pdTRUE) {
            read_buffer[0] = 0;
            return 0;
        }

        memcpy(buffer, read_buffer + 2, len);
        return len;
    }

    return recvLen;
}


static int esp_ssl_write(Network* n, unsigned char* buffer, unsigned int len, unsigned int timeout_ms)
{
    portTickType xTicksToWait = timeout_ms / portTICK_RATE_MS; /* convert milliseconds to ticks */
    xTimeOutType xTimeOut;
    int sentLen = 0;
    int rc = 0;
    int readysock;

    struct timeval timeout;
    fd_set writeset;
    fd_set errset;

    FD_ZERO(&writeset);
    FD_ZERO(&errset);
    FD_SET(n->my_socket, &writeset);
    FD_SET(n->my_socket, &errset);

    timeout.tv_sec = 0;
    timeout.tv_usec = timeout_ms * 1000;

    vTaskSetTimeOutState(&xTimeOut); /* Record the time at which this function was entered. */

    do {
        readysock = select(n->my_socket + 1, NULL, &writeset, &errset, &timeout);
    } while (readysock <= 0);

    if (FD_ISSET(n->my_socket, &errset)) {
        return sentLen;
    } else if (FD_ISSET(n->my_socket, &writeset)) {
        do {
            rc = SSL_write(n->ssl, buffer + sentLen, len - sentLen);

            if (rc > 0) {
                sentLen += rc;
            } else if (rc < 0) {
                sentLen = rc;
                break;
            }
        } while (sentLen < len && xTaskCheckForTimeOut(&xTimeOut, &xTicksToWait) == pdFALSE);
    }

    return sentLen;
}


static void esp_ssl_disconnect(Network* n)
{
    close(n->my_socket);
    SSL_free(n->ssl);
    SSL_CTX_free(n->ctx);
    n->read_count = 0;
}


void NetworkInitSSL(Network* n)
{
    n->my_socket = 0;
    n->mqttread = esp_ssl_read;
    n->mqttwrite = esp_ssl_write;
    n->disconnect = esp_ssl_disconnect;
    n->read_count = 0;
    n->ctx = NULL;
    n->ssl = NULL;
}


int NetworkConnectSSL(Network* n, char* addr, int port, ssl_ca_crt_key_t* ssl_cck, const SSL_METHOD* method, int verify_mode, size_t frag_len)
{
    struct sockaddr_in sAddr;
    int retVal = -1;
    struct hostent* ipAddress;

    if ((ipAddress = gethostbyname(addr)) == 0) {
        goto exit;
    }

    n->ctx = SSL_CTX_new(method);

    if (!n->ctx) {
        goto exit;
    }

    if (ssl_cck->cacrt) {
        retVal = SSL_CTX_load_verify_buffer(n->ctx, ssl_cck->cacrt, ssl_cck->cacrt_len);

        if (retVal != 1) {
            goto exit1;
        }
    }

    if (ssl_cck->cert && ssl_cck->key) {
        retVal = SSL_CTX_use_certificate_ASN1(n->ctx, ssl_cck->cert_len, ssl_cck->cert);

        if (!retVal) {
            goto exit1;
        }

        retVal = SSL_CTX_use_PrivateKey_ASN1(0, n->ctx, ssl_cck->key, ssl_cck->key_len);

        if (!retVal) {
            goto exit1;
        }
    }

    if (ssl_cck->cacrt) {
        SSL_CTX_set_verify(n->ctx, verify_mode, NULL);
    } else {
        SSL_CTX_set_verify(n->ctx, SSL_VERIFY_NONE, NULL);
    }

    sAddr.sin_family = AF_INET;
    sAddr.sin_addr.s_addr = ((struct in_addr*)(ipAddress->h_addr))->s_addr;
    sAddr.sin_port = htons(port);

    if ((n->my_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        goto exit1;
    }

    if ((retVal = connect(n->my_socket, (struct sockaddr*)&sAddr, sizeof(sAddr))) < 0) {
        goto exit2;
    }

    n->ssl = SSL_new(n->ctx);

    if (!n->ssl) {
        goto exit2;
    }

    SSL_set_fd(n->ssl, n->my_socket);

    if ((retVal = SSL_connect(n->ssl)) <= 0) {
        goto exit3;
    } else {
        goto exit;
    }

exit3:
    SSL_free(n->ssl);
exit2:
    close(n->my_socket);
exit1:
    SSL_CTX_free(n->ctx);
exit:
    return retVal;
}
#endif

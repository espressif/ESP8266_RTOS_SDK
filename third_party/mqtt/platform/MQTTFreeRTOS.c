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

#include "MQTTFreeRTOS.h"
#include "lwip/netdb.h"

int ThreadStart(Thread* thread, void (*fn)(void*), void* arg)
{
    int rc = 0;
    uint16_t usTaskStackSize = (configMINIMAL_STACK_SIZE * 5);
    unsigned portBASE_TYPE uxTaskPriority = uxTaskPriorityGet(NULL); /* set the priority as the same as the calling task*/

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
    return xSemaphoreGive(mutex->sem);
}


void TimerCountdownMS(Timer* timer, unsigned int timeout_ms)
{
    timer->xTicksToWait = timeout_ms / portTICK_RATE_MS; /* convert milliseconds to ticks */
    vTaskSetTimeOutState(&timer->xTimeOut); /* Record the time at which this function was entered. */
}


void TimerCountdown(Timer* timer, unsigned int timeout) 
{
    TimerCountdownMS(timer, timeout * 1000);
}


int TimerLeftMS(Timer* timer) 
{
    xTaskCheckForTimeOut(&timer->xTimeOut, &timer->xTicksToWait); /* updates xTicksToWait to the number left */
    return (timer->xTicksToWait < 0) ? 0 : (timer->xTicksToWait * portTICK_RATE_MS);
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


int esp_read(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
    portTickType xTicksToWait = timeout_ms / portTICK_RATE_MS; /* convert milliseconds to ticks */
    xTimeOutType xTimeOut;
    int recvLen = 0;
    int recv_timeout = timeout_ms;
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


int esp_write(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
    portTickType xTicksToWait = timeout_ms / portTICK_RATE_MS; /* convert milliseconds to ticks */
    xTimeOutType xTimeOut;
    int sentLen = 0;
    int send_timeout = timeout_ms;
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


void esp_disconnect(Network* n)
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
    struct hostent *ipAddress;

    if ((ipAddress = gethostbyname(addr)) == 0)
        goto exit;

    sAddr.sin_family = AF_INET;
    sAddr.sin_addr.s_addr = ((struct in_addr*)(ipAddress->h_addr))->s_addr;
    sAddr.sin_port = htons(port);

    if ((n->my_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        goto exit;

    if ((retVal = connect(n->my_socket, (struct sockaddr*)&sAddr, sizeof(sAddr))) < 0)
    {
        close(n->my_socket);
        goto exit;
    }

exit:
    return retVal;
}


#if 0
int NetworkConnectTLS(Network *n, char* addr, int port, SlSockSecureFiles_t* certificates, unsigned char sec_method, unsigned int cipher, char server_verify)
{
    SlSockAddrIn_t sAddr;
    int addrSize;
    int retVal;
    unsigned long ipAddress;

    retVal = sl_NetAppDnsGetHostByName(addr, strlen(addr), &ipAddress, AF_INET);
    if (retVal < 0) {
        return -1;
    }

    sAddr.sin_family = AF_INET;
    sAddr.sin_port = sl_Htons((unsigned short)port);
    sAddr.sin_addr.s_addr = sl_Htonl(ipAddress);

    addrSize = sizeof(SlSockAddrIn_t);

    n->my_socket = sl_Socket(SL_AF_INET, SL_SOCK_STREAM, SL_SEC_SOCKET);
    if (n->my_socket < 0) {
        return -1;
    }

    SlSockSecureMethod method;
    method.secureMethod = sec_method;
    retVal = sl_SetSockOpt(n->my_socket, SL_SOL_SOCKET, SL_SO_SECMETHOD, &method, sizeof(method));
    if (retVal < 0) {
        return retVal;
    }

    SlSockSecureMask mask;
    mask.secureMask = cipher;
    retVal = sl_SetSockOpt(n->my_socket, SL_SOL_SOCKET, SL_SO_SECURE_MASK, &mask, sizeof(mask));
    if (retVal < 0) {
        return retVal;
    }

    if (certificates != NULL) {
        retVal = sl_SetSockOpt(n->my_socket, SL_SOL_SOCKET, SL_SO_SECURE_FILES, certificates->secureFiles, sizeof(SlSockSecureFiles_t));
        if (retVal < 0)
        {
            return retVal;
        }
    }

    retVal = sl_Connect(n->my_socket, (SlSockAddr_t *)&sAddr, addrSize);
    if (retVal < 0) {
        if (server_verify || retVal != -453) {
            sl_Close(n->my_socket);
            return retVal;
        }
    }

    SysTickIntRegister(SysTickIntHandler);
    SysTickPeriodSet(80000);
    SysTickEnable();

    return retVal;
}
#endif

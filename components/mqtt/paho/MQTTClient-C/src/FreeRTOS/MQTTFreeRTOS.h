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
 *******************************************************************************/

#ifndef MQTTFreeRTOS_H
#define MQTTFreeRTOS_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#ifdef CONFIG_SSL_USING_MBEDTLS
#include "openssl/ssl.h"
#endif

#define    OVER_TCP                 0       // 0: MQTT over TCP
#define    TLS_VERIFY_NONE          1       // 1: enable SSL/TLS, but there is no a certificate verify
#define    TLS_VERIFY_PEER          2       // 2: enable SSL/TLS, and verify the MQTT broker certificate
#define    TLS_VERIFY_CLIENT        3       // 3: enable SSL/TLS, and verify the MQTT broker certificate, and enable certificate for MQTT broker

typedef struct Timer {
    TickType_t xTicksToWait;
    TimeOut_t xTimeOut;
} Timer;

typedef struct Network Network;

struct Network {
    int my_socket;
    int (*mqttread)(Network *, unsigned char *, unsigned int, unsigned int);
    int (*mqttwrite)(Network *, unsigned char *, unsigned int, unsigned int);
    void (*disconnect)(Network *);

    int read_count;
#ifdef CONFIG_SSL_USING_MBEDTLS
    SSL_CTX *ctx;
    SSL *ssl;
#endif
};

void TimerInit(Timer *);
char TimerIsExpired(Timer *);
void TimerCountdownMS(Timer *, unsigned int);
void TimerCountdown(Timer *, unsigned int);
int TimerLeftMS(Timer *);

typedef struct Mutex {
    SemaphoreHandle_t sem;
} Mutex;

void MutexInit(Mutex *);
int MutexLock(Mutex *);
int MutexUnlock(Mutex *);

typedef struct Thread {
    TaskHandle_t task;
} Thread;

int ThreadStart(Thread *, void (*fn)(void *), void *arg);

/**
 * @brief Initialize the network structure
 *
 * @param m - network structure
 *
 * @return void
 */
void NetworkInit(Network *);

/**
 * @brief connect with mqtt broker
 *
 * @param n           - mqtt network struct
 * @param addr        - mqtt broker address
 * @param port        - mqtt broker port
 *
 * @return connect status
 */
int NetworkConnect(Network *n, char *addr, int port);

#ifdef CONFIG_SSL_USING_MBEDTLS
typedef struct ssl_ca_crt_key {
    unsigned char *cacrt;
    unsigned int cacrt_len;
    unsigned char *cert;
    unsigned int cert_len;
    unsigned char *key;
    unsigned int key_len;
} ssl_ca_crt_key_t;

/**
 * @brief Initialize the network structure for SSL connection
 *
 * @param m - network structure
 *
 * @return void
 */
void NetworkInitSSL(Network *n);

/**
 * @brief Use SSL to connect with mqtt broker
 *
 * @param n           - mqtt network struct
 * @param addr        - mqtt broker address
 * @param port        - mqtt broker port
 * @param ssl_cck     - client CA, certificate and private key
 * @param method      - SSL context client method
 * @param verify_mode - SSL verifying mode
 * @param frag_len    - SSL read buffer length
 *
 * @return connect status
 */
int NetworkConnectSSL(Network *n, char *addr, int port, ssl_ca_crt_key_t *ssl_cck, const SSL_METHOD *method, int verify_mode, unsigned int frag_len);

/*int NetworkConnectTLS(Network*, char*, int, SlSockSecureFiles_t*, unsigned char, unsigned int, char);*/

#endif

#endif

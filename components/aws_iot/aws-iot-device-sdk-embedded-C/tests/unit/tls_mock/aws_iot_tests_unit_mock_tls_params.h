/*
* Copyright 2015-2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License").
* You may not use this file except in compliance with the License.
* A copy of the License is located at
*
* http://aws.amazon.com/apache2.0
*
* or in the "license" file accompanying this file. This file is distributed
* on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
* express or implied. See the License for the specific language governing
* permissions and limitations under the License.
*/

/**
 * @file aws_iot_tests_unit_mock_tls_params.h
 * @brief IoT Client Unit Testing Mock TLS Params
 */

#ifndef UNITTESTS_MOCKS_TLS_PARAMS_H_
#define UNITTESTS_MOCKS_TLS_PARAMS_H_

#include <sys/time.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define TLSMaxBufferSize 1024
#define NO_MSG_LENGTH_MENTIONED -1

typedef struct {
	unsigned char *pBuffer;
	size_t len;
	bool NoMsgFlag;
	struct timeval expiry_time;
	size_t BufMaxSize;
} TlsBuffer;


extern TlsBuffer RxBuffer;
extern TlsBuffer TxBuffer;

extern size_t RxIndex;
extern unsigned char RxBuf[TLSMaxBufferSize];
extern unsigned char TxBuf[TLSMaxBufferSize];
extern char LastSubscribeMessage[TLSMaxBufferSize];
extern size_t lastSubscribeMsgLen;
extern char SecondLastSubscribeMessage[TLSMaxBufferSize];
extern size_t secondLastSubscribeMsgLen;

extern char hostAddress[512];
extern uint16_t port;
extern uint32_t handshakeTimeout_ms;

extern char *invalidEndpointFilter;
extern char *invalidRootCAPathFilter;
extern char *invalidCertPathFilter;
extern char *invalidPrivKeyPathFilter;
extern uint16_t invalidPortFilter;

#endif /* UNITTESTS_MOCKS_TLS_PARAMS_H_ */

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
 * @file aws_iot_tests_unit_mock_tls_params.c
 * @brief IoT Client Unit Testing Mock TLS Params
 */

#include "aws_iot_tests_unit_mock_tls_params.h"

unsigned char RxBuf[TLSMaxBufferSize];
unsigned char TxBuf[TLSMaxBufferSize];
char LastSubscribeMessage[TLSMaxBufferSize];
size_t lastSubscribeMsgLen;
char SecondLastSubscribeMessage[TLSMaxBufferSize];
size_t secondLastSubscribeMsgLen;

TlsBuffer RxBuffer = {.pBuffer = RxBuf,.len = 512, .NoMsgFlag=1, .expiry_time = {0, 0}, .BufMaxSize = TLSMaxBufferSize};
TlsBuffer TxBuffer = {.pBuffer = TxBuf,.len = 512, .NoMsgFlag=1, .expiry_time = {0, 0}, .BufMaxSize = TLSMaxBufferSize};

size_t RxIndex = 0;

char *invalidEndpointFilter;
char *invalidRootCAPathFilter;
char *invalidCertPathFilter;
char *invalidPrivKeyPathFilter;
uint16_t invalidPortFilter;

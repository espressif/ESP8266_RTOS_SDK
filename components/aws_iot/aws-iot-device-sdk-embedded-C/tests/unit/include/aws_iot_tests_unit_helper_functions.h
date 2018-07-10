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
 * @file aws_iot_tests_unit_helper_functions.h
 * @brief IoT Client Unit Testing - Helper Functions
 */

#ifndef IOT_TESTS_UNIT_HELPER_FUNCTIONS_H_
#define IOT_TESTS_UNIT_HELPER_FUNCTIONS_H_

#include <string.h>
#include "aws_iot_mqtt_client_interface.h"

typedef struct {
	unsigned char PacketType;
	unsigned int RemainingLength;
	unsigned int ProtocolLength;
	unsigned char ProtocolName[4];
	unsigned int ProtocolLevel;
	unsigned char ConnectFlag;
	unsigned int KeepAlive;
} ConnectBufferProofread;

void ResetInvalidParameters(void);

void InitMQTTParamsSetup(IoT_Client_Init_Params *params, char *pHost, uint16_t port, bool enableAutoReconnect,
								iot_disconnect_handler disconnectHandler);

void ConnectMQTTParamsSetup(IoT_Client_Connect_Params *params, char *pClientID, uint16_t clientIDLen);

void ConnectMQTTParamsSetup_Detailed(IoT_Client_Connect_Params *params, char *pClientID, uint16_t clientIDLen,
											QoS qos, bool isCleanSession, bool isWillMsgPresent, char *pWillTopicName,
											uint16_t willTopicNameLen, char *pWillMessage, uint16_t willMsgLen,
											char *pUsername, uint16_t userNameLen, char *pPassword,
											uint16_t passwordLen);

void printBuffer(unsigned char *buffer, size_t len);

void setTLSRxBufferForConnack(IoT_Client_Connect_Params *params, unsigned char sessionPresent,
							  unsigned char connackResponseCode);

void setTLSRxBufferForPuback(void);

void setTLSRxBufferForSuback(char *topicName, size_t topicNameLen, QoS qos, IoT_Publish_Message_Params params);

void setTLSRxBufferForDoubleSuback(char *topicName, size_t topicNameLen, QoS qos, IoT_Publish_Message_Params params);

void setTLSRxBufferForSubFail(void);

void setTLSRxBufferWithMsgOnSubscribedTopic(char *topicName, size_t topicNameLen, QoS qos,
											IoT_Publish_Message_Params params, char *pMsg);

void setTLSRxBufferForUnsuback(void);

void setTLSRxBufferForPingresp(void);

void setTLSRxBufferForConnackAndSuback(IoT_Client_Connect_Params *conParams, unsigned char sessionPresent,
											  char *topicName, size_t topicNameLen, QoS qos);

unsigned char isLastTLSTxMessagePuback(void);

unsigned char isLastTLSTxMessagePingreq(void);

unsigned char isLastTLSTxMessageDisconnect(void);

void setTLSRxBufferDelay(int seconds, int microseconds);

void ResetTLSBuffer(void);

unsigned char generateMultipleSubTopics(char *des, int boundary);

void encodeRemainingLength(unsigned char *buf, size_t *st, size_t length);

unsigned char *connectTxBufferHeaderParser(ConnectBufferProofread *params, unsigned char *buf);

bool isConnectTxBufFlagCorrect(IoT_Client_Connect_Params *settings, ConnectBufferProofread *readRes);

bool isConnectTxBufPayloadCorrect(IoT_Client_Connect_Params *settings, unsigned char *payloadBuf);

void printPrfrdParams(ConnectBufferProofread *params);

#endif /* IOT_TESTS_UNIT_HELPER_FUNCTIONS_H_ */

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
 * @file aws_iot_tests_unit_helper_functions.c
 * @brief IoT Client Unit Testing - Helper Functions
 */

#include <stdio.h>
#include <string.h>
#include "aws_iot_mqtt_client.h"
#include "aws_iot_tests_unit_mock_tls_params.h"
#include "aws_iot_tests_unit_helper_functions.h"
#include "aws_iot_version.h"

#if !DISABLE_METRICS
#define SDK_METRICS_LEN 25
#define SDK_METRICS_TEMPLATE "?SDK=C&Version=%d.%d.%d"
static char pUsernameTemp[SDK_METRICS_LEN] = {0};
#endif

#define CONNACK_SUBACK_PACKET_SIZE 9
#define PUBACK_PACKET_SIZE 4
#define SUBACK_PACKET_SIZE 5
#define UNSUBACK_PACKET_SIZE 4
#define PINGRESP_PACKET_SIZE 2

void ResetInvalidParameters(void) {
	invalidEndpointFilter = NULL;
	invalidRootCAPathFilter = NULL;
	invalidCertPathFilter = NULL;
	invalidPrivKeyPathFilter = NULL;
	invalidPortFilter = 0;
}

void InitMQTTParamsSetup(IoT_Client_Init_Params *params, char *pHost, uint16_t port, bool enableAutoReconnect,
						 iot_disconnect_handler disconnectHandler) {
	params->pHostURL = pHost;
	params->port = port;
	params->mqttCommandTimeout_ms = 5000;
	params->tlsHandshakeTimeout_ms = 5000;
	params->enableAutoReconnect = enableAutoReconnect;
	params->disconnectHandler = disconnectHandler;
	params->disconnectHandlerData = NULL;
	params->isSSLHostnameVerify = true;
	params->pDeviceCertLocation = AWS_IOT_ROOT_CA_FILENAME;
	params->pDevicePrivateKeyLocation = AWS_IOT_CERTIFICATE_FILENAME;
	params->pRootCALocation = AWS_IOT_PRIVATE_KEY_FILENAME;
}

void ConnectMQTTParamsSetup(IoT_Client_Connect_Params *params, char *pClientID, uint16_t clientIDLen) {
	params->keepAliveIntervalInSec = 10;
	params->isCleanSession = 1;
	params->MQTTVersion = MQTT_3_1_1;
	params->pClientID = pClientID;
	params->clientIDLen = clientIDLen;
	params->isWillMsgPresent = false;
	params->pUsername = NULL;
	params->usernameLen = 0;
	params->pPassword = NULL;
	params->passwordLen = 0;
}

void ConnectMQTTParamsSetup_Detailed(IoT_Client_Connect_Params *params, char *pClientID, uint16_t clientIDLen, QoS qos,
									 bool isCleanSession, bool isWillMsgPresent, char *pWillTopicName,
									 uint16_t willTopicNameLen, char *pWillMessage, uint16_t willMsgLen,
									 char *pUsername, uint16_t userNameLen, char *pPassword, uint16_t passwordLen) {
	params->keepAliveIntervalInSec = 10;
	params->isCleanSession = isCleanSession;
	params->MQTTVersion = MQTT_3_1_1;
	params->pClientID = pClientID;
	params->clientIDLen = clientIDLen;
	params->pUsername = pUsername;
	params->usernameLen = userNameLen;
	params->pPassword = pPassword;
	params->passwordLen = passwordLen;
	params->isWillMsgPresent = isWillMsgPresent;
	params->will.pMessage = pWillMessage;
	params->will.msgLen = willMsgLen;
	params->will.pTopicName = pWillTopicName;
	params->will.topicNameLen = willTopicNameLen;
	params->will.qos = qos;
	params->will.isRetained = false;
}

void printBuffer(unsigned char *buffer, size_t len) {
	size_t i;
	printf("\n--\n");
	for(i = 0; i < len; i++) {
		printf("%d: %c, %d\n", (uint32_t)i, buffer[i], buffer[i]);
	}
	printf("\n--\n");
}

#define CONNACK_PACKET_SIZE 4

void setTLSRxBufferForConnack(IoT_Client_Connect_Params *params, unsigned char sessionPresent,
							  unsigned char connackResponseCode) {
	RxBuffer.NoMsgFlag = false;

	if(params->isCleanSession) {
		sessionPresent = 0;
	}

	RxBuffer.pBuffer[0] = (unsigned char) (0x20);
	RxBuffer.pBuffer[1] = (unsigned char) (0x02);
	RxBuffer.pBuffer[2] = sessionPresent;
	RxBuffer.pBuffer[3] = connackResponseCode;

	RxBuffer.len = CONNACK_PACKET_SIZE;
	RxIndex = 0;
}

void setTLSRxBufferForConnackAndSuback(IoT_Client_Connect_Params *conParams, unsigned char sessionPresent,
									   char *topicName, size_t topicNameLen, QoS qos) {
	IOT_UNUSED(topicName);
	IOT_UNUSED(topicNameLen);

	RxBuffer.NoMsgFlag = false;

	if(conParams->isCleanSession) {
		sessionPresent = 0;
	}

	RxBuffer.pBuffer[0] = (unsigned char) (0x20);
	RxBuffer.pBuffer[1] = (unsigned char) (0x02);
	RxBuffer.pBuffer[2] = sessionPresent;
	RxBuffer.pBuffer[3] = (unsigned char) (0x0);

	RxBuffer.pBuffer[4] = (unsigned char) (0x90);
	RxBuffer.pBuffer[5] = (unsigned char) (0x2 + 1);
	// Variable header - packet identifier
	RxBuffer.pBuffer[6] = (unsigned char) (2);
	RxBuffer.pBuffer[7] = (unsigned char) (0);
	// payload
	RxBuffer.pBuffer[8] = (unsigned char) (qos);

	RxBuffer.len = CONNACK_SUBACK_PACKET_SIZE;
	RxIndex = 0;
}

void setTLSRxBufferForPuback(void) {
	size_t i;

	RxBuffer.NoMsgFlag = true;
	RxBuffer.len = PUBACK_PACKET_SIZE;
	RxIndex = 0;

	for(i = 0; i < RxBuffer.BufMaxSize; i++) {
		RxBuffer.pBuffer[i] = 0;
	}

	RxBuffer.pBuffer[0] = (unsigned char) (0x40);
	RxBuffer.pBuffer[1] = (unsigned char) (0x02);
	RxBuffer.pBuffer[2] = (unsigned char) (0x02);
	RxBuffer.pBuffer[3] = (unsigned char) (0x00);
	RxBuffer.NoMsgFlag = false;
}

void setTLSRxBufferForSubFail(void) {
	RxBuffer.NoMsgFlag = false;
	RxBuffer.pBuffer[0] = (unsigned char) (0x90);
	RxBuffer.pBuffer[1] = (unsigned char) (0x2 + 1);
	// Variable header - packet identifier
	RxBuffer.pBuffer[2] = (unsigned char) (2);
	RxBuffer.pBuffer[3] = (unsigned char) (0);
	// payload
	RxBuffer.pBuffer[4] = (unsigned char) (128);

	RxBuffer.len = SUBACK_PACKET_SIZE;
	RxIndex = 0;
}

void setTLSRxBufferForDoubleSuback(char *topicName, size_t topicNameLen, QoS qos, IoT_Publish_Message_Params params) {
	IOT_UNUSED(topicName);
	IOT_UNUSED(topicNameLen);
	IOT_UNUSED(params);

	RxBuffer.NoMsgFlag = false;
	RxBuffer.pBuffer[0] = (unsigned char) (0x90);
	RxBuffer.pBuffer[1] = (unsigned char) (0x2 + 1);
	// Variable header - packet identifier
	RxBuffer.pBuffer[2] = (unsigned char) (2);
	RxBuffer.pBuffer[3] = (unsigned char) (0);
	// payload
	RxBuffer.pBuffer[4] = (unsigned char) (qos);

	RxBuffer.pBuffer[5] = (unsigned char) (0x90);
	RxBuffer.pBuffer[6] = (unsigned char) (0x2 + 1);
	// Variable header - packet identifier
	RxBuffer.pBuffer[7] = (unsigned char) (2);
	RxBuffer.pBuffer[8] = (unsigned char) (0);
	// payload
	RxBuffer.pBuffer[9] = (unsigned char) (qos);

	RxBuffer.len = SUBACK_PACKET_SIZE * 2;
	RxIndex = 0;
}

void setTLSRxBufferForSuback(char *topicName, size_t topicNameLen, QoS qos, IoT_Publish_Message_Params params) {
	IOT_UNUSED(topicName);
	IOT_UNUSED(topicNameLen);
	IOT_UNUSED(params);

	RxBuffer.NoMsgFlag = false;
	RxBuffer.pBuffer[0] = (unsigned char) (0x90);
	RxBuffer.pBuffer[1] = (unsigned char) (0x2 + 1);
	// Variable header - packet identifier
	RxBuffer.pBuffer[2] = (unsigned char) (2);
	RxBuffer.pBuffer[3] = (unsigned char) (0);
	// payload
	RxBuffer.pBuffer[4] = (unsigned char) (qos);

	RxBuffer.len = SUBACK_PACKET_SIZE;
	RxIndex = 0;
}

void setTLSRxBufferForUnsuback(void) {
	RxBuffer.NoMsgFlag = false;
	RxBuffer.pBuffer[0] = (unsigned char) (0xB0);
	RxBuffer.pBuffer[1] = (unsigned char) (0x02);
	// Variable header - packet identifier
	RxBuffer.pBuffer[2] = (unsigned char) (2);
	RxBuffer.pBuffer[3] = (unsigned char) (0);
	// No payload
	RxBuffer.len = UNSUBACK_PACKET_SIZE;
	RxIndex = 0;
}

void setTLSRxBufferForPingresp(void) {
	RxBuffer.NoMsgFlag = false;
	RxBuffer.pBuffer[0] = (unsigned char) (0xD0);
	RxBuffer.pBuffer[1] = (unsigned char) (0x00);
	RxBuffer.len = PINGRESP_PACKET_SIZE;
	RxIndex = 0;
}

void ResetTLSBuffer(void) {
	size_t i;
	RxBuffer.len = 0;
	RxBuffer.NoMsgFlag = true;

	for(i = 0; i < RxBuffer.BufMaxSize; i++) {
		RxBuffer.pBuffer[i] = 0;
	}

	RxIndex = 0;
	RxBuffer.expiry_time.tv_sec = 0;
	RxBuffer.expiry_time.tv_usec = 0;
	TxBuffer.len = 0;
	for(i = 0; i < TxBuffer.BufMaxSize; i++) {
		TxBuffer.pBuffer[i] = 0;
	}
}

void setTLSRxBufferDelay(int seconds, int microseconds) {
	struct timeval now, duration, result;
	duration.tv_sec = seconds;
	duration.tv_usec = microseconds;

	gettimeofday(&now, NULL);
	timeradd(&now, &duration, &result);
	RxBuffer.expiry_time.tv_sec = result.tv_sec;
	RxBuffer.expiry_time.tv_usec = result.tv_usec;
}

void setTLSRxBufferWithMsgOnSubscribedTopic(char *topicName, size_t topicNameLen, QoS qos,
											IoT_Publish_Message_Params params, char *pMsg) {
	size_t VariableLen = topicNameLen + 2 + 2;
	size_t i = 0, cursor = 0, packetIdStartLoc = 0, payloadStartLoc = 0, VarHeaderStartLoc = 0;
	size_t PayloadLen = strlen(pMsg) + 1;

	RxBuffer.NoMsgFlag = false;
	RxBuffer.pBuffer[0] = (unsigned char) (0x30 | ((params.qos << 1) & 0xF));// QoS1
	cursor++; // Move the cursor

	// Remaining Length
	// Translate the Remaining Length into packet bytes
	encodeRemainingLength(RxBuffer.pBuffer, &cursor, VariableLen + PayloadLen);

	VarHeaderStartLoc = cursor - 1;
	// Variable header
	RxBuffer.pBuffer[VarHeaderStartLoc + 1] = (unsigned char) ((topicNameLen & 0xFF00) >> 8);
	RxBuffer.pBuffer[VarHeaderStartLoc + 2] = (unsigned char) (topicNameLen & 0xFF);
	for(i = 0; i < topicNameLen; i++) {
		RxBuffer.pBuffer[VarHeaderStartLoc + 3 + i] = (unsigned char) topicName[i];
	}

	packetIdStartLoc = VarHeaderStartLoc + topicNameLen + 2;
	payloadStartLoc = (packetIdStartLoc + 1);

	if(QOS0 != qos) {
		// packet id only for QoS 1 or 2
		RxBuffer.pBuffer[packetIdStartLoc + 1] = 2;
		RxBuffer.pBuffer[packetIdStartLoc + 2] = 3;
		payloadStartLoc = packetIdStartLoc + 3;
	}

	// payload
	for(i = 0; i < PayloadLen; i++) {
		RxBuffer.pBuffer[payloadStartLoc + i] = (unsigned char) pMsg[i];
	}

	RxBuffer.len = VariableLen + PayloadLen + 2; // 2 for fixed header
	RxIndex = 0;
	//printBuffer(RxBuffer.pBuffer, RxBuffer.len);
}

unsigned char isLastTLSTxMessagePuback() {
	return (unsigned char) (TxBuffer.pBuffer[0] == 0x40 ? 1 : 0);
}

unsigned char isLastTLSTxMessagePingreq() {
	return (unsigned char) (TxBuffer.pBuffer[0] == 0xC0 ? 1 : 0);
}

unsigned char isLastTLSTxMessageDisconnect() {
	return (unsigned char) (TxBuffer.pBuffer[0] == 0xE0 ? 1 : 0);
}

unsigned char generateMultipleSubTopics(char *des, int boundary) {
	int i;
	int currLen = 0;
	char *op1 = des;
	unsigned char ret = (unsigned char) (des == NULL ? 0 : 1);
	while(*op1 != '\0') {
		currLen++;
		op1++;
	}
	// Save 1 byte for terminator '\0'
	for(i = 0; i < boundary - currLen - 1; i++) {
		//printf("%d\n", i);
		strcat(des, "a");
	}
	return ret;
}

void encodeRemainingLength(unsigned char *buf, size_t *st, size_t length) {
	unsigned char c;
	// watch out for the type of length, could be over flow. Limits = 256MB
	// No boundary check for 256MB!!
	do {
		c = (unsigned char) (length % 128);
		length /= 128;
		if(length > 0) c |= (unsigned char) (0x80); // If there is still another byte following
		buf[(*st)++] = c;
	} while(length > 0);
	// At this point, *st should be the next position for a new part of data in the packet
}

unsigned char *connectTxBufferHeaderParser(ConnectBufferProofread *params, unsigned char *buf) {
	unsigned char *op = buf;
	// Get packet type
	unsigned char *ele1 = op;
	unsigned int multiplier = 1;
	int cnt = 0;
	unsigned char *ele2;
	unsigned int x;
	unsigned char *op2;
	params->PacketType = *ele1;
	op++;
	// Get remaining length (length bytes more than 4 bytes are ignored)
	params->RemainingLength = 0;
	do {
		ele2 = op;
		params->RemainingLength += ((unsigned int) (*ele2 & (0x7F)) * multiplier);
		multiplier *= 128;
		cnt++;
		op++;
	} while((*ele2 & (0x80)) != 0 && cnt < 4);
	// At this point, op should be updated to the start address of the next chunk of information
	// Get protocol length
	params->ProtocolLength = 0;
	params->ProtocolLength += (256 * (unsigned int) (*op++));
	params->ProtocolLength += (unsigned int) (*op++);
	// Get protocol name
	for(x = 0; x < params->ProtocolLength; x++) {
		params->ProtocolName[x] = *op;
		op++;
	}
	// Get protocol level
	params->ProtocolLevel = (unsigned int) (*op++);
	// Get connect flags
	params->ConnectFlag = (*op++);
	// Get keepalive
	op2 = op;
	params->KeepAlive = 0;
	params->KeepAlive += (256 * (unsigned int) (*op2++)); // get rid of the sign bit
	op++;
	params->KeepAlive += (unsigned int) (*op2++);
	op++;

	return op;
}

bool isConnectTxBufFlagCorrect(IoT_Client_Connect_Params *settings, ConnectBufferProofread *readRes) {
	bool ret = true;
	int i;
	unsigned char myByte[8]; // Construct our own connect flag byte according to the settings
#if !DISABLE_METRICS
	myByte[0] = (unsigned char) (1); // User Name Flag
#else
	myByte[0] = (unsigned char) (settings->pUsername == NULL ? 0 : 1); // User Name Flag
#endif
	myByte[1] = (unsigned char) (settings->pPassword == NULL ? 0 : 1); // Password Flag
	myByte[2] = 0; // Will Retain
	// QoS
	if(QOS1 == settings->will.qos) {
		myByte[3] = 0;
		myByte[4] = 1;
	} else {    // default QoS is QOS0
		myByte[3] = 0;
		myByte[4] = 0;
	}
	//
	myByte[5] = (unsigned char) settings->isWillMsgPresent; // Will Flag
	myByte[6] = (unsigned char) settings->isCleanSession; // Clean Session
	myByte[7] = 0; // Retained
	//
	for(i = 0; i < 8; i++) {
		if(myByte[i] != (unsigned char) (((readRes->ConnectFlag) >> (7 - i)) & 0x01)) {
			printf("ex %x ac %x\n", (unsigned char) (((readRes->ConnectFlag) >> (7 - i)) & 0x01) + '0', myByte[i]);
			ret = false;
			break;
		}
	}
	return ret;
}

bool isConnectTxBufPayloadCorrect(IoT_Client_Connect_Params *settings, unsigned char *payloadBuf) {
	// Construct our own payload according to the settings to see if the real one matches with it
	unsigned int ClientIDLen = (unsigned int) strlen(settings->pClientID);
	unsigned int WillTopicLen = (unsigned int) strlen(settings->will.pTopicName);
	unsigned int WillMsgLen = (unsigned int) strlen(settings->will.pMessage);
#if !DISABLE_METRICS
	if (0 == strlen(pUsernameTemp)) {
		snprintf(pUsernameTemp, SDK_METRICS_LEN, SDK_METRICS_TEMPLATE,
			 VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
	}
	unsigned int UsernameLen = (unsigned int)strlen(pUsernameTemp);
#else
	unsigned int UsernameLen = (unsigned int) settings->usernameLen;
#endif
	unsigned int PasswordLen = (unsigned int) settings->passwordLen;
	unsigned int myPayloadLen = ClientIDLen + 2 + WillTopicLen + 2 + WillMsgLen + UsernameLen + 2 + PasswordLen;
	char *myPayload = (char *) malloc(sizeof(char) * (myPayloadLen + 1)); // reserve 1 byte for '\0'
	// Construction starts...
	unsigned int i;
	bool ret = false;
	char *op = myPayload;
	*op = (char) (ClientIDLen & 0x0FF00); // MSB There is a writeInt inside paho... MQTTString.lenstring.len != 0
	op++;
	*op = (char) (ClientIDLen & 0x00FF); // LSB
	op++;
	// ClientID
	for(i = 0; i < ClientIDLen; i++) {
		*op = settings->pClientID[i];
		op++;
	}
	if(true == settings->isWillMsgPresent) {
		// WillTopic
		for(i = 0; i < WillTopicLen; i++) {
			*op = settings->will.pTopicName[i];
			op++;
		}
		// WillMsg Len
		*op = (char) (WillMsgLen & 0x0FF00); // MSB
		op++;
		*op = (char) (WillMsgLen & 0x00FF); // LSB
		op++;
		// WillMsg
		for(i = 0; i < WillMsgLen; i++) {
			*op = settings->will.pMessage[i];
			op++;
		}
	}
	// Username
#if !DISABLE_METRICS
	for(i = 0; i < strlen(pUsernameTemp); i++)
	{
		*op = pUsernameTemp[i];
		op++;
	}
#else
	if(NULL != settings->pUsername) {
		for(i = 0; i < UsernameLen; i++) {
			*op = settings->pUsername[i];
			op++;
		}
	}
#endif
	// PasswordLen + Password
	if(NULL != settings->pPassword) {
		*op = (char) (PasswordLen & 0x0FF00); // MSB
		op++;
		*op = (char) (PasswordLen & 0x00FF); // LSB
		op++;
		//
		for(i = 0; i < PasswordLen; i++) {
			*op = settings->pPassword[i];
			op++;
		}
	}
	//
	*op = '\0';
	ret = strcmp(myPayload, (const char *)payloadBuf) == 0 ? true : false;
	free(myPayload);
	return ret;
}

void printPrfrdParams(ConnectBufferProofread *params) {
	unsigned int i;
	printf("\n----------------\n");
	printf("PacketType: %x\n", params->PacketType);
	printf("RemainingLength: %u\n", params->RemainingLength);
	printf("ProtocolLength: %u\n", params->ProtocolLength);
	printf("ProtocolName: ");
	for(i = 0; i < params->ProtocolLength; i++) {
		printf("%c", params->ProtocolName[i]);
	}
	printf("\n");
	printf("ProtocolLevel: %u\n", params->ProtocolLevel);
	printf("ConnectFlag: %x\n", params->ConnectFlag);
	printf("KeepAliveInterval: %u\n", params->KeepAlive);
	printf("----------------\n");
}

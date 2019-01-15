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
 * @file aws_iot_tests_unit_common_tests_helper.h
 * @brief IoT Client Unit Testing - Common Tests Helper
 */

#include <stdio.h>
#include <string.h>
#include <CppUTest/TestHarness_c.h>

#include "aws_iot_mqtt_client_interface.h"
#include "aws_iot_log.h"
#include "aws_iot_tests_unit_helper_functions.h"

static IoT_Client_Init_Params initParams;
static IoT_Client_Connect_Params connectParams;
static IoT_Publish_Message_Params testPubMsgParams;
static AWS_IoT_Client iotClient;

static char subTopic[10] = "sdk/Test";
static uint16_t subTopicLen = 8;
char cPayload[100];

char cbBuffer[AWS_IOT_MQTT_TX_BUF_LEN + 2];

static void iot_tests_unit_common_subscribe_callback_handler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen, IoT_Publish_Message_Params *params,
				   void *pData) {
	char *tmp = params->payload;
	unsigned int i;

	IOT_UNUSED(pClient);
	IOT_UNUSED(topicName);
	IOT_UNUSED(topicNameLen);
	IOT_UNUSED(pData);

	for(i = 0; i < params->payloadLen; i++) {
		cbBuffer[i] = tmp[i];
	}
}

TEST_GROUP_C_SETUP(CommonTests) {
	ResetTLSBuffer();
	InitMQTTParamsSetup(&initParams, AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT, false, NULL);
	initParams.mqttCommandTimeout_ms = 2000;
	IoT_Error_t rc = aws_iot_mqtt_init(&iotClient, &initParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	ConnectMQTTParamsSetup(&connectParams, AWS_IOT_MQTT_CLIENT_ID, (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID));
	setTLSRxBufferForConnack(&connectParams, 0, 0);
	rc = aws_iot_mqtt_connect(&iotClient, &connectParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	IOT_DEBUG("\n\nMQTT Status State : %d, RC : %d\n\n", aws_iot_mqtt_get_client_state(&iotClient), rc);

	testPubMsgParams.qos = QOS1;
	testPubMsgParams.isRetained = 0;
	snprintf(cPayload, 100, "%s : %d ", "hello from SDK", 0);
	testPubMsgParams.payload = (void *) cPayload;
	testPubMsgParams.payloadLen = strlen(cPayload);

	ResetTLSBuffer();
}

TEST_GROUP_C_TEARDOWN(CommonTests) {
	/* Clean up. Not checking return code here because this is common to all tests.
	 * A test might have already caused a disconnect by this point.
	 */
	IoT_Error_t rc = aws_iot_mqtt_disconnect(&iotClient);
	IOT_UNUSED(rc);
}

TEST_C(CommonTests, NullClientGetState) {
	ClientState cs = aws_iot_mqtt_get_client_state(NULL);
	CHECK_EQUAL_C_INT(CLIENT_STATE_INVALID, cs);
}

TEST_C(CommonTests, NullClientSetAutoreconnect) {
	IoT_Error_t rc = aws_iot_mqtt_autoreconnect_set_status(NULL, true);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, rc);
}

// Unexpected Ack section
TEST_C(CommonTests, UnexpectedAckFiltering) {
	IoT_Error_t rc = FAILURE;

	IOT_DEBUG("\n-->Running CommonTests -  Unexpected Ack Filtering\n");
	// Assume we are connected and have not done anything yet
	// Connack
	setTLSRxBufferForConnack(&connectParams, 0, 0);
	rc = aws_iot_mqtt_yield(&iotClient, 1000);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	ResetTLSBuffer();
	// Puback
	setTLSRxBufferForPuback();
	rc = aws_iot_mqtt_yield(&iotClient, 1000);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	ResetTLSBuffer();
	// Suback: OoS1
	setTLSRxBufferForSuback(subTopic, subTopicLen, QOS1, testPubMsgParams);
	rc = aws_iot_mqtt_yield(&iotClient, 1000);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	ResetTLSBuffer();
	// Suback: QoS0
	setTLSRxBufferForSuback(subTopic, subTopicLen, QOS0, testPubMsgParams);
	rc = aws_iot_mqtt_yield(&iotClient, 1000);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	ResetTLSBuffer();
	// Unsuback
	setTLSRxBufferForUnsuback();
	rc = aws_iot_mqtt_yield(&iotClient, 1000);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
}

TEST_C(CommonTests, BigMQTTRxMessageIgnore) {
	uint32_t i = 0;
	IoT_Error_t rc = FAILURE;
	char expectedCallbackString[AWS_IOT_MQTT_TX_BUF_LEN + 2];

	IOT_DEBUG("\n-->Running CommonTests - Ignore Large Incoming Message \n");

	setTLSRxBufferForSuback("limitTest/topic1", 16, QOS0, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, "limitTest/topic1", 16, QOS0, iot_tests_unit_common_subscribe_callback_handler, NULL);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	for(i = 0; i < AWS_IOT_MQTT_TX_BUF_LEN; i++) {
		expectedCallbackString[i] = 'X';
	}
	expectedCallbackString[i + 1] = '\0';

	setTLSRxBufferWithMsgOnSubscribedTopic("limitTest/topic1", 16, QOS0, testPubMsgParams, expectedCallbackString);
	rc = aws_iot_mqtt_yield(&iotClient, 1000);
	CHECK_EQUAL_C_INT(MQTT_RX_BUFFER_TOO_SHORT_ERROR, rc);
}

/**
 *
 * On receiving a big message into the TLS buffer the MQTT client should flush it out, otherwise it can cause undefined behavior.
 */
TEST_C(CommonTests, BigMQTTRxMessageReadNextMessage) {
	uint32_t i = 0;
	IoT_Error_t rc = FAILURE;
	char expectedCallbackString[AWS_IOT_MQTT_TX_BUF_LEN + 2];

	IOT_DEBUG("\n-->Running CommonTests - Clear Buffer when large message received and continue reading \n");

	setTLSRxBufferForSuback("limitTest/topic1", 16, QOS0, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, "limitTest/topic1", 16, QOS0, iot_tests_unit_common_subscribe_callback_handler, NULL);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	for(i = 0; i < AWS_IOT_MQTT_TX_BUF_LEN; i++) {
		expectedCallbackString[i] = 'X';
	}
	expectedCallbackString[i + 1] = '\0';

	setTLSRxBufferWithMsgOnSubscribedTopic("limitTest/topic1", 16, QOS0, testPubMsgParams, expectedCallbackString);
	rc = aws_iot_mqtt_yield(&iotClient, 1000);
	CHECK_EQUAL_C_INT(MQTT_RX_BUFFER_TOO_SHORT_ERROR, rc);

	ResetTLSBuffer();

	rc = aws_iot_mqtt_yield(&iotClient, 1000);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	expectedCallbackString[3] = '\0';
	setTLSRxBufferWithMsgOnSubscribedTopic("limitTest/topic1", 16, QOS1, testPubMsgParams, expectedCallbackString);
	rc = aws_iot_mqtt_yield(&iotClient, 1000);
	CHECK_EQUAL_C_INT(rc, SUCCESS);
	CHECK_EQUAL_C_STRING("XXX", cbBuffer);
}

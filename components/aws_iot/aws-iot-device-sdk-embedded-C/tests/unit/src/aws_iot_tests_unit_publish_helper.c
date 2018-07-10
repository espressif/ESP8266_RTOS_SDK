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
 * @file aws_iot_tests_unit_publish_helper.c
 * @brief IoT Client Unit Testing - Publish API Tests Helper
 */

#include <stdio.h>
#include <string.h>
#include <CppUTest/TestHarness_c.h>

#include "aws_iot_mqtt_client_interface.h"
#include "aws_iot_tests_unit_helper_functions.h"
#include "aws_iot_log.h"

static IoT_Client_Init_Params initParams;
static IoT_Client_Connect_Params connectParams;
static IoT_Publish_Message_Params testPubMsgParams;
static char subTopic[10] = "sdk/Test";
static uint16_t subTopicLen = 8;

static AWS_IoT_Client iotClient;
char cPayload[100];

TEST_GROUP_C_SETUP(PublishTests) {
	IoT_Error_t rc = SUCCESS;
	ResetTLSBuffer();
	InitMQTTParamsSetup(&initParams, AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT, false, NULL);
	initParams.mqttCommandTimeout_ms = 2000;
	rc = aws_iot_mqtt_init(&iotClient, &initParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	ConnectMQTTParamsSetup(&connectParams, AWS_IOT_MQTT_CLIENT_ID, (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID));
	setTLSRxBufferForConnack(&connectParams, 0, 0);
	rc = aws_iot_mqtt_connect(&iotClient, &connectParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	IOT_DEBUG("MQTT Status State : %d, RC : %d\n\n", aws_iot_mqtt_get_client_state(&iotClient), rc);

	testPubMsgParams.qos = QOS1;
	testPubMsgParams.isRetained = 0;
	sprintf(cPayload, "%s : %d ", "hello from SDK", 0);
	testPubMsgParams.payload = (void *) cPayload;
	testPubMsgParams.payloadLen = strlen(cPayload);

	ResetTLSBuffer();
}

TEST_GROUP_C_TEARDOWN(PublishTests) { }

/* E:1 - Publish with Null/empty client instance */
TEST_C(PublishTests, PublishNullClient) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Publish Tests - E:1 - Publish with Null/empty client instance \n");

	testPubMsgParams.qos = QOS1;
	testPubMsgParams.isRetained = 0;
	testPubMsgParams.payload = "Message";
	testPubMsgParams.payloadLen = 7;

	rc = aws_iot_mqtt_publish(NULL, "sdkTest/Sub", 11, &testPubMsgParams);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, rc);

	IOT_DEBUG("-->Success - E:1 - Publish with Null/empty client instance \n");
}

/* E:2 - Publish with Null/empty Topic Name */
TEST_C(PublishTests, PublishNullTopic) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Publish Tests - E:2 - Publish with Null/empty Topic Name \n");

	testPubMsgParams.qos = QOS1;
	testPubMsgParams.isRetained = 0;
	testPubMsgParams.payload = "Message";
	testPubMsgParams.payloadLen = 7;

	rc = aws_iot_mqtt_publish(&iotClient, NULL, 11, &testPubMsgParams);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, rc);

	rc = aws_iot_mqtt_publish(&iotClient, "sdkTest/Sub", 0, &testPubMsgParams);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, rc);

	IOT_DEBUG("-->Success - E:2 - Publish with Null/empty Topic Name \n");
}

/* E:3 - Publish with Null/empty payload */
TEST_C(PublishTests, PublishNullParams) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Publish Tests - E:3 - Publish with Null/empty payload \n");

	rc = aws_iot_mqtt_publish(&iotClient, "sdkTest/Sub", 11, NULL);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, rc);

	testPubMsgParams.qos = QOS1;
	testPubMsgParams.isRetained = 0;
	testPubMsgParams.payload = NULL;
	testPubMsgParams.payloadLen = 0;

	rc = aws_iot_mqtt_publish(&iotClient, "sdkTest/Sub", 11, &testPubMsgParams);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, rc);

	IOT_DEBUG("-->Success - E:3 - Publish with Null/empty payload \n");
}

/* E:4 - Publish with network disconnected */
TEST_C(PublishTests, PublishNetworkDisconnected) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Publish Tests - E:4 - Publish with network disconnected \n");

	/* Ensure network is disconnected */
	rc = aws_iot_mqtt_disconnect(&iotClient);

	testPubMsgParams.qos = QOS1;
	testPubMsgParams.isRetained = 0;
	testPubMsgParams.payload = NULL;
	testPubMsgParams.payloadLen = 0;

	rc = aws_iot_mqtt_publish(&iotClient, "sdkTest/Sub", 11, &testPubMsgParams);
	CHECK_EQUAL_C_INT(NETWORK_DISCONNECTED_ERROR, rc);

	IOT_DEBUG("-->Success - E:4 - Publish with network disconnected \n");
}

/* E:6 - Publish with QoS1 send success, Puback not received */
TEST_C(PublishTests, publishQoS1FailureToReceivePuback) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Publish Tests - E:6 - Publish with QoS1 send success, Puback not received \n");

	rc = aws_iot_mqtt_publish(&iotClient, subTopic, subTopicLen, &testPubMsgParams);
	CHECK_EQUAL_C_INT(MQTT_REQUEST_TIMEOUT_ERROR, rc);

	IOT_DEBUG("-->Success - E:6 - Publish with QoS1 send success, Puback not received \n");
}

/* E:7 - Publish with QoS1 send success, Delayed Puback received after command timeout */
TEST_C(PublishTests, publishQoS1FailureDelayedPuback) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Publish Tests - E:7 - Publish with QoS1 send success, Delayed Puback received after command timeout \n");

	setTLSRxBufferDelay(10, 0);
	setTLSRxBufferForPuback();
	rc = aws_iot_mqtt_publish(&iotClient, subTopic, subTopicLen, &testPubMsgParams);
	CHECK_EQUAL_C_INT(MQTT_REQUEST_TIMEOUT_ERROR, rc);

	IOT_DEBUG("-->Success - E:7 - Publish with QoS1 send success, Delayed Puback received after command timeout \n");
}

/* E:8 - Publish with send success, Delayed Puback received before command timeout */
TEST_C(PublishTests, publishQoS1Success10msDelayedPuback) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Publish Tests - E:8 - Publish with send success, Delayed Puback received before command timeout \n");

	ResetTLSBuffer();
	setTLSRxBufferDelay(0, (int) iotClient.clientData.commandTimeoutMs/2);
	setTLSRxBufferForPuback();
	rc = aws_iot_mqtt_publish(&iotClient, subTopic, subTopicLen, &testPubMsgParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	IOT_DEBUG("-->Success - E:8 - Publish with send success, Delayed Puback received before command timeout \n");
}

/* E:9 - Publish QoS0 success */
TEST_C(PublishTests, publishQoS0NoPubackSuccess) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Publish Tests - E:9 - Publish QoS0 success \n");

	testPubMsgParams.qos = QOS0; // switch to a Qos0 PUB
	rc = aws_iot_mqtt_publish(&iotClient, subTopic, subTopicLen, &testPubMsgParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	IOT_DEBUG("-->Success - E:9 - Publish QoS0 success \n");
}

/* E:10 - Publish with QoS1 send success, Puback received */
TEST_C(PublishTests, publishQoS1Success) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Publish Tests - E:10 - Publish with QoS1 send success, Puback received \n");

	setTLSRxBufferForPuback();
	rc = aws_iot_mqtt_publish(&iotClient, subTopic, subTopicLen, &testPubMsgParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	IOT_DEBUG("-->Success - E:10 - Publish with QoS1 send success, Puback received \n");
}

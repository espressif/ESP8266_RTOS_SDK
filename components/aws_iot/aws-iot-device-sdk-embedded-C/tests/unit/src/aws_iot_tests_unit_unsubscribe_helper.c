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
 * @file aws_iot_tests_unit_unsubscribe_helper.c
 * @brief IoT Client Unit Testing - Unsubscribe API Tests Helper
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
static char CallbackMsgString[100];
char cPayload[100];

static void iot_subscribe_callback_handler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
										   IoT_Publish_Message_Params *params, void *pData) {
	if(NULL == pClient || NULL == topicName || 0 == topicNameLen) {
		return;
	}

	IOT_UNUSED(pData);

	char *tmp = params->payload;
	unsigned int i;

	for(i = 0; i < (params->payloadLen); i++) {
		CallbackMsgString[i] = tmp[i];
	}
}

TEST_GROUP_C_SETUP(UnsubscribeTests) {
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
	snprintf(cPayload, 100, "%s : %d ", "hello from SDK", 0);
	testPubMsgParams.payload = (void *) cPayload;
	testPubMsgParams.payloadLen = strlen(cPayload);

	ResetTLSBuffer();
}

TEST_GROUP_C_TEARDOWN(UnsubscribeTests) { }

/* D:1 - Unsubscribe with Null/empty client instance */
TEST_C(UnsubscribeTests, UnsubscribeNullClient) {
	IoT_Error_t rc = aws_iot_mqtt_unsubscribe(NULL, "sdkTest/Sub", 11);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, rc);
}

/* D:2 - Unsubscribe with Null/empty topic name */
TEST_C(UnsubscribeTests, UnsubscribeNullTopic) {
	IoT_Error_t rc = aws_iot_mqtt_unsubscribe(&iotClient, NULL, 11);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, rc);
}

/* D:3 - Unsubscribe, Not subscribed to topic */
TEST_C(UnsubscribeTests, UnsubscribeNotSubscribed) {
	IoT_Error_t rc = aws_iot_mqtt_unsubscribe(&iotClient, "sdkTest/Sub", 11);
	CHECK_EQUAL_C_INT(FAILURE, rc);
}

/* D:4 - Unsubscribe, QoS0, No response, timeout */
TEST_C(UnsubscribeTests, unsubscribeQoS0FailureOnNoUnsuback) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Unsubscribe Tests - D:4 - Unsubscribe, QoS0, No response, timeout \n");

	// First, subscribe to a topic
	setTLSRxBufferForSuback(subTopic, subTopicLen, QOS0, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, subTopic, subTopicLen, QOS0, iot_subscribe_callback_handler,
								NULL);
	if(SUCCESS == rc) {
		// Then, unsubscribe
		rc = aws_iot_mqtt_unsubscribe(&iotClient, subTopic, (uint16_t) strlen(subTopic));
		CHECK_EQUAL_C_INT(MQTT_REQUEST_TIMEOUT_ERROR, rc);
	}

	IOT_DEBUG("-->Success - D:4 - Unsubscribe, QoS0, No response, timeout \n");
}

/* D:5 - Unsubscribe, QoS1, No response, timeout */
TEST_C(UnsubscribeTests, unsubscribeQoS1FailureOnNoUnsuback) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Unsubscribe Tests - D:5 - Unsubscribe, QoS1, No response, timeout \n");

	// First, subscribe to a topic
	setTLSRxBufferForSuback(subTopic, subTopicLen, QOS1, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, subTopic, subTopicLen, QOS1, iot_subscribe_callback_handler,
								NULL);
	if(SUCCESS == rc) {
		// Then, unsubscribe
		rc = aws_iot_mqtt_unsubscribe(&iotClient, subTopic, (uint16_t) strlen(subTopic));
		CHECK_EQUAL_C_INT(MQTT_REQUEST_TIMEOUT_ERROR, rc);
	}

	IOT_DEBUG("-->Success - D:5 - Unsubscribe, QoS1, No response, timeout \n");
}

/* D:6 - Unsubscribe, QoS0, success */
TEST_C(UnsubscribeTests, unsubscribeQoS0WithUnsubackSuccess) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Unsubscribe Tests - D:6 - Unsubscribe, QoS0, success \n");

	// First, subscribe to a topic
	setTLSRxBufferForSuback(subTopic, subTopicLen, QOS0, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, subTopic, subTopicLen, QOS0, iot_subscribe_callback_handler,
								NULL);
	if(SUCCESS == rc) {
		// Then, unsubscribe
		setTLSRxBufferForUnsuback();
		rc = aws_iot_mqtt_unsubscribe(&iotClient, subTopic, (uint16_t) strlen(subTopic));
		CHECK_EQUAL_C_INT(SUCCESS, rc);
	}

	IOT_DEBUG("-->Success - D:6 - Unsubscribe, QoS0, success \n");
}

/* D:7 - Unsubscribe, QoS0, half command timeout delayed unsuback, success */
TEST_C(UnsubscribeTests, unsubscribeQoS0WithDelayedUnsubackSuccess) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Unsubscribe Tests - D:7 - Unsubscribe, QoS0, half command timeout delayed unsuback, success \n");

	// First, subscribe to a topic
	setTLSRxBufferForSuback(subTopic, subTopicLen, QOS0, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, subTopic, subTopicLen, QOS0, iot_subscribe_callback_handler,
								NULL);
	if(SUCCESS == rc) {
		// Then, unsubscribe
		setTLSRxBufferForUnsuback();
		setTLSRxBufferDelay(0, (int) iotClient.clientData.commandTimeoutMs/2);
		rc = aws_iot_mqtt_unsubscribe(&iotClient, subTopic, (uint16_t) strlen(subTopic));
		CHECK_EQUAL_C_INT(SUCCESS, rc);
	}

	IOT_DEBUG("-->Success - D:7 - Unsubscribe, QoS0, half command timeout delayed unsuback, success \n");
}

/* D:8 - Unsubscribe, QoS1, success */
TEST_C(UnsubscribeTests, unsubscribeQoS1WithUnsubackSuccess) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Unsubscribe Tests - D:8 - Unsubscribe, QoS1, success \n");

	// First, subscribe to a topic
	setTLSRxBufferForSuback(subTopic, subTopicLen, QOS1, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, subTopic, subTopicLen, QOS1, iot_subscribe_callback_handler,
								NULL);
	if(SUCCESS == rc) {
		// Then, unsubscribe
		setTLSRxBufferForUnsuback();
		rc = aws_iot_mqtt_unsubscribe(&iotClient, subTopic, (uint16_t) strlen(subTopic));
		CHECK_EQUAL_C_INT(SUCCESS, rc);
	}

	IOT_DEBUG("-->Success - D:8 - Unsubscribe, QoS1, success \n");
}

/* D:9 - Unsubscribe, QoS1, half command timeout delayed unsuback, success */
TEST_C(UnsubscribeTests, unsubscribeQoS1WithDelayedUnsubackSuccess) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Unsubscribe Tests - D:9 - Unsubscribe, QoS1, half command timeout delayed unsuback, success \n");

	// First, subscribe to a topic
	setTLSRxBufferForSuback(subTopic, subTopicLen, QOS1, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, subTopic, subTopicLen, QOS1, iot_subscribe_callback_handler,
								NULL);
	if(SUCCESS == rc) {
		// Then, unsubscribe
		setTLSRxBufferForUnsuback();
		setTLSRxBufferDelay(0, (int) iotClient.clientData.commandTimeoutMs/2);
		rc = aws_iot_mqtt_unsubscribe(&iotClient, subTopic, (uint16_t) strlen(subTopic));
		CHECK_EQUAL_C_INT(SUCCESS, rc);
	}

	IOT_DEBUG("-->Success - D:9 - Unsubscribe, QoS1, half command timeout delayed unsuback, success \n");
}

/* D:10 - Unsubscribe, success, message on topic ignored
 * 1. Subscribe to topic 1
 * 2. Send message and receive it
 * 3. Unsubscribe to topic 1
 * 4. Should not receive message
 */
TEST_C(UnsubscribeTests, MsgAfterUnsubscribe) {
	IoT_Error_t rc = SUCCESS;
	char expectedCallbackString[100];

	IOT_DEBUG("-->Running Unsubscribe Tests - D:10 - Unsubscribe, success, message on topic ignored \n");

	// 1.
	setTLSRxBufferForSuback("topic1", 6, QOS0, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, "topic1", 6, QOS0, iot_subscribe_callback_handler, NULL);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	// 2.
	snprintf(expectedCallbackString, 100, "Message for topic1");
	setTLSRxBufferWithMsgOnSubscribedTopic("topic1", 6, QOS1, testPubMsgParams, expectedCallbackString);
	rc = aws_iot_mqtt_yield(&iotClient, 100);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_STRING(expectedCallbackString, CallbackMsgString);

	//3.
	setTLSRxBufferForUnsuback();
	rc = aws_iot_mqtt_unsubscribe(&iotClient, "topic1", 6);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	//reset the string
	snprintf(CallbackMsgString, 100, " ");

	// 4.
	// Have a new message published to that topic coming in
	snprintf(expectedCallbackString, 100, "Message after unsubscribe");
	setTLSRxBufferWithMsgOnSubscribedTopic("topic1", 6, QOS0, testPubMsgParams, expectedCallbackString);
	rc = aws_iot_mqtt_yield(&iotClient, 100);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	// No new msg was received
	CHECK_EQUAL_C_STRING(" ", CallbackMsgString);

	IOT_DEBUG("-->Success - D:10 - Unsubscribe, success, message on topic ignored \n");
}

/* D:11 - Unsubscribe after max topics reached
 * 1. Subscribe to max topics + 1 fail for last subscription
 * 2. Unsubscribe from one topic
 * 3. Subscribe again and should have no error
 * 4. Receive msg test - last subscribed topic
 */
TEST_C(UnsubscribeTests, MaxTopicsSubscription) {
	IoT_Error_t rc = SUCCESS;
	int i = 0;
	char topics[AWS_IOT_MQTT_NUM_SUBSCRIBE_HANDLERS + 1][10];
	char expectedCallbackString[] = "Message after subscribe - topic[i]";

	IOT_DEBUG("-->Running Unsubscribe Tests - D:11 - Unsubscribe after max topics reached \n");

	// 1.
	for(i = 0; i < AWS_IOT_MQTT_NUM_SUBSCRIBE_HANDLERS; i++) {
		snprintf(topics[i], 10, "topic-%d", i);
		setTLSRxBufferForSuback(topics[i], strlen(topics[i]), QOS0, testPubMsgParams);
		rc = aws_iot_mqtt_subscribe(&iotClient, topics[i], (uint16_t) strlen(topics[i]), QOS0, iot_subscribe_callback_handler,
									NULL);
		CHECK_EQUAL_C_INT(SUCCESS, rc);
	}
	snprintf(topics[i], 10, "topic-%d", i);
	setTLSRxBufferForSuback(topics[i], strlen(topics[i]), QOS0, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, topics[i], (uint16_t) strlen(topics[i]), QOS0, iot_subscribe_callback_handler, NULL);
	CHECK_EQUAL_C_INT(MQTT_MAX_SUBSCRIPTIONS_REACHED_ERROR, rc);

	// 2.
	setTLSRxBufferForUnsuback();
	rc = aws_iot_mqtt_unsubscribe(&iotClient, topics[i - 1], (uint16_t) strlen(topics[i - 1]));
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	//3.
	setTLSRxBufferForSuback(topics[i], strlen(topics[i]), QOS0, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, topics[i], (uint16_t) strlen(topics[i]), QOS0, iot_subscribe_callback_handler, NULL);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	//4.
	setTLSRxBufferWithMsgOnSubscribedTopic(topics[i], strlen(topics[i]), QOS1, testPubMsgParams,
										   expectedCallbackString);
	rc = aws_iot_mqtt_yield(&iotClient, 100);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_STRING(expectedCallbackString, CallbackMsgString);

	IOT_DEBUG("-->Success - D:11 - Unsubscribe after max topics reached \n");
}

/* D:12 - Repeated Subscribe and Unsubscribe
 * 1. subscribe and unsubscribe for more than the max subscribed topic
 * 2. ensure every time the subscribed topic msg is received
 */
TEST_C(UnsubscribeTests, RepeatedSubUnSub) {
	IoT_Error_t rc = SUCCESS;
	int i = 0;
	char expectedCallbackString[100];
	char topics[3 * AWS_IOT_MQTT_NUM_SUBSCRIBE_HANDLERS][10];

	IOT_DEBUG("-->Running Unsubscribe Tests - D:12 - Repeated Subscribe and Unsubscribe \n");


	for(i = 0; i < 3 * AWS_IOT_MQTT_NUM_SUBSCRIBE_HANDLERS; i++) {
		//1.
		snprintf(topics[i], 10, "topic-%d", i);
		setTLSRxBufferForSuback(topics[i], 10, QOS0, testPubMsgParams);
		rc = aws_iot_mqtt_subscribe(&iotClient, topics[i], 10, QOS0, iot_subscribe_callback_handler, NULL);
		CHECK_EQUAL_C_INT(SUCCESS, rc);
		snprintf(expectedCallbackString, 10, "message##%d", i);
		testPubMsgParams.payload = (void *) expectedCallbackString;
		testPubMsgParams.payloadLen = strlen(expectedCallbackString);
		setTLSRxBufferWithMsgOnSubscribedTopic(topics[i], strlen(topics[i]), QOS1, testPubMsgParams,
											   expectedCallbackString);
		rc = aws_iot_mqtt_yield(&iotClient, 100);
		CHECK_EQUAL_C_INT(SUCCESS, rc);
		CHECK_EQUAL_C_STRING(expectedCallbackString, CallbackMsgString);

		//2.
		setTLSRxBufferForUnsuback();
		rc = aws_iot_mqtt_unsubscribe(&iotClient, topics[i], (uint16_t) strlen(topics[i]));
		CHECK_EQUAL_C_INT(SUCCESS, rc);
	}

	IOT_DEBUG("-->Success - D:12 - Repeated Subscribe and Unsubscribe \n");
}

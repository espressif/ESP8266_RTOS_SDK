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
 * @file aws_iot_tests_unit_subscribe_helper.c
 * @brief IoT Client Unit Testing - Subscribe API Tests Helper
 */

#include <stdio.h>
#include <string.h>
#include <CppUTest/TestHarness_c.h>

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

static char CallbackMsgString1[100] = {"XXXX"};
static char CallbackMsgString2[100] = {"XXXX"};
static char CallbackMsgString3[100] = {"XXXX"};
static char CallbackMsgString4[100] = {"XXXX"};
static char CallbackMsgString5[100] = {"XXXX"};
static char CallbackMsgString6[100] = {"XXXX"};

static void iot_subscribe_callback_handler(AWS_IoT_Client *pClient, char *topicName,
										   uint16_t topicNameLen, IoT_Publish_Message_Params *params, void *pData) {
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

static void iot_subscribe_callback_handler1(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
											IoT_Publish_Message_Params *params, void *pData) {
	if(NULL == pClient || NULL == topicName || 0 == topicNameLen) {
		return;
	}

	IOT_UNUSED(pData);

	char *tmp = params->payload;
	unsigned int i;

	printf("callback topic %s\n", topicName);
	for(i = 0; i < (params->payloadLen); i++) {
		CallbackMsgString1[i] = tmp[i];
	}
}

static void iot_subscribe_callback_handler2(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
											IoT_Publish_Message_Params *params, void *pData) {
	if(NULL == pClient || NULL == topicName || 0 == topicNameLen) {
		return;
	}

	IOT_UNUSED(pData);

	char *tmp = params->payload;
	unsigned int i;

	for(i = 0; i < (params->payloadLen); i++) {
		CallbackMsgString2[i] = tmp[i];
	}
}

static void iot_subscribe_callback_handler3(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
											IoT_Publish_Message_Params *params, void *pData) {
	if(NULL == pClient || NULL == topicName || 0 == topicNameLen) {
		return;
	}

	IOT_UNUSED(pData);

	char *tmp = params->payload;
	unsigned int i;

	for(i = 0; i < (params->payloadLen); i++) {
		CallbackMsgString3[i] = tmp[i];
	}
}

static void iot_subscribe_callback_handler4(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
											IoT_Publish_Message_Params *params, void *pData) {
	if(NULL == pClient || NULL == topicName || 0 == topicNameLen) {
		return;
	}

	IOT_UNUSED(pData);

	char *tmp = params->payload;
	unsigned int i;

	for(i = 0; i < (params->payloadLen); i++) {
		CallbackMsgString4[i] = tmp[i];
	}
}

static void iot_subscribe_callback_handler5(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
											IoT_Publish_Message_Params *params, void *pData) {
	if(NULL == pClient || NULL == topicName || 0 == topicNameLen) {
		return;
	}

	IOT_UNUSED(pData);

	char *tmp = params->payload;
	unsigned int i;

	for(i = 0; i < (params->payloadLen); i++) {
		CallbackMsgString5[i] = tmp[i];
	}
}

static void iot_subscribe_callback_handler6(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
											IoT_Publish_Message_Params *params, void *pData) {
	if(NULL == pClient || NULL == topicName || 0 == topicNameLen) {
		return;
	}

	IOT_UNUSED(pData);

	char *tmp = params->payload;
	unsigned int i;

	for(i = 0; i < (params->payloadLen); i++) {
		CallbackMsgString6[i] = tmp[i];
	}
}

TEST_GROUP_C_SETUP(SubscribeTests) {
	IoT_Error_t rc;
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

TEST_GROUP_C_TEARDOWN(SubscribeTests) { }



/* C:1 - Subscribe with Null/empty Client Instance */
TEST_C(SubscribeTests, SubscribeNullClient) {
	IoT_Error_t rc = aws_iot_mqtt_subscribe(NULL, "sdkTest/Sub", 11, QOS1, iot_subscribe_callback_handler, &iotClient);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, rc);
}

/* C:2 - Subscribe with Null/empty Topic Name */
TEST_C(SubscribeTests, SubscribeNullTopic) {
	IoT_Error_t rc = aws_iot_mqtt_subscribe(&iotClient, NULL, 11, QOS1, iot_subscribe_callback_handler, &iotClient);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, rc);
}

/* C:3 - Subscribe with Null client callback */
TEST_C(SubscribeTests, SubscribeNullSubscribeHandler) {
	IoT_Error_t rc = aws_iot_mqtt_subscribe(&iotClient, "sdkTest/Sub", 11, QOS1, NULL, &iotClient);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, rc);
}
/* C:4 - Subscribe with Null client callback data */
TEST_C(SubscribeTests, SubscribeNullSubscribeHandlerData) {
	setTLSRxBufferForSuback(subTopic, subTopicLen, QOS0, testPubMsgParams);
	IoT_Error_t rc = aws_iot_mqtt_subscribe(&iotClient, "sdkTest/Sub", 11, QOS1, iot_subscribe_callback_handler, NULL);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
}
/* C:5 - Subscribe with no connection */
TEST_C(SubscribeTests, SubscribeNoConnection) {
	/* Disconnect first */
	IoT_Error_t rc = aws_iot_mqtt_disconnect(&iotClient);

	rc = aws_iot_mqtt_subscribe(&iotClient, "sdkTest/Sub", 11, QOS1, iot_subscribe_callback_handler, NULL);
	CHECK_EQUAL_C_INT(NETWORK_DISCONNECTED_ERROR, rc);
}
/* C:6 - Subscribe QoS2, error */
/* Not required, QoS enum doesn't have value for QoS2 */

/* C:7 - Subscribe attempt, QoS0, no response timeout */
TEST_C(SubscribeTests, subscribeQoS0FailureOnNoSuback) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Subscribe Tests - C:7 - Subscribe attempt, QoS0, no response timeout \n");

	rc = aws_iot_mqtt_subscribe(&iotClient, subTopic, subTopicLen, QOS0, iot_subscribe_callback_handler, NULL);
	CHECK_EQUAL_C_INT(MQTT_REQUEST_TIMEOUT_ERROR, rc);

	IOT_DEBUG("-->Success - C:7 - Subscribe attempt, QoS0, no response timeout \n");
}
/* C:8 - Subscribe attempt, QoS1, no response timeout */
TEST_C(SubscribeTests, subscribeQoS1FailureOnNoSuback) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Subscribe Tests - C:8 - Subscribe attempt, QoS1, no response timeout \n");

	rc = aws_iot_mqtt_subscribe(&iotClient, subTopic, subTopicLen, QOS1, iot_subscribe_callback_handler, NULL);
	CHECK_EQUAL_C_INT(MQTT_REQUEST_TIMEOUT_ERROR, rc);

	IOT_DEBUG("-->Success - C:8 - Subscribe attempt, QoS1, no response timeout \n");
}

/* C:9 - Subscribe QoS0 success, suback received */
TEST_C(SubscribeTests, subscribeQoS0Success) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Subscribe Tests - C:9 - Subscribe QoS0 success, suback received \n");

	setTLSRxBufferForSuback(subTopic, subTopicLen, QOS0, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, subTopic, subTopicLen, QOS0, iot_subscribe_callback_handler, NULL);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	IOT_DEBUG("-->Success - C:9 - Subscribe QoS0 success, suback received \n");
}

/* C:10 - Subscribe QoS1 success, suback received */
TEST_C(SubscribeTests, subscribeQoS1Success) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Subscribe Tests - C:10 - Subscribe QoS1 success, suback received \n");

	setTLSRxBufferForSuback(subTopic, subTopicLen, QOS1, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, subTopic, subTopicLen, QOS1, iot_subscribe_callback_handler, NULL);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	IOT_DEBUG("-->Success - C:10 - Subscribe QoS1 success, suback received \n");
}

/* C:11 - Subscribe, QoS0, delayed suback, success */
TEST_C(SubscribeTests, subscribeQoS0WithDelayedSubackSuccess) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Subscribe Tests - C:11 - Subscribe, QoS0, delayed suback, success \n");

	setTLSRxBufferForSuback(subTopic, subTopicLen, QOS0, testPubMsgParams);
	setTLSRxBufferDelay(0, (int) iotClient.clientData.commandTimeoutMs/2);
	rc = aws_iot_mqtt_subscribe(&iotClient, subTopic, subTopicLen, QOS0, iot_subscribe_callback_handler, NULL);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	IOT_DEBUG("-->Success - C:11 - Subscribe, QoS0, delayed suback, success \n");
}

/* C:12 - Subscribe, QoS1, delayed suback, success */
TEST_C(SubscribeTests, subscribeQoS1WithDelayedSubackSuccess) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Subscribe Tests - C:12 - Subscribe, QoS1, delayed suback, success \n");

	setTLSRxBufferForSuback(subTopic, subTopicLen, QOS1, testPubMsgParams);
	setTLSRxBufferDelay(0, (int) iotClient.clientData.commandTimeoutMs/2);
	rc = aws_iot_mqtt_subscribe(&iotClient, subTopic, subTopicLen, QOS1, iot_subscribe_callback_handler, NULL);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	IOT_DEBUG("-->Success - C:12 - Subscribe, QoS1, delayed suback, success \n");
}

/* C:13 - Subscribe QoS0 success, no puback sent on message */
TEST_C(SubscribeTests, subscribeQoS0MsgReceivedAndNoPubackSent) {
	IoT_Error_t rc = SUCCESS;
	char expectedCallbackString[100] = "Test msg - unit test";

	IOT_DEBUG("-->Running Subscribe Tests - C:13 - Subscribe QoS0 success, no puback sent on message \n");

	ResetTLSBuffer();
	setTLSRxBufferForSuback(subTopic, subTopicLen, QOS0, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, subTopic, subTopicLen, QOS0, iot_subscribe_callback_handler, NULL);
	if(SUCCESS == rc) {
		testPubMsgParams.qos = QOS0;
		setTLSRxBufferWithMsgOnSubscribedTopic(subTopic, subTopicLen, QOS0, testPubMsgParams, expectedCallbackString);
		rc = aws_iot_mqtt_yield(&iotClient, 1000);
		if(SUCCESS == rc) {
			CHECK_EQUAL_C_STRING(expectedCallbackString, CallbackMsgString);
		}
	}
	CHECK_EQUAL_C_INT(0, isLastTLSTxMessagePuback());

	IOT_DEBUG("-->Success - C:13 - Subscribe QoS0 success, no puback sent on message \n");
}

/* C:14 - Subscribe QoS1 success, puback sent on message */
TEST_C(SubscribeTests, subscribeQoS1MsgReceivedAndSendPuback) {
	IoT_Error_t rc = SUCCESS;
	char expectedCallbackString[] = "0xA5A5A3";

	IOT_DEBUG("-->Running Subscribe Tests - C:14 - Subscribe QoS1 success, puback sent on message \n");

	setTLSRxBufferForSuback(subTopic, subTopicLen, QOS1, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, subTopic, subTopicLen, QOS1, iot_subscribe_callback_handler, NULL);
	if(SUCCESS == rc) {
		setTLSRxBufferWithMsgOnSubscribedTopic(subTopic, subTopicLen, QOS1, testPubMsgParams, expectedCallbackString);
		rc = aws_iot_mqtt_yield(&iotClient, 1000);
		if(SUCCESS == rc) {
			CHECK_EQUAL_C_STRING(expectedCallbackString, CallbackMsgString);
		}
		CHECK_EQUAL_C_INT(1, isLastTLSTxMessagePuback());
	}

	IOT_DEBUG("-->Success - C:14 - Subscribe QoS1 success, puback sent on message \n");
}

/* C:15 - Subscribe, malformed response */
TEST_C(SubscribeTests, subscribeMalformedResponse) {}

/* C:16 - Subscribe, multiple topics, messages on each topic */
TEST_C(SubscribeTests, SubscribeToMultipleTopicsSuccess) {
	IoT_Error_t rc = SUCCESS;
	char expectedCallbackString[] = "0xA5A5A3";

	IOT_DEBUG("-->Running Subscribe Tests - C:16 - Subscribe, multiple topics, messages on each topic \n");

	setTLSRxBufferForSuback("sdk/Test1", 9, QOS1, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, "sdk/Test1", 9, QOS1, iot_subscribe_callback_handler1, NULL);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	setTLSRxBufferForSuback("sdk/Test2", 9, QOS1, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, "sdk/Test2", 9, QOS1, iot_subscribe_callback_handler2, NULL);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	setTLSRxBufferWithMsgOnSubscribedTopic("sdk/Test1", 9, QOS1, testPubMsgParams, expectedCallbackString);
	rc = aws_iot_mqtt_yield(&iotClient, 1000);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_STRING(expectedCallbackString, CallbackMsgString1);

	IOT_DEBUG("-->Success - C:16 - Subscribe, multiple topics, messages on each topic \n");
}
/* C:17 - Subscribe, max topics, messages on each topic */
TEST_C(SubscribeTests, SubcribeToMaxAllowedTopicsSuccess) {
	IoT_Error_t rc = SUCCESS;
	char expectedCallbackString[] = "topics sdk/Test1";
	char expectedCallbackString2[] = "topics sdk/Test2";
	char expectedCallbackString3[] = "topics sdk/Test3";
	char expectedCallbackString4[] = "topics sdk/Test4";
	char expectedCallbackString5[] = "topics sdk/Test5";

	IOT_DEBUG("-->Running Subscribe Tests - C:17 - Subscribe, max topics, messages on each topic \n");

	setTLSRxBufferForSuback("sdk/Test1", 9, QOS1, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, "sdk/Test1", 9, QOS1, iot_subscribe_callback_handler1, NULL);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	setTLSRxBufferForSuback("sdk/Test2", 9, QOS1, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, "sdk/Test2", 9, QOS1, iot_subscribe_callback_handler2, NULL);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	setTLSRxBufferForSuback("sdk/Test3", 9, QOS1, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, "sdk/Test3", 9, QOS1, iot_subscribe_callback_handler3, NULL);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	setTLSRxBufferForSuback("sdk/Test4", 9, QOS1, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, "sdk/Test4", 9, QOS1, iot_subscribe_callback_handler4, NULL);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	setTLSRxBufferForSuback("sdk/Test5", 9, QOS1, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, "sdk/Test5", 9, QOS1, iot_subscribe_callback_handler5, NULL);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	setTLSRxBufferWithMsgOnSubscribedTopic("sdk/Test1", 9, QOS1, testPubMsgParams, expectedCallbackString);
	rc = aws_iot_mqtt_yield(&iotClient, 1000);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	setTLSRxBufferWithMsgOnSubscribedTopic("sdk/Test2", 9, QOS1, testPubMsgParams, expectedCallbackString2);
	rc = aws_iot_mqtt_yield(&iotClient, 1000);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	setTLSRxBufferWithMsgOnSubscribedTopic("sdk/Test3", 9, QOS1, testPubMsgParams, expectedCallbackString3);
	rc = aws_iot_mqtt_yield(&iotClient, 1000);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	setTLSRxBufferWithMsgOnSubscribedTopic("sdk/Test4", 9, QOS1, testPubMsgParams, expectedCallbackString4);
	rc = aws_iot_mqtt_yield(&iotClient, 1000);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	setTLSRxBufferWithMsgOnSubscribedTopic("sdk/Test5", 9, QOS1, testPubMsgParams, expectedCallbackString5);
	rc = aws_iot_mqtt_yield(&iotClient, 1000);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	CHECK_EQUAL_C_STRING(expectedCallbackString, CallbackMsgString1);
	CHECK_EQUAL_C_STRING(expectedCallbackString2, CallbackMsgString2);
	CHECK_EQUAL_C_STRING(expectedCallbackString3, CallbackMsgString3);
	CHECK_EQUAL_C_STRING(expectedCallbackString4, CallbackMsgString4);
	CHECK_EQUAL_C_STRING(expectedCallbackString5, CallbackMsgString5);

	IOT_DEBUG("-->Success - C:17 - Subscribe, max topics, messages on each topic \n");
}
/* C:18 - Subscribe, max topics, another subscribe */
TEST_C(SubscribeTests, SubcribeToMaxPlusOneAllowedTopicsFailure) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Subscribe Tests - C:18 - Subscribe, max topics, another subscribe \n");

	setTLSRxBufferForSuback("sdk/Test1", 9, QOS1, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, "sdk/Test1", 9, QOS1, iot_subscribe_callback_handler1, NULL);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	setTLSRxBufferForSuback("sdk/Test2", 9, QOS1, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, "sdk/Test2", 9, QOS1, iot_subscribe_callback_handler2, NULL);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	setTLSRxBufferForSuback("sdk/Test3", 9, QOS1, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, "sdk/Test3", 9, QOS1, iot_subscribe_callback_handler3, NULL);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	setTLSRxBufferForSuback("sdk/Test4", 9, QOS1, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, "sdk/Test4", 9, QOS1, iot_subscribe_callback_handler4, NULL);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	setTLSRxBufferForSuback("sdk/Test5", 9, QOS1, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, "sdk/Test5", 9, QOS1, iot_subscribe_callback_handler5, NULL);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	setTLSRxBufferForSuback("sdk/Test6", 9, QOS1, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, "sdk/Test6", 9, QOS1, iot_subscribe_callback_handler6, NULL);
	CHECK_EQUAL_C_INT(MQTT_MAX_SUBSCRIPTIONS_REACHED_ERROR, rc);

	IOT_DEBUG("-->Success - C:18 - Subscribe, max topics, another subscribe \n");
}

/* C:19 - Subscribe, '#' not last character in topic name, Failure */
TEST_C(SubscribeTests, subscribeTopicWithHashkeyAllSubTopicSuccess) {
	IoT_Error_t rc = SUCCESS;
	char expectedCallbackString[100] = "New message: sub/sub, Hashkey";

	IOT_DEBUG("-->Running Subscribe Tests - C:19 - Subscribe, '#' not last character in topic name, Failure \n");

	// Set up the subscribed topic, including '#'
	setTLSRxBufferForSuback("sdk/Test/#", strlen("sdk/Test/#"), QOS1, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, "sdk/Test/#", strlen("sdk/Test/#"), QOS1,
								iot_subscribe_callback_handler, NULL);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	// Now provide a published message from a sub topic
	IOT_DEBUG("[Matching '#'] Checking first sub topic message, with '#' be the last..\n");
	setTLSRxBufferWithMsgOnSubscribedTopic("sdk/Test/sub/sub", strlen("sdk/Test/sub/sub"), QOS1, testPubMsgParams,
										   expectedCallbackString);
	snprintf(CallbackMsgString, 100, "NOT_VISITED");

	rc = aws_iot_mqtt_yield(&iotClient, 1000);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_STRING(expectedCallbackString, CallbackMsgString);
	CHECK_EQUAL_C_INT(1, isLastTLSTxMessagePuback());

	// Re-initialize Rx Tx Buffer
	ResetTLSBuffer();

	// Now provide another message from a different sub topic
	IOT_DEBUG("[Matching '#'] Checking second sub topic message, with '#' be the last...\n");
	snprintf(expectedCallbackString, 100, "New message: sub2, Hashkey");
	setTLSRxBufferWithMsgOnSubscribedTopic("sdk/Test/sub2", strlen("sdk/Test/sub2"), QOS1, testPubMsgParams,
										   expectedCallbackString);
	snprintf(CallbackMsgString, 100, "NOT_VISITED");

	rc = aws_iot_mqtt_yield(&iotClient, 1000);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_STRING(expectedCallbackString, CallbackMsgString);
	CHECK_EQUAL_C_INT(1, isLastTLSTxMessagePuback());

	IOT_DEBUG("-->Success - C:19 - Subscribe, '#' not last character in topic name, Failure \n");
}
/* C:20 - Subscribe with '#', subscribed to all subtopics */
TEST_C(SubscribeTests, subscribeTopicHashkeyMustBeTheLastFail) {
	IoT_Error_t rc = SUCCESS;
	char expectedCallbackString[100] = "New message: foo1/sub, Hashkey";

	IOT_DEBUG("-->Running Subscribe Tests - C:20 - Subscribe with '#', subscribed to all subtopics \n");

	// Set up the subscribed topic, with '#' in the middle
	// Topic directory not permitted, should fail
	setTLSRxBufferForSuback("sdk/#/sub", strlen("sdk/#/sub"), QOS1, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, "sdk/#/sub", strlen("sdk/#/sub"), QOS1,
								iot_subscribe_callback_handler, NULL);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	ResetTLSBuffer();
	// Now provide a published message from a sub directoy with this sub topic
	IOT_DEBUG("[Matching '#'] Checking sub topic message, with '#' in the middle...\n");
	setTLSRxBufferWithMsgOnSubscribedTopic("sdk/Test/foo1/sub", strlen("sdk/Test/foo1/sub"), QOS1, testPubMsgParams,
										   expectedCallbackString);
	snprintf(CallbackMsgString, 100, "NOT_VISITED");

	rc = aws_iot_mqtt_yield(&iotClient, 1000);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_STRING("NOT_VISITED", CallbackMsgString);

	IOT_DEBUG("-->Success - C:20 - Subscribe with '#', subscribed to all subtopics \n");
}
/* C:21 - Subscribe with '+' as wildcard success */
TEST_C(SubscribeTests, subscribeTopicWithPluskeySuccess) {
	IoT_Error_t rc = SUCCESS;
	char expectedCallbackString[100] = "New message: 1/sub, Pluskey";

	IOT_DEBUG("-->Running Subscribe Tests - C:21 - Subscribe with '+' as wildcard success \n");

	// Set up the subscribed topic, including '+'
	setTLSRxBufferForSuback("sdk/Test/+/sub", strlen("sdk/Test/+/sub"), QOS1, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, "sdk/Test/+/sub", strlen("sdk/Test/+/sub"), QOS1,
								iot_subscribe_callback_handler, NULL);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	// Now provide a published message from a sub topic
	IOT_DEBUG("[Matching '+'] Checking first sub topic message, with '+' in the middle...\n");
	setTLSRxBufferWithMsgOnSubscribedTopic("sdk/Test/1/sub", strlen("sdk/Test/1/sub"), QOS1, testPubMsgParams,
										   expectedCallbackString);
	snprintf(CallbackMsgString, 100, "NOT_VISITED");

	rc = aws_iot_mqtt_yield(&iotClient, 1000);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_STRING(expectedCallbackString, CallbackMsgString);
	CHECK_EQUAL_C_INT(1, isLastTLSTxMessagePuback());

	// Re-initialize Rx Tx Buffer
	ResetTLSBuffer();

	// Now provide another message from a different sub topic
	IOT_DEBUG("[Matching '+'] Checking second sub topic message, with '+' in the middle...\n");
	snprintf(expectedCallbackString, 100, "New message: 2/sub, Pluskey");
	setTLSRxBufferWithMsgOnSubscribedTopic("sdk/Test/2/sub", strlen("sdk/Test/2/sub"), QOS1, testPubMsgParams,
										   expectedCallbackString);
	snprintf(CallbackMsgString, 100, "NOT_VISITED");

	rc = aws_iot_mqtt_yield(&iotClient, 1000);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_STRING(expectedCallbackString, CallbackMsgString);
	CHECK_EQUAL_C_INT(1, isLastTLSTxMessagePuback());

	IOT_DEBUG("-->Success - C:21 - Subscribe with '+' as wildcard success \n");
}
/* C:22 - Subscribe with '+' as last character in topic name, Success */
TEST_C(SubscribeTests, subscribeTopicPluskeyComesLastSuccess) {
	IoT_Error_t rc = SUCCESS;
	char expectedCallbackString[100] = "New message: foo1, Pluskey";

	IOT_DEBUG("-->Running Subscribe Tests - C:22 - Subscribe with '+' as last character in topic name, Success \n");

	// Set up the subscribed topic, with '+' comes the last
	setTLSRxBufferForSuback("sdk/Test/+", strlen("sdk/Test/+"), QOS1, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, "sdk/Test/+", strlen("sdk/Test/+"), QOS1,
								iot_subscribe_callback_handler, NULL);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	// Now provide a published message from a single layer of sub directroy
	IOT_DEBUG("[Matching '+'] Checking first sub topic message, with '+' be the last...\n");
	setTLSRxBufferWithMsgOnSubscribedTopic("sdk/Test/foo1", strlen("sdk/Test/foo1"), QOS1, testPubMsgParams,
										   expectedCallbackString);
	snprintf(CallbackMsgString, 100, "NOT_VISITED");

	rc = aws_iot_mqtt_yield(&iotClient, 1000);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_STRING(expectedCallbackString, CallbackMsgString);
	CHECK_EQUAL_C_INT(1, isLastTLSTxMessagePuback());

	// Re-initialize Rx Tx Buffer
	ResetTLSBuffer();

	// Now provide a published message from another single layer of sub directroy
	IOT_DEBUG("[Matching '+'] Checking second sub topic message, with '+' be the last...\n");
	snprintf(expectedCallbackString, 100, "New message: foo2, Pluskey");
	setTLSRxBufferWithMsgOnSubscribedTopic("sdk/Test/foo2", strlen("sdk/Test/foo2"), QOS1, testPubMsgParams,
										   expectedCallbackString);
	snprintf(CallbackMsgString, 100, "NOT_VISITED");

	rc = aws_iot_mqtt_yield(&iotClient, 1000);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_STRING(expectedCallbackString, CallbackMsgString);
	CHECK_EQUAL_C_INT(1, isLastTLSTxMessagePuback());

	IOT_DEBUG("-->Success - C:22 - Subscribe with '+' as last character in topic name, Success \n");
}

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
 * @file aws_iot_tests_unit_shadow_action_helper.c
 * @brief IoT Client Unit Testing - Shadow Action API Tests Helper
 */

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <CppUTest/TestHarness_c.h>
#include <aws_iot_tests_unit_mock_tls_params.h>

#include "aws_iot_tests_unit_mock_tls_params.h"
#include "aws_iot_tests_unit_helper_functions.h"
#include "aws_iot_tests_unit_shadow_helper.h"

#include "aws_iot_shadow_interface.h"
#include "aws_iot_shadow_actions.h"
#include "aws_iot_shadow_json.h"
#include "aws_iot_log.h"

#define SIZE_OF_UPDATE_DOCUMENT 200
#define TEST_JSON_RESPONSE_FULL_DOCUMENT "{\"state\":{\"reported\":{\"sensor1\":98}}, \"clientToken\":\"" AWS_IOT_MQTT_CLIENT_ID "-0\"}"
#define TEST_JSON_RESPONSE_DELETE_DOCUMENT "{\"version\":2,\"timestamp\":1443473857,\"clientToken\":\"" AWS_IOT_MQTT_CLIENT_ID "-0\"}"
#define TEST_JSON_RESPONSE_UPDATE_DOCUMENT "{\"state\":{\"reported\":{\"doubleData\":4.090800,\"floatData\":3.445000}}, \"clientToken\":\"" AWS_IOT_MQTT_CLIENT_ID "-0\"}"

static AWS_IoT_Client client;
static IoT_Client_Connect_Params connectParams;
static IoT_Publish_Message_Params testPubMsgParams;
static ShadowInitParameters_t shadowInitParams;
static ShadowConnectParameters_t shadowConnectParams;

static Shadow_Ack_Status_t ackStatusRx;
static char jsonFullDocument[200];
static ShadowActions_t actionRx;

static void topicNameFromThingAndAction(char *pTopic, const char *pThingName, ShadowActions_t action) {
	char actionBuf[10];

	if(SHADOW_GET == action) {
		strcpy(actionBuf, "get");
	} else if(SHADOW_UPDATE == action) {
		strcpy(actionBuf, "update");
	} else if(SHADOW_DELETE == action) {
		strcpy(actionBuf, "delete");
	}

	snprintf(pTopic, 100, "$aws/things/%s/shadow/%s", pThingName, actionBuf);
}

TEST_GROUP_C_SETUP(ShadowActionTests) {
	IoT_Error_t ret_val = SUCCESS;
	char cPayload[100];
	char topic[120];

	shadowInitParams.pHost = AWS_IOT_MQTT_HOST;
	shadowInitParams.port = AWS_IOT_MQTT_PORT;
	shadowInitParams.pClientCRT = AWS_IOT_CERTIFICATE_FILENAME;
	shadowInitParams.pRootCA = AWS_IOT_ROOT_CA_FILENAME;
	shadowInitParams.pClientKey = AWS_IOT_PRIVATE_KEY_FILENAME;
	shadowInitParams.disconnectHandler = NULL;
	shadowInitParams.enableAutoReconnect = false;
	ret_val = aws_iot_shadow_init(&client, &shadowInitParams);
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);

	shadowConnectParams.pMyThingName = AWS_IOT_MY_THING_NAME;
	shadowConnectParams.pMqttClientId = AWS_IOT_MQTT_CLIENT_ID;
	shadowConnectParams.mqttClientIdLen = (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID);
	ConnectMQTTParamsSetup(&connectParams, AWS_IOT_MQTT_CLIENT_ID, (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID));
	setTLSRxBufferForConnack(&connectParams, 0, 0);
	ret_val = aws_iot_shadow_connect(&client, &shadowConnectParams);
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);

	setTLSRxBufferForPuback();
	testPubMsgParams.qos = QOS1;
	testPubMsgParams.isRetained = 0;
	snprintf(cPayload, 100, "%s : %d ", "hello from SDK", 0);
	testPubMsgParams.payload = (void *) cPayload;
	testPubMsgParams.payloadLen = strlen(cPayload) + 1;
	topicNameFromThingAndAction(topic, AWS_IOT_MY_THING_NAME, SHADOW_GET);
	setTLSRxBufferForDoubleSuback(topic, strlen(topic), QOS1, testPubMsgParams);
}

TEST_GROUP_C_TEARDOWN(ShadowActionTests) {
	/* Clean up. Not checking return code here because this is common to all tests.
	 * A test might have already caused a disconnect by this point.
	 */
	IoT_Error_t rc = aws_iot_shadow_disconnect(&client);
	IOT_UNUSED(rc);
}

static void actionCallback(const char *pThingName, ShadowActions_t action, Shadow_Ack_Status_t status,
					const char *pReceivedJsonDocument, void *pContextData) {
	IOT_UNUSED(pThingName);
	IOT_UNUSED(pContextData);
	IOT_DEBUG("%s", pReceivedJsonDocument);
	actionRx = action;
	ackStatusRx = status;
	if(SHADOW_ACK_TIMEOUT != status) {
		strcpy(jsonFullDocument, pReceivedJsonDocument);
	}
}

// Happy path for Get, Update, Delete
TEST_C(ShadowActionTests, GetTheFullJSONDocument) {
	IoT_Error_t ret_val = SUCCESS;
	char getRequestJson[120];
	IoT_Publish_Message_Params params;

	IOT_DEBUG("-->Running Shadow Action Tests - Get full json document \n");

	aws_iot_shadow_internal_get_request_json(getRequestJson);
	ret_val = aws_iot_shadow_internal_action(AWS_IOT_MY_THING_NAME, SHADOW_GET, getRequestJson, actionCallback, NULL, 4,
											 false);
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);

	ResetTLSBuffer();
	params.payloadLen = strlen(TEST_JSON_RESPONSE_FULL_DOCUMENT);
	params.payload = TEST_JSON_RESPONSE_FULL_DOCUMENT;
	params.qos = QOS0;
	setTLSRxBufferWithMsgOnSubscribedTopic(GET_ACCEPTED_TOPIC, strlen(GET_ACCEPTED_TOPIC), QOS0, params,
										   params.payload);
	ret_val = aws_iot_shadow_yield(&client, 200);

	CHECK_EQUAL_C_INT(SUCCESS, ret_val);
	CHECK_EQUAL_C_STRING(TEST_JSON_RESPONSE_FULL_DOCUMENT, jsonFullDocument);
	CHECK_EQUAL_C_INT(SHADOW_GET, actionRx);
	CHECK_EQUAL_C_INT(SHADOW_ACK_ACCEPTED, ackStatusRx);

	IOT_DEBUG("-->Success - Get full json document \n");
}

TEST_C(ShadowActionTests, DeleteTheJSONDocument) {
	IoT_Error_t ret_val = SUCCESS;
	IoT_Publish_Message_Params params;
	char deleteRequestJson[120];

	IOT_DEBUG("-->Running Shadow Action Tests - Delete json document \n");

	aws_iot_shadow_internal_delete_request_json(deleteRequestJson);
	ret_val = aws_iot_shadow_internal_action(AWS_IOT_MY_THING_NAME, SHADOW_DELETE, deleteRequestJson, actionCallback,
											 NULL, 4, false);
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);

	params.payloadLen = strlen(TEST_JSON_RESPONSE_DELETE_DOCUMENT);
	params.payload = TEST_JSON_RESPONSE_DELETE_DOCUMENT;
	params.qos = QOS0;
	setTLSRxBufferWithMsgOnSubscribedTopic(DELETE_ACCEPTED_TOPIC, strlen(DELETE_ACCEPTED_TOPIC), QOS0, params,
										   params.payload);
	ret_val = aws_iot_shadow_yield(&client, 200);

	CHECK_EQUAL_C_INT(SUCCESS, ret_val);
	CHECK_EQUAL_C_STRING(TEST_JSON_RESPONSE_DELETE_DOCUMENT, jsonFullDocument);
	CHECK_EQUAL_C_INT(SHADOW_DELETE, actionRx);
	CHECK_EQUAL_C_INT(SHADOW_ACK_ACCEPTED, ackStatusRx);

	IOT_DEBUG("-->Success - Delete json document \n");
}

TEST_C(ShadowActionTests, UpdateTheJSONDocument) {
	IoT_Error_t ret_val = SUCCESS;
	char updateRequestJson[SIZE_OF_UPDATE_DOCUMENT];
	char expectedUpdateRequestJson[SIZE_OF_UPDATE_DOCUMENT];
	double doubleData = 4.0908f;
	float floatData = 3.445f;
	bool boolData = true;
	jsonStruct_t dataFloatHandler;
	jsonStruct_t dataDoubleHandler;
	jsonStruct_t dataBoolHandler;
	IoT_Publish_Message_Params params;

	IOT_DEBUG("-->Running Shadow Action Tests - Update json document \n");

	dataFloatHandler.cb = NULL;
	dataFloatHandler.pData = &floatData;
	dataFloatHandler.pKey = "floatData";
	dataFloatHandler.type = SHADOW_JSON_FLOAT;

	dataDoubleHandler.cb = NULL;
	dataDoubleHandler.pData = &doubleData;
	dataDoubleHandler.pKey = "doubleData";
	dataDoubleHandler.type = SHADOW_JSON_DOUBLE;

	dataBoolHandler.cb = NULL;
	dataBoolHandler.pData = &boolData;
	dataBoolHandler.pKey = "boolData";
	dataBoolHandler.type = SHADOW_JSON_BOOL;

	ret_val = aws_iot_shadow_init_json_document(updateRequestJson, SIZE_OF_UPDATE_DOCUMENT);
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);
	ret_val = aws_iot_shadow_add_reported(updateRequestJson, SIZE_OF_UPDATE_DOCUMENT, 2, &dataDoubleHandler,
										  &dataFloatHandler);
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);
	ret_val = aws_iot_shadow_add_desired(updateRequestJson, SIZE_OF_UPDATE_DOCUMENT, 1, &dataBoolHandler);
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);
	ret_val = aws_iot_finalize_json_document(updateRequestJson, SIZE_OF_UPDATE_DOCUMENT);
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);

	snprintf(expectedUpdateRequestJson, SIZE_OF_UPDATE_DOCUMENT,
			 "{\"state\":{\"reported\":{\"doubleData\":4.090800,\"floatData\":3.445000},\"desired\":{\"boolData\":true}}, \"clientToken\":\"%s-0\"}",
			AWS_IOT_MQTT_CLIENT_ID);
	CHECK_EQUAL_C_STRING(expectedUpdateRequestJson, updateRequestJson);

	ret_val = aws_iot_shadow_internal_action(AWS_IOT_MY_THING_NAME, SHADOW_UPDATE, updateRequestJson, actionCallback,
											 NULL, 4, false);
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);

	ResetTLSBuffer();
	snprintf(jsonFullDocument, 200, "%s", "");
	params.payloadLen = strlen(TEST_JSON_RESPONSE_UPDATE_DOCUMENT);
	params.payload = TEST_JSON_RESPONSE_UPDATE_DOCUMENT;
	params.qos = QOS0;
	setTLSRxBufferWithMsgOnSubscribedTopic(UPDATE_ACCEPTED_TOPIC, strlen(UPDATE_ACCEPTED_TOPIC), QOS0, params,
										   params.payload);
	ret_val = aws_iot_shadow_yield(&client, 200);

	CHECK_EQUAL_C_INT(SUCCESS, ret_val);
	CHECK_EQUAL_C_STRING(TEST_JSON_RESPONSE_UPDATE_DOCUMENT, jsonFullDocument);
	CHECK_EQUAL_C_INT(SHADOW_UPDATE, actionRx);
	CHECK_EQUAL_C_INT(SHADOW_ACK_ACCEPTED, ackStatusRx);

	IOT_DEBUG("-->Success - Update json document \n");
}

TEST_C(ShadowActionTests, GetTheFullJSONDocumentTimeout) {
	IoT_Error_t ret_val = SUCCESS;
	char getRequestJson[120];
	IoT_Publish_Message_Params params;

	IOT_DEBUG("-->Running Shadow Action Tests - Get full json document timeout \n");

	aws_iot_shadow_internal_get_request_json(getRequestJson);
	ret_val = aws_iot_shadow_internal_action(AWS_IOT_MY_THING_NAME, SHADOW_GET, getRequestJson, actionCallback, NULL, 4,
											 false);
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);

	sleep(4 + 1);

	params.payloadLen = strlen(TEST_JSON_RESPONSE_FULL_DOCUMENT);
	params.payload = TEST_JSON_RESPONSE_FULL_DOCUMENT;
	params.qos = QOS0;
	setTLSRxBufferWithMsgOnSubscribedTopic(GET_ACCEPTED_TOPIC, strlen(GET_ACCEPTED_TOPIC), QOS0, params,
										   params.payload);
	ret_val = aws_iot_shadow_yield(&client, 200);

	CHECK_EQUAL_C_INT(SUCCESS, ret_val);
	CHECK_EQUAL_C_STRING(TEST_JSON_RESPONSE_FULL_DOCUMENT, jsonFullDocument);
	CHECK_EQUAL_C_INT(SHADOW_GET, actionRx);
	CHECK_EQUAL_C_INT(SHADOW_ACK_TIMEOUT, ackStatusRx);

	IOT_DEBUG("-->Success - Get full json document timeout \n");
}

// Subscribe and UnSubscribe on reception of thing names. Will perform the test with  Get operation
TEST_C(ShadowActionTests, SubscribeToAcceptedRejectedOnGet) {
	IoT_Error_t ret_val = SUCCESS;
	char getRequestJson[120];

	uint8_t firstByte, secondByte;
	uint16_t topicNameLen;
	char topicName[128] = "test";

	IOT_DEBUG("-->Running Shadow Action Tests - Subscribe to get/accepted and get/rejected \n");

	lastSubscribeMsgLen = 11;
	snprintf(LastSubscribeMessage, lastSubscribeMsgLen, "No Message");
	secondLastSubscribeMsgLen = 11;
	snprintf(SecondLastSubscribeMessage, secondLastSubscribeMsgLen, "No Message");

	aws_iot_shadow_internal_get_request_json(getRequestJson);
	ret_val = aws_iot_shadow_internal_action(AWS_IOT_MY_THING_NAME, SHADOW_GET, getRequestJson, actionCallback, NULL, 4,
											 false);
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);

	CHECK_EQUAL_C_STRING(GET_REJECTED_TOPIC, LastSubscribeMessage);
	CHECK_EQUAL_C_STRING(GET_ACCEPTED_TOPIC, SecondLastSubscribeMessage);

	firstByte = (uint8_t)(TxBuffer.pBuffer[2]);
	secondByte = (uint8_t)(TxBuffer.pBuffer[3]);
	topicNameLen = (uint16_t) (secondByte + (256 * firstByte));

	snprintf(topicName, topicNameLen + 1u, "%s", &(TxBuffer.pBuffer[4])); // Added one for null character

	// Verify publish happens
	CHECK_EQUAL_C_STRING(GET_PUB_TOPIC, topicName);

	IOT_DEBUG("-->Success - Subscribe to get/accepted and get/rejected \n");
}

TEST_C(ShadowActionTests, UnSubscribeToAcceptedRejectedOnGetResponse) {
	IoT_Error_t ret_val = SUCCESS;
	char getRequestJson[120];
	IoT_Publish_Message_Params params;

	IOT_DEBUG("-->Running Shadow Action Tests - unsubscribe to get/accepted and get/rejected on response \n");

	lastSubscribeMsgLen = 11;
	snprintf(LastSubscribeMessage, lastSubscribeMsgLen, "No Message");
	secondLastSubscribeMsgLen = 11;
	snprintf(SecondLastSubscribeMessage, secondLastSubscribeMsgLen, "No Message");

	aws_iot_shadow_internal_get_request_json(getRequestJson);
	ret_val = aws_iot_shadow_internal_action(AWS_IOT_MY_THING_NAME, SHADOW_GET, getRequestJson, actionCallback, NULL, 4,
											 false);
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);

	params.payloadLen = strlen(TEST_JSON_RESPONSE_FULL_DOCUMENT);
	params.payload = TEST_JSON_RESPONSE_FULL_DOCUMENT;
	params.qos = QOS0;
	setTLSRxBufferWithMsgOnSubscribedTopic(GET_ACCEPTED_TOPIC, strlen(GET_ACCEPTED_TOPIC), QOS0, params,
										   params.payload);
	ret_val = aws_iot_shadow_yield(&client, 200);

	CHECK_EQUAL_C_INT(SUCCESS, ret_val);

	CHECK_EQUAL_C_STRING(GET_REJECTED_TOPIC, LastSubscribeMessage);
	CHECK_EQUAL_C_STRING(GET_ACCEPTED_TOPIC, SecondLastSubscribeMessage);

	IOT_DEBUG("-->Success - unsubscribe to get/accepted and get/rejected on response \n");
}

TEST_C(ShadowActionTests, UnSubscribeToAcceptedRejectedOnGetTimeout) {
	IoT_Error_t ret_val = SUCCESS;
	char getRequestJson[120];
	IoT_Publish_Message_Params params;

	IOT_DEBUG("-->Running Shadow Action Tests - Unsubscribe to get/accepted and get/rejected on get timeout \n");

	snprintf(jsonFullDocument, 200, "aa");
	lastSubscribeMsgLen = 11;
	snprintf(LastSubscribeMessage, lastSubscribeMsgLen, "No Message");
	secondLastSubscribeMsgLen = 11;
	snprintf(SecondLastSubscribeMessage, secondLastSubscribeMsgLen, "No Message");

	aws_iot_shadow_internal_get_request_json(getRequestJson);
	ret_val = aws_iot_shadow_internal_action(AWS_IOT_MY_THING_NAME, SHADOW_GET, getRequestJson, actionCallback, NULL, 4,
											 false);
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);

	sleep(4 + 1);

	params.payloadLen = strlen(TEST_JSON_RESPONSE_FULL_DOCUMENT);
	params.payload = TEST_JSON_RESPONSE_FULL_DOCUMENT;
	params.qos = QOS0;
	setTLSRxBufferWithMsgOnSubscribedTopic(GET_ACCEPTED_TOPIC, strlen(GET_ACCEPTED_TOPIC), QOS0, params,
										   params.payload);
	ret_val = aws_iot_shadow_yield(&client, 200);

	CHECK_EQUAL_C_INT(SUCCESS, ret_val);
	CHECK_EQUAL_C_STRING("aa", jsonFullDocument);

	CHECK_EQUAL_C_STRING(GET_REJECTED_TOPIC, LastSubscribeMessage);
	CHECK_EQUAL_C_STRING(GET_ACCEPTED_TOPIC, SecondLastSubscribeMessage);

	IOT_DEBUG("-->Success - Unsubscribe to get/accepted and get/rejected on get timeout \n");
}


TEST_C(ShadowActionTests, UnSubscribeToAcceptedRejectedOnGetTimeoutWithSticky) {
	IoT_Error_t ret_val = SUCCESS;
	char getRequestJson[120];
	IoT_Publish_Message_Params params;

	IOT_DEBUG("-->Running Shadow Action Tests - No unsubscribe to get/accepted and get/rejected on get timeout with a sticky subscription \n");

	snprintf(jsonFullDocument, 200, "timeout");
	lastSubscribeMsgLen = 11;
	snprintf(LastSubscribeMessage, lastSubscribeMsgLen, "No Message");
	secondLastSubscribeMsgLen = 11;
	snprintf(SecondLastSubscribeMessage, secondLastSubscribeMsgLen, "No Message");

	aws_iot_shadow_internal_get_request_json(getRequestJson);
	ret_val = aws_iot_shadow_internal_action(AWS_IOT_MY_THING_NAME, SHADOW_GET, getRequestJson, actionCallback, NULL, 4,
											 true);
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);

	sleep(4 + 1);

	ResetTLSBuffer();
	params.payloadLen = strlen(TEST_JSON_RESPONSE_FULL_DOCUMENT);
	params.payload = TEST_JSON_RESPONSE_FULL_DOCUMENT;
	params.qos = QOS0;
	setTLSRxBufferWithMsgOnSubscribedTopic(GET_ACCEPTED_TOPIC, strlen(GET_ACCEPTED_TOPIC), QOS0, params,
										   params.payload);
	ret_val = aws_iot_shadow_yield(&client, 200);

	CHECK_EQUAL_C_INT(SUCCESS, ret_val);
	CHECK_EQUAL_C_STRING("timeout", jsonFullDocument);

	CHECK_EQUAL_C_STRING(GET_REJECTED_TOPIC, LastSubscribeMessage);
	CHECK_EQUAL_C_STRING(GET_ACCEPTED_TOPIC, SecondLastSubscribeMessage);

	IOT_DEBUG("-->Success - No unsubscribe to get/accepted and get/rejected on get timeout with a sticky subscription \n");
}

#define TEST_JSON_RESPONSE_FULL_DOCUMENT_WITH_VERSION(num) "{\"state\":{\"reported\":{\"sensor1\":98}}, \"clientToken\":\"" AWS_IOT_MQTT_CLIENT_ID "-0\",\"version\":" #num "}"

TEST_C(ShadowActionTests, GetVersionFromAckStatus) {
	IoT_Error_t ret_val = SUCCESS;
	char getRequestJson[120];
	IoT_Publish_Message_Params params;
	IoT_Publish_Message_Params params2;

	IOT_DEBUG("-->Running Shadow Action Tests - Get version from Ack status \n");

	snprintf(jsonFullDocument, 200, "timeout");

	aws_iot_shadow_internal_get_request_json(getRequestJson);
	ret_val = aws_iot_shadow_internal_action(AWS_IOT_MY_THING_NAME, SHADOW_GET, getRequestJson, actionCallback, NULL, 4,
											 true);
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);

	ResetTLSBuffer();
	params.payload = TEST_JSON_RESPONSE_FULL_DOCUMENT_WITH_VERSION(1);
	params.payloadLen = strlen(params.payload);
	params.qos = QOS0;
	setTLSRxBufferWithMsgOnSubscribedTopic(GET_ACCEPTED_TOPIC, strlen(GET_ACCEPTED_TOPIC), QOS0, params,
										   params.payload);
	ret_val = aws_iot_shadow_yield(&client, 200);

	CHECK_EQUAL_C_INT(SUCCESS, ret_val);
	CHECK_C(1u == aws_iot_shadow_get_last_received_version());

	ResetTLSBuffer();
	params2.payload = TEST_JSON_RESPONSE_FULL_DOCUMENT_WITH_VERSION(132387);
	params2.payloadLen = strlen(params2.payload);
	params2.qos = QOS0;
	setTLSRxBufferWithMsgOnSubscribedTopic(GET_ACCEPTED_TOPIC, strlen(GET_ACCEPTED_TOPIC), QOS0, params2,
										   params2.payload);
	ret_val = aws_iot_shadow_yield(&client, 200);

	CHECK_EQUAL_C_INT(SUCCESS, ret_val);
	CHECK_C(132387u == aws_iot_shadow_get_last_received_version());

	IOT_DEBUG("-->Success - Get version from Ack status \n");
}

#define TEST_JSON_RESPONSE_FULL_DOCUMENT_ALWAYS_WRONG_TOKEN "{\"state\":{\"reported\":{\"sensor1\":98}}, \"clientToken\":\"TroubleToken1234\"}"

TEST_C(ShadowActionTests, WrongTokenInGetResponse) {
	IoT_Error_t ret_val = SUCCESS;
	char getRequestJson[120];
	IoT_Publish_Message_Params params;

	IOT_DEBUG("-->Running Shadow Action Tests - Wrong token in get response \n");

	snprintf(jsonFullDocument, 200, "timeout");

	aws_iot_shadow_internal_get_request_json(getRequestJson);
	ret_val = aws_iot_shadow_internal_action(AWS_IOT_MY_THING_NAME, SHADOW_GET, getRequestJson, actionCallback, NULL, 4,
											 false);
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);

	sleep(4 + 1);

	params.payloadLen = strlen(TEST_JSON_RESPONSE_FULL_DOCUMENT_ALWAYS_WRONG_TOKEN);
	params.payload = TEST_JSON_RESPONSE_FULL_DOCUMENT_ALWAYS_WRONG_TOKEN;
	params.qos = QOS0;
	setTLSRxBufferWithMsgOnSubscribedTopic(GET_ACCEPTED_TOPIC, strlen(GET_ACCEPTED_TOPIC), QOS0, params,
										   params.payload);
	ret_val = aws_iot_shadow_yield(&client, 200);

	CHECK_EQUAL_C_INT(SUCCESS, ret_val);
	CHECK_EQUAL_C_STRING("timeout", jsonFullDocument);
	CHECK_EQUAL_C_INT(SHADOW_GET, actionRx);
	CHECK_EQUAL_C_INT(SHADOW_ACK_TIMEOUT, ackStatusRx);

	IOT_DEBUG("-->Success - Wrong token in get response \n");
}

#define TEST_JSON_RESPONSE_FULL_DOCUMENT_NO_TOKEN "{\"state\":{\"reported\":{\"sensor1\":98}}}"

TEST_C(ShadowActionTests, NoTokenInGetResponse) {
	IoT_Error_t ret_val = SUCCESS;
	char getRequestJson[120];
	IoT_Publish_Message_Params params;

	IOT_DEBUG("-->Running Shadow Action Tests - No token in get response \n");

	snprintf(jsonFullDocument, 200, "timeout");

	aws_iot_shadow_internal_get_request_json(getRequestJson);
	ret_val = aws_iot_shadow_internal_action(AWS_IOT_MY_THING_NAME, SHADOW_GET, getRequestJson, actionCallback, NULL, 4,
											 false);
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);

	sleep(4 + 1);

	params.payloadLen = strlen(TEST_JSON_RESPONSE_FULL_DOCUMENT_NO_TOKEN);
	params.payload = TEST_JSON_RESPONSE_FULL_DOCUMENT_NO_TOKEN;
	params.qos = QOS0;
	setTLSRxBufferWithMsgOnSubscribedTopic(GET_ACCEPTED_TOPIC, strlen(GET_ACCEPTED_TOPIC), QOS0, params,
										   params.payload);
	ret_val = aws_iot_shadow_yield(&client, 200);

	CHECK_EQUAL_C_INT(SUCCESS, ret_val);
	CHECK_EQUAL_C_STRING("timeout", jsonFullDocument);
	CHECK_EQUAL_C_INT(SHADOW_GET, actionRx);
	CHECK_EQUAL_C_INT(SHADOW_ACK_TIMEOUT, ackStatusRx);

	IOT_DEBUG("-->Success - No token in get response \n");
}

TEST_C(ShadowActionTests, InvalidInboundJSONInGetResponse) {
	IoT_Error_t ret_val = SUCCESS;
	char getRequestJson[120];
	IoT_Publish_Message_Params params;

	IOT_DEBUG("-->Running Shadow Action Tests - Invalid inbound json in get response \n");

	snprintf(jsonFullDocument, 200, "NOT_VISITED");

	aws_iot_shadow_internal_get_request_json(getRequestJson);
	ret_val = aws_iot_shadow_internal_action(AWS_IOT_MY_THING_NAME, SHADOW_GET, getRequestJson, actionCallback, NULL, 4,
											 false);
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);

	params.payloadLen = strlen("{\"state\":{{");
	params.payload = "{\"state\":{{";
	params.qos = QOS0;
	setTLSRxBufferWithMsgOnSubscribedTopic(GET_ACCEPTED_TOPIC, strlen(GET_ACCEPTED_TOPIC), QOS0, params,
										   params.payload);
	ret_val = aws_iot_shadow_yield(&client, 200);

	CHECK_EQUAL_C_INT(SUCCESS, ret_val);
	CHECK_EQUAL_C_STRING("NOT_VISITED", jsonFullDocument);

	IOT_DEBUG("-->Success - Invalid inbound json in get response \n");
}

TEST_C(ShadowActionTests, AcceptedSubFailsGetRequest) {
	IoT_Error_t ret_val = SUCCESS;
	char getRequestJson[120];
	IoT_Publish_Message_Params params;

	IOT_DEBUG("-->Running Shadow Action Tests - Accepted sub fails get request \n");

	snprintf(jsonFullDocument, 200, "NOT_SENT");

	ResetTLSBuffer();
	aws_iot_shadow_internal_get_request_json(getRequestJson);
	ret_val = aws_iot_shadow_internal_action(AWS_IOT_MY_THING_NAME, SHADOW_GET, getRequestJson, actionCallback, NULL, 4,
											 false);
	CHECK_EQUAL_C_INT(MQTT_REQUEST_TIMEOUT_ERROR, ret_val); // Should never subscribe and publish

	ResetTLSBuffer();
	params.payloadLen = strlen(TEST_JSON_RESPONSE_FULL_DOCUMENT);
	params.payload = TEST_JSON_RESPONSE_FULL_DOCUMENT;
	params.qos = QOS0;
	setTLSRxBufferWithMsgOnSubscribedTopic(GET_ACCEPTED_TOPIC, strlen(GET_ACCEPTED_TOPIC), QOS0, params,
										   params.payload);
	ret_val = aws_iot_shadow_yield(&client, 200);

	CHECK_EQUAL_C_INT(SUCCESS, ret_val);
	CHECK_EQUAL_C_STRING("NOT_SENT", jsonFullDocument); // Never called callback

	IOT_DEBUG("-->Success - Accepted sub fails get request \n");
}

TEST_C(ShadowActionTests, RejectedSubFailsGetRequest) {
	IoT_Error_t ret_val = SUCCESS;
	char getRequestJson[120];
	IoT_Publish_Message_Params params;

	IOT_DEBUG("-->Running Shadow Action Tests - Rejected sub fails get request \n");

	snprintf(jsonFullDocument, 200, "NOT_SENT");

	params.payloadLen = strlen(TEST_JSON_RESPONSE_FULL_DOCUMENT);
	params.payload = TEST_JSON_RESPONSE_FULL_DOCUMENT;
	params.qos = QOS0;

	ResetTLSBuffer();
	setTLSRxBufferForSuback(GET_ACCEPTED_TOPIC, strlen(GET_ACCEPTED_TOPIC), QOS0, params);
	aws_iot_shadow_internal_get_request_json(getRequestJson);
	ret_val = aws_iot_shadow_internal_action(AWS_IOT_MY_THING_NAME, SHADOW_GET, getRequestJson, actionCallback, NULL, 4,
											 false);
	CHECK_EQUAL_C_INT(MQTT_REQUEST_TIMEOUT_ERROR, ret_val); // Should never subscribe and publish

	ResetTLSBuffer();
	setTLSRxBufferWithMsgOnSubscribedTopic(GET_REJECTED_TOPIC, strlen(GET_REJECTED_TOPIC), QOS0, params,
										   params.payload);
	ret_val = aws_iot_shadow_yield(&client, 200);

	CHECK_EQUAL_C_INT(SUCCESS, ret_val);
	CHECK_EQUAL_C_STRING("NOT_SENT", jsonFullDocument); // Never called callback

	IOT_DEBUG("-->Success - Rejected sub fails get request \n");
}

TEST_C(ShadowActionTests, PublishFailsGetRequest) {
	IoT_Error_t ret_val = SUCCESS;
	char getRequestJson[120];
	IoT_Publish_Message_Params params;

	IOT_DEBUG("-->Running Shadow Action Tests - publish fails on get request \n");

	snprintf(jsonFullDocument, 200, "NOT_SENT");

	ResetTLSBuffer();

	aws_iot_shadow_internal_get_request_json(getRequestJson);
	ret_val = aws_iot_shadow_internal_action(AWS_IOT_MY_THING_NAME, SHADOW_GET, getRequestJson, actionCallback, NULL, 4,
											 false);
	CHECK_EQUAL_C_INT(MQTT_REQUEST_TIMEOUT_ERROR, ret_val); // Should never subscribe and publish

	ResetTLSBuffer();
	params.payloadLen = strlen(TEST_JSON_RESPONSE_FULL_DOCUMENT);
	params.payload = TEST_JSON_RESPONSE_FULL_DOCUMENT;
	params.qos = QOS0;
	setTLSRxBufferWithMsgOnSubscribedTopic(GET_ACCEPTED_TOPIC, strlen(GET_ACCEPTED_TOPIC), QOS0, params,
										   params.payload);
	ret_val = aws_iot_shadow_yield(&client, 200);

	CHECK_EQUAL_C_INT(SUCCESS, ret_val);
	CHECK_EQUAL_C_STRING("NOT_SENT", jsonFullDocument); // Never called callback

	IOT_DEBUG("-->Success - publish fails on get request \n");
}

TEST_C(ShadowActionTests, StickyNonStickyNeverConflict) {
	IoT_Error_t ret_val = SUCCESS;
	char getRequestJson[120];
	IoT_Publish_Message_Params params;

	IOT_DEBUG("-->Running Shadow Action Tests - Sticky and non-sticky subscriptions do not conflict \n");

	lastSubscribeMsgLen = 11;
	snprintf(LastSubscribeMessage, lastSubscribeMsgLen, "No Message");
	secondLastSubscribeMsgLen = 11;
	snprintf(SecondLastSubscribeMessage, secondLastSubscribeMsgLen, "No Message");

	aws_iot_shadow_internal_get_request_json(getRequestJson);
	ret_val = aws_iot_shadow_internal_action(AWS_IOT_MY_THING_NAME, SHADOW_GET, getRequestJson, actionCallback, NULL, 4,
											 true);
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);

	params.payloadLen = strlen(TEST_JSON_RESPONSE_FULL_DOCUMENT);
	params.payload = TEST_JSON_RESPONSE_FULL_DOCUMENT;
	params.qos = QOS0;
	setTLSRxBufferWithMsgOnSubscribedTopic(GET_ACCEPTED_TOPIC, strlen(GET_ACCEPTED_TOPIC), QOS0, params,
										   params.payload);
	ret_val = aws_iot_shadow_yield(&client, 200);

	CHECK_EQUAL_C_INT(SUCCESS, ret_val);
	CHECK_EQUAL_C_STRING(TEST_JSON_RESPONSE_FULL_DOCUMENT, jsonFullDocument);
	CHECK_EQUAL_C_INT(SHADOW_GET, actionRx);
	CHECK_EQUAL_C_INT(SHADOW_ACK_ACCEPTED, ackStatusRx);

	lastSubscribeMsgLen = 11;
	snprintf(LastSubscribeMessage, lastSubscribeMsgLen, "No Message");
	secondLastSubscribeMsgLen = 11;
	snprintf(SecondLastSubscribeMessage, secondLastSubscribeMsgLen, "No Message");

	// Non-sticky shadow get, same thing name. Should never unsub since they are sticky
	aws_iot_shadow_internal_get_request_json(getRequestJson);
	ret_val = aws_iot_shadow_internal_action(AWS_IOT_MY_THING_NAME, SHADOW_GET, getRequestJson, actionCallback, NULL, 4,
											 false);
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);

	ResetTLSBuffer();

	params.payloadLen = strlen(TEST_JSON_RESPONSE_FULL_DOCUMENT);
	params.payload = TEST_JSON_RESPONSE_FULL_DOCUMENT;
	params.qos = QOS0;
	setTLSRxBufferWithMsgOnSubscribedTopic(GET_ACCEPTED_TOPIC, strlen(GET_ACCEPTED_TOPIC), QOS0, params,
										   params.payload);
	ret_val = aws_iot_shadow_yield(&client, 200);

	CHECK_EQUAL_C_INT(SUCCESS, ret_val);
	CHECK_EQUAL_C_STRING(TEST_JSON_RESPONSE_FULL_DOCUMENT, jsonFullDocument);
	CHECK_EQUAL_C_INT(SHADOW_GET, actionRx);
	CHECK_EQUAL_C_INT(SHADOW_ACK_ACCEPTED, ackStatusRx);

	CHECK_EQUAL_C_STRING("No Message", LastSubscribeMessage);
	CHECK_EQUAL_C_STRING("No Message", SecondLastSubscribeMessage);

	IOT_DEBUG("-->Success - Sticky and non-sticky subscriptions do not conflict \n");

}

TEST_C(ShadowActionTests, ACKWaitingMoreThanAllowed) {
	IoT_Error_t ret_val = SUCCESS;
	char getRequestJson[120];

	IOT_DEBUG("-->Running Shadow Action Tests - Ack waiting more than allowed wait time \n");

	// 1st
	aws_iot_shadow_internal_get_request_json(getRequestJson);
	ret_val = aws_iot_shadow_internal_action(AWS_IOT_MY_THING_NAME, SHADOW_GET, getRequestJson, actionCallback, NULL,
											 100, false); // 100 sec to timeout
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);
	// 2nd
	aws_iot_shadow_internal_get_request_json(getRequestJson);
	ret_val = aws_iot_shadow_internal_action(AWS_IOT_MY_THING_NAME, SHADOW_GET, getRequestJson, actionCallback, NULL,
											 100, false); // 100 sec to timeout
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);
	// 3rd
	aws_iot_shadow_internal_get_request_json(getRequestJson);
	ret_val = aws_iot_shadow_internal_action(AWS_IOT_MY_THING_NAME, SHADOW_GET, getRequestJson, actionCallback, NULL,
											 100, false); // 100 sec to timeout
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);
	// 4th
	aws_iot_shadow_internal_get_request_json(getRequestJson);
	ret_val = aws_iot_shadow_internal_action(AWS_IOT_MY_THING_NAME, SHADOW_GET, getRequestJson, actionCallback, NULL,
											 100, false); // 100 sec to timeout
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);
	// 5th
	aws_iot_shadow_internal_get_request_json(getRequestJson);
	ret_val = aws_iot_shadow_internal_action(AWS_IOT_MY_THING_NAME, SHADOW_GET, getRequestJson, actionCallback, NULL,
											 100, false); // 100 sec to timeout
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);
	// 6th
	aws_iot_shadow_internal_get_request_json(getRequestJson);
	ret_val = aws_iot_shadow_internal_action(AWS_IOT_MY_THING_NAME, SHADOW_GET, getRequestJson, actionCallback, NULL,
											 100, false); // 100 sec to timeout
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);
	// 7th
	aws_iot_shadow_internal_get_request_json(getRequestJson);
	ret_val = aws_iot_shadow_internal_action(AWS_IOT_MY_THING_NAME, SHADOW_GET, getRequestJson, actionCallback, NULL,
											 100, false); // 100 sec to timeout
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);
	// 8th
	aws_iot_shadow_internal_get_request_json(getRequestJson);
	ret_val = aws_iot_shadow_internal_action(AWS_IOT_MY_THING_NAME, SHADOW_GET, getRequestJson, actionCallback, NULL,
											 100, false); // 100 sec to timeout
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);
	// 9th
	aws_iot_shadow_internal_get_request_json(getRequestJson);
	ret_val = aws_iot_shadow_internal_action(AWS_IOT_MY_THING_NAME, SHADOW_GET, getRequestJson, actionCallback, NULL,
											 100, false); // 100 sec to timeout
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);
	// 10th
	aws_iot_shadow_internal_get_request_json(getRequestJson);
	ret_val = aws_iot_shadow_internal_action(AWS_IOT_MY_THING_NAME, SHADOW_GET, getRequestJson, actionCallback, NULL,
											 100, false); // 100 sec to timeout
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);
	// 11th
	// Should return some error code, since we are running out of ACK space
	aws_iot_shadow_internal_get_request_json(getRequestJson);
	ret_val = aws_iot_shadow_internal_action(AWS_IOT_MY_THING_NAME, SHADOW_GET, getRequestJson, actionCallback, NULL,
											 100, false); // 100 sec to timeout
	CHECK_EQUAL_C_INT(FAILURE, ret_val);

	IOT_DEBUG("-->Success - Ack waiting more than allowed wait time \n");
}

#define JSON_SIZE_OVERFLOW "{\"state\":{\"reported\":{\"file\":\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\"}}, \"clientToken\":\"" AWS_IOT_MQTT_CLIENT_ID "-0\"}"

TEST_C(ShadowActionTests, InboundDataTooBigForBuffer) {
	IoT_Error_t ret_val = SUCCESS;
	char getRequestJson[120];
	IoT_Publish_Message_Params params;

	IOT_DEBUG("-->Running Shadow Action Tests - Inbound data too big for buffer \n");

	snprintf(jsonFullDocument, 200, "NOT_VISITED");

	aws_iot_shadow_internal_get_request_json(getRequestJson);
	ret_val = aws_iot_shadow_internal_action(AWS_IOT_MY_THING_NAME, SHADOW_GET, getRequestJson, actionCallback, NULL, 4,
											 false);
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);

	params.payloadLen = strlen(JSON_SIZE_OVERFLOW);
	params.payload = JSON_SIZE_OVERFLOW;
	params.qos = QOS0;
	setTLSRxBufferWithMsgOnSubscribedTopic(GET_ACCEPTED_TOPIC, strlen(GET_ACCEPTED_TOPIC), QOS0, params,
										   params.payload);
	ret_val = aws_iot_shadow_yield(&client, 200);
	CHECK_EQUAL_C_INT(MQTT_RX_BUFFER_TOO_SHORT_ERROR, ret_val);
	CHECK_EQUAL_C_STRING("NOT_VISITED", jsonFullDocument);

	IOT_DEBUG("-->Success - Inbound data too big for buffer \n");
}

#define TEST_JSON_RESPONSE_NO_TOKEN "{\"state\":{\"reported\":{\"sensor1\":98}}}"

TEST_C(ShadowActionTests, NoClientTokenForShadowAction) {
	IoT_Error_t ret_val = SUCCESS;
	char getRequestJson[120];
	IoT_Publish_Message_Params params;

	uint8_t firstByte, secondByte;
	uint16_t topicNameLen;
	char topicName[128] = "test";

	IOT_DEBUG("-->Running Shadow Action Tests - No client token for shadow action \n");

	snprintf(getRequestJson, 120, "{}");
	snprintf(jsonFullDocument, 200, "NOT_VISITED");

	ret_val = aws_iot_shadow_internal_action(AWS_IOT_MY_THING_NAME, SHADOW_GET, getRequestJson, actionCallback, NULL, 4,
											 false);
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);

	params.payloadLen = strlen(TEST_JSON_RESPONSE_NO_TOKEN);
	params.payload = TEST_JSON_RESPONSE_NO_TOKEN;
	params.qos = QOS0;
	setTLSRxBufferWithMsgOnSubscribedTopic(GET_ACCEPTED_TOPIC, strlen(GET_ACCEPTED_TOPIC), QOS0, params,
										   params.payload);
	ret_val = aws_iot_shadow_yield(&client, 200);

	CHECK_EQUAL_C_INT(SUCCESS, ret_val);
	// Should never subscribe to accepted/rejected topics since we have no token to track the response
	CHECK_EQUAL_C_STRING("NOT_VISITED", jsonFullDocument);

	firstByte = (uint8_t)(TxBuffer.pBuffer[2]);
	secondByte = (uint8_t)(TxBuffer.pBuffer[3]);
	topicNameLen = (uint16_t) (secondByte + (256 * firstByte));

	snprintf(topicName, topicNameLen + 1u, "%s", &(TxBuffer.pBuffer[4])); // Added one for null character

	// Verify publish happens
	CHECK_EQUAL_C_STRING(GET_PUB_TOPIC, topicName);

	IOT_DEBUG("-->Success - No client token for shadow action \n");
}

TEST_C(ShadowActionTests, NoCallbackForShadowAction) {
	IoT_Error_t ret_val = SUCCESS;
	char getRequestJson[120];
	IoT_Publish_Message_Params params;

	uint8_t firstByte, secondByte;
	uint16_t topicNameLen;
	char topicName[128] = "test";

	IOT_DEBUG("-->Running Shadow Action Tests - No callback for shadow action \n");

	snprintf(jsonFullDocument, 200, "NOT_VISITED");

	aws_iot_shadow_internal_get_request_json(getRequestJson);
	ret_val = aws_iot_shadow_internal_action(AWS_IOT_MY_THING_NAME, SHADOW_GET, getRequestJson, NULL, NULL, 4, false);
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);

	params.payloadLen = strlen(TEST_JSON_RESPONSE_FULL_DOCUMENT);
	params.payload = TEST_JSON_RESPONSE_FULL_DOCUMENT;
	setTLSRxBufferWithMsgOnSubscribedTopic(GET_ACCEPTED_TOPIC, strlen(GET_ACCEPTED_TOPIC), QOS0, params,
										   params.payload);
	ret_val = aws_iot_shadow_yield(&client, 200);

	CHECK_EQUAL_C_INT(SUCCESS, ret_val);
	// Should never subscribe to accepted/rejected topics since we have no callback to track the response
	CHECK_EQUAL_C_STRING("NOT_VISITED", jsonFullDocument);

	firstByte = (uint8_t)(TxBuffer.pBuffer[2]);
	secondByte = (uint8_t)(TxBuffer.pBuffer[3]);
	topicNameLen = (uint16_t) (secondByte + (256 * firstByte));

	snprintf(topicName, topicNameLen + 1u, "%s", &(TxBuffer.pBuffer[4])); // Added one for null character

	// Verify publish happens
	CHECK_EQUAL_C_STRING(GET_PUB_TOPIC, topicName);

	IOT_DEBUG("-->Success - No callback for shadow action");
}

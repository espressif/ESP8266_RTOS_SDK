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
 * @file aws_iot_tests_unit_shadow_delta_helper.c
 * @brief IoT Client Unit Testing - Shadow Delta Tests Helper
 */

#include <string.h>
#include <stdio.h>
#include <CppUTest/TestHarness_c.h>

#include "aws_iot_shadow_interface.h"
#include "aws_iot_tests_unit_helper_functions.h"
#include "aws_iot_log.h"

static AWS_IoT_Client client;
static IoT_Client_Connect_Params connectParams;
static ShadowInitParameters_t shadowInitParams;
static ShadowConnectParameters_t shadowConnectParams;

static char receivedNestedObject[100] = "";
static char sentNestedObjectData[100] = "{\"sensor1\":23}";
static char shadowDeltaTopic[MAX_SHADOW_TOPIC_LENGTH_BYTES];

#define SHADOW_DELTA_UPDATE "$aws/things/%s/shadow/update/delta"

#undef AWS_IOT_MY_THING_NAME
#define AWS_IOT_MY_THING_NAME "AWS-IoT-C-SDK"

void genericCallback(const char *pJsonStringData, uint32_t JsonStringDataLen, jsonStruct_t *pContext) {
	printf("\nkey[%s]==Data[%.*s]\n", pContext->pKey, JsonStringDataLen, pJsonStringData);
}

void nestedObjectCallback(const char *pJsonStringData, uint32_t JsonStringDataLen, jsonStruct_t *pContext) {
	printf("\nkey[%s]==Data[%.*s]\n", pContext->pKey, JsonStringDataLen, pJsonStringData);
	snprintf(receivedNestedObject, 100, "%.*s", JsonStringDataLen, pJsonStringData);
}

TEST_GROUP_C_SETUP(ShadowDeltaTest) {
	IoT_Error_t ret_val = SUCCESS;

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

	snprintf(shadowDeltaTopic, MAX_SHADOW_TOPIC_LENGTH_BYTES, SHADOW_DELTA_UPDATE, AWS_IOT_MY_THING_NAME);
}

TEST_GROUP_C_TEARDOWN(ShadowDeltaTest) {

}

TEST_C(ShadowDeltaTest, registerDeltaSuccess) {
	jsonStruct_t windowHandler;
	char deltaJSONString[] = "{\"state\":{\"delta\":{\"window\":true}},\"version\":1}";
	bool windowOpenData = false;
	IoT_Publish_Message_Params params;
	IoT_Error_t ret_val = SUCCESS;

	IOT_DEBUG("\n-->Running Shadow Delta Tests - Register delta success \n");

	windowHandler.cb = genericCallback;
	windowHandler.pKey = "window";
	windowHandler.type = SHADOW_JSON_BOOL;
	windowHandler.pData = &windowOpenData;

	params.payloadLen = strlen(deltaJSONString);
	params.payload = deltaJSONString;
	params.qos = QOS0;

	ResetTLSBuffer();
	setTLSRxBufferForSuback(shadowDeltaTopic, strlen(shadowDeltaTopic), QOS0, params);

	ret_val = aws_iot_shadow_register_delta(&client, &windowHandler);

	ResetTLSBuffer();
	setTLSRxBufferWithMsgOnSubscribedTopic(shadowDeltaTopic, strlen(shadowDeltaTopic), QOS0, params, params.payload);

	ret_val = aws_iot_shadow_yield(&client, 3000);
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);

	CHECK_EQUAL_C_INT(true, windowOpenData);
}


TEST_C(ShadowDeltaTest, registerDeltaInt) {
	IoT_Error_t ret_val = SUCCESS;
	jsonStruct_t intHandler;
	int intData = 0;
	char deltaJSONString[] = "{\"state\":{\"delta\":{\"length\":23}},\"version\":1}";
	IoT_Publish_Message_Params params;

	IOT_DEBUG("\n-->Running Shadow Delta Tests - Register delta integer \n");

	intHandler.cb = genericCallback;
	intHandler.pKey = "length";
	intHandler.type = SHADOW_JSON_INT32;
	intHandler.pData = &intData;

	params.payloadLen = strlen(deltaJSONString);
	params.payload = deltaJSONString;
	params.qos = QOS0;

	ResetTLSBuffer();
	setTLSRxBufferForSuback(shadowDeltaTopic, strlen(shadowDeltaTopic), QOS0, params);

	ret_val = aws_iot_shadow_register_delta(&client, &intHandler);

	ResetTLSBuffer();
	setTLSRxBufferWithMsgOnSubscribedTopic(shadowDeltaTopic, strlen(shadowDeltaTopic), QOS0, params, params.payload);

	aws_iot_shadow_yield(&client, 3000);
	CHECK_EQUAL_C_INT(23, intData);
}

TEST_C(ShadowDeltaTest, registerDeltaIntNoCallback) {
	IoT_Error_t ret_val = SUCCESS;
	jsonStruct_t intHandler;
	int intData = 0;
	char deltaJSONString[] = "{\"state\":{\"delta\":{\"length_nocb\":23}},\"version\":1}";
	IoT_Publish_Message_Params params;

	IOT_DEBUG("\n-->Running Shadow Delta Tests - Register delta integer with no callback \n");

	intHandler.cb = NULL;
	intHandler.pKey = "length_nocb";
	intHandler.type = SHADOW_JSON_INT32;
	intHandler.pData = &intData;

	params.payloadLen = strlen(deltaJSONString);
	params.payload = deltaJSONString;
	params.qos = QOS0;

	ResetTLSBuffer();
	setTLSRxBufferForSuback(shadowDeltaTopic, strlen(shadowDeltaTopic), QOS0, params);

	ret_val = aws_iot_shadow_register_delta(&client, &intHandler);

	ResetTLSBuffer();
	setTLSRxBufferWithMsgOnSubscribedTopic(shadowDeltaTopic, strlen(shadowDeltaTopic), QOS0, params, params.payload);

	aws_iot_shadow_yield(&client, 3000);
	CHECK_EQUAL_C_INT(23, intData);
}

TEST_C(ShadowDeltaTest, DeltaNestedObject) {
	IoT_Error_t ret_val = SUCCESS;
	IoT_Publish_Message_Params params;
	jsonStruct_t nestedObjectHandler;
	char nestedObject[100];
	char deltaJSONString[100];

	printf("\n-->Running Shadow Delta Tests - Delta received with nested object \n");

	nestedObjectHandler.cb = nestedObjectCallback;
	nestedObjectHandler.pKey = "sensors";
	nestedObjectHandler.type = SHADOW_JSON_OBJECT;
	nestedObjectHandler.pData = &nestedObject;

	snprintf(deltaJSONString, 100, "{\"state\":{\"delta\":{\"%s\":%s}},\"version\":1}", nestedObjectHandler.pKey,
			sentNestedObjectData);
	params.payloadLen = strlen(deltaJSONString);
	params.payload = deltaJSONString;
	params.qos = QOS0;

	ResetTLSBuffer();
	setTLSRxBufferForSuback(shadowDeltaTopic, strlen(shadowDeltaTopic), QOS0, params);

	ret_val = aws_iot_shadow_register_delta(&client, &nestedObjectHandler);

	ResetTLSBuffer();
	setTLSRxBufferWithMsgOnSubscribedTopic(shadowDeltaTopic, strlen(shadowDeltaTopic), QOS0, params, params.payload);

	aws_iot_shadow_yield(&client, 3000);
	CHECK_EQUAL_C_STRING(sentNestedObjectData, receivedNestedObject);
}


// Send back to back version and ensure a wrong version is ignored with old message enabled
TEST_C(ShadowDeltaTest, DeltaVersionIgnoreOldVersion) {
	IoT_Error_t ret_val = SUCCESS;
	char deltaJSONString[100];
	jsonStruct_t nestedObjectHandler;
	char nestedObject[100];
	IoT_Publish_Message_Params params;

	printf("\n-->Running Shadow Delta Tests - delta received, old version ignored \n");

	nestedObjectHandler.cb = nestedObjectCallback;
	nestedObjectHandler.pKey = "sensors";
	nestedObjectHandler.type = SHADOW_JSON_OBJECT;
	nestedObjectHandler.pData = &nestedObject;

	snprintf(deltaJSONString, 100, "{\"state\":{\"delta\":{\"%s\":%s}},\"version\":1}", nestedObjectHandler.pKey,
			sentNestedObjectData);
	params.payloadLen = strlen(deltaJSONString);
	params.payload = deltaJSONString;
	params.qos = QOS0;

	ResetTLSBuffer();
	setTLSRxBufferForSuback(shadowDeltaTopic, strlen(shadowDeltaTopic), QOS0, params);

	ret_val = aws_iot_shadow_register_delta(&client, &nestedObjectHandler);

	ResetTLSBuffer();
	setTLSRxBufferWithMsgOnSubscribedTopic(shadowDeltaTopic, strlen(shadowDeltaTopic), QOS0, params, params.payload);

	aws_iot_shadow_yield(&client, 100);
	CHECK_EQUAL_C_STRING(sentNestedObjectData, receivedNestedObject);

	snprintf(receivedNestedObject, 100, " ");
	snprintf(deltaJSONString, 100, "{\"state\":{\"delta\":{\"%s\":%s}},\"version\":2}", nestedObjectHandler.pKey,
			sentNestedObjectData);
	params.payloadLen = strlen(deltaJSONString);
	params.payload = deltaJSONString;
	params.qos = QOS0;

	ResetTLSBuffer();
	setTLSRxBufferWithMsgOnSubscribedTopic(shadowDeltaTopic, strlen(shadowDeltaTopic), QOS0, params, params.payload);

	aws_iot_shadow_yield(&client, 100);
	CHECK_EQUAL_C_STRING(sentNestedObjectData, receivedNestedObject);

	snprintf(receivedNestedObject, 100, " ");
	snprintf(deltaJSONString, 100, "{\"state\":{\"delta\":{\"%s\":%s}},\"version\":2}", nestedObjectHandler.pKey,
			sentNestedObjectData);
	params.payloadLen = strlen(deltaJSONString);
	params.payload = deltaJSONString;
	params.qos = QOS0;

	ResetTLSBuffer();
	setTLSRxBufferWithMsgOnSubscribedTopic(shadowDeltaTopic, strlen(shadowDeltaTopic), QOS0, params, params.payload);

	aws_iot_shadow_yield(&client, 100);
	CHECK_EQUAL_C_STRING(" ", receivedNestedObject);

	snprintf(receivedNestedObject, 100, " ");
	snprintf(deltaJSONString, 100, "{\"state\":{\"delta\":{\"%s\":%s}},\"version\":3}", nestedObjectHandler.pKey,
			sentNestedObjectData);
	params.payloadLen = strlen(deltaJSONString);
	params.payload = deltaJSONString;
	params.qos = QOS0;

	ResetTLSBuffer();
	setTLSRxBufferWithMsgOnSubscribedTopic(shadowDeltaTopic, strlen(shadowDeltaTopic), QOS0, params, params.payload);

	aws_iot_shadow_yield(&client, 100);
	CHECK_EQUAL_C_STRING(sentNestedObjectData, receivedNestedObject);

	aws_iot_shadow_reset_last_received_version();

	snprintf(receivedNestedObject, 100, " ");
	snprintf(deltaJSONString, 100, "{\"state\":{\"delta\":{\"%s\":%s}},\"version\":3}", nestedObjectHandler.pKey,
			sentNestedObjectData);
	params.payloadLen = strlen(deltaJSONString);
	params.payload = deltaJSONString;
	params.qos = QOS0;

	ResetTLSBuffer();
	setTLSRxBufferWithMsgOnSubscribedTopic(shadowDeltaTopic, strlen(shadowDeltaTopic), QOS0, params, params.payload);

	aws_iot_shadow_yield(&client, 100);
	CHECK_EQUAL_C_STRING(sentNestedObjectData, receivedNestedObject);

	snprintf(receivedNestedObject, 100, " ");
	snprintf(deltaJSONString, 100, "{\"state\":{\"delta\":{\"%s\":%s}},\"version\":3}", nestedObjectHandler.pKey,
			sentNestedObjectData);
	params.payloadLen = strlen(deltaJSONString);
	params.payload = deltaJSONString;
	params.qos = QOS0;

	ResetTLSBuffer();
	setTLSRxBufferWithMsgOnSubscribedTopic(shadowDeltaTopic, strlen(shadowDeltaTopic), QOS0, params, params.payload);

	aws_iot_shadow_yield(&client, 100);
	CHECK_EQUAL_C_STRING(" ", receivedNestedObject);

	aws_iot_shadow_disable_discard_old_delta_msgs();

	snprintf(receivedNestedObject, 100, " ");
	snprintf(deltaJSONString, 100, "{\"state\":{\"delta\":{\"%s\":%s}},\"version\":3}", nestedObjectHandler.pKey,
			sentNestedObjectData);
	params.payloadLen = strlen(deltaJSONString);
	params.payload = deltaJSONString;
	params.qos = QOS0;

	ResetTLSBuffer();
	setTLSRxBufferWithMsgOnSubscribedTopic(shadowDeltaTopic, strlen(shadowDeltaTopic), QOS0, params, params.payload);

	aws_iot_shadow_yield(&client, 100);
	CHECK_EQUAL_C_STRING(sentNestedObjectData, receivedNestedObject);
}

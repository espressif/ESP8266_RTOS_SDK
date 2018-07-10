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
 * @file aws_iot_tests_unit_shadow_json_builder_helper.c
 * @brief IoT Client Unit Testing - Shadow JSON Builder Tests Helper
 */

#include <string.h>
#include <CppUTest/TestHarness_c.h>
#include <aws_iot_shadow_interface.h>

#include "aws_iot_shadow_actions.h"
#include "aws_iot_log.h"
#include "aws_iot_tests_unit_helper_functions.h"

static jsonStruct_t dataFloatHandler;
static jsonStruct_t dataDoubleHandler;
static double doubleData = 4.0908f;
static float floatData = 3.445f;
static AWS_IoT_Client iotClient;
static IoT_Client_Connect_Params connectParams;
static ShadowInitParameters_t shadowInitParams;
static ShadowConnectParameters_t shadowConnectParams;

TEST_GROUP_C_SETUP(ShadowJsonBuilderTests) {
	IoT_Error_t ret_val;

	shadowInitParams.pHost = AWS_IOT_MQTT_HOST;
	shadowInitParams.port = AWS_IOT_MQTT_PORT;
	shadowInitParams.pClientCRT = AWS_IOT_CERTIFICATE_FILENAME;
	shadowInitParams.pRootCA = AWS_IOT_ROOT_CA_FILENAME;
	shadowInitParams.pClientKey = AWS_IOT_PRIVATE_KEY_FILENAME;
	shadowInitParams.disconnectHandler = NULL;
	shadowInitParams.enableAutoReconnect = false;
	ret_val = aws_iot_shadow_init(&iotClient, &shadowInitParams);
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);

	shadowConnectParams.pMyThingName = AWS_IOT_MY_THING_NAME;
	shadowConnectParams.pMqttClientId = AWS_IOT_MQTT_CLIENT_ID;
	shadowConnectParams.mqttClientIdLen = (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID);
	ConnectMQTTParamsSetup(&connectParams, AWS_IOT_MQTT_CLIENT_ID, (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID));
	setTLSRxBufferForConnack(&connectParams, 0, 0);
	ret_val = aws_iot_shadow_connect(&iotClient, &shadowConnectParams);
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);

	dataFloatHandler.cb = NULL;
	dataFloatHandler.pData = &floatData;
	dataFloatHandler.pKey = "floatData";
	dataFloatHandler.type = SHADOW_JSON_FLOAT;

	dataDoubleHandler.cb = NULL;
	dataDoubleHandler.pData = &doubleData;
	dataDoubleHandler.pKey = "doubleData";
	dataDoubleHandler.type = SHADOW_JSON_DOUBLE;
}

TEST_GROUP_C_TEARDOWN(ShadowJsonBuilderTests) {
	/* Clean up. Not checking return code here because this is common to all tests.
	 * A test might have already caused a disconnect by this point.
	 */
	IoT_Error_t rc = aws_iot_mqtt_disconnect(&iotClient);
	IOT_UNUSED(rc);
}

#define TEST_JSON_RESPONSE_UPDATE_DOCUMENT "{\"state\":{\"reported\":{\"doubleData\":4.090800,\"floatData\":3.445000}}, \"clientToken\":\"" AWS_IOT_MQTT_CLIENT_ID "-0\"}"

#define SIZE_OF_UPFATE_BUF 200

TEST_C(ShadowJsonBuilderTests, UpdateTheJSONDocumentBuilder) {
	IoT_Error_t ret_val;
	char updateRequestJson[SIZE_OF_UPFATE_BUF];
	size_t jsonBufSize = sizeof(updateRequestJson) / sizeof(updateRequestJson[0]);

	IOT_DEBUG("\n-->Running Shadow Json Builder Tests - Update the Json document builder \n");

	ret_val = aws_iot_shadow_init_json_document(updateRequestJson, jsonBufSize);
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);
	ret_val = aws_iot_shadow_add_reported(updateRequestJson, jsonBufSize, 2, &dataDoubleHandler, &dataFloatHandler);
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);
	ret_val = aws_iot_finalize_json_document(updateRequestJson, jsonBufSize);
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);

	CHECK_EQUAL_C_STRING(TEST_JSON_RESPONSE_UPDATE_DOCUMENT, updateRequestJson);
}

TEST_C(ShadowJsonBuilderTests, PassingNullValue) {
	IoT_Error_t ret_val;

	IOT_DEBUG("\n-->Running Shadow Json Builder Tests - Passing Null value to Shadow json builder \n");

	ret_val = aws_iot_shadow_init_json_document(NULL, SIZE_OF_UPFATE_BUF);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, ret_val);
	ret_val = aws_iot_shadow_add_reported(NULL, SIZE_OF_UPFATE_BUF, 2, &dataDoubleHandler, &dataFloatHandler);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, ret_val);
	ret_val = aws_iot_shadow_add_desired(NULL, SIZE_OF_UPFATE_BUF, 2, &dataDoubleHandler, &dataFloatHandler);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, ret_val);
	ret_val = aws_iot_finalize_json_document(NULL, SIZE_OF_UPFATE_BUF);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, ret_val);
}

TEST_C(ShadowJsonBuilderTests, SmallBuffer) {
	IoT_Error_t ret_val;
	char updateRequestJson[14];
	size_t jsonBufSize = sizeof(updateRequestJson) / sizeof(updateRequestJson[0]);

	IOT_DEBUG("\n-->Running Shadow Json Builder Tests - Json Buffer is too small \n");

	ret_val = aws_iot_shadow_init_json_document(updateRequestJson, jsonBufSize);
	CHECK_EQUAL_C_INT(SUCCESS, ret_val);
	ret_val = aws_iot_shadow_add_reported(updateRequestJson, jsonBufSize, 2, &dataDoubleHandler, &dataFloatHandler);
	CHECK_EQUAL_C_INT(SHADOW_JSON_BUFFER_TRUNCATED, ret_val);
	ret_val = aws_iot_shadow_add_desired(updateRequestJson, jsonBufSize, 2, &dataDoubleHandler, &dataFloatHandler);
	CHECK_EQUAL_C_INT(SHADOW_JSON_ERROR, ret_val);
	ret_val = aws_iot_finalize_json_document(updateRequestJson, jsonBufSize);
	CHECK_EQUAL_C_INT(SHADOW_JSON_ERROR, ret_val);
}

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
 * @file aws_iot_tests_unit_shadow_null_fields_helper.c
 * @brief IoT Client Unit Testing - Shadow APIs NULL field Tests helper
 */

#include <stdio.h>
#include <string.h>
#include <CppUTest/TestHarness_c.h>
#include <aws_iot_shadow_interface.h>

#include "aws_iot_tests_unit_helper_functions.h"
#include "aws_iot_shadow_interface.h"
#include "aws_iot_shadow_actions.h"
#include "aws_iot_log.h"

static AWS_IoT_Client client;
static ShadowInitParameters_t shadowInitParams;
static ShadowConnectParameters_t shadowConnectParams;

static Shadow_Ack_Status_t ackStatusRx;
static ShadowActions_t actionRx;
static char jsonFullDocument[200];

void actionCallbackNullTest(const char *pThingName, ShadowActions_t action, Shadow_Ack_Status_t status,
							const char *pReceivedJsonDocument, void *pContextData) {
	IOT_UNUSED(pThingName);
	IOT_UNUSED(pContextData);
	IOT_DEBUG("%s", pReceivedJsonDocument);
	actionRx = action;
	ackStatusRx = status;
	if(status != SHADOW_ACK_TIMEOUT) {
		strcpy(jsonFullDocument, pReceivedJsonDocument);
	}
}

TEST_GROUP_C_SETUP(ShadowNullFields) {
	ResetTLSBuffer();
}

TEST_GROUP_C_TEARDOWN(ShadowNullFields) { }

TEST_C(ShadowNullFields, NullHost) {
	shadowInitParams.pHost = NULL;
	shadowInitParams.port = AWS_IOT_MQTT_PORT;
	IoT_Error_t rc = aws_iot_shadow_init(&client, &shadowInitParams);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, rc);
}

TEST_C(ShadowNullFields, NullPort) {
	shadowInitParams.pHost = AWS_IOT_MQTT_HOST;
	shadowInitParams.port = 0;
	IoT_Error_t rc = aws_iot_shadow_init(&client, &shadowInitParams);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, rc);
}

TEST_C(ShadowNullFields, NullClientID) {
	shadowInitParams.pHost = AWS_IOT_MQTT_HOST;
	shadowInitParams.port = AWS_IOT_MQTT_PORT;
	shadowInitParams.pClientCRT = AWS_IOT_CERTIFICATE_FILENAME;
	shadowInitParams.pRootCA = AWS_IOT_ROOT_CA_FILENAME;
	shadowInitParams.pClientKey = AWS_IOT_PRIVATE_KEY_FILENAME;
	shadowInitParams.disconnectHandler = NULL;
	shadowInitParams.enableAutoReconnect = false;
	IoT_Error_t rc = aws_iot_shadow_init(&client, &shadowInitParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	shadowConnectParams.pMyThingName = AWS_IOT_MY_THING_NAME;
	shadowConnectParams.pMqttClientId = NULL;
	rc = aws_iot_shadow_connect(&client, &shadowConnectParams);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, rc);
}

TEST_C(ShadowNullFields, NullClientInit) {
	shadowInitParams.pHost = AWS_IOT_MQTT_HOST;
	shadowInitParams.port = AWS_IOT_MQTT_PORT;
	IoT_Error_t rc = aws_iot_shadow_init(NULL, &shadowInitParams);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, rc);
}

TEST_C(ShadowNullFields, NullClientConnect) {
	shadowInitParams.pHost = AWS_IOT_MQTT_HOST;
	shadowInitParams.port = AWS_IOT_MQTT_PORT;
	shadowInitParams.pClientCRT = AWS_IOT_CERTIFICATE_FILENAME;
	shadowInitParams.pRootCA = AWS_IOT_ROOT_CA_FILENAME;
	shadowInitParams.pClientKey = AWS_IOT_PRIVATE_KEY_FILENAME;
	shadowInitParams.disconnectHandler = NULL;
	shadowInitParams.enableAutoReconnect = false;
	IoT_Error_t rc = aws_iot_shadow_init(&client, &shadowInitParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	shadowConnectParams.pMyThingName = AWS_IOT_MY_THING_NAME;
	shadowConnectParams.pMqttClientId = AWS_IOT_MQTT_CLIENT_ID;
	shadowConnectParams.mqttClientIdLen = (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID);
	rc = aws_iot_shadow_connect(NULL, &shadowConnectParams);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, rc);
}

TEST_C(ShadowNullFields, NullUpdateDocument) {
	IoT_Error_t rc = aws_iot_shadow_internal_action(AWS_IOT_MY_THING_NAME, SHADOW_UPDATE, NULL, actionCallbackNullTest,
													NULL, 4, false);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, rc);
}

TEST_C(ShadowNullFields, NullClientYield) {
	IoT_Error_t rc = aws_iot_shadow_yield(NULL, 1000);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, rc);
}

TEST_C(ShadowNullFields, NullClientDisconnect) {
	IoT_Error_t rc = aws_iot_shadow_disconnect(NULL);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, rc);
}

TEST_C(ShadowNullFields, NullClientShadowGet) {
	IoT_Error_t rc = aws_iot_shadow_get(NULL, AWS_IOT_MY_THING_NAME, actionCallbackNullTest, NULL, 100, true);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, rc);
}

TEST_C(ShadowNullFields, NullClientShadowUpdate) {
	IoT_Error_t rc = aws_iot_shadow_update(NULL, AWS_IOT_MY_THING_NAME, jsonFullDocument,
										   actionCallbackNullTest, NULL, 100, true);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, rc);
}

TEST_C(ShadowNullFields, NullClientShadowDelete) {
	IoT_Error_t rc = aws_iot_shadow_delete(NULL, AWS_IOT_MY_THING_NAME, actionCallbackNullTest, NULL, 100, true);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, rc);
}

TEST_C(ShadowNullFields, NullClientSetAutoreconnect) {
	IoT_Error_t rc = aws_iot_shadow_set_autoreconnect_status(NULL, true);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, rc);
}

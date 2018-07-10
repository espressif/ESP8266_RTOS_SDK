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
 * @file aws_iot_tests_unit_disconnect_helper.c
 * @brief IoT Client Unit Testing - Disconnect Tests helper
 */

#include <stdio.h>
#include <unistd.h>
#include <CppUTest/TestHarness_c.h>

#include "aws_iot_tests_unit_helper_functions.h"
#include "aws_iot_log.h"

static IoT_Client_Init_Params initParams;
static IoT_Client_Connect_Params connectParams;
static AWS_IoT_Client iotClient;

static bool handlerInvoked = false;

void disconnectTestHandler(AWS_IoT_Client *pClient, void *disconHandlerParam) {
	IOT_UNUSED(pClient);
	IOT_UNUSED(disconHandlerParam);

	handlerInvoked = true;
}

TEST_GROUP_C_SETUP(DisconnectTests) {
	IoT_Error_t rc;
	ResetTLSBuffer();
	InitMQTTParamsSetup(&initParams, AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT, true, disconnectTestHandler);
	initParams.mqttCommandTimeout_ms = 2000;
	rc = aws_iot_mqtt_init(&iotClient, &initParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	ConnectMQTTParamsSetup(&connectParams, AWS_IOT_MQTT_CLIENT_ID, (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID));
	connectParams.keepAliveIntervalInSec = 5;
	setTLSRxBufferForConnack(&connectParams, 0, 0);
	rc = aws_iot_mqtt_connect(&iotClient, &connectParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	IOT_DEBUG("MQTT Status State : %d, RC : %d\n\n", aws_iot_mqtt_get_client_state(&iotClient), rc);

	ResetTLSBuffer();
}

TEST_GROUP_C_TEARDOWN(DisconnectTests) { }


/* F:1 - Disconnect with Null/empty client instance */
TEST_C(DisconnectTests, NullClientDisconnect) {
	IoT_Error_t rc = aws_iot_mqtt_disconnect(NULL);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, rc);
}

/* F:2 - Set Disconnect Handler with Null/empty Client */
TEST_C(DisconnectTests, NullClientSetDisconnectHandler) {
	IoT_Error_t rc = aws_iot_mqtt_set_disconnect_handler(NULL, disconnectTestHandler, NULL);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, rc);
}

/* F:3 - Call Set Disconnect handler with Null handler */
TEST_C(DisconnectTests, SetDisconnectHandlerNullHandler) {
	IoT_Error_t rc = aws_iot_mqtt_set_disconnect_handler(&iotClient, NULL, NULL);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, rc);
}

/* F:4 - Disconnect attempt, not connected */
TEST_C(DisconnectTests, disconnectNotConnected) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Disconnect Tests - F:4 - Disconnect attempt, not connected \n");

	/* First make sure client is disconnected */
	rc = aws_iot_mqtt_disconnect(&iotClient);

	/* Check client is disconnected */
	CHECK_EQUAL_C_INT(false, aws_iot_mqtt_is_client_connected(&iotClient));

	/* Now call disconnect again */
	rc = aws_iot_mqtt_disconnect(&iotClient);
	CHECK_EQUAL_C_INT(NETWORK_DISCONNECTED_ERROR, rc);

	IOT_DEBUG("-->Success - F:4 - Disconnect attempt, not connected \n");
}

/* F:5 - Disconnect success */
TEST_C(DisconnectTests, disconnectNoAckSuccess) {
	IoT_Error_t rc = SUCCESS;
	rc = aws_iot_mqtt_disconnect(&iotClient);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
}

/* F:6 - Disconnect, Handler invoked on disconnect */
TEST_C(DisconnectTests, HandlerInvokedOnDisconnect) {
	bool connected = false;
	bool currentAutoReconnectStatus = false;
	int i;
	int j;
	int attempt = 3;
	uint32_t dcCount = 0;
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Disconnect Tests - F:6 - Disconnect, Handler invoked on disconnect \n");

	handlerInvoked = false;

	IOT_DEBUG("Current Keep Alive Interval is set to %d sec.\n", connectParams.keepAliveIntervalInSec);
	currentAutoReconnectStatus = aws_iot_is_autoreconnect_enabled(&iotClient);

	connected = aws_iot_mqtt_is_client_connected(&iotClient);
	CHECK_EQUAL_C_INT(1, connected);

	aws_iot_mqtt_autoreconnect_set_status(&iotClient, false);

	// 3 cycles of half keep alive time expiring
	// verify a ping request is sent and give a ping response
	for(i = 0; i < attempt; i++) {
		/* Set TLS buffer for ping response */
		ResetTLSBuffer();
		setTLSRxBufferForPingresp();
		for(j = 0; j <= connectParams.keepAliveIntervalInSec; j++) {
			sleep(1);
			rc = aws_iot_mqtt_yield(&iotClient, 100);
			CHECK_EQUAL_C_INT(SUCCESS, rc);
		}
		CHECK_EQUAL_C_INT(1, isLastTLSTxMessagePingreq());
	}

	// keepalive() waits for 1/2 of keepalive time after sending ping request
	// to receive a pingresponse before determining the connection is not alive
	// wait for keepalive time and then yield()
	sleep(connectParams.keepAliveIntervalInSec);
	rc = aws_iot_mqtt_yield(&iotClient, 100);
	CHECK_EQUAL_C_INT(NETWORK_DISCONNECTED_ERROR, rc);
	CHECK_EQUAL_C_INT(1, isLastTLSTxMessageDisconnect());

	connected = aws_iot_mqtt_is_client_connected(&iotClient);
	CHECK_EQUAL_C_INT(0, connected);

	CHECK_EQUAL_C_INT(true, handlerInvoked);

	dcCount = aws_iot_mqtt_get_network_disconnected_count(&iotClient);
	CHECK_C(1 == dcCount);

	aws_iot_mqtt_reset_network_disconnected_count(&iotClient);

	dcCount = aws_iot_mqtt_get_network_disconnected_count(&iotClient);
	CHECK_C(0 == dcCount);

	ResetTLSBuffer();
	aws_iot_mqtt_autoreconnect_set_status(&iotClient, currentAutoReconnectStatus);

	IOT_DEBUG("-->Success - F:6 - Disconnect, Handler invoked on disconnect \n");
}


/* F:7 - Disconnect, with set handler and invoked on disconnect */
TEST_C(DisconnectTests, SetHandlerAndInvokedOnDisconnect) {
	bool connected = false;
	bool currentAutoReconnectStatus = false;
	int i;
	int j;
	int attempt = 3;
	uint32_t dcCount = 0;
	IoT_Error_t rc = SUCCESS;
	IOT_DEBUG("-->Running Disconnect Tests - F:7 - Disconnect, with set handler and invoked on disconnect \n");

	handlerInvoked = false;
	InitMQTTParamsSetup(&initParams, "localhost", 8883, false, NULL);
	rc = aws_iot_mqtt_init(&iotClient, &initParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	ConnectMQTTParamsSetup(&connectParams, AWS_IOT_MQTT_CLIENT_ID, (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID));
	connectParams.keepAliveIntervalInSec = 5;
	setTLSRxBufferForConnack(&connectParams, 0, 0);
	rc = aws_iot_mqtt_connect(&iotClient, &connectParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	aws_iot_mqtt_set_disconnect_handler(&iotClient, disconnectTestHandler, NULL);
	aws_iot_mqtt_autoreconnect_set_status(&iotClient, true);

	IOT_DEBUG("Current Keep Alive Interval is set to %d sec.\n", connectParams.keepAliveIntervalInSec);
	currentAutoReconnectStatus = aws_iot_is_autoreconnect_enabled(&iotClient);

	connected = aws_iot_mqtt_is_client_connected(&iotClient);
	CHECK_EQUAL_C_INT(1, connected);

	aws_iot_mqtt_autoreconnect_set_status(&iotClient, false);

	// 3 cycles of keep alive time expiring
	// verify a ping request is sent and give a ping response
	for(i = 0; i < attempt; i++) {
		/* Set TLS buffer for ping response */
		ResetTLSBuffer();
		setTLSRxBufferForPingresp();
		for(j = 0; j <= connectParams.keepAliveIntervalInSec; j++) {
			sleep(1);
			rc = aws_iot_mqtt_yield(&iotClient, 100);
			CHECK_EQUAL_C_INT(SUCCESS, rc);
		}
		CHECK_EQUAL_C_INT(1, isLastTLSTxMessagePingreq());
	}
	ResetTLSBuffer();

	// keepalive() waits for 1/2 of keepalive time after sending ping request
	// to receive a pingresponse before determining the connection is not alive
	// wait for keepalive time and then yield()
	sleep(connectParams.keepAliveIntervalInSec);
	rc = aws_iot_mqtt_yield(&iotClient, 100);
	CHECK_EQUAL_C_INT(NETWORK_DISCONNECTED_ERROR, rc);
	CHECK_EQUAL_C_INT(1, isLastTLSTxMessageDisconnect());

	connected = aws_iot_mqtt_is_client_connected(&iotClient);
	CHECK_EQUAL_C_INT(0, connected);

	CHECK_EQUAL_C_INT(true, handlerInvoked);

	dcCount = aws_iot_mqtt_get_network_disconnected_count(&iotClient);
	CHECK_C(1 == dcCount);

	aws_iot_mqtt_reset_network_disconnected_count(&iotClient);

	dcCount = aws_iot_mqtt_get_network_disconnected_count(&iotClient);
	CHECK_C(0 == dcCount);

	ResetTLSBuffer();
	aws_iot_mqtt_autoreconnect_set_status(&iotClient, currentAutoReconnectStatus);

	IOT_DEBUG("-->Success - F:7 - Disconnect, with set handler and invoked on disconnect \n");
}


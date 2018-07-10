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
 * @file aws_iot_tests_unit_connect_helper.c
 * @brief IoT Client Unit Testing - Connect API Tests Helper
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <CppUTest/TestHarness_c.h>
#include <aws_iot_mqtt_client.h>

#include "aws_iot_tests_unit_mock_tls_params.h"
#include "aws_iot_tests_unit_helper_functions.h"

#include "aws_iot_log.h"

static bool unitTestIsMqttConnected = false;

static IoT_Client_Init_Params initParams;
static IoT_Client_Connect_Params connectParams;
static AWS_IoT_Client iotClient;

static IoT_Publish_Message_Params testPubMsgParams;
static ConnectBufferProofread prfrdParams;

static char subTopic1[12] = "sdk/Topic1";
static char subTopic2[12] = "sdk/Topic2";

#define NO_MSG_XXXX "XXXX"
static char CallbackMsgStringclean[100] = NO_MSG_XXXX;

static void iot_subscribe_callback_handler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
										   IoT_Publish_Message_Params *params, void *pData) {
	char *tmp = params->payload;
	unsigned int i;

	if(NULL == pClient || NULL == topicName || 0 == topicNameLen) {
		return;
	}

	IOT_UNUSED(pData);

	for(i = 0; i < (params->payloadLen); i++) {
		CallbackMsgStringclean[i] = tmp[i];
	}
}

TEST_GROUP_C_SETUP(ConnectTests) {
	IoT_Error_t rc = SUCCESS;
	unitTestIsMqttConnected = false;
	rc = aws_iot_mqtt_disconnect(&iotClient);
	ResetTLSBuffer();
	ResetInvalidParameters();
}

TEST_GROUP_C_TEARDOWN(ConnectTests) {
	/* Clean up. Not checking return code here because this is common to all tests.
	 * A test might have already caused a disconnect by this point.
	 */
	IoT_Error_t rc = aws_iot_mqtt_disconnect(&iotClient);
	IOT_UNUSED(rc);
}

/* B:1 - Init with Null/empty client instance */
TEST_C(ConnectTests, NullClientInit) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Connect Tests - B:1 - Init with Null/empty client instance \n");

	InitMQTTParamsSetup(&initParams, AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT, false, NULL);
	rc = aws_iot_mqtt_init(NULL, &initParams);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, rc);

	IOT_DEBUG("-->Success - B:1 - Init with Null/empty client instance \n");
}

/* B:2 - Connect with Null/empty client instance */
TEST_C(ConnectTests, NullClientConnect) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Connect Tests - B:2 - Connect with Null/empty client instance \n");

	ConnectMQTTParamsSetup(&connectParams, AWS_IOT_MQTT_CLIENT_ID, (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID));
	setTLSRxBufferForConnack(&connectParams, 0, 0);
	rc = aws_iot_mqtt_connect(NULL, &connectParams);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, rc);

	IOT_DEBUG("-->Success - B:2 - Connect with Null/empty client instance \n");
}

/* B:3 - Connect with Null/Empty endpoint */
TEST_C(ConnectTests, NullHost) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Connect Tests - B:3 - Connect with Null/Empty endpoint \n");

	InitMQTTParamsSetup(&initParams, NULL, AWS_IOT_MQTT_PORT, false, NULL);
	rc = aws_iot_mqtt_init(&iotClient, &initParams);
	CHECK_C(SUCCESS != rc);

	IOT_DEBUG("-->Success - B:3 - Connect with Null/Empty endpoint \n");
}

/* B:4 - Connect with Null/Empty port */
TEST_C(ConnectTests, NullPort) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Connect Tests - B:4 - Connect with Null/Empty port \n");

	InitMQTTParamsSetup(&initParams, AWS_IOT_MQTT_HOST, 0, false, NULL);
	rc = aws_iot_mqtt_init(&iotClient, &initParams);
	CHECK_C(SUCCESS != rc);

	IOT_DEBUG("-->Success - B:4 - Connect with Null/Empty port \n");
}

/* B:5 - Connect with Null/Empty root CA path */
TEST_C(ConnectTests, NullRootCAPath) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Connect Tests - B:6 - Connect with Null/Empty root CA path \n");

	InitMQTTParamsSetup(&initParams, AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT, false, NULL);
	initParams.pRootCALocation = NULL;
	rc = aws_iot_mqtt_init(&iotClient, &initParams);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, rc);

	IOT_DEBUG("-->Success - B:6 - Connect with Null/Empty root CA path \n");
}

/* B:6 - Connect with Null/Empty Client certificate path */
TEST_C(ConnectTests, NullClientCertificate) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Connect Tests - B:7 - Connect with Null/Empty Client certificate path \n");

	InitMQTTParamsSetup(&initParams, AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT, false, NULL);
	initParams.pDeviceCertLocation = NULL;
	rc = aws_iot_mqtt_init(&iotClient, &initParams);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, rc);

	IOT_DEBUG("-->Success - B:7 - Connect with Null/Empty Client certificate path \n");
}

/* B:7 - Connect with Null/Empty private key Path */
TEST_C(ConnectTests, NullPrivateKeyPath) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Connect Tests - B:8 - Connect with Null/Empty private key Path \n");

	InitMQTTParamsSetup(&initParams, AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT, false, NULL);
	initParams.pDevicePrivateKeyLocation = NULL;
	rc = aws_iot_mqtt_init(&iotClient, &initParams);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, rc);

	IOT_DEBUG("-->Success - B:8 - Connect with Null/Empty private key Path \n");
}

/* B:8 - Connect with Null/Empty client ID */
TEST_C(ConnectTests, NullClientID) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Connect Tests - B:8 - Connect with Null/Empty client ID \n");

	InitMQTTParamsSetup(&initParams, AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT, false, NULL);
	rc = aws_iot_mqtt_init(&iotClient, &initParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	setTLSRxBufferForConnack(&connectParams, 0, 0);

	/* If no client id is passed but a length was passed, return error */
	ConnectMQTTParamsSetup(&connectParams, NULL, (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID));
	rc = aws_iot_mqtt_connect(&iotClient, &connectParams);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, rc);

	/* If client id is passed but 0 length was passed, return error */
	ConnectMQTTParamsSetup(&connectParams, AWS_IOT_MQTT_CLIENT_ID, 0);
	rc = aws_iot_mqtt_connect(&iotClient, &connectParams);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, rc);

	/* If client id is NULL and length is 0 then request succeeds */
	ConnectMQTTParamsSetup(&connectParams, NULL, 0);
	rc = aws_iot_mqtt_connect(&iotClient, &connectParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	IOT_DEBUG("-->Success - B:8 - Connect with Null/Empty client ID \n");
}

/* B:9 - Connect with invalid Endpoint */
TEST_C(ConnectTests, InvalidEndpoint) {
	IoT_Error_t rc = SUCCESS;
	char invalidEndPoint[20];
	snprintf(invalidEndPoint, 20, "invalid");

	IOT_DEBUG("-->Running Connect Tests - B:9 - Connect with invalid Endpoint \n");

	InitMQTTParamsSetup(&initParams, AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT, false, NULL);
	invalidEndpointFilter = invalidEndPoint;
	initParams.pHostURL = invalidEndpointFilter;
	rc = aws_iot_mqtt_init(&iotClient, &initParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	ConnectMQTTParamsSetup(&connectParams, AWS_IOT_MQTT_CLIENT_ID, (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID));
	setTLSRxBufferForConnack(&connectParams, 0, 0);
	rc = aws_iot_mqtt_connect(&iotClient, &connectParams);
	CHECK_C(SUCCESS != rc);

	IOT_DEBUG("-->Success - B:9 - Connect with invalid Endpoint \n");
}

/* B:10 - Connect with invalid correct endpoint but invalid port */
TEST_C(ConnectTests, InvalidPort) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Connect Tests - B:10 - Connect with invalid correct endpoint but invalid port \n");

	InitMQTTParamsSetup(&initParams, AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT, false, NULL);
	invalidPortFilter = 1234;
	initParams.port = invalidPortFilter;
	rc = aws_iot_mqtt_init(&iotClient, &initParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	ConnectMQTTParamsSetup(&connectParams, AWS_IOT_MQTT_CLIENT_ID, (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID));
	setTLSRxBufferForConnack(&connectParams, 0, 0);
	rc = aws_iot_mqtt_connect(&iotClient, &connectParams);
	CHECK_C(SUCCESS != rc);

	IOT_DEBUG("-->Success - B:10 - Connect with invalid correct endpoint but invalid port \n");
}

/* B:11 - Connect with invalid Root CA path */
TEST_C(ConnectTests, InvalidRootCAPath) {
	IoT_Error_t rc = SUCCESS;
	char invalidRootCAPath[20];
	snprintf(invalidRootCAPath, 20, "invalid");

	IOT_DEBUG("-->Running Connect Tests - B:11 - Connect with invalid Root CA path \n");

	InitMQTTParamsSetup(&initParams, AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT, false, NULL);
	invalidRootCAPathFilter = invalidRootCAPath;
	initParams.pRootCALocation = invalidRootCAPathFilter;
	rc = aws_iot_mqtt_init(&iotClient, &initParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	ConnectMQTTParamsSetup(&connectParams, AWS_IOT_MQTT_CLIENT_ID, (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID));
	setTLSRxBufferForConnack(&connectParams, 0, 0);
	rc = aws_iot_mqtt_connect(&iotClient, &connectParams);
	CHECK_C(SUCCESS != rc);

	IOT_DEBUG("-->Success - B:11 - Connect with invalid Root CA path \n");
}

/* B:12 - Connect with invalid Client certificate path */
TEST_C(ConnectTests, InvalidClientCertPath) {
	IoT_Error_t rc = SUCCESS;
	char invalidCertPath[20];
	snprintf(invalidCertPath, 20, "invalid");

	IOT_DEBUG("-->Running Connect Tests - B:12 - Connect with invalid Client certificate path \n");

	InitMQTTParamsSetup(&initParams, AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT, false, NULL);
	invalidCertPathFilter = invalidCertPath;
	initParams.pDeviceCertLocation = invalidCertPathFilter;
	rc = aws_iot_mqtt_init(&iotClient, &initParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	ConnectMQTTParamsSetup(&connectParams, AWS_IOT_MQTT_CLIENT_ID, (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID));
	setTLSRxBufferForConnack(&connectParams, 0, 0);
	rc = aws_iot_mqtt_connect(&iotClient, &connectParams);
	CHECK_C(SUCCESS != rc);

	IOT_DEBUG("-->Success - B:12 - Connect with invalid Client certificate path \n");
}

/* B:13 - Connect with invalid private key path */
TEST_C(ConnectTests, InvalidPrivateKeyPath) {
	IoT_Error_t rc = SUCCESS;
	char invalidPrivKeyPath[20];
	snprintf(invalidPrivKeyPath, 20, "invalid");

	IOT_DEBUG("-->Running Connect Tests - B:13 - Connect with invalid private key path \n");

	InitMQTTParamsSetup(&initParams, AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT, false, NULL);
	invalidPrivKeyPathFilter = invalidPrivKeyPath;
	initParams.pDevicePrivateKeyLocation = invalidPrivKeyPathFilter;
	rc = aws_iot_mqtt_init(&iotClient, &initParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	ConnectMQTTParamsSetup(&connectParams, AWS_IOT_MQTT_CLIENT_ID, (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID));
	setTLSRxBufferForConnack(&connectParams, 0, 0);
	rc = aws_iot_mqtt_connect(&iotClient, &connectParams);
	CHECK_C(SUCCESS != rc);

	IOT_DEBUG("-->Success - B:13 - Connect with invalid private key path \n");
}

/* B:14 - Connect, no response timeout */
TEST_C(ConnectTests, NoResponseTimeout) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Connect Tests - B:14 - Connect, no response timeout \n");

	InitMQTTParamsSetup(&initParams, AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT, false, NULL);
	rc = aws_iot_mqtt_init(&iotClient, &initParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	ResetTLSBuffer();
	ConnectMQTTParamsSetup(&connectParams, AWS_IOT_MQTT_CLIENT_ID, (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID));
	rc = aws_iot_mqtt_connect(&iotClient, &connectParams);
	CHECK_EQUAL_C_INT(MQTT_REQUEST_TIMEOUT_ERROR, rc);

	IOT_DEBUG("-->Success - B:14 - Connect, no response timeout \n");
}

/* B:15 - Connect, connack malformed, too large */
TEST_C(ConnectTests, ConnackTooLarge) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Connect Tests - B:15 - Connect, connack malformed, too large \n");

	InitMQTTParamsSetup(&initParams, AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT, false, NULL);
	rc = aws_iot_mqtt_init(&iotClient, &initParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	ConnectMQTTParamsSetup(&connectParams, AWS_IOT_MQTT_CLIENT_ID, (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID));
	setTLSRxBufferForConnack(&connectParams, 0, 0);
	RxBuffer.pBuffer[1] = (char) (0x15); /* Set remaining length to a larger than expected value */
	rc = aws_iot_mqtt_connect(&iotClient, &connectParams);
	CHECK_C(SUCCESS != rc);

	IOT_DEBUG("-->Success - B:15 - Connect, connack malformed, too large \n");
}

/* B:16 - Connect, connack malformed, fixed header corrupted */
TEST_C(ConnectTests, FixedHeaderCorrupted) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Connect Tests - B:16 - Connect, connack malformed, fixed header corrupted \n");

	InitMQTTParamsSetup(&initParams, AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT, false, NULL);
	rc = aws_iot_mqtt_init(&iotClient, &initParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	ConnectMQTTParamsSetup(&connectParams, AWS_IOT_MQTT_CLIENT_ID, (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID));
	setTLSRxBufferForConnack(&connectParams, 0, 0);
	RxBuffer.pBuffer[0] = (char) (0x00);
	rc = aws_iot_mqtt_connect(&iotClient, &connectParams);
	CHECK_C(SUCCESS != rc);

	IOT_DEBUG("-->Success - B:16 - Connect, connack malformed, fixed header corrupted \n");
}

/* B:17 - Connect, connack malformed, invalid remaining length */
TEST_C(ConnectTests, InvalidRemainingLength) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Connect Tests - B:17 - Connect, connack malformed, invalid remaining length \n");

	InitMQTTParamsSetup(&initParams, AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT, false, NULL);
	rc = aws_iot_mqtt_init(&iotClient, &initParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	ConnectMQTTParamsSetup(&connectParams, AWS_IOT_MQTT_CLIENT_ID, (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID));
	setTLSRxBufferForConnack(&connectParams, 0, 0);
	RxBuffer.pBuffer[1] = (char) (0x00);
	rc = aws_iot_mqtt_connect(&iotClient, &connectParams);
	CHECK_EQUAL_C_INT(MQTT_DECODE_REMAINING_LENGTH_ERROR, rc);

	IOT_DEBUG("-->Success - B:17 - Connect, connack malformed, invalid remaining length \n");
}

/* B:18 - Connect, connack returned error, unacceptable protocol version */
TEST_C(ConnectTests, UnacceptableProtocolVersion) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Connect Tests - B:18 - Connect, connack returned error, unacceptable protocol version \n");

	InitMQTTParamsSetup(&initParams, AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT, false, NULL);
	rc = aws_iot_mqtt_init(&iotClient, &initParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	ConnectMQTTParamsSetup(&connectParams, AWS_IOT_MQTT_CLIENT_ID, (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID));
	connectParams.MQTTVersion = 7;
	setTLSRxBufferForConnack(&connectParams, 0, 1);
	rc = aws_iot_mqtt_connect(&iotClient, &connectParams);
	CHECK_EQUAL_C_INT(MQTT_CONNACK_UNACCEPTABLE_PROTOCOL_VERSION_ERROR, rc);

	IOT_DEBUG("-->Success - B:18 - Connect, connack returned error, unacceptable protocol version \n");
}

/* B:19 - Connect, connack returned error, identifier rejected */
TEST_C(ConnectTests, IndentifierRejected) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Connect Tests - B:19 - Connect, connack returned error, identifier rejected \n");

	InitMQTTParamsSetup(&initParams, AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT, false, NULL);
	rc = aws_iot_mqtt_init(&iotClient, &initParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	ConnectMQTTParamsSetup(&connectParams, AWS_IOT_MQTT_CLIENT_ID, (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID));
	setTLSRxBufferForConnack(&connectParams, 0, 2);
	rc = aws_iot_mqtt_connect(&iotClient, &connectParams);
	CHECK_EQUAL_C_INT(MQTT_CONNACK_IDENTIFIER_REJECTED_ERROR, rc);

	IOT_DEBUG("-->Success - B:19 - Connect, connack returned error, identifier rejected \n");
}

/* B:20 - Connect, connack returned error, Server unavailable */
TEST_C(ConnectTests, ServerUnavailable) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Connect Tests - B:20 - Connect, connack returned error, Server unavailable \n");

	InitMQTTParamsSetup(&initParams, AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT, false, NULL);
	rc = aws_iot_mqtt_init(&iotClient, &initParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	ConnectMQTTParamsSetup(&connectParams, AWS_IOT_MQTT_CLIENT_ID, (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID));
	setTLSRxBufferForConnack(&connectParams, 0, 3);
	rc = aws_iot_mqtt_connect(&iotClient, &connectParams);
	CHECK_EQUAL_C_INT(MQTT_CONNACK_SERVER_UNAVAILABLE_ERROR, rc);

	IOT_DEBUG("-->Success - B:20 - Connect, connack returned error, Server unavailable \n");
}

/* B:21 - Connect, connack returned error, bad user name or password */
TEST_C(ConnectTests, BadUserNameOrPassword) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Connect Tests - B:21 - Connect, connack returned error, bad user name or password \n");

	InitMQTTParamsSetup(&initParams, AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT, false, NULL);
	rc = aws_iot_mqtt_init(&iotClient, &initParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	ConnectMQTTParamsSetup(&connectParams, AWS_IOT_MQTT_CLIENT_ID, (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID));
	setTLSRxBufferForConnack(&connectParams, 0, 4);
	rc = aws_iot_mqtt_connect(&iotClient, &connectParams);
	CHECK_EQUAL_C_INT(MQTT_CONNACK_BAD_USERDATA_ERROR, rc);

	IOT_DEBUG("-->Success - B:21 - Connect, connack returned error, bad user name or password \n");
}

/* B:22 - Connect, connack returned error, not authorized */
TEST_C(ConnectTests, NotAuthorized) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("\n-->Running Connect Tests - B:22 - Connect, connack returned error, not authorized \n");

	InitMQTTParamsSetup(&initParams, AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT, false, NULL);
	rc = aws_iot_mqtt_init(&iotClient, &initParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	ConnectMQTTParamsSetup(&connectParams, AWS_IOT_MQTT_CLIENT_ID, (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID));
	setTLSRxBufferForConnack(&connectParams, 0, 5);
	rc = aws_iot_mqtt_connect(&iotClient, &connectParams);
	CHECK_EQUAL_C_INT(MQTT_CONNACK_NOT_AUTHORIZED_ERROR, rc);

	IOT_DEBUG("\n-->Success - B:22 - Connect, connack returned error, not authorized \n");
}

/* B:23 - Connect, connack return after half command timeout delay, success */
TEST_C(ConnectTests, SuccessAfterDelayedConnack) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Connect Tests - B:23 - Connect, connack return after half command timeout delay, success \n");

	InitMQTTParamsSetup(&initParams, AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT, false, NULL);
	rc = aws_iot_mqtt_init(&iotClient, &initParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	ConnectMQTTParamsSetup(&connectParams, AWS_IOT_MQTT_CLIENT_ID, (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID));
	setTLSRxBufferForConnack(&connectParams, 0, 0);
	setTLSRxBufferDelay(0, (int) initParams.mqttCommandTimeout_ms/2);
	rc = aws_iot_mqtt_connect(&iotClient, &connectParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	IOT_DEBUG("-->Success - B:23 - Connect, connack return after half command timeout delay, success \n");
}

/* B:24 - Connect, connack returned success */
TEST_C(ConnectTests, ConnectSuccess) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Connect Tests - B:24 - Connect, connack returned success \n");

	InitMQTTParamsSetup(&initParams, AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT, false, NULL);
	rc = aws_iot_mqtt_init(&iotClient, &initParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	ConnectMQTTParamsSetup(&connectParams, AWS_IOT_MQTT_CLIENT_ID, (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID));
	setTLSRxBufferForConnack(&connectParams, 0, 0);
	rc = aws_iot_mqtt_connect(&iotClient, &connectParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	IOT_DEBUG("-->Success - B:24 - Connect, connack returned success \n");
}

/* B:25 - Connect, flag settings and parameters are recorded in buffer */
TEST_C(ConnectTests, FlagSettingsAndParamsAreRecordedIntoBuf) {
	IoT_Error_t rc = SUCCESS;
	unsigned char *currPayload = NULL;

	IOT_DEBUG("-->Running Connect Tests - B:25 - Connect, flag settings and parameters are recorded in buffer \n");

	InitMQTTParamsSetup(&initParams, AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT, false, NULL);
	rc = aws_iot_mqtt_init(&iotClient, &initParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	ResetTLSBuffer();
	ConnectMQTTParamsSetup_Detailed(&connectParams, AWS_IOT_MQTT_CLIENT_ID, (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID),
									QOS1, false, true, "willTopicName", (uint16_t) strlen("willTopicName"), "willMsg",
									(uint16_t) strlen("willMsg"), NULL, 0, NULL, 0);
	connectParams.keepAliveIntervalInSec = (1 << 16) - 1;
	setTLSRxBufferForConnack(&connectParams, 0, 0);

	rc = aws_iot_mqtt_connect(&iotClient, &connectParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	currPayload = connectTxBufferHeaderParser(&prfrdParams, TxBuffer.pBuffer);
	CHECK_C(true == isConnectTxBufFlagCorrect(&connectParams, &prfrdParams));
	CHECK_C(true == isConnectTxBufPayloadCorrect(&connectParams, currPayload));

	IOT_DEBUG("-->Success - B:25 - Connect, flag settings and parameters are recorded in buffer \n");
}

/* B:26 - Connect attempt, Disconnect, Manually reconnect */
TEST_C(ConnectTests, ConnectDisconnectConnect) {
	IoT_Error_t rc = SUCCESS;

	IOT_DEBUG("-->Running Connect Tests - B:26 - Connect attempt, Disconnect, Manually reconnect \n");

	InitMQTTParamsSetup(&initParams, AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT, false, NULL);
	rc = aws_iot_mqtt_init(&iotClient, &initParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	ConnectMQTTParamsSetup(&connectParams, AWS_IOT_MQTT_CLIENT_ID, (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID));
	setTLSRxBufferForConnack(&connectParams, 0, 0);

	// connect
	rc = aws_iot_mqtt_connect(&iotClient, &connectParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	// check the is_connected call
	unitTestIsMqttConnected = aws_iot_mqtt_is_client_connected(&iotClient);
	CHECK_EQUAL_C_INT(true, unitTestIsMqttConnected);

	// disconnect
	rc = aws_iot_mqtt_disconnect(&iotClient);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	// check the is_connected call
	unitTestIsMqttConnected = aws_iot_mqtt_is_client_connected(&iotClient);
	CHECK_EQUAL_C_INT(false, unitTestIsMqttConnected);

	ResetTLSBuffer();
	setTLSRxBufferForConnack(&connectParams, 0, 0);

	// connect
	rc = aws_iot_mqtt_connect(&iotClient, &connectParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	// check the is_connected call
	unitTestIsMqttConnected = aws_iot_mqtt_is_client_connected(&iotClient);
	CHECK_EQUAL_C_INT(true, unitTestIsMqttConnected);

	IOT_DEBUG("-->Success - B:26 - Connect attempt, Disconnect, Manually reconnect \n");
}

/* B:27 - Connect attempt, Clean session, Subscribe
 * connect with clean session true and subscribe to a topic1, set msg to topic1 and ensure it is received
 * connect cs false, set msg to topic1 and nothing should come in, Sub to topic2 and check if msg is received
 * connect cs false and send msg to topic2 and should be received
 * cs true and everything should be clean again
 */
TEST_C(ConnectTests, cleanSessionInitSubscribers) {
	IoT_Error_t rc = SUCCESS;
	char expectedCallbackString[] = "msg topic";

	IOT_DEBUG("-->Running Connect Tests - B:27 - Connect attempt, Clean session, Subscribe \n");

	InitMQTTParamsSetup(&initParams, AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT, false, NULL);
	rc = aws_iot_mqtt_init(&iotClient, &initParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	ResetTLSBuffer();

	//1. connect with clean session true and
	ConnectMQTTParamsSetup(&connectParams, AWS_IOT_MQTT_CLIENT_ID, (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID));
	connectParams.isCleanSession = true;
	setTLSRxBufferForConnack(&connectParams, 0, 0);
	rc = aws_iot_mqtt_connect(&iotClient, &connectParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	//1. subscribe to a topic1 and
	testPubMsgParams.payload = expectedCallbackString;
	testPubMsgParams.payloadLen = (uint16_t) strlen(expectedCallbackString);
	setTLSRxBufferForSuback(subTopic1, strlen(subTopic1), QOS0, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, subTopic1, (uint16_t) strlen(subTopic1), QOS0, iot_subscribe_callback_handler, NULL);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	ResetTLSBuffer();
	//1. receive message
	setTLSRxBufferWithMsgOnSubscribedTopic(subTopic1, strlen(subTopic1), QOS0, testPubMsgParams,
										   expectedCallbackString);
	rc = aws_iot_mqtt_yield(&iotClient, 1000);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_STRING(expectedCallbackString, CallbackMsgStringclean);

	ResetTLSBuffer();
	rc = aws_iot_mqtt_disconnect(&iotClient);

	//2. connect cs false and
	ConnectMQTTParamsSetup(&connectParams, AWS_IOT_MQTT_CLIENT_ID, (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID));
	connectParams.isCleanSession = false;
	setTLSRxBufferForConnack(&connectParams, 0, 0);
	rc = aws_iot_mqtt_connect(&iotClient, &connectParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	ResetTLSBuffer();
	//3. set msg to topic1 and should receive the topic1 message
	snprintf(CallbackMsgStringclean, 100, NO_MSG_XXXX);
	setTLSRxBufferWithMsgOnSubscribedTopic(subTopic1, strlen(subTopic1), QOS0, testPubMsgParams,
										   expectedCallbackString);
	rc = aws_iot_mqtt_yield(&iotClient, 1000);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_STRING(expectedCallbackString, CallbackMsgStringclean);

	ResetTLSBuffer();
	//4. ,sub to topic2
	snprintf(CallbackMsgStringclean, 100, NO_MSG_XXXX);
	setTLSRxBufferForSuback(subTopic1, strlen(subTopic1), QOS0, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, subTopic2, (uint16_t) strlen(subTopic2), QOS0, iot_subscribe_callback_handler, NULL);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	ResetTLSBuffer();
	//5. and check if topic 2 msg is received
	setTLSRxBufferWithMsgOnSubscribedTopic(subTopic2, strlen(subTopic2), QOS0, testPubMsgParams,
										   expectedCallbackString);
	rc = aws_iot_mqtt_yield(&iotClient, 1000);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_STRING(expectedCallbackString, CallbackMsgStringclean);

	rc = aws_iot_mqtt_disconnect(&iotClient);

	//6. connect cs false and
	ConnectMQTTParamsSetup(&connectParams, AWS_IOT_MQTT_CLIENT_ID, (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID));
	connectParams.isCleanSession = false;
	setTLSRxBufferForConnack(&connectParams, 0, 0);
	rc = aws_iot_mqtt_connect(&iotClient, &connectParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	//7. set msg to topic2 and
	snprintf(CallbackMsgStringclean, 100, NO_MSG_XXXX);
	setTLSRxBufferWithMsgOnSubscribedTopic(subTopic2, strlen(subTopic2), QOS0, testPubMsgParams,
										   expectedCallbackString);
	//8. should be received
	rc = aws_iot_mqtt_yield(&iotClient, 1000);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_STRING(expectedCallbackString, CallbackMsgStringclean);

	IOT_DEBUG("-->Success - B:27 - Connect attempt, Clean session, Subscribe \n");

}

/* B:28 - Connect attempt, power cycle with clean session false
 * This test is to ensure we can initialize the subscribe table in mqtt even when connecting with CS = false
 * currently the AWS_IOT_MQTT_NUM_SUBSCRIBE_HANDLERS is set to 5
 */
TEST_C(ConnectTests, PowerCycleWithCleanSessionFalse) {
	IoT_Error_t rc = SUCCESS;
	int itr = 0;
	char subTestTopic[12];
	uint16_t subTestTopicLen = 0;

	IOT_DEBUG("-->Running Connect Tests - B:28 - Connect attempt, power cycle with clean session false \n");

	InitMQTTParamsSetup(&initParams, AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT, false, NULL);
	rc = aws_iot_mqtt_init(&iotClient, &initParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	ResetTLSBuffer();

	//1. connect with clean session false and
	ConnectMQTTParamsSetup(&connectParams, AWS_IOT_MQTT_CLIENT_ID, (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID));
	connectParams.isCleanSession = true;
	setTLSRxBufferForConnack(&connectParams, 0, 0);
	rc = aws_iot_mqtt_connect(&iotClient, &connectParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	//2. subscribe to max number of topics
	for(itr = 0; itr < AWS_IOT_MQTT_NUM_SUBSCRIBE_HANDLERS; itr++) {
		snprintf(subTestTopic, 12, "sdk/topic%d", itr + 1);
		subTestTopicLen = (uint16_t) strlen(subTestTopic);
		setTLSRxBufferForSuback(subTestTopic, subTestTopicLen, QOS0, testPubMsgParams);
		rc = aws_iot_mqtt_subscribe(&iotClient, subTestTopic, subTestTopicLen, QOS0, iot_subscribe_callback_handler,
									NULL);
		CHECK_EQUAL_C_INT(SUCCESS, rc);
	}

	//3. Subscribe to one more topic. Should return error
	snprintf(subTestTopic, 12, "sdk/topic%d", itr + 1);
	subTestTopicLen = (uint16_t) strlen(subTestTopic);
	setTLSRxBufferForSuback(subTestTopic, subTestTopicLen, QOS0, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, subTestTopic, subTestTopicLen, QOS0, iot_subscribe_callback_handler,
								NULL);
	CHECK_EQUAL_C_INT(MQTT_MAX_SUBSCRIPTIONS_REACHED_ERROR, rc);

	IOT_DEBUG("-->Success - B:28 - Connect attempt, power cycle with clean session false \n");
}

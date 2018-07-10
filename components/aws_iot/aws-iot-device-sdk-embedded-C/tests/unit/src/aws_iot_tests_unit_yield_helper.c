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
 * @file aws_iot_tests_unit_yield_helper.c
 * @brief IoT Client Unit Testing - Yield API Tests Helper
 */

#include <stdio.h>
#include <unistd.h>
#include <CppUTest/TestHarness_c.h>

#include "aws_iot_tests_unit_helper_functions.h"
#include "aws_iot_tests_unit_mock_tls_params.h"
#include "aws_iot_log.h"

static IoT_Client_Init_Params initParams;
static IoT_Client_Connect_Params connectParams;
static AWS_IoT_Client iotClient;
static IoT_Publish_Message_Params testPubMsgParams;

static ConnectBufferProofread prfrdParams;
static char CallbackMsgString[100];
static char subTopic[10] = "sdk/Test";
static uint16_t subTopicLen = 8;

static bool dcHandlerInvoked = false;

static void iot_tests_unit_acr_subscribe_callback_handler(AWS_IoT_Client *pClient, char *topicName,
														  uint16_t topicNameLen,
														  IoT_Publish_Message_Params *params, void *pData) {
	char *tmp = params->payload;
	unsigned int i;

	IOT_UNUSED(pClient);
	IOT_UNUSED(topicName);
	IOT_UNUSED(topicNameLen);
	IOT_UNUSED(pData);

	for(i = 0; i < params->payloadLen; i++) {
		CallbackMsgString[i] = tmp[i];
	}
}

static void iot_tests_unit_yield_test_subscribe_callback_handler(AWS_IoT_Client *pClient, char *topicName,
																 uint16_t topicNameLen,
																 IoT_Publish_Message_Params *params, void *pData) {
	char *tmp = params->payload;
	unsigned int i;
	IoT_Error_t rc = SUCCESS;

	IOT_UNUSED(pClient);
	IOT_UNUSED(topicName);
	IOT_UNUSED(topicNameLen);
	IOT_UNUSED(pData);

	rc = aws_iot_mqtt_yield(pClient, 1000);
	CHECK_EQUAL_C_INT(MQTT_CLIENT_NOT_IDLE_ERROR, rc);

	for(i = 0; i < params->payloadLen; i++) {
		CallbackMsgString[i] = tmp[i];
	}
}

void iot_tests_unit_disconnect_handler(AWS_IoT_Client *pClient, void *disconParam) {
	IOT_UNUSED(pClient);
	IOT_UNUSED(disconParam);
	dcHandlerInvoked = true;
}


TEST_GROUP_C_SETUP(YieldTests) {
	IoT_Error_t rc = SUCCESS;
	dcHandlerInvoked = false;

	InitMQTTParamsSetup(&initParams, AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT, false, iot_tests_unit_disconnect_handler);
	rc = aws_iot_mqtt_init(&iotClient, &initParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);


	ConnectMQTTParamsSetup_Detailed(&connectParams, AWS_IOT_MQTT_CLIENT_ID, (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID),
									QOS1, false, true, "willTopicName", (uint16_t) strlen("willTopicName"), "willMsg",
									(uint16_t) strlen("willMsg"), NULL, 0, NULL, 0);
	connectParams.keepAliveIntervalInSec = 5;
	setTLSRxBufferForConnack(&connectParams, 0, 0);
	rc = aws_iot_mqtt_connect(&iotClient, &connectParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	rc = aws_iot_mqtt_autoreconnect_set_status(&iotClient, false);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	ResetTLSBuffer();
}

TEST_GROUP_C_TEARDOWN(YieldTests) {
	/* Clean up. Not checking return code here because this is common to all tests.
	 * A test might have already caused a disconnect by this point.
	 */
	IoT_Error_t rc = aws_iot_mqtt_disconnect(&iotClient);
	IOT_UNUSED(rc);
}

/* G:1 - Yield with Null/empty Client Instance */
TEST_C(YieldTests, NullClientYield) {
	IoT_Error_t rc = aws_iot_mqtt_yield(NULL, 1000);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, rc);
}

/* G:2 - Yield with zero yield timeout */

TEST_C(YieldTests, ZeroTimeoutYield) {
	IoT_Error_t rc = aws_iot_mqtt_yield(&iotClient, 0);
	CHECK_EQUAL_C_INT(NULL_VALUE_ERROR, rc);
}
/* G:3 - Yield, network disconnected, never connected */

TEST_C(YieldTests, YieldNetworkDisconnectedNeverConnected) {
	AWS_IoT_Client tempIotClient;
	IoT_Error_t rc;

	InitMQTTParamsSetup(&initParams, AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT, false, iot_tests_unit_disconnect_handler);
	rc = aws_iot_mqtt_init(&tempIotClient, &initParams);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	rc = aws_iot_mqtt_yield(&tempIotClient, 1000);
	CHECK_EQUAL_C_INT(NETWORK_DISCONNECTED_ERROR, rc);
}

/* G:4 - Yield, network disconnected, disconnected manually */
TEST_C(YieldTests, YieldNetworkDisconnectedDisconnectedManually) {
	IoT_Error_t rc = aws_iot_mqtt_disconnect(&iotClient);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	rc = aws_iot_mqtt_yield(&iotClient, 1000);
	CHECK_EQUAL_C_INT(NETWORK_MANUALLY_DISCONNECTED, rc);
}

/* G:5 - Yield, network connected, yield called while in subscribe application callback */
TEST_C(YieldTests, YieldInSubscribeCallback) {
	IoT_Error_t rc = SUCCESS;
	char expectedCallbackString[] = "0xA5A5A3";

	IOT_DEBUG("-->Running Yield Tests - G:5 - Yield, network connected, yield called while in subscribe application callback \n");

	setTLSRxBufferForSuback(subTopic, subTopicLen, QOS1, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, subTopic, subTopicLen, QOS1,
								iot_tests_unit_yield_test_subscribe_callback_handler, NULL);
	if(SUCCESS == rc) {
		setTLSRxBufferWithMsgOnSubscribedTopic(subTopic, subTopicLen, QOS1, testPubMsgParams, expectedCallbackString);
		rc = aws_iot_mqtt_yield(&iotClient, 1000);
		if(SUCCESS == rc) {
			CHECK_EQUAL_C_STRING(expectedCallbackString, CallbackMsgString);
		}
		CHECK_EQUAL_C_INT(1, isLastTLSTxMessagePuback());
	}

	IOT_DEBUG("-->Success - G:5 - Yield, network connected, yield called while in subscribe application callback \n");
}

/* G:6 - Yield, network disconnected, ping timeout, auto-reconnect disabled */
TEST_C(YieldTests, disconnectNoAutoReconnect) {
	IoT_Error_t rc = FAILURE;

	IOT_DEBUG("-->Running Yield Tests - G:6 - Yield, network disconnected, ping timeout, auto-reconnect disabled \n");

	rc = aws_iot_mqtt_autoreconnect_set_status(&iotClient, true);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	CHECK_EQUAL_C_INT(true, aws_iot_mqtt_is_client_connected(&iotClient));
	CHECK_EQUAL_C_INT(true, aws_iot_is_autoreconnect_enabled(&iotClient));

	/* Disable Autoreconnect, then let ping request time out and call yield */
	aws_iot_mqtt_autoreconnect_set_status(&iotClient, false);
	sleep((uint16_t)(iotClient.clientData.keepAliveInterval));

	ResetTLSBuffer();

	/* Sleep for keep alive interval to allow the first ping to be sent out */
	sleep(iotClient.clientData.keepAliveInterval);
	rc = aws_iot_mqtt_yield(&iotClient, 100);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_INT(true, isLastTLSTxMessagePingreq());

	/* Let ping request time out and call yield */
	sleep(iotClient.clientData.keepAliveInterval + 1);
	rc = aws_iot_mqtt_yield(&iotClient, 100);
	CHECK_EQUAL_C_INT(NETWORK_DISCONNECTED_ERROR, rc);
	CHECK_EQUAL_C_INT(1, isLastTLSTxMessageDisconnect());
	CHECK_EQUAL_C_INT(0, aws_iot_mqtt_is_client_connected(&iotClient));
	CHECK_EQUAL_C_INT(true, dcHandlerInvoked);

	IOT_DEBUG("-->Success - G:6 - Yield, network disconnected, ping timeout, auto-reconnect disabled \n");
}

/* G:7 - Yield, network connected, no incoming messages */
TEST_C(YieldTests, YieldSuccessNoMessages) {
	IoT_Error_t rc;
	int i;

	IOT_DEBUG("-->Running Yield Tests - G:7 - Yield, network connected, no incoming messages \n");

	for(i = 0; i < 100; i++) {
		CallbackMsgString[i] = 'x';
	}

	rc = aws_iot_mqtt_yield(&iotClient, 1000);
	if(SUCCESS == rc) {
		/* Check no messages were received */
		for(i = 0; i < 100; i++) {
			if('x' != CallbackMsgString[i]) {
				rc = FAILURE;
			}
		}
	}

	CHECK_EQUAL_C_INT(SUCCESS, rc);

	IOT_DEBUG("-->Success - G:7 - Yield, network connected, no incoming messages \n");
}

/* G:8 - Yield, network connected, ping request/response */
TEST_C(YieldTests, PingRequestPingResponse) {
	IoT_Error_t rc = SUCCESS;
	int i = 0;
	int j = 0;
	int attempt = 3;

	IOT_DEBUG("-->Running Yield Tests - G:8 - Yield, network connected, ping request/response \n");
	IOT_DEBUG("Current Keep Alive Interval is set to %d sec.\n", iotClient.clientData.keepAliveInterval);

	for(i = 0; i < attempt; i++) {
		IOT_DEBUG("[Round_%d/Total_%d] Waiting for %d sec...\n", i + 1, attempt, iotClient.clientData.keepAliveInterval);
		/* Set TLS buffer for ping response */
		ResetTLSBuffer();
		setTLSRxBufferForPingresp();

		for(j = 0; j <= iotClient.clientData.keepAliveInterval; j++) {
			sleep(1);
			IOT_DEBUG("[Waited %d secs]", j + 1);
			rc = aws_iot_mqtt_yield(&iotClient, 100);
			CHECK_EQUAL_C_INT(SUCCESS, rc);
		}

		/* Check whether ping was processed correctly and new Ping request was generated */
		CHECK_EQUAL_C_INT(1, isLastTLSTxMessagePingreq());
	}

	IOT_DEBUG("-->Success - G:8 - Yield, network connected, ping request/response \n");
}

/* G:9 - Yield, disconnected, Auto-reconnect timed-out */
TEST_C(YieldTests, disconnectAutoReconnectTimeout) {
	IoT_Error_t rc = FAILURE;

	IOT_DEBUG("-->Running Yield Tests - G:9 - Yield, disconnected, Auto-reconnect timed-out \n");

	rc = aws_iot_mqtt_autoreconnect_set_status(&iotClient, true);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	CHECK_EQUAL_C_INT(true, aws_iot_mqtt_is_client_connected(&iotClient));
	CHECK_EQUAL_C_INT(true, aws_iot_is_autoreconnect_enabled(&iotClient));

	ResetTLSBuffer();

	/* Sleep for keep alive interval to allow the first ping to be sent out */
	sleep(iotClient.clientData.keepAliveInterval);
	rc = aws_iot_mqtt_yield(&iotClient, 100);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_INT(true, isLastTLSTxMessagePingreq());

	/* Let ping request time out and call yield */
	sleep(iotClient.clientData.keepAliveInterval + 1);
	rc = aws_iot_mqtt_yield(&iotClient, 100);
	CHECK_EQUAL_C_INT(NETWORK_ATTEMPTING_RECONNECT, rc);
	CHECK_EQUAL_C_INT(0, aws_iot_mqtt_is_client_connected(&iotClient));
	CHECK_EQUAL_C_INT(true, dcHandlerInvoked);

	IOT_DEBUG("-->Success - G:9 - Yield, disconnected, Auto-reconnect timed-out \n");
}

/* G:10 - Yield, disconnected, Auto-reconnect successful */
TEST_C(YieldTests, disconnectAutoReconnectSuccess) {
	IoT_Error_t rc = FAILURE;
	unsigned char *currPayload = NULL;

	IOT_DEBUG("-->Running Yield Tests - G:10 - Yield, disconnected, Auto-reconnect successful \n");

	rc = aws_iot_mqtt_autoreconnect_set_status(&iotClient, true);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	CHECK_EQUAL_C_INT(true, aws_iot_mqtt_is_client_connected(&iotClient));
	CHECK_EQUAL_C_INT(true, aws_iot_is_autoreconnect_enabled(&iotClient));

	/* Sleep for keep alive interval to allow the first ping to be sent out */
	sleep(iotClient.clientData.keepAliveInterval);
	rc = aws_iot_mqtt_yield(&iotClient, 100);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_INT(true, isLastTLSTxMessagePingreq());

	/* Let ping request time out and call yield */
	sleep(iotClient.clientData.keepAliveInterval + 1);
	rc = aws_iot_mqtt_yield(&iotClient, 100);
	CHECK_EQUAL_C_INT(NETWORK_ATTEMPTING_RECONNECT, rc);

	sleep(2); /* Default min reconnect delay is 1 sec */
	printf("\nWakeup");
	setTLSRxBufferForConnack(&connectParams, 0, 0);
	rc = aws_iot_mqtt_yield(&iotClient, 100);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	currPayload = connectTxBufferHeaderParser(&prfrdParams, TxBuffer.pBuffer);
	CHECK_C(true == isConnectTxBufFlagCorrect(&connectParams, &prfrdParams));
	CHECK_C(true == isConnectTxBufPayloadCorrect(&connectParams, currPayload));
	CHECK_EQUAL_C_INT(true, aws_iot_mqtt_is_client_connected(&iotClient));
	CHECK_EQUAL_C_INT(true, dcHandlerInvoked);

	IOT_DEBUG("-->Success - G:10 - Yield, disconnected, Auto-reconnect successful \n");
}

/* G:11 - Yield, disconnected, Manual reconnect */
TEST_C(YieldTests, disconnectManualAutoReconnect) {
	IoT_Error_t rc = FAILURE;
	unsigned char *currPayload = NULL;

	IOT_DEBUG("-->Running Yield Tests - G:11 - Yield, disconnected, Manual reconnect \n");

	CHECK_C(aws_iot_mqtt_is_client_connected(&iotClient));

	/* Disable Autoreconnect, then let ping request time out and call yield */
	aws_iot_mqtt_autoreconnect_set_status(&iotClient, false);
	CHECK_C(!aws_iot_is_autoreconnect_enabled(&iotClient));

	/* Sleep for keep alive interval to allow the first ping to be sent out */
	sleep(iotClient.clientData.keepAliveInterval);
	rc = aws_iot_mqtt_yield(&iotClient, 100);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_INT(true, isLastTLSTxMessagePingreq());

	/* Let ping request time out and call yield */
	sleep(iotClient.clientData.keepAliveInterval + 1);
	rc = aws_iot_mqtt_yield(&iotClient, 100);
	CHECK_EQUAL_C_INT(NETWORK_DISCONNECTED_ERROR, rc);
	CHECK_EQUAL_C_INT(1, isLastTLSTxMessageDisconnect());
	CHECK_C(!aws_iot_mqtt_is_client_connected(&iotClient));
	CHECK_EQUAL_C_INT(true, dcHandlerInvoked);

	dcHandlerInvoked = false;
	setTLSRxBufferForConnack(&connectParams, 0, 0);
	rc = aws_iot_mqtt_attempt_reconnect(&iotClient);
	CHECK_EQUAL_C_INT(NETWORK_RECONNECTED, rc);

	currPayload = connectTxBufferHeaderParser(&prfrdParams, TxBuffer.pBuffer);
	CHECK_C(true == isConnectTxBufFlagCorrect(&connectParams, &prfrdParams));
	CHECK_C(true == isConnectTxBufPayloadCorrect(&connectParams, currPayload));
	CHECK_C(aws_iot_mqtt_is_client_connected(&iotClient));
	CHECK_EQUAL_C_INT(false, dcHandlerInvoked);

	IOT_DEBUG("-->Success - G:11 - Yield, disconnected, Manual reconnect \n");
}

/* G:12 - Yield, resubscribe to all topics on reconnect */
TEST_C(YieldTests, resubscribeSuccessfulReconnect) {
	IoT_Error_t rc = FAILURE;
	char cPayload[100];
	bool connected = false;
	bool autoReconnectEnabled = false;
	char expectedCallbackString[100];

	IOT_DEBUG("-->Running Yield Tests - G:12 - Yield, resubscribe to all topics on reconnect \n");

	rc = aws_iot_mqtt_autoreconnect_set_status(&iotClient, true);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	snprintf(CallbackMsgString, 100, "NOT_VISITED");

	testPubMsgParams.qos = QOS1;
	testPubMsgParams.isRetained = 0;
	snprintf(cPayload, 100, "%s : %d ", "hello from SDK", 0);
	testPubMsgParams.payload = (void *) cPayload;
	testPubMsgParams.payloadLen = strlen(cPayload);

	connected = aws_iot_mqtt_is_client_connected(&iotClient);
	CHECK_EQUAL_C_INT(1, connected);

	ResetTLSBuffer();

	/* Subscribe to a topic */
	setTLSRxBufferForSuback(subTopic, subTopicLen, QOS1, testPubMsgParams);
	rc = aws_iot_mqtt_subscribe(&iotClient, subTopic, subTopicLen, QOS0, iot_tests_unit_acr_subscribe_callback_handler,
								NULL);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	ResetTLSBuffer();

	/* Check subscribe */
	snprintf(expectedCallbackString, 100, "Message for %s", subTopic);
	setTLSRxBufferWithMsgOnSubscribedTopic(subTopic, subTopicLen, QOS1, testPubMsgParams, expectedCallbackString);
	rc = aws_iot_mqtt_yield(&iotClient, 100);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_STRING(expectedCallbackString, CallbackMsgString);

	ResetTLSBuffer();

	autoReconnectEnabled = aws_iot_is_autoreconnect_enabled(&iotClient);
	CHECK_EQUAL_C_INT(1, autoReconnectEnabled);

	/* Sleep for keep alive interval to allow the first ping to be sent out */
	sleep(iotClient.clientData.keepAliveInterval);
	rc = aws_iot_mqtt_yield(&iotClient, 100);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_INT(true, isLastTLSTxMessagePingreq());

	/* Let ping request time out and call yield */
	sleep(iotClient.clientData.keepAliveInterval + 1);
	rc = aws_iot_mqtt_yield(&iotClient, 100);
	CHECK_EQUAL_C_INT(NETWORK_ATTEMPTING_RECONNECT, rc);

	sleep(2); /* Default min reconnect delay is 1 sec */

	ResetTLSBuffer();
	setTLSRxBufferForConnackAndSuback(&connectParams, 0, subTopic, subTopicLen, QOS1);

	rc = aws_iot_mqtt_yield(&iotClient, 100);
	CHECK_EQUAL_C_INT(SUCCESS, rc);

	/* Test if reconnect worked */
	connected = aws_iot_mqtt_is_client_connected(&iotClient);
	CHECK_EQUAL_C_INT(true, connected);

	ResetTLSBuffer();

	/* Check subscribe */
	snprintf(expectedCallbackString, 100, "Message for %s after resub", subTopic);
	setTLSRxBufferWithMsgOnSubscribedTopic(subTopic, subTopicLen, QOS1, testPubMsgParams, expectedCallbackString);
	rc = aws_iot_mqtt_yield(&iotClient, 100);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_STRING(expectedCallbackString, CallbackMsgString);
	CHECK_EQUAL_C_INT(true, dcHandlerInvoked);

	IOT_DEBUG("-->Success - G:12 - Yield, resubscribe to all topics on reconnect \n");
}

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
 * @file aws_iot_test_auto_reconnect.c
 * @brief Integration Test for automatic reconnect
 */

#include "aws_iot_test_integration_common.h"

static char ModifiedPathBuffer[PATH_MAX + 1];
char root_CA[PATH_MAX + 1];

bool terminate_yield_with_rc_thread = false;
IoT_Error_t yieldRC;
bool captureYieldReturnCode = false;

/**
 * This function renames the rootCA.crt file to a temporary name to cause connect failure
 */
int aws_iot_mqtt_tests_block_tls_connect() {
	char replaceFileName[] = {"rootCATemp.crt"};
	char *pFileNamePosition = NULL;

	char mvCommand[2 * PATH_MAX + 10];
	strcpy(ModifiedPathBuffer, root_CA);
	pFileNamePosition = strstr(ModifiedPathBuffer, AWS_IOT_ROOT_CA_FILENAME);
	strncpy(pFileNamePosition, replaceFileName, strlen(replaceFileName));
	snprintf(mvCommand, 2 * PATH_MAX + 10, "mv %s %s", root_CA, ModifiedPathBuffer);
	return system(mvCommand);
}

/**
 * Always ensure this function is called after block_tls_connect
 */
int aws_iot_mqtt_tests_unblock_tls_connect() {
	char mvCommand[2 * PATH_MAX + 10];
	snprintf(mvCommand, 2 * PATH_MAX + 10, "mv %s %s", ModifiedPathBuffer, root_CA);
	return system(mvCommand);
}

void *aws_iot_mqtt_tests_yield_with_rc(void *ptr) {
	IoT_Error_t rc = SUCCESS;

	struct timeval start, end, result;
	static int cntr = 0;
	AWS_IoT_Client *pClient = (AWS_IoT_Client *) ptr;

	while(terminate_yield_with_rc_thread == false
		  && (NETWORK_ATTEMPTING_RECONNECT == rc || NETWORK_RECONNECTED == rc || SUCCESS == rc)) {
		usleep(500000);
		printf(" Client state : %d ", aws_iot_mqtt_get_client_state(pClient));
		rc = aws_iot_mqtt_yield(pClient, 100);
		printf("yield rc %d\n", rc);
		if(captureYieldReturnCode && SUCCESS != rc) {
			printf("yield rc capture %d\n", rc);
			captureYieldReturnCode = false;
			yieldRC = rc;
		}
	}
}

unsigned int disconnectedCounter = 0;

void aws_iot_mqtt_tests_disconnect_callback_handler(AWS_IoT_Client *pClient, void *param) {
	disconnectedCounter++;
}

int aws_iot_mqtt_tests_auto_reconnect() {
	pthread_t reconnectTester_thread, yield_thread;
	int yieldThreadReturn = 0;
	int test_result = 0;

	char certDirectory[15] = "../../certs";
	char CurrentWD[PATH_MAX + 1];
	char clientCRT[PATH_MAX + 1];
	char clientKey[PATH_MAX + 1];
	char clientId[50];
	AWS_IoT_Client client;

	IoT_Error_t rc = SUCCESS;
	getcwd(CurrentWD, sizeof(CurrentWD));
	snprintf(root_CA, PATH_MAX + 1, "%s/%s/%s", CurrentWD, certDirectory, AWS_IOT_ROOT_CA_FILENAME);
	snprintf(clientCRT, PATH_MAX + 1, "%s/%s/%s", CurrentWD, certDirectory, AWS_IOT_CERTIFICATE_FILENAME);
	snprintf(clientKey, PATH_MAX + 1, "%s/%s/%s", CurrentWD, certDirectory, AWS_IOT_PRIVATE_KEY_FILENAME);
	srand((unsigned int) time(NULL));
	snprintf(clientId, 50, "%s_%d", INTEGRATION_TEST_CLIENT_ID, rand() % 10000);

	printf(" Root CA Path : %s\n clientCRT : %s\n clientKey : %s\n", root_CA, clientCRT, clientKey);
	IoT_Client_Init_Params initParams;
	initParams.pHostURL = AWS_IOT_MQTT_HOST;
	initParams.port = 8883;
	initParams.pRootCALocation = root_CA;
	initParams.pDeviceCertLocation = clientCRT;
	initParams.pDevicePrivateKeyLocation = clientKey;
	initParams.mqttCommandTimeout_ms = 20000;
	initParams.tlsHandshakeTimeout_ms = 5000;
	initParams.disconnectHandler = aws_iot_mqtt_tests_disconnect_callback_handler;
	initParams.enableAutoReconnect = false;
	aws_iot_mqtt_init(&client, &initParams);

	IoT_Client_Connect_Params connectParams;
	connectParams.keepAliveIntervalInSec = 5;
	connectParams.isCleanSession = true;
	connectParams.MQTTVersion = MQTT_3_1_1;
	connectParams.pClientID = (char *) &clientId;
	connectParams.clientIDLen = strlen(clientId);
	connectParams.isWillMsgPresent = 0;
	connectParams.pUsername = NULL;
	connectParams.usernameLen = 0;
	connectParams.pPassword = NULL;
	connectParams.passwordLen = 0;

	rc = aws_iot_mqtt_connect(&client, &connectParams);
	if(rc != SUCCESS) {
		printf("ERROR Connecting %d\n", rc);
		return -1;
	}

	yieldThreadReturn = pthread_create(&yield_thread, NULL, aws_iot_mqtt_tests_yield_with_rc, &client);

	/*
	 * Test disconnect handler
	 */
	printf("1. Test Disconnect Handler\n");
	aws_iot_mqtt_tests_block_tls_connect();
	iot_tls_disconnect(&(client.networkStack));
	sleep(connectParams.keepAliveIntervalInSec + 1);
	if(disconnectedCounter == 1) {
		printf("Success invoking Disconnect Handler\n");
	} else {
		aws_iot_mqtt_tests_unblock_tls_connect();
		printf("Failure to invoke Disconnect Handler\n");
		return -1;
	}
	aws_iot_mqtt_tests_unblock_tls_connect();
	terminate_yield_with_rc_thread = true;
	pthread_join(yield_thread, NULL);

	/*
	 * Manual Reconnect Test
	 */
	printf("2. Test Manual Reconnect, Current Client state : %d \n", aws_iot_mqtt_get_client_state(&client));
	rc = aws_iot_mqtt_attempt_reconnect(&client);
	if(rc != NETWORK_RECONNECTED) {
		printf("ERROR reconnecting manually %d\n", rc);
		return -4;
	}
	terminate_yield_with_rc_thread = false;
	yieldThreadReturn = pthread_create(&yield_thread, NULL, aws_iot_mqtt_tests_yield_with_rc, &client);

	yieldRC = FAILURE;
	captureYieldReturnCode = true;

	// ensure atleast 1 cycle of yield is executed to get the yield status to SUCCESS
	sleep(1);
	if(!captureYieldReturnCode) {
		if(yieldRC == NETWORK_ATTEMPTING_RECONNECT) {
			printf("Success reconnecting manually\n");
		} else {
			printf("Failure to reconnect manually\n");
			return -3;
		}
	}
	terminate_yield_with_rc_thread = true;

	/*
	 * Auto Reconnect Test
	 */

	printf("3. Test Auto_reconnect \n");

	rc = aws_iot_mqtt_autoreconnect_set_status(&client, true);
	if(rc != SUCCESS) {
		printf("Error: Failed to enable auto-reconnect %d \n", rc);
	}

	yieldRC = FAILURE;
	captureYieldReturnCode = true;

	// Disconnect
	aws_iot_mqtt_tests_block_tls_connect();
	iot_tls_disconnect(&(client.networkStack));

	terminate_yield_with_rc_thread = false;
	yieldThreadReturn = pthread_create(&yield_thread, NULL, aws_iot_mqtt_tests_yield_with_rc, &client);

	sleep(connectParams.keepAliveIntervalInSec + 1);
	if(!captureYieldReturnCode) {
		if(yieldRC == NETWORK_ATTEMPTING_RECONNECT) {
			printf("Success attempting reconnect\n");
		} else {
			printf("Failure to attempt to reconnect\n");
			return -6;
		}
	}
	if(disconnectedCounter == 2) {
		printf("Success: disconnect handler invoked on enabling auto-reconnect\n");
	} else {
		printf("Failure: disconnect handler not invoked on enabling auto-reconnect : %d\n", disconnectedCounter);
		return -7;
	}
	aws_iot_mqtt_tests_unblock_tls_connect();
	sleep(connectParams.keepAliveIntervalInSec + 1);
	captureYieldReturnCode = true;
	sleep(connectParams.keepAliveIntervalInSec + 1);
	if(!captureYieldReturnCode) {
		if(yieldRC == SUCCESS) {
			printf("Success attempting reconnect\n");
		} else {
			printf("Failure to attempt to reconnect\n");
			return -6;
		}
	}
	if(true == aws_iot_mqtt_is_client_connected(&client)) {
		printf("Success: is Mqtt connected api\n");
	} else {
		printf("Failure: is Mqtt Connected api\n");
		return -7;
	}

	rc = aws_iot_mqtt_disconnect(&client);
	return rc;
}

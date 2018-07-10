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
 * @file aws_iot_test_multiple_clients.c
 * @brief Integration Test for multiple clients from the same application
 */

#include "aws_iot_test_integration_common.h"

static bool terminate_yield_thread;
static bool isPubThreadFinished;

static unsigned int countArray[PUBLISH_COUNT];
static unsigned int rxMsgBufferTooBigCounter;
static unsigned int rxUnexpectedNumberCounter;
static unsigned int rePublishCount;

static void aws_iot_mqtt_tests_message_aggregator(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
												 IoT_Publish_Message_Params *params, void *pData) {
	char tempBuf[10];
	unsigned int tempInt = 0;

	IOT_UNUSED(pClient);
	IOT_UNUSED(topicName);
	IOT_UNUSED(topicNameLen);
	IOT_UNUSED(pData);

	if(10 >= params->payloadLen) {
		snprintf(tempBuf, params->payloadLen, params->payload);
		printf("\nMsg received : %s", tempBuf);
		tempInt = atoi(tempBuf);
		if(0 < tempInt && PUBLISH_COUNT >= tempInt) {
			countArray[tempInt - 1]++;
		} else {
			rxUnexpectedNumberCounter++;
		}
	} else {
		rxMsgBufferTooBigCounter++;
	}
}

static void aws_iot_mqtt_tests_disconnect_callback_handler(AWS_IoT_Client *pClient, void *param) {
	IOT_UNUSED(pClient);
	IOT_UNUSED(param);
}

static IoT_Error_t aws_iot_mqtt_tests_connect_client_to_service(AWS_IoT_Client *pClient, struct timeval *pConnectTime,
															   char *clientId, char *rootCA, char *clientCRT,
															   char *clientKey) {
	IoT_Client_Init_Params initParams;
	IoT_Client_Connect_Params connectParams;
	IoT_Error_t rc;
	struct timeval start, end;

	initParams.pHostURL = AWS_IOT_MQTT_HOST;
	initParams.port = 8883;
	initParams.pRootCALocation = rootCA;
	initParams.pDeviceCertLocation = clientCRT;
	initParams.pDevicePrivateKeyLocation = clientKey;
	initParams.mqttCommandTimeout_ms = 5000;
	initParams.tlsHandshakeTimeout_ms = 2000;
	initParams.disconnectHandler = aws_iot_mqtt_tests_disconnect_callback_handler;
	initParams.enableAutoReconnect = false;
	rc = aws_iot_mqtt_init(pClient, &initParams);
	printf("\n Init response : %d", rc);

	printf("\nRoot CA Path : %s\nClientCRT : %s\nClientKey : %s \nClient ID : %s", rootCA, clientCRT,
		   clientKey, clientId);
	connectParams.keepAliveIntervalInSec = 5;
	connectParams.isCleanSession = true;
	connectParams.MQTTVersion = MQTT_3_1_1;
	connectParams.pClientID = clientId;
	connectParams.clientIDLen = strlen(clientId);
	connectParams.isWillMsgPresent = 0;
	connectParams.pUsername = NULL;
	connectParams.usernameLen = 0;
	connectParams.pPassword = NULL;
	connectParams.passwordLen = 0;

	gettimeofday(&start, NULL);
	rc = aws_iot_mqtt_connect(pClient, &connectParams);
	printf("\nConnect response : %d ", rc);
	gettimeofday(&end, NULL);
	timersub(&end, &start, pConnectTime);

	return rc;
}

static IoT_Error_t aws_iot_mqtt_tests_subscribe_to_test_topic(AWS_IoT_Client *pClient, QoS qos, struct timeval *pSubscribeTime) {
	IoT_Error_t rc = SUCCESS;
	struct timeval start, end;

	gettimeofday(&start, NULL);
	rc = aws_iot_mqtt_subscribe(pClient, INTEGRATION_TEST_TOPIC, strlen(INTEGRATION_TEST_TOPIC), qos,
								aws_iot_mqtt_tests_message_aggregator, NULL);
	printf("\nSub response : %d\n", rc);
	gettimeofday(&end, NULL);

	timersub(&end, &start, pSubscribeTime);

	return rc;
}

static void *aws_iot_mqtt_tests_yield_thread_runner(void *ptr) {
	IoT_Error_t rc = SUCCESS;
	AWS_IoT_Client *pClient = (AWS_IoT_Client *) ptr;
	while(SUCCESS == rc && false == terminate_yield_thread) {
		do {
			usleep(THREAD_SLEEP_INTERVAL_USEC);
			rc = aws_iot_mqtt_yield(pClient, 100);
		} while(MQTT_CLIENT_NOT_IDLE_ERROR == rc);

		if(SUCCESS != rc) {
			IOT_ERROR("\nYield Returned : %d ", rc);
		}
	}
}

static void *aws_iot_mqtt_tests_publish_thread_runner(void *ptr) {
	int itr = 0;
	char cPayload[10];
	IoT_Publish_Message_Params params;
	IoT_Error_t rc = SUCCESS;
	AWS_IoT_Client *pClient = (AWS_IoT_Client *) ptr;

	for(itr = 0; itr < PUBLISH_COUNT; itr++) {
		sprintf(cPayload, "%d", itr + 1);
		params.payload = (void *) cPayload;
		params.payloadLen = strlen(cPayload) + 1;
		params.qos = QOS1;
		params.isRetained = 0;
		rc = aws_iot_mqtt_publish(pClient, INTEGRATION_TEST_TOPIC, strlen(INTEGRATION_TEST_TOPIC), &params);
		printf("\n Publishing %s", cPayload);
		if(SUCCESS != rc) {
			printf("Error Publishing #%d --> %d\n ", itr, rc);
			usleep(300000);
			rc = aws_iot_mqtt_publish(pClient, INTEGRATION_TEST_TOPIC, strlen(INTEGRATION_TEST_TOPIC), &params);
			rePublishCount++;
			if(SUCCESS != rc) {
				printf("Error Publishing #%d --> %d Second Attempt \n", itr, rc);
			}
		}
		usleep(300000);
	}
	isPubThreadFinished = true;
}


int aws_iot_mqtt_tests_multiple_clients() {
	char certDirectory[15] = "../../certs";
	char CurrentWD[PATH_MAX + 1];
	char rootCA[PATH_MAX + 1];
	char clientCRT[PATH_MAX + 1];
	char clientKey[PATH_MAX + 1];

	char subClientId[50];
	char pubClientId[50];

	int itr = 0;
	int rxMsgCount = 0;
	int test_result = 0;
	int pubThreadReturn = 0;
	int yieldThreadReturn = 0;
	unsigned int connectCounter = 0;
	float percentOfRxMsg = 0.0;

	IoT_Error_t rc = SUCCESS;
	pthread_t yield_thread;
	pthread_t publish_thread;
	struct timeval connectTime;
	struct timeval subscribeTopic;

	AWS_IoT_Client pubClient;
	AWS_IoT_Client subClient;

	terminate_yield_thread = false;
	isPubThreadFinished = false;
	rxMsgBufferTooBigCounter = 0;
	rxUnexpectedNumberCounter = 0;
	rePublishCount = 0;

	srand((unsigned int)time(NULL));
	snprintf(subClientId, 50, "%s_%d", INTEGRATION_TEST_CLIENT_ID_SUB, rand() % 10000);
	snprintf(pubClientId, 50, "%s_%d", INTEGRATION_TEST_CLIENT_ID_PUB, rand() % 10000);

	getcwd(CurrentWD, sizeof(CurrentWD));

	snprintf(rootCA, PATH_MAX + 1, "%s/%s/%s", CurrentWD, certDirectory, AWS_IOT_ROOT_CA_FILENAME);
	snprintf(clientCRT, PATH_MAX + 1, "%s/%s/%s", CurrentWD, certDirectory, AWS_IOT_CERTIFICATE_FILENAME);
	snprintf(clientKey, PATH_MAX + 1, "%s/%s/%s", CurrentWD, certDirectory, AWS_IOT_PRIVATE_KEY_FILENAME);

	for(itr = 0; itr < PUBLISH_COUNT; itr++) {
		countArray[itr] = 0;
	}

	printf(" \n Connecting Pub Client ");
	do {
		rc = aws_iot_mqtt_tests_connect_client_to_service(&pubClient, &connectTime, pubClientId, rootCA,
														  clientCRT, clientKey);
		connectCounter++;
	} while(SUCCESS != rc && CONNECT_MAX_ATTEMPT_COUNT > connectCounter);

	if(SUCCESS == rc) {
		printf("\n## Connect Success. Time sec: %d, usec: %d\n", connectTime.tv_sec, connectTime.tv_usec);
	} else {
		printf("\n## Connect Failed. error code %d\n", rc);
		return -1;
	}

	printf("\n Connecting Sub Client ");
	do {
		rc = aws_iot_mqtt_tests_connect_client_to_service(&subClient, &connectTime, subClientId, rootCA,
														  clientCRT, clientKey);
		connectCounter++;
	} while(SUCCESS != rc && connectCounter < CONNECT_MAX_ATTEMPT_COUNT);

	if(SUCCESS == rc) {
		printf("## Connect Success. Time sec: %d, usec: %d\n", connectTime.tv_sec, connectTime.tv_usec);
	} else {
		printf("## Connect Failed. error code %d\n", rc);
		return -1;
	}

	aws_iot_mqtt_tests_subscribe_to_test_topic(&subClient, QOS1, &subscribeTopic);

	yieldThreadReturn = pthread_create(&yield_thread, NULL, aws_iot_mqtt_tests_yield_thread_runner, &subClient);
	pubThreadReturn = pthread_create(&publish_thread, NULL, aws_iot_mqtt_tests_publish_thread_runner, &pubClient);

	/* This sleep is to ensure that the last publish message has enough time to be received by us */
	do {
		sleep(1);
	} while(!isPubThreadFinished);

	/* Kill yield thread */
	terminate_yield_thread = true;
	sleep(1);

	aws_iot_mqtt_disconnect(&pubClient);
	aws_iot_mqtt_disconnect(&subClient);

	for(itr = 0; itr < PUBLISH_COUNT; itr++) {
		if(countArray[itr] > 0) {
			rxMsgCount++;
		}
	}

	percentOfRxMsg = (float) rxMsgCount * 100 / PUBLISH_COUNT;
	if(percentOfRxMsg >= RX_RECEIVE_PERCENTAGE && rxMsgBufferTooBigCounter == 0 && rxUnexpectedNumberCounter == 0) {
		printf("\nSuccess: %f \%\n", percentOfRxMsg);
		printf("Published Messages: %d , Received Messages: %d \n", PUBLISH_COUNT, rxMsgCount);
		printf("QoS 1 re publish count %d\n", rePublishCount);
		printf("Connection Attempts %d\n", connectCounter);
		test_result = 0;
	} else {
		printf("\nFailure: %f\n", percentOfRxMsg);
		printf("\"Received message was too big than anything sent\" count: %d\n", rxMsgBufferTooBigCounter);
		printf("\"The number received is out of the range\" count: %d\n", rxUnexpectedNumberCounter);
		test_result = -2;
	}
	return test_result;
}

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
 * @file aws_iot_test_basic_connectivity.c
 * @brief Integration Test for basic client connectivity
 */

#include "aws_iot_test_integration_common.h"

static bool terminate_yield_thread;
static bool isPubThreadFinished;

static unsigned int countArray[PUBLISH_COUNT];
static unsigned int rxMsgBufferTooBigCounter;
static unsigned int rxUnexpectedNumberCounter;
static unsigned int rePublishCount;
static unsigned int wrongYieldCount;

typedef struct ThreadData {
	AWS_IoT_Client *client;
	int threadId;
} ThreadData;

static void aws_iot_mqtt_tests_message_aggregator(AWS_IoT_Client *pClient, char *topicName,
							  uint16_t topicNameLen, IoT_Publish_Message_Params *params, void *pData) {
	char tempBuf[30];
	char *temp = NULL;
	unsigned int tempRow = 0, tempCol = 0;
	IoT_Error_t rc;

	if(params->payloadLen <= 30) {
		snprintf(tempBuf, params->payloadLen, params->payload);
		printf("\nMsg received : %s", tempBuf);
		temp = strtok(tempBuf, " ,:");
		temp = strtok(NULL, " ,:");
		if(NULL == temp) {
			return;
		}
		tempRow = atoi(temp);
		temp = strtok(NULL, " ,:");
		temp = strtok(NULL, " ,:");
		tempCol = atoi(temp);
		if(NULL == temp) {
			return;
		}
		if(tempCol > 0 && tempCol <= PUBLISH_COUNT) {
			countArray[tempCol - 1]++;
		} else {
			IOT_WARN(" \n Thread : %d, Msg : %d ", tempRow, tempCol);
			rxUnexpectedNumberCounter++;
		}
		rc = aws_iot_mqtt_yield(pClient, 10);
		if(MQTT_CLIENT_NOT_IDLE_ERROR != rc) {
			IOT_ERROR("\n Yield succeeded in callback!!! Client state : %d Rc : %d\n",
				  aws_iot_mqtt_get_client_state(pClient), rc);
			wrongYieldCount++;
		}
	} else {
		rxMsgBufferTooBigCounter++;
	}
}

static void aws_iot_mqtt_tests_disconnect_callback_handler(AWS_IoT_Client *pClient, void *param) {
}

static IoT_Error_t aws_iot_mqtt_tests_subscribe_to_test_topic(AWS_IoT_Client *pClient, QoS qos,
															 struct timeval *pSubscribeTime) {
	IoT_Error_t rc = SUCCESS;
	struct timeval start, end;

	gettimeofday(&start, NULL);
	rc = aws_iot_mqtt_subscribe(pClient, INTEGRATION_TEST_TOPIC, strlen(INTEGRATION_TEST_TOPIC), qos,
								aws_iot_mqtt_tests_message_aggregator, NULL);
	IOT_DEBUG("Sub response : %d\n", rc);
	gettimeofday(&end, NULL);

	timersub(&end, &start, pSubscribeTime);

	return rc;
}

static void *aws_iot_mqtt_tests_yield_thread_runner(void *ptr) {
	IoT_Error_t rc = SUCCESS;
	AWS_IoT_Client *pClient = (AWS_IoT_Client *) ptr;
	while(SUCCESS == rc && terminate_yield_thread == false) {
		do {
			usleep(THREAD_SLEEP_INTERVAL_USEC);
			rc = aws_iot_mqtt_yield(pClient, 100);
		} while(MQTT_CLIENT_NOT_IDLE_ERROR == rc); // Client is busy, wait to get lock

		if(SUCCESS != rc) {
			IOT_DEBUG("\nYield Returned : %d ", rc);
		}
	}
}

static void *aws_iot_mqtt_tests_publish_thread_runner(void *ptr) {
	int i = 0;
	char cPayload[100];
	IoT_Publish_Message_Params params;
	IoT_Error_t rc = SUCCESS;
	ThreadData *threadData = (ThreadData *) ptr;
	AWS_IoT_Client *pClient = threadData->client;
	int threadId = threadData->threadId;

	for(i = 0; i < PUBLISH_COUNT; i++) {
		snprintf(cPayload, 100, "Thread : %d, Msg : %d", threadId, i + 1);
		printf("\nMsg being published: %s \n", cPayload);
		params.payload = (void *) cPayload;
		params.payloadLen = strlen(cPayload) + 1;
		params.qos = QOS1;
		params.isRetained = 0;

		do {
			rc = aws_iot_mqtt_publish(pClient, INTEGRATION_TEST_TOPIC, strlen(INTEGRATION_TEST_TOPIC), &params);
			usleep(THREAD_SLEEP_INTERVAL_USEC);
		} while(MUTEX_LOCK_ERROR == rc || MQTT_CLIENT_NOT_IDLE_ERROR == rc);
		if(rc != SUCCESS) {
			IOT_WARN("Error Publishing #%d --> %d\n ", i, rc);
			do {
				rc = aws_iot_mqtt_publish(pClient, INTEGRATION_TEST_TOPIC, strlen(INTEGRATION_TEST_TOPIC), &params);
				usleep(THREAD_SLEEP_INTERVAL_USEC);
			} while(MUTEX_LOCK_ERROR == rc || MQTT_CLIENT_NOT_IDLE_ERROR == rc);
			rePublishCount++;
			if(rc != SUCCESS) {
				IOT_ERROR("Error Publishing #%d --> %d Second Attempt \n", i, rc);
			}
		}
	}
	isPubThreadFinished = true;
	return 0;
}

int aws_iot_mqtt_tests_basic_connectivity() {
	pthread_t publish_thread, yield_thread;
	char certDirectory[15] = "../../certs";
	char clientCRT[PATH_MAX + 1];
	char root_CA[PATH_MAX + 1];
	char clientKey[PATH_MAX + 1];
	char CurrentWD[PATH_MAX + 1];
	char clientId[50];
	IoT_Client_Init_Params initParams;
	IoT_Client_Connect_Params connectParams;
	int pubThreadReturn;
	int yieldThreadReturn = 0;
	float percentOfRxMsg = 0.0;
	IoT_Error_t rc = SUCCESS;
	int i, rxMsgCount = 0, j = 0;
	struct timeval connectTime, subscribeTopic;
	struct timeval start, end;
	unsigned int connectCounter = 0;
	int test_result = 0;
	ThreadData threadData;
	AWS_IoT_Client client;

	terminate_yield_thread = false;
	isPubThreadFinished = false;

	rxMsgBufferTooBigCounter = 0;
	rxUnexpectedNumberCounter = 0;
	rePublishCount = 0;
	wrongYieldCount = 0;
	for(i = 0; i < PUBLISH_COUNT; i++) {
		countArray[i] = 0;
	}

	IOT_DEBUG("\nConnecting Client ");
	do {
		getcwd(CurrentWD, sizeof(CurrentWD));
		snprintf(root_CA, PATH_MAX + 1, "%s/%s/%s", CurrentWD, certDirectory, AWS_IOT_ROOT_CA_FILENAME);
		snprintf(clientCRT, PATH_MAX + 1, "%s/%s/%s", CurrentWD, certDirectory, AWS_IOT_CERTIFICATE_FILENAME);
		snprintf(clientKey, PATH_MAX + 1, "%s/%s/%s", CurrentWD, certDirectory, AWS_IOT_PRIVATE_KEY_FILENAME);
		srand((unsigned int)time(NULL));
		snprintf(clientId, 50, "%s_%d", INTEGRATION_TEST_CLIENT_ID, rand() % 10000);

		printf("\n\nClient ID : %s \n", clientId);

		IOT_DEBUG("Root CA Path : %s\n clientCRT : %s\n clientKey : %s\n", root_CA, clientCRT, clientKey);
		initParams.pHostURL = AWS_IOT_MQTT_HOST;
		initParams.port = 8883;
		initParams.pRootCALocation = root_CA;
		initParams.pDeviceCertLocation = clientCRT;
		initParams.pDevicePrivateKeyLocation = clientKey;
		initParams.mqttCommandTimeout_ms = 10000;
		initParams.tlsHandshakeTimeout_ms = 10000;
		initParams.disconnectHandler = aws_iot_mqtt_tests_disconnect_callback_handler;
		initParams.enableAutoReconnect = false;
		aws_iot_mqtt_init(&client, &initParams);

		connectParams.keepAliveIntervalInSec = 10;
		connectParams.isCleanSession = true;
		connectParams.MQTTVersion = MQTT_3_1_1;
		connectParams.pClientID = (char *)&clientId;
		connectParams.clientIDLen = strlen(clientId);
		connectParams.isWillMsgPresent = false;
		connectParams.pUsername = NULL;
		connectParams.usernameLen = 0;
		connectParams.pPassword = NULL;
		connectParams.passwordLen = 0;

		gettimeofday(&connectTime, NULL);
		rc = aws_iot_mqtt_connect(&client, &connectParams);
		gettimeofday(&end, NULL);
		timersub(&end, &start, &connectTime);

		connectCounter++;
	} while(rc != SUCCESS && connectCounter < CONNECT_MAX_ATTEMPT_COUNT);

	if(SUCCESS == rc) {
		IOT_DEBUG("## Connect Success. Time sec: %d, usec: %d\n", connectTime.tv_sec, connectTime.tv_usec);
	} else {
		IOT_ERROR("## Connect Failed. error code %d\n", rc);
		return -1;
	}

	aws_iot_mqtt_tests_subscribe_to_test_topic(&client, QOS1, &subscribeTopic);

	yieldThreadReturn = pthread_create(&yield_thread, NULL, aws_iot_mqtt_tests_yield_thread_runner, &client);
	sleep(1);

	threadData.client = &client;
	threadData.threadId = 1;
	pubThreadReturn = pthread_create(&publish_thread, NULL, aws_iot_mqtt_tests_publish_thread_runner, &threadData);

	do {
		sleep(1); //Let all threads run
	} while(!isPubThreadFinished);

	// This sleep is to ensure that the last publish message has enough time to be received by us
	sleep(1);

	terminate_yield_thread = true;
	sleep(1);

	/* Not using pthread_join because all threads should have terminated gracefully at this point. If they haven't,
	 * which should not be possible, something below will fail. */

	for(i = 0; i < PUBLISH_COUNT; i++) {
		if(countArray[i] > 0) {
			rxMsgCount++;
		}
	}

	IOT_DEBUG("\n\nResult : \n");
	percentOfRxMsg = (float) rxMsgCount * 100 / PUBLISH_COUNT;
	if(percentOfRxMsg >= RX_RECEIVE_PERCENTAGE && rxMsgBufferTooBigCounter == 0 && rxUnexpectedNumberCounter == 0 &&
	   wrongYieldCount == 0) {
		IOT_DEBUG("\n\nSuccess: %f \%\n", percentOfRxMsg);
		IOT_DEBUG("Published Messages: %d , Received Messages: %d \n", PUBLISH_COUNT, rxMsgCount);
		IOT_DEBUG("QoS 1 re publish count %d\n", rePublishCount);
		IOT_DEBUG("Connection Attempts %d\n", connectCounter);
		IOT_DEBUG("Yield count without error during callback %d\n", wrongYieldCount);
		test_result = 0;
	} else {
		IOT_ERROR("\n\nFailure: %f\n", percentOfRxMsg);
		IOT_ERROR("\"Received message was too big than anything sent\" count: %d\n", rxMsgBufferTooBigCounter);
		IOT_ERROR("\"The number received is out of the range\" count: %d\n", rxUnexpectedNumberCounter);
		IOT_ERROR("Yield count without error during callback %d\n", wrongYieldCount);
		test_result = -2;
	}
	aws_iot_mqtt_disconnect(&client);
	return test_result;
}

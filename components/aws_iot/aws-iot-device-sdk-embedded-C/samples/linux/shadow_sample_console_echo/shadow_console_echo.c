/*
 * Copyright 2010-2015 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>

#include "aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"
#include "aws_iot_shadow_interface.h"

/**
 * @file shadow_console_echo.c
 * @brief  Echo received Delta message
 *
 * This application will echo the message received in delta, as reported.
 * for example:
 * Received Delta message
 * {
 *    "state": {
 *       "switch": "on"
 *   }
 * }
 * This delta message means the desired switch position has changed to "on"
 *
 * This application will take this delta message and publish it back as the reported message from the device.
 * {
 *    "state": {
 *     "reported": {
 *       "switch": "on"
 *      }
 *    }
 * }
 *
 * This update message will remove the delta that was created. If this message was not removed then the AWS IoT Thing Shadow is going to always have a delta and keep sending delta any time an update is applied to the Shadow
 * This example will not use any of the json builder/helper functions provided in the aws_iot_shadow_json_data.h.
 * @note Ensure the buffer sizes in aws_iot_config.h are big enough to receive the delta message. The delta message will also contain the metadata with the timestamps
 */

char certDirectory[PATH_MAX + 1] = "../../../certs";
char HostAddress[255] = AWS_IOT_MQTT_HOST;
uint32_t port = AWS_IOT_MQTT_PORT;
bool messageArrivedOnDelta = false;

/*
 * @note The delta message is always sent on the "state" key in the json
 * @note Any time messages are bigger than AWS_IOT_MQTT_RX_BUF_LEN the underlying MQTT library will ignore it. The maximum size of the message that can be received is limited to the AWS_IOT_MQTT_RX_BUF_LEN
 */
char stringToEchoDelta[SHADOW_MAX_SIZE_OF_RX_BUFFER];

// Helper functions
void parseInputArgsForConnectParams(int argc, char** argv);

// Shadow Callback for receiving the delta
void DeltaCallback(const char *pJsonValueBuffer, uint32_t valueLength, jsonStruct_t *pJsonStruct_t);

void UpdateStatusCallback(const char *pThingName, ShadowActions_t action, Shadow_Ack_Status_t status,
		const char *pReceivedJsonDocument, void *pContextData);

int main(int argc, char** argv) {
	IoT_Error_t rc = SUCCESS;
	int32_t i = 0;

	char rootCA[PATH_MAX + 1];
	char clientCRT[PATH_MAX + 1];
	char clientKey[PATH_MAX + 1];
	char CurrentWD[PATH_MAX + 1];

	IOT_INFO("\nAWS IoT SDK Version %d.%d.%d-%s\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);

	getcwd(CurrentWD, sizeof(CurrentWD));
	snprintf(rootCA, PATH_MAX + 1, "%s/%s/%s", CurrentWD, certDirectory, AWS_IOT_ROOT_CA_FILENAME);
	snprintf(clientCRT, PATH_MAX + 1, "%s/%s/%s", CurrentWD, certDirectory, AWS_IOT_CERTIFICATE_FILENAME);
	snprintf(clientKey, PATH_MAX + 1, "%s/%s/%s", CurrentWD, certDirectory, AWS_IOT_PRIVATE_KEY_FILENAME);

	IOT_DEBUG("rootCA %s", rootCA);
	IOT_DEBUG("clientCRT %s", clientCRT);
	IOT_DEBUG("clientKey %s", clientKey);

	parseInputArgsForConnectParams(argc, argv);

	// initialize the mqtt client
	AWS_IoT_Client mqttClient;

	ShadowInitParameters_t sp = ShadowInitParametersDefault;
	sp.pHost = AWS_IOT_MQTT_HOST;
	sp.port = AWS_IOT_MQTT_PORT;
	sp.pClientCRT = clientCRT;
	sp.pClientKey = clientKey;
	sp.pRootCA = rootCA;
	sp.enableAutoReconnect = false;
	sp.disconnectHandler = NULL;

	IOT_INFO("Shadow Init");
	rc = aws_iot_shadow_init(&mqttClient, &sp);
	if (SUCCESS != rc) {
		IOT_ERROR("Shadow Connection Error");
		return rc;
	}

	ShadowConnectParameters_t scp = ShadowConnectParametersDefault;
	scp.pMyThingName = AWS_IOT_MY_THING_NAME;
	scp.pMqttClientId = AWS_IOT_MQTT_CLIENT_ID;
	scp.mqttClientIdLen = (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID);

	IOT_INFO("Shadow Connect");
	rc = aws_iot_shadow_connect(&mqttClient, &scp);
	if (SUCCESS != rc) {
		IOT_ERROR("Shadow Connection Error");
		return rc;
	}

	/*
	 * Enable Auto Reconnect functionality. Minimum and Maximum time of Exponential backoff are set in aws_iot_config.h
	 *  #AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL
	 *  #AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL
	 */
	rc = aws_iot_shadow_set_autoreconnect_status(&mqttClient, true);
	if(SUCCESS != rc){
		IOT_ERROR("Unable to set Auto Reconnect to true - %d", rc);
		return rc;
	}

	jsonStruct_t deltaObject;
	deltaObject.pData = stringToEchoDelta;
	deltaObject.pKey = "state";
	deltaObject.type = SHADOW_JSON_OBJECT;
	deltaObject.cb = DeltaCallback;

	/*
	 * Register the jsonStruct object
	 */
	rc = aws_iot_shadow_register_delta(&mqttClient, &deltaObject);

	// Now wait in the loop to receive any message sent from the console
	while (NETWORK_ATTEMPTING_RECONNECT == rc || NETWORK_RECONNECTED == rc || SUCCESS == rc) {
		/*
		 * Lets check for the incoming messages for 200 ms.
		 */
		rc = aws_iot_shadow_yield(&mqttClient, 200);

		if (NETWORK_ATTEMPTING_RECONNECT == rc) {
			sleep(1);
			// If the client is attempting to reconnect we will skip the rest of the loop.
			continue;
		}

		if (messageArrivedOnDelta) {
			IOT_INFO("\nSending delta message back %s\n", stringToEchoDelta);
			rc = aws_iot_shadow_update(&mqttClient, AWS_IOT_MY_THING_NAME, stringToEchoDelta, UpdateStatusCallback, NULL, 2, true);
			messageArrivedOnDelta = false;
		}

		// sleep for some time in seconds
		sleep(1);
	}

	if (SUCCESS != rc) {
		IOT_ERROR("An error occurred in the loop %d", rc);
	}

	IOT_INFO("Disconnecting");
	rc = aws_iot_shadow_disconnect(&mqttClient);

	if (SUCCESS != rc) {
		IOT_ERROR("Disconnect error %d", rc);
	}

	return rc;
}
/**
 * @brief This function builds a full Shadow expected JSON document by putting the data in the reported section
 *
 * @param pJsonDocument Buffer to be filled up with the JSON data
 * @param maxSizeOfJsonDocument maximum size of the buffer that could be used to fill
 * @param pReceivedDeltaData This is the data that will be embedded in the reported section of the JSON document
 * @param lengthDelta Length of the data
 */
bool buildJSONForReported(char *pJsonDocument, size_t maxSizeOfJsonDocument, const char *pReceivedDeltaData, uint32_t lengthDelta) {
	int32_t ret;

	if (NULL == pJsonDocument) {
		return false;
	}

	char tempClientTokenBuffer[MAX_SIZE_CLIENT_TOKEN_CLIENT_SEQUENCE];

	if(aws_iot_fill_with_client_token(tempClientTokenBuffer, MAX_SIZE_CLIENT_TOKEN_CLIENT_SEQUENCE) != SUCCESS){
		return false;
	}

	ret = snprintf(pJsonDocument, maxSizeOfJsonDocument, "{\"state\":{\"reported\":%.*s}, \"clientToken\":\"%s\"}", lengthDelta, pReceivedDeltaData, tempClientTokenBuffer);

	if (ret >= maxSizeOfJsonDocument || ret < 0) {
		return false;
	}

	return true;
}

void parseInputArgsForConnectParams(int argc, char** argv) {
	int opt;

	while (-1 != (opt = getopt(argc, argv, "h:p:c:"))) {
		switch (opt) {
		case 'h':
			strcpy(HostAddress, optarg);
			IOT_DEBUG("Host %s", optarg);
			break;
		case 'p':
			port = atoi(optarg);
			IOT_DEBUG("arg %s", optarg);
			break;
		case 'c':
			strcpy(certDirectory, optarg);
			IOT_DEBUG("cert root directory %s", optarg);
			break;
		case '?':
			if (optopt == 'c') {
				IOT_ERROR("Option -%c requires an argument.", optopt);
			} else if (isprint(optopt)) {
				IOT_WARN("Unknown option `-%c'.", optopt);
			} else {
				IOT_WARN("Unknown option character `\\x%x'.", optopt);
			}
			break;
		default:
			IOT_ERROR("ERROR in command line argument parsing");
			break;
		}
	}

}


void DeltaCallback(const char *pJsonValueBuffer, uint32_t valueLength, jsonStruct_t *pJsonStruct_t) {
	IOT_UNUSED(pJsonStruct_t);

	IOT_DEBUG("Received Delta message %.*s", valueLength, pJsonValueBuffer);

	if (buildJSONForReported(stringToEchoDelta, SHADOW_MAX_SIZE_OF_RX_BUFFER, pJsonValueBuffer, valueLength)) {
		messageArrivedOnDelta = true;
	}
}

void UpdateStatusCallback(const char *pThingName, ShadowActions_t action, Shadow_Ack_Status_t status,
		const char *pReceivedJsonDocument, void *pContextData) {
	IOT_UNUSED(pThingName);
	IOT_UNUSED(action);
	IOT_UNUSED(pReceivedJsonDocument);
	IOT_UNUSED(pContextData);

	if(SHADOW_ACK_TIMEOUT == status) {
		IOT_INFO("Update Timeout--");
	} else if(SHADOW_ACK_REJECTED == status) {
		IOT_INFO("Update RejectedXX");
	} else if(SHADOW_ACK_ACCEPTED == status) {
		IOT_INFO("Update Accepted !!");
	}
}

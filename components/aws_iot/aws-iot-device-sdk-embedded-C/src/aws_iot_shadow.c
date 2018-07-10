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

/**
 * @file aws_iot_shadow.c
 * @brief Shadow client API definitions
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include "aws_iot_mqtt_client_interface.h"
#include "aws_iot_shadow_interface.h"
#include "aws_iot_error.h"
#include "aws_iot_log.h"
#include "aws_iot_shadow_actions.h"
#include "aws_iot_shadow_json.h"
#include "aws_iot_shadow_key.h"
#include "aws_iot_shadow_records.h"

const ShadowInitParameters_t ShadowInitParametersDefault = {(char *) AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT, NULL, NULL,
															NULL, false, NULL};

const ShadowConnectParameters_t ShadowConnectParametersDefault = {(char *) AWS_IOT_MY_THING_NAME,
								  (char *) AWS_IOT_MQTT_CLIENT_ID, 0, NULL};

static char deleteAcceptedTopic[MAX_SHADOW_TOPIC_LENGTH_BYTES];

void aws_iot_shadow_reset_last_received_version(void) {
	shadowJsonVersionNum = 0;
}

uint32_t aws_iot_shadow_get_last_received_version(void) {
	return shadowJsonVersionNum;
}

void aws_iot_shadow_enable_discard_old_delta_msgs(void) {
	shadowDiscardOldDeltaFlag = true;
}

void aws_iot_shadow_disable_discard_old_delta_msgs(void) {
	shadowDiscardOldDeltaFlag = false;
}

IoT_Error_t aws_iot_shadow_init(AWS_IoT_Client *pClient, const ShadowInitParameters_t *pParams) {
	IoT_Client_Init_Params mqttInitParams;
	IoT_Error_t rc;

	FUNC_ENTRY;

	if(NULL == pClient || NULL == pParams) {
		FUNC_EXIT_RC(NULL_VALUE_ERROR);
	}

	mqttInitParams.enableAutoReconnect = pParams->enableAutoReconnect;
	mqttInitParams.pHostURL = pParams->pHost;
	mqttInitParams.port = pParams->port;
	mqttInitParams.pRootCALocation = pParams->pRootCA;
	mqttInitParams.pDeviceCertLocation = pParams->pClientCRT;
	mqttInitParams.pDevicePrivateKeyLocation = pParams->pClientKey;
	mqttInitParams.mqttPacketTimeout_ms = 5000;
	mqttInitParams.mqttCommandTimeout_ms = 20000;
	mqttInitParams.tlsHandshakeTimeout_ms = 5000;
	mqttInitParams.isSSLHostnameVerify = true;
	mqttInitParams.disconnectHandler = pParams->disconnectHandler;

	rc = aws_iot_mqtt_init(pClient, &mqttInitParams);
	if(SUCCESS != rc) {
		FUNC_EXIT_RC(rc);
	}

	resetClientTokenSequenceNum();
	aws_iot_shadow_reset_last_received_version();
	initDeltaTokens();

	FUNC_EXIT_RC(SUCCESS);
}

IoT_Error_t aws_iot_shadow_connect(AWS_IoT_Client *pClient, const ShadowConnectParameters_t *pParams) {
	IoT_Error_t rc = SUCCESS;
	uint16_t deleteAcceptedTopicLen;
	IoT_Client_Connect_Params ConnectParams = iotClientConnectParamsDefault;

	FUNC_ENTRY;

	if(NULL == pClient || NULL == pParams || NULL == pParams->pMqttClientId) {
		FUNC_EXIT_RC(NULL_VALUE_ERROR);
	}

	snprintf(myThingName, MAX_SIZE_OF_THING_NAME, "%s", pParams->pMyThingName);
	snprintf(mqttClientID, MAX_SIZE_OF_UNIQUE_CLIENT_ID_BYTES, "%s", pParams->pMqttClientId);

	ConnectParams.keepAliveIntervalInSec = 600; // NOTE: Temporary fix
	ConnectParams.MQTTVersion = MQTT_3_1_1;
	ConnectParams.isCleanSession = true;
	ConnectParams.isWillMsgPresent = false;
	ConnectParams.pClientID = pParams->pMqttClientId;
	ConnectParams.clientIDLen = pParams->mqttClientIdLen;
	ConnectParams.pPassword = NULL;
	ConnectParams.pUsername = NULL;

	rc = aws_iot_mqtt_connect(pClient, &ConnectParams);

	if(SUCCESS != rc) {
		FUNC_EXIT_RC(rc);
	}

	initializeRecords(pClient);

	if(NULL != pParams->deleteActionHandler) {
		snprintf(deleteAcceptedTopic, MAX_SHADOW_TOPIC_LENGTH_BYTES,
				 "$aws/things/%s/shadow/delete/accepted", myThingName);
		deleteAcceptedTopicLen = (uint16_t) strlen(deleteAcceptedTopic);
		rc = aws_iot_mqtt_subscribe(pClient, deleteAcceptedTopic, deleteAcceptedTopicLen, QOS1,
									pParams->deleteActionHandler, (void *) myThingName);
	}

	FUNC_EXIT_RC(rc);
}

IoT_Error_t aws_iot_shadow_register_delta(AWS_IoT_Client *pMqttClient, jsonStruct_t *pStruct) {
	if(NULL == pMqttClient || NULL == pStruct) {
		return NULL_VALUE_ERROR;
	}

	if(!aws_iot_mqtt_is_client_connected(pMqttClient)) {
		return MQTT_CONNECTION_ERROR;
	}

	return registerJsonTokenOnDelta(pStruct);
}

IoT_Error_t aws_iot_shadow_yield(AWS_IoT_Client *pClient, uint32_t timeout) {
	if(NULL == pClient) {
		return NULL_VALUE_ERROR;
	}

	HandleExpiredResponseCallbacks();
	return aws_iot_mqtt_yield(pClient, timeout);
}

IoT_Error_t aws_iot_shadow_disconnect(AWS_IoT_Client *pClient) {
	return aws_iot_mqtt_disconnect(pClient);
}

IoT_Error_t aws_iot_shadow_update(AWS_IoT_Client *pClient, const char *pThingName, char *pJsonString,
								  fpActionCallback_t callback, void *pContextData, uint8_t timeout_seconds,
								  bool isPersistentSubscribe) {
	IoT_Error_t rc;

	if(NULL == pClient) {
		FUNC_EXIT_RC(NULL_VALUE_ERROR);
	}

	if(!aws_iot_mqtt_is_client_connected(pClient)) {
		FUNC_EXIT_RC(MQTT_CONNECTION_ERROR);
	}

	rc = aws_iot_shadow_internal_action(pThingName, SHADOW_UPDATE, pJsonString, callback, pContextData,
										timeout_seconds, isPersistentSubscribe);

	FUNC_EXIT_RC(rc);
}

IoT_Error_t aws_iot_shadow_delete(AWS_IoT_Client *pClient, const char *pThingName, fpActionCallback_t callback,
								  void *pContextData, uint8_t timeout_seconds, bool isPersistentSubscribe) {
	char deleteRequestJsonBuf[MAX_SIZE_CLIENT_TOKEN_CLIENT_SEQUENCE];
	IoT_Error_t rc;

	FUNC_ENTRY;

	if(NULL == pClient) {
		FUNC_EXIT_RC(NULL_VALUE_ERROR);
	}

	if(!aws_iot_mqtt_is_client_connected(pClient)) {
		FUNC_EXIT_RC(MQTT_CONNECTION_ERROR);
	}

	aws_iot_shadow_internal_delete_request_json(deleteRequestJsonBuf);
	rc = aws_iot_shadow_internal_action(pThingName, SHADOW_DELETE, deleteRequestJsonBuf, callback, pContextData,
										timeout_seconds, isPersistentSubscribe);

	FUNC_EXIT_RC(rc);
}

IoT_Error_t aws_iot_shadow_get(AWS_IoT_Client *pClient, const char *pThingName, fpActionCallback_t callback,
							   void *pContextData, uint8_t timeout_seconds, bool isPersistentSubscribe) {
	char getRequestJsonBuf[MAX_SIZE_CLIENT_TOKEN_CLIENT_SEQUENCE];
	IoT_Error_t rc;

	FUNC_ENTRY;

	if(NULL == pClient) {
		FUNC_EXIT_RC(NULL_VALUE_ERROR);
	}

	if(!aws_iot_mqtt_is_client_connected(pClient)) {
		FUNC_EXIT_RC(MQTT_CONNECTION_ERROR);
	}

	aws_iot_shadow_internal_get_request_json(getRequestJsonBuf);
	rc = aws_iot_shadow_internal_action(pThingName, SHADOW_GET, getRequestJsonBuf, callback, pContextData,
										timeout_seconds, isPersistentSubscribe);
	FUNC_EXIT_RC(rc);
}

IoT_Error_t aws_iot_shadow_set_autoreconnect_status(AWS_IoT_Client *pClient, bool newStatus) {
	return aws_iot_mqtt_autoreconnect_set_status(pClient, newStatus);
}

#ifdef __cplusplus
}
#endif


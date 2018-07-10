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
 * @file aws_iot_mqtt_client_subscribe.c
 * @brief MQTT client subscribe API definitions
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "aws_iot_shadow_records.h"

#include <string.h>
#include <stdio.h>

#include "timer_interface.h"
#include "aws_iot_json_utils.h"
#include "aws_iot_log.h"
#include "aws_iot_shadow_json.h"
#include "aws_iot_config.h"

typedef struct {
	char clientTokenID[MAX_SIZE_CLIENT_ID_WITH_SEQUENCE];
	char thingName[MAX_SIZE_OF_THING_NAME];
	ShadowActions_t action;
	fpActionCallback_t callback;
	void *pCallbackContext;
	bool isFree;
	Timer timer;
} ToBeReceivedAckRecord_t;

typedef struct {
	const char *pKey;
	void *pStruct;
	jsonStructCallback_t callback;
	bool isFree;
} JsonTokenTable_t;

typedef struct {
	char Topic[MAX_SHADOW_TOPIC_LENGTH_BYTES];
	uint8_t count;
	bool isFree;
	bool isSticky;
} SubscriptionRecord_t;

typedef enum {
	SHADOW_ACCEPTED, SHADOW_REJECTED, SHADOW_ACTION
} ShadowAckTopicTypes_t;

ToBeReceivedAckRecord_t AckWaitList[MAX_ACKS_TO_COMEIN_AT_ANY_GIVEN_TIME];

AWS_IoT_Client *pMqttClient;

char myThingName[MAX_SIZE_OF_THING_NAME];
char mqttClientID[MAX_SIZE_OF_UNIQUE_CLIENT_ID_BYTES];

char shadowDeltaTopic[MAX_SHADOW_TOPIC_LENGTH_BYTES];

#define MAX_TOPICS_AT_ANY_GIVEN_TIME 2*MAX_THINGNAME_HANDLED_AT_ANY_GIVEN_TIME
SubscriptionRecord_t SubscriptionList[MAX_TOPICS_AT_ANY_GIVEN_TIME];

#define SUBSCRIBE_SETTLING_TIME 2
char shadowRxBuf[SHADOW_MAX_SIZE_OF_RX_BUFFER];

static JsonTokenTable_t tokenTable[MAX_JSON_TOKEN_EXPECTED];
static uint32_t tokenTableIndex = 0;
static bool deltaTopicSubscribedFlag = false;
uint32_t shadowJsonVersionNum = 0;
bool shadowDiscardOldDeltaFlag = true;

// local helper functions
static void AckStatusCallback(AWS_IoT_Client *pClient, char *topicName,
							  uint16_t topicNameLen, IoT_Publish_Message_Params *params, void *pData);

static void shadow_delta_callback(AWS_IoT_Client *pClient, char *topicName,
								  uint16_t topicNameLen, IoT_Publish_Message_Params *params, void *pData);

static void topicNameFromThingAndAction(char *pTopic, const char *pThingName, ShadowActions_t action,
										ShadowAckTopicTypes_t ackType);

static int16_t getNextFreeIndexOfSubscriptionList(void);

static void unsubscribeFromAcceptedAndRejected(uint8_t index);

void initDeltaTokens(void) {
	uint32_t i;
	for(i = 0; i < MAX_JSON_TOKEN_EXPECTED; i++) {
		tokenTable[i].isFree = true;
	}
	tokenTableIndex = 0;
	deltaTopicSubscribedFlag = false;
}

IoT_Error_t registerJsonTokenOnDelta(jsonStruct_t *pStruct) {

	IoT_Error_t rc = SUCCESS;

	if(!deltaTopicSubscribedFlag) {
		snprintf(shadowDeltaTopic, MAX_SHADOW_TOPIC_LENGTH_BYTES, "$aws/things/%s/shadow/update/delta", myThingName);
		rc = aws_iot_mqtt_subscribe(pMqttClient, shadowDeltaTopic, (uint16_t) strlen(shadowDeltaTopic), QOS0,
									shadow_delta_callback, NULL);
		deltaTopicSubscribedFlag = true;
	}

	if(tokenTableIndex >= MAX_JSON_TOKEN_EXPECTED) {
		return FAILURE;
	}

	tokenTable[tokenTableIndex].pKey = pStruct->pKey;
	tokenTable[tokenTableIndex].callback = pStruct->cb;
	tokenTable[tokenTableIndex].pStruct = pStruct;
	tokenTable[tokenTableIndex].isFree = false;
	tokenTableIndex++;

	return rc;
}

static int16_t getNextFreeIndexOfSubscriptionList(void) {
	uint8_t i;
	for(i = 0; i < MAX_TOPICS_AT_ANY_GIVEN_TIME; i++) {
		if(SubscriptionList[i].isFree) {
			SubscriptionList[i].isFree = false;
			return i;
		}
	}
	return -1;
}

static void topicNameFromThingAndAction(char *pTopic, const char *pThingName, ShadowActions_t action,
										ShadowAckTopicTypes_t ackType) {

	char actionBuf[10];
	char ackTypeBuf[10];

	if(SHADOW_GET == action) {
		strncpy(actionBuf, "get", 10);
	} else if(SHADOW_UPDATE == action) {
		strncpy(actionBuf, "update", 10);
	} else if(SHADOW_DELETE == action) {
		strncpy(actionBuf, "delete", 10);
	}

	if(SHADOW_ACCEPTED == ackType) {
		strncpy(ackTypeBuf, "accepted", 10);
	} else if(SHADOW_REJECTED == ackType) {
		strncpy(ackTypeBuf, "rejected", 10);
	}

	if(SHADOW_ACTION == ackType) {
		snprintf(pTopic, MAX_SHADOW_TOPIC_LENGTH_BYTES, "$aws/things/%s/shadow/%s", pThingName, actionBuf);
	} else {
		snprintf(pTopic, MAX_SHADOW_TOPIC_LENGTH_BYTES, "$aws/things/%s/shadow/%s/%s", pThingName, actionBuf,
				 ackTypeBuf);
	}
}

static bool isValidShadowVersionUpdate(const char *pTopicName) {
	if(strstr(pTopicName, myThingName) != NULL &&
	   ((strstr(pTopicName, "get/accepted") != NULL) ||
		(strstr(pTopicName, "delta") != NULL))) {
		return true;
	}
	return false;
}

static void AckStatusCallback(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
							  IoT_Publish_Message_Params *params, void *pData) {
	int32_t tokenCount;
	uint8_t i;
	void *pJsonHandler = NULL;
	char temporaryClientToken[MAX_SIZE_CLIENT_TOKEN_CLIENT_SEQUENCE];

	IOT_UNUSED(pClient);
	IOT_UNUSED(topicNameLen);
	IOT_UNUSED(pData);

	if(params->payloadLen >= SHADOW_MAX_SIZE_OF_RX_BUFFER) {
		IOT_WARN("Payload larger than RX Buffer");
		return;
	}

	memcpy(shadowRxBuf, params->payload, params->payloadLen);
	shadowRxBuf[params->payloadLen] = '\0';    // jsmn_parse relies on a string

	if(!isJsonValidAndParse(shadowRxBuf, pJsonHandler, &tokenCount)) {
		IOT_WARN("Received JSON is not valid");
		return;
	}

	if(isValidShadowVersionUpdate(topicName)) {
		uint32_t tempVersionNumber = 0;
		if(extractVersionNumber(shadowRxBuf, pJsonHandler, tokenCount, &tempVersionNumber)) {
			if(tempVersionNumber > shadowJsonVersionNum) {
				shadowJsonVersionNum = tempVersionNumber;
			}
		}
	}

	if(extractClientToken(shadowRxBuf, temporaryClientToken)) {
		for(i = 0; i < MAX_ACKS_TO_COMEIN_AT_ANY_GIVEN_TIME; i++) {
			if(!AckWaitList[i].isFree) {
				if(strcmp(AckWaitList[i].clientTokenID, temporaryClientToken) == 0) {
					Shadow_Ack_Status_t status = SHADOW_ACK_REJECTED;
					if(strstr(topicName, "accepted") != NULL) {
						status = SHADOW_ACK_ACCEPTED;
					} else if(strstr(topicName, "rejected") != NULL) {
						status = SHADOW_ACK_REJECTED;
					}
					if(status == SHADOW_ACK_ACCEPTED || status == SHADOW_ACK_REJECTED) {
						if(AckWaitList[i].callback != NULL) {
							AckWaitList[i].callback(AckWaitList[i].thingName, AckWaitList[i].action, status,
													shadowRxBuf, AckWaitList[i].pCallbackContext);
						}
						unsubscribeFromAcceptedAndRejected(i);
						AckWaitList[i].isFree = true;
						return;
					}
				}
			}
		}
	}
}

static int16_t findIndexOfSubscriptionList(const char *pTopic) {
	uint8_t i;
	for(i = 0; i < MAX_TOPICS_AT_ANY_GIVEN_TIME; i++) {
		if(!SubscriptionList[i].isFree) {
			if((strcmp(pTopic, SubscriptionList[i].Topic) == 0)) {
				return i;
			}
		}
	}
	return -1;
}

static void unsubscribeFromAcceptedAndRejected(uint8_t index) {

	char TemporaryTopicNameAccepted[MAX_SHADOW_TOPIC_LENGTH_BYTES];
	char TemporaryTopicNameRejected[MAX_SHADOW_TOPIC_LENGTH_BYTES];
	IoT_Error_t ret_val = SUCCESS;

	int16_t indexSubList;

	topicNameFromThingAndAction(TemporaryTopicNameAccepted, AckWaitList[index].thingName, AckWaitList[index].action,
								SHADOW_ACCEPTED);
	topicNameFromThingAndAction(TemporaryTopicNameRejected, AckWaitList[index].thingName, AckWaitList[index].action,
								SHADOW_REJECTED);

	indexSubList = findIndexOfSubscriptionList(TemporaryTopicNameAccepted);
	if((indexSubList >= 0)) {
		if(!SubscriptionList[indexSubList].isSticky && (SubscriptionList[indexSubList].count == 1)) {
			ret_val = aws_iot_mqtt_unsubscribe(pMqttClient, TemporaryTopicNameAccepted,
											   (uint16_t) strlen(TemporaryTopicNameAccepted));
			if(ret_val == SUCCESS) {
				SubscriptionList[indexSubList].isFree = true;
			}
		} else if(SubscriptionList[indexSubList].count > 1) {
			SubscriptionList[indexSubList].count--;
		}
	}

	indexSubList = findIndexOfSubscriptionList(TemporaryTopicNameRejected);
	if((indexSubList >= 0)) {
		if(!SubscriptionList[indexSubList].isSticky && (SubscriptionList[indexSubList].count == 1)) {
			ret_val = aws_iot_mqtt_unsubscribe(pMqttClient, TemporaryTopicNameRejected,
											   (uint16_t) strlen(TemporaryTopicNameRejected));
			if(ret_val == SUCCESS) {
				SubscriptionList[indexSubList].isFree = true;
			}
		} else if(SubscriptionList[indexSubList].count > 1) {
			SubscriptionList[indexSubList].count--;
		}
	}
}

void initializeRecords(AWS_IoT_Client *pClient) {
	uint8_t i;
	for(i = 0; i < MAX_ACKS_TO_COMEIN_AT_ANY_GIVEN_TIME; i++) {
		AckWaitList[i].isFree = true;
	}
	for(i = 0; i < MAX_TOPICS_AT_ANY_GIVEN_TIME; i++) {
		SubscriptionList[i].isFree = true;
		SubscriptionList[i].count = 0;
		SubscriptionList[i].isSticky = false;
	}

	pMqttClient = pClient;
}

bool isSubscriptionPresent(const char *pThingName, ShadowActions_t action) {

	uint8_t i = 0;
	bool isAcceptedPresent = false;
	bool isRejectedPresent = false;
	char TemporaryTopicNameAccepted[MAX_SHADOW_TOPIC_LENGTH_BYTES];
	char TemporaryTopicNameRejected[MAX_SHADOW_TOPIC_LENGTH_BYTES];

	topicNameFromThingAndAction(TemporaryTopicNameAccepted, pThingName, action, SHADOW_ACCEPTED);
	topicNameFromThingAndAction(TemporaryTopicNameRejected, pThingName, action, SHADOW_REJECTED);

	for(i = 0; i < MAX_TOPICS_AT_ANY_GIVEN_TIME; i++) {
		if(!SubscriptionList[i].isFree) {
			if((strcmp(TemporaryTopicNameAccepted, SubscriptionList[i].Topic) == 0)) {
				isAcceptedPresent = true;
			} else if((strcmp(TemporaryTopicNameRejected, SubscriptionList[i].Topic) == 0)) {
				isRejectedPresent = true;
			}
		}
	}

	if(isRejectedPresent && isAcceptedPresent) {
		return true;
	}

	return false;
}

IoT_Error_t subscribeToShadowActionAcks(const char *pThingName, ShadowActions_t action, bool isSticky) {
	IoT_Error_t ret_val = SUCCESS;

	bool clearBothEntriesFromList = true;
	int16_t indexAcceptedSubList = 0;
	int16_t indexRejectedSubList = 0;
	Timer subSettlingtimer;
	indexAcceptedSubList = getNextFreeIndexOfSubscriptionList();
	indexRejectedSubList = getNextFreeIndexOfSubscriptionList();

	if(indexAcceptedSubList >= 0 && indexRejectedSubList >= 0) {
		topicNameFromThingAndAction(SubscriptionList[indexAcceptedSubList].Topic, pThingName, action, SHADOW_ACCEPTED);
		ret_val = aws_iot_mqtt_subscribe(pMqttClient, SubscriptionList[indexAcceptedSubList].Topic,
										 (uint16_t) strlen(SubscriptionList[indexAcceptedSubList].Topic), QOS0,
										 AckStatusCallback, NULL);
		if(ret_val == SUCCESS) {
			SubscriptionList[indexAcceptedSubList].count = 1;
			SubscriptionList[indexAcceptedSubList].isSticky = isSticky;
			topicNameFromThingAndAction(SubscriptionList[indexRejectedSubList].Topic, pThingName, action,
										SHADOW_REJECTED);
			ret_val = aws_iot_mqtt_subscribe(pMqttClient, SubscriptionList[indexRejectedSubList].Topic,
											 (uint16_t) strlen(SubscriptionList[indexRejectedSubList].Topic), QOS0,
											 AckStatusCallback, NULL);
			if(ret_val == SUCCESS) {
				SubscriptionList[indexRejectedSubList].count = 1;
				SubscriptionList[indexRejectedSubList].isSticky = isSticky;
				clearBothEntriesFromList = false;

				// wait for SUBSCRIBE_SETTLING_TIME seconds to let the subscription take effect
				init_timer(&subSettlingtimer);
				countdown_sec(&subSettlingtimer, SUBSCRIBE_SETTLING_TIME);
				while(!has_timer_expired(&subSettlingtimer));

			}
		}
	}

	if(clearBothEntriesFromList) {
		if(indexAcceptedSubList >= 0) {
			SubscriptionList[indexAcceptedSubList].isFree = true;
		}
		if(indexRejectedSubList >= 0) {
			SubscriptionList[indexRejectedSubList].isFree = true;
		}
		if(SubscriptionList[indexAcceptedSubList].count == 1) {
			aws_iot_mqtt_unsubscribe(pMqttClient, SubscriptionList[indexAcceptedSubList].Topic,
									 (uint16_t) strlen(SubscriptionList[indexAcceptedSubList].Topic));
		}
	}

	return ret_val;
}

void incrementSubscriptionCnt(const char *pThingName, ShadowActions_t action, bool isSticky) {
	char TemporaryTopicNameAccepted[MAX_SHADOW_TOPIC_LENGTH_BYTES];
	char TemporaryTopicNameRejected[MAX_SHADOW_TOPIC_LENGTH_BYTES];
	uint8_t i;
	topicNameFromThingAndAction(TemporaryTopicNameAccepted, pThingName, action, SHADOW_ACCEPTED);
	topicNameFromThingAndAction(TemporaryTopicNameRejected, pThingName, action, SHADOW_REJECTED);

	for(i = 0; i < MAX_TOPICS_AT_ANY_GIVEN_TIME; i++) {
		if(!SubscriptionList[i].isFree) {
			if((strcmp(TemporaryTopicNameAccepted, SubscriptionList[i].Topic) == 0)
			   || (strcmp(TemporaryTopicNameRejected, SubscriptionList[i].Topic) == 0)) {
				SubscriptionList[i].count++;
				SubscriptionList[i].isSticky = isSticky;
			}
		}
	}
}

IoT_Error_t publishToShadowAction(const char *pThingName, ShadowActions_t action, const char *pJsonDocumentToBeSent) {
	IoT_Error_t ret_val = SUCCESS;
	char TemporaryTopicName[MAX_SHADOW_TOPIC_LENGTH_BYTES];
	IoT_Publish_Message_Params msgParams;

	if(NULL == pThingName || NULL == pJsonDocumentToBeSent) {
		return NULL_VALUE_ERROR;
	}

	topicNameFromThingAndAction(TemporaryTopicName, pThingName, action, SHADOW_ACTION);

	msgParams.qos = QOS0;
	msgParams.isRetained = 0;
	msgParams.payloadLen = strlen(pJsonDocumentToBeSent);
	msgParams.payload = (char *) pJsonDocumentToBeSent;
	ret_val = aws_iot_mqtt_publish(pMqttClient, TemporaryTopicName, (uint16_t) strlen(TemporaryTopicName), &msgParams);

	return ret_val;
}

bool getNextFreeIndexOfAckWaitList(uint8_t *pIndex) {
	uint8_t i;
	bool rc = false;

	if(NULL == pIndex) {
		return false;
	}

	for(i = 0; i < MAX_ACKS_TO_COMEIN_AT_ANY_GIVEN_TIME; i++) {
		if(AckWaitList[i].isFree) {
			*pIndex = i;
			rc = true;
			break;
		}
	}

	return rc;
}

void addToAckWaitList(uint8_t indexAckWaitList, const char *pThingName, ShadowActions_t action,
					  const char *pExtractedClientToken, fpActionCallback_t callback, void *pCallbackContext,
					  uint32_t timeout_seconds) {
	AckWaitList[indexAckWaitList].callback = callback;
	strncpy(AckWaitList[indexAckWaitList].clientTokenID, pExtractedClientToken, MAX_SIZE_CLIENT_ID_WITH_SEQUENCE);
	strncpy(AckWaitList[indexAckWaitList].thingName, pThingName, MAX_SIZE_OF_THING_NAME);
	AckWaitList[indexAckWaitList].pCallbackContext = pCallbackContext;
	AckWaitList[indexAckWaitList].action = action;
	init_timer(&(AckWaitList[indexAckWaitList].timer));
	countdown_sec(&(AckWaitList[indexAckWaitList].timer), timeout_seconds);
	AckWaitList[indexAckWaitList].isFree = false;
}

void HandleExpiredResponseCallbacks(void) {
	uint8_t i;
	for(i = 0; i < MAX_ACKS_TO_COMEIN_AT_ANY_GIVEN_TIME; i++) {
		if(!AckWaitList[i].isFree) {
			if(has_timer_expired(&(AckWaitList[i].timer))) {
				if(AckWaitList[i].callback != NULL) {
					AckWaitList[i].callback(AckWaitList[i].thingName, AckWaitList[i].action, SHADOW_ACK_TIMEOUT,
											shadowRxBuf, AckWaitList[i].pCallbackContext);
				}
				AckWaitList[i].isFree = true;
				unsubscribeFromAcceptedAndRejected(i);
			}
		}
	}
}

static void shadow_delta_callback(AWS_IoT_Client *pClient, char *topicName,
								  uint16_t topicNameLen, IoT_Publish_Message_Params *params, void *pData) {
	int32_t tokenCount;
	uint32_t i = 0;
	void *pJsonHandler = NULL;
	int32_t DataPosition;
	uint32_t dataLength;
	uint32_t tempVersionNumber = 0;

	FUNC_ENTRY;

	IOT_UNUSED(pClient);
	IOT_UNUSED(topicName);
	IOT_UNUSED(topicNameLen);
	IOT_UNUSED(pData);

	if(params->payloadLen >= SHADOW_MAX_SIZE_OF_RX_BUFFER) {
		IOT_WARN("Payload larger than RX Buffer");
		return;
	}

	memcpy(shadowRxBuf, params->payload, params->payloadLen);
	shadowRxBuf[params->payloadLen] = '\0';    // jsmn_parse relies on a string

	if(!isJsonValidAndParse(shadowRxBuf, pJsonHandler, &tokenCount)) {
		IOT_WARN("Received JSON is not valid");
		return;
	}

	if(shadowDiscardOldDeltaFlag) {
		if(extractVersionNumber(shadowRxBuf, pJsonHandler, tokenCount, &tempVersionNumber)) {
			if(tempVersionNumber > shadowJsonVersionNum) {
				shadowJsonVersionNum = tempVersionNumber;
			} else {
				IOT_WARN("Old Delta Message received - Ignoring rx: %d local: %d", tempVersionNumber,
						 shadowJsonVersionNum);
				return;
			}
		}
	}

	for(i = 0; i < tokenTableIndex; i++) {
		if(!tokenTable[i].isFree) {
			if(isJsonKeyMatchingAndUpdateValue(shadowRxBuf, pJsonHandler, tokenCount,
											   (jsonStruct_t *) tokenTable[i].pStruct, &dataLength, &DataPosition)) {
				if(tokenTable[i].callback != NULL) {
					tokenTable[i].callback(shadowRxBuf + DataPosition, dataLength,
										   (jsonStruct_t *) tokenTable[i].pStruct);
				}
			}
		}
	}
}

#ifdef __cplusplus
}
#endif

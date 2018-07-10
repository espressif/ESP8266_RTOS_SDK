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

#ifndef SRC_SHADOW_AWS_IOT_SHADOW_RECORDS_H_
#define SRC_SHADOW_AWS_IOT_SHADOW_RECORDS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "aws_iot_shadow_interface.h"
#include "aws_iot_config.h"


extern uint32_t shadowJsonVersionNum;
extern bool shadowDiscardOldDeltaFlag;

extern char myThingName[MAX_SIZE_OF_THING_NAME];
extern uint16_t myThingNameLen;
extern char mqttClientID[MAX_SIZE_OF_UNIQUE_CLIENT_ID_BYTES];
extern uint16_t mqttClientIDLen;

void initializeRecords(AWS_IoT_Client *pClient);
bool isSubscriptionPresent(const char *pThingName, ShadowActions_t action);
IoT_Error_t subscribeToShadowActionAcks(const char *pThingName, ShadowActions_t action, bool isSticky);
void incrementSubscriptionCnt(const char *pThingName, ShadowActions_t action, bool isSticky);

IoT_Error_t publishToShadowAction(const char *pThingName, ShadowActions_t action, const char *pJsonDocumentToBeSent);
void addToAckWaitList(uint8_t indexAckWaitList, const char *pThingName, ShadowActions_t action,
					  const char *pExtractedClientToken, fpActionCallback_t callback, void *pCallbackContext,
					  uint32_t timeout_seconds);
bool getNextFreeIndexOfAckWaitList(uint8_t *pIndex);
void HandleExpiredResponseCallbacks(void);
void initDeltaTokens(void);
IoT_Error_t registerJsonTokenOnDelta(jsonStruct_t *pStruct);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SHADOW_AWS_IOT_SHADOW_RECORDS_H_ */

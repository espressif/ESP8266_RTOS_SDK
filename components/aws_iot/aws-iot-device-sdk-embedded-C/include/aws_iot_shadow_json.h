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
#ifndef AWS_IOT_SDK_SRC_IOT_SHADOW_JSON_H_
#define AWS_IOT_SDK_SRC_IOT_SHADOW_JSON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include "aws_iot_error.h"
#include "aws_iot_shadow_json_data.h"

bool isJsonValidAndParse(const char *pJsonDocument, void *pJsonHandler, int32_t *pTokenCount);

bool isJsonKeyMatchingAndUpdateValue(const char *pJsonDocument, void *pJsonHandler, int32_t tokenCount,
									 jsonStruct_t *pDataStruct, uint32_t *pDataLength, int32_t *pDataPosition);

void aws_iot_shadow_internal_get_request_json(char *pJsonDocument);

void aws_iot_shadow_internal_delete_request_json(char *pJsonDocument);

void resetClientTokenSequenceNum(void);


bool isReceivedJsonValid(const char *pJsonDocument);

void FillWithClientToken(char *pStringToUpdateClientToken);

bool extractClientToken(const char *pJsonDocumentToBeSent, char *pExtractedClientToken);

bool extractVersionNumber(const char *pJsonDocument, void *pJsonHandler, int32_t tokenCount, uint32_t *pVersionNumber);

#ifdef __cplusplus
}
#endif

#endif // AWS_IOT_SDK_SRC_IOT_SHADOW_JSON_H_

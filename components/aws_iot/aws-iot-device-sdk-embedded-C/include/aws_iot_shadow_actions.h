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

#ifndef SRC_SHADOW_AWS_IOT_SHADOW_ACTIONS_H_
#define SRC_SHADOW_AWS_IOT_SHADOW_ACTIONS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "aws_iot_shadow_interface.h"

IoT_Error_t aws_iot_shadow_internal_action(const char *pThingName, ShadowActions_t action,
										   const char *pJsonDocumentToBeSent, fpActionCallback_t callback,
										   void *pCallbackContext, uint32_t timeout_seconds, bool isSticky);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SHADOW_AWS_IOT_SHADOW_ACTIONS_H_ */

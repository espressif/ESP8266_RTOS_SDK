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
 * @file aws_iot_tests_unit_shadow_helper.h
 * @brief IoT Client Unit Testing - Shadow Helper functions
 */

#ifndef IOT_TESTS_UNIT_SHADOW_HELPER_FUNCTIONS_H_
#define IOT_TESTS_UNIT_SHADOW_HELPER_FUNCTIONS_H_

#define AWS_THINGS_TOPIC "$aws/things/"
#define SHADOW_TOPIC "/shadow/"
#define ACCEPTED_TOPIC "/accepted"
#define REJECTED_TOPIC "/rejected"
#define UPDATE_TOPIC "update"
#define GET_TOPIC "get"
#define DELETE_TOPIC "delete"


#define GET_ACCEPTED_TOPIC AWS_THINGS_TOPIC AWS_IOT_MY_THING_NAME SHADOW_TOPIC GET_TOPIC ACCEPTED_TOPIC
#define GET_REJECTED_TOPIC AWS_THINGS_TOPIC AWS_IOT_MY_THING_NAME SHADOW_TOPIC GET_TOPIC REJECTED_TOPIC
#define GET_PUB_TOPIC AWS_THINGS_TOPIC AWS_IOT_MY_THING_NAME SHADOW_TOPIC GET_TOPIC

#define DELETE_ACCEPTED_TOPIC AWS_THINGS_TOPIC AWS_IOT_MY_THING_NAME SHADOW_TOPIC DELETE_TOPIC ACCEPTED_TOPIC
#define DELETE_REJECTED_TOPIC AWS_THINGS_TOPIC AWS_IOT_MY_THING_NAME SHADOW_TOPIC DELETE_TOPIC REJECTED_TOPIC

#define UPDATE_ACCEPTED_TOPIC AWS_THINGS_TOPIC AWS_IOT_MY_THING_NAME SHADOW_TOPIC UPDATE_TOPIC ACCEPTED_TOPIC
#define UPDATE_REJECTED_TOPIC AWS_THINGS_TOPIC AWS_IOT_MY_THING_NAME SHADOW_TOPIC UPDATE_TOPIC REJECTED_TOPIC

#endif /* IOT_TESTS_UNIT_SHADOW_HELPER_FUNCTIONS_H_ */

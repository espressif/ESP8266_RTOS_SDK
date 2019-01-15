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
 * @file aws_iot_tests_unit_shadow_action.cpp
 * @brief IoT Client Unit Testing - Shadow Action Tests
 */

#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestHarness_c.h>

TEST_GROUP_C(ShadowActionTests) {
	TEST_GROUP_C_SETUP_WRAPPER(ShadowActionTests)
	TEST_GROUP_C_TEARDOWN_WRAPPER(ShadowActionTests)
};

TEST_GROUP_C_WRAPPER(ShadowActionTests, GetTheFullJSONDocument)
TEST_GROUP_C_WRAPPER(ShadowActionTests, DeleteTheJSONDocument)
TEST_GROUP_C_WRAPPER(ShadowActionTests, UpdateTheJSONDocument)
TEST_GROUP_C_WRAPPER(ShadowActionTests, GetTheFullJSONDocumentTimeout)
TEST_GROUP_C_WRAPPER(ShadowActionTests, SubscribeToAcceptedRejectedOnGet)
TEST_GROUP_C_WRAPPER(ShadowActionTests, UnSubscribeToAcceptedRejectedOnGetResponse)
TEST_GROUP_C_WRAPPER(ShadowActionTests, UnSubscribeToAcceptedRejectedOnGetTimeout)
TEST_GROUP_C_WRAPPER(ShadowActionTests, UnSubscribeToAcceptedRejectedOnGetTimeoutWithSticky)
TEST_GROUP_C_WRAPPER(ShadowActionTests, WrongTokenInGetResponse)
TEST_GROUP_C_WRAPPER(ShadowActionTests, NoTokenInGetResponse)
TEST_GROUP_C_WRAPPER(ShadowActionTests, InvalidInboundJSONInGetResponse)
TEST_GROUP_C_WRAPPER(ShadowActionTests, AcceptedSubFailsGetRequest)
TEST_GROUP_C_WRAPPER(ShadowActionTests, RejectedSubFailsGetRequest)
TEST_GROUP_C_WRAPPER(ShadowActionTests, PublishFailsGetRequest)
TEST_GROUP_C_WRAPPER(ShadowActionTests, GetVersionFromAckStatus)
TEST_GROUP_C_WRAPPER(ShadowActionTests, StickyNonStickyNeverConflict)
TEST_GROUP_C_WRAPPER(ShadowActionTests, ACKWaitingMoreThanAllowed)
TEST_GROUP_C_WRAPPER(ShadowActionTests, InboundDataTooBigForBuffer)
TEST_GROUP_C_WRAPPER(ShadowActionTests, NoClientTokenForShadowAction)
TEST_GROUP_C_WRAPPER(ShadowActionTests, NoCallbackForShadowAction)

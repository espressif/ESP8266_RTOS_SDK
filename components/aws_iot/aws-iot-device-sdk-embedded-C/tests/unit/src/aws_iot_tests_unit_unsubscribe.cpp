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
 * @file aws_iot_tests_unit_unsubscribe.cpp
 * @brief IoT Client Unit Testing - Unsubscribe API Tests
 */

#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestHarness_c.h>

TEST_GROUP_C(UnsubscribeTests){
	TEST_GROUP_C_SETUP_WRAPPER(UnsubscribeTests)
	TEST_GROUP_C_TEARDOWN_WRAPPER(UnsubscribeTests)
};

/* D:1 - Unsubscribe with Null/empty client instance */
TEST_GROUP_C_WRAPPER(UnsubscribeTests, UnsubscribeNullClient)
/* D:2 - Unsubscribe with Null/empty topic name */
TEST_GROUP_C_WRAPPER(UnsubscribeTests, UnsubscribeNullTopic)
/* D:3 - Unsubscribe, Not subscribed to topic */
TEST_GROUP_C_WRAPPER(UnsubscribeTests, UnsubscribeNotSubscribed)

/* D:4 - Unsubscribe, QoS0, No response, timeout */
TEST_GROUP_C_WRAPPER(UnsubscribeTests, unsubscribeQoS0FailureOnNoUnsuback)
/* D:5 - Unsubscribe, QoS1, No response, timeout */
TEST_GROUP_C_WRAPPER(UnsubscribeTests, unsubscribeQoS1FailureOnNoUnsuback)

/* D:6 - Unsubscribe, QoS0, success */
TEST_GROUP_C_WRAPPER(UnsubscribeTests, unsubscribeQoS0WithUnsubackSuccess)
/* D:7 - Unsubscribe, QoS0, half command timeout delayed unsuback, success */
TEST_GROUP_C_WRAPPER(UnsubscribeTests, unsubscribeQoS0WithDelayedUnsubackSuccess)
/* D:8 - Unsubscribe, QoS1, success */
TEST_GROUP_C_WRAPPER(UnsubscribeTests, unsubscribeQoS1WithUnsubackSuccess)
/* D:9 - Unsubscribe, QoS1, half command timeout delayed unsuback, success */
TEST_GROUP_C_WRAPPER(UnsubscribeTests, unsubscribeQoS1WithDelayedUnsubackSuccess)

/* D:10 - Unsubscribe, success, message on topic ignored */
TEST_GROUP_C_WRAPPER(UnsubscribeTests, MsgAfterUnsubscribe)
/* D:11 - Unsubscribe after max topics reached */
TEST_GROUP_C_WRAPPER(UnsubscribeTests, MaxTopicsSubscription)
/* D:12 - Repeated Subscribe and Unsubscribe */
TEST_GROUP_C_WRAPPER(UnsubscribeTests, RepeatedSubUnSub)

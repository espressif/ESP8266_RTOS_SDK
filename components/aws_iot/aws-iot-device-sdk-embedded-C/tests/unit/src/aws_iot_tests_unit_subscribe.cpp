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
 * @file aws_iot_tests_unit_subscribe.cpp
 * @brief IoT Client Unit Testing - Subscribe API Tests
 */

#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestHarness_c.h>

TEST_GROUP_C(SubscribeTests){
	TEST_GROUP_C_SETUP_WRAPPER(SubscribeTests)
	TEST_GROUP_C_TEARDOWN_WRAPPER(SubscribeTests)
};

/* C:1 - Subscribe with Null/empty Client Instance */
TEST_GROUP_C_WRAPPER(SubscribeTests, SubscribeNullClient)
/* C:2 - Subscribe with Null/empty Topic Name */
TEST_GROUP_C_WRAPPER(SubscribeTests, SubscribeNullTopic)
/* C:3 - Subscribe with Null client callback */
TEST_GROUP_C_WRAPPER(SubscribeTests, SubscribeNullSubscribeHandler)
/* C:4 - Subscribe with Null client callback data */
TEST_GROUP_C_WRAPPER(SubscribeTests, SubscribeNullSubscribeHandlerData)
/* C:5 - Subscribe with no connection */
TEST_GROUP_C_WRAPPER(SubscribeTests, SubscribeNoConnection)
/* C:6 - Subscribe QoS2, error */
/* Not required, QoS enum doesn't have value for QoS2 */

/* C:7 - Subscribe attempt, QoS0, no response timeout */
TEST_GROUP_C_WRAPPER(SubscribeTests, subscribeQoS0FailureOnNoSuback)
/* C:8 - Subscribe attempt, QoS1, no response timeout */
TEST_GROUP_C_WRAPPER(SubscribeTests, subscribeQoS1FailureOnNoSuback)

/* C:9 - Subscribe QoS0 success, suback received */
TEST_GROUP_C_WRAPPER(SubscribeTests, subscribeQoS0Success)
/* C:10 - Subscribe QoS1 success, suback received */
TEST_GROUP_C_WRAPPER(SubscribeTests, subscribeQoS1Success)

/* C:11 - Subscribe, QoS0, delayed suback, success */
TEST_GROUP_C_WRAPPER(SubscribeTests, subscribeQoS0WithDelayedSubackSuccess)
/* C:12 - Subscribe, QoS1, delayed suback, success */
TEST_GROUP_C_WRAPPER(SubscribeTests, subscribeQoS1WithDelayedSubackSuccess)

/* C:13 - Subscribe QoS0 success, no puback sent on message */
TEST_GROUP_C_WRAPPER(SubscribeTests, subscribeQoS0MsgReceivedAndNoPubackSent)
/* C:14 - Subscribe QoS1 success, puback sent on message */
TEST_GROUP_C_WRAPPER(SubscribeTests, subscribeQoS1MsgReceivedAndSendPuback)

/* C:15 - Subscribe, malformed response */
TEST_GROUP_C_WRAPPER(SubscribeTests, subscribeMalformedResponse)

/* C:16 - Subscribe, multiple topics, messages on each topic */
TEST_GROUP_C_WRAPPER(SubscribeTests, SubscribeToMultipleTopicsSuccess)
/* C:17 - Subscribe, max topics, messages on each topic */
TEST_GROUP_C_WRAPPER(SubscribeTests, SubcribeToMaxAllowedTopicsSuccess)
/* C:18 - Subscribe, max topics, another subscribe */
TEST_GROUP_C_WRAPPER(SubscribeTests, SubcribeToMaxPlusOneAllowedTopicsFailure)

/* C:19 - Subscribe, '#' not last character in topic name, Failure */
TEST_GROUP_C_WRAPPER(SubscribeTests, subscribeTopicWithHashkeyAllSubTopicSuccess)
/* C:20 - Subscribe with '#', subscribed to all subtopics */
TEST_GROUP_C_WRAPPER(SubscribeTests, subscribeTopicHashkeyMustBeTheLastFail)
/* C:21 - Subscribe with '+' as wildcard success */
TEST_GROUP_C_WRAPPER(SubscribeTests, subscribeTopicWithPluskeySuccess)
/* C:22 - Subscribe with '+' as last character in topic name, Success */
TEST_GROUP_C_WRAPPER(SubscribeTests, subscribeTopicPluskeyComesLastSuccess)

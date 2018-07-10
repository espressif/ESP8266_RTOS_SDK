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
 * @file aws_iot_tests_unit_publish.cpp
 * @brief IoT Client Unit Testing - Publish API Tests
 */

#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestHarness_c.h>

TEST_GROUP_C(PublishTests) {
	TEST_GROUP_C_SETUP_WRAPPER(PublishTests)
	TEST_GROUP_C_TEARDOWN_WRAPPER(PublishTests)
};

/* E:1 - Publish with Null/empty client instance */
TEST_GROUP_C_WRAPPER(PublishTests, PublishNullClient)
/* E:2 - Publish with Null/empty Topic Name */
TEST_GROUP_C_WRAPPER(PublishTests, PublishNullTopic)
/* E:3 - Publish with Null/empty payload */
TEST_GROUP_C_WRAPPER(PublishTests, PublishNullParams)
/* E:4 - Publish with network disconnected */
TEST_GROUP_C_WRAPPER(PublishTests, PublishNetworkDisconnected)
/* E:5 - Publish with QoS2 */
/* Not required here, enum value doesn't exist for QoS2 */
/* E:6 - Publish with QoS1 send success, Puback not received */
TEST_GROUP_C_WRAPPER(PublishTests, publishQoS1FailureToReceivePuback)
/* E:7 - Publish with QoS1 send success, Delayed Puback received after command timeout */
TEST_GROUP_C_WRAPPER(PublishTests, publishQoS1FailureDelayedPuback)
/* E:8 - Publish with send success, Delayed Puback received before command timeout */
TEST_GROUP_C_WRAPPER(PublishTests, publishQoS1Success10msDelayedPuback)
/* E:9 - Publish QoS0 success */
TEST_GROUP_C_WRAPPER(PublishTests, publishQoS0NoPubackSuccess)
/* E:10 - Publish with QoS1 send success, Puback received */
TEST_GROUP_C_WRAPPER(PublishTests, publishQoS1Success)

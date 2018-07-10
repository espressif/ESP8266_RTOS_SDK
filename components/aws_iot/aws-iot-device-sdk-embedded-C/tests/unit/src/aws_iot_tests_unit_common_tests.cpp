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
 * @file aws_iot_tests_unit_common_tests.cpp
 * @brief IoT Client Unit Testing - Common Tests
 */

#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestHarness_c.h>

TEST_GROUP_C(CommonTests){
	TEST_GROUP_C_SETUP_WRAPPER(CommonTests)
	TEST_GROUP_C_TEARDOWN_WRAPPER(CommonTests)
};

TEST_GROUP_C_WRAPPER(CommonTests, NullClientGetState)
TEST_GROUP_C_WRAPPER(CommonTests, NullClientSetAutoreconnect)

TEST_GROUP_C_WRAPPER(CommonTests, UnexpectedAckFiltering)
TEST_GROUP_C_WRAPPER(CommonTests, BigMQTTRxMessageIgnore)
TEST_GROUP_C_WRAPPER(CommonTests, BigMQTTRxMessageReadNextMessage)

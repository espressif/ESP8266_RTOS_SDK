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
 * @file aws_iot_tests_unit_disconnect.cpp
 * @brief IoT Client Unit Testing - Disconnect API Tests
 */

#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestHarness_c.h>

TEST_GROUP_C(DisconnectTests){
		TEST_GROUP_C_SETUP_WRAPPER(DisconnectTests)
		TEST_GROUP_C_TEARDOWN_WRAPPER(DisconnectTests)
};

/* F:1 - Disconnect with Null/empty client instance */
TEST_GROUP_C_WRAPPER(DisconnectTests, NullClientDisconnect)
/* F:2 - Set Disconnect Handler with Null/empty Client */
TEST_GROUP_C_WRAPPER(DisconnectTests, NullClientSetDisconnectHandler)
/* F:3 - Call Set Disconnect handler with Null handler */
TEST_GROUP_C_WRAPPER(DisconnectTests, SetDisconnectHandlerNullHandler)
/* F:4 - Disconnect attempt, not connected */
TEST_GROUP_C_WRAPPER(DisconnectTests, disconnectNotConnected)
/* F:5 - Disconnect success */
TEST_GROUP_C_WRAPPER(DisconnectTests, disconnectNoAckSuccess)
/* F:6 - Disconnect, Handler invoked on disconnect */
TEST_GROUP_C_WRAPPER(DisconnectTests, HandlerInvokedOnDisconnect)
/* F:7 - Disconnect, with set handler and invoked on disconnect */
TEST_GROUP_C_WRAPPER(DisconnectTests, SetHandlerAndInvokedOnDisconnect)

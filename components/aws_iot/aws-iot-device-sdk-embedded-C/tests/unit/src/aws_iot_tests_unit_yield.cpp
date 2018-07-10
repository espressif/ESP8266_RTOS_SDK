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
 * @file aws_iot_tests_unit_yield.cpp
 * @brief IoT Client Unit Testing - Yield API Tests
 */

#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestHarness_c.h>

TEST_GROUP_C(YieldTests){
	TEST_GROUP_C_SETUP_WRAPPER(YieldTests)
	TEST_GROUP_C_TEARDOWN_WRAPPER(YieldTests)
};

/* G:1 - Yield with Null/empty Client Instance */
TEST_GROUP_C_WRAPPER(YieldTests, NullClientYield)
/* G:2 - Yield with zero yield timeout */
TEST_GROUP_C_WRAPPER(YieldTests, ZeroTimeoutYield)
/* G:3 - Yield, network disconnected, never connected */
TEST_GROUP_C_WRAPPER(YieldTests, YieldNetworkDisconnectedNeverConnected)
/* G:4 - Yield, network disconnected, disconnected manually */
TEST_GROUP_C_WRAPPER(YieldTests, YieldNetworkDisconnectedDisconnectedManually)
/* G:5 - Yield, network connected, yield called while in subscribe application callback */
TEST_GROUP_C_WRAPPER(YieldTests, YieldInSubscribeCallback)
/* G:6 - Yield, network disconnected, ping timeout, auto-reconnect disabled */
TEST_GROUP_C_WRAPPER(YieldTests, disconnectNoAutoReconnect)

/* G:7 - Yield, network connected, no incoming messages */
TEST_GROUP_C_WRAPPER(YieldTests, YieldSuccessNoMessages)
/* G:8 - Yield, network connected, ping request/response */
TEST_GROUP_C_WRAPPER(YieldTests, PingRequestPingResponse)

/* G:9 - Yield, disconnected, Auto-reconnect timed-out */
TEST_GROUP_C_WRAPPER(YieldTests, disconnectAutoReconnectTimeout)
/* G:10 - Yield, disconnected, Auto-reconnect successful */
TEST_GROUP_C_WRAPPER(YieldTests, disconnectAutoReconnectSuccess)
/* G:11 - Yield, disconnected, Manual reconnect */
TEST_GROUP_C_WRAPPER(YieldTests, disconnectManualAutoReconnect)
/* G:12 - Yield, resubscribe to all topics on reconnect */
TEST_GROUP_C_WRAPPER(YieldTests, resubscribeSuccessfulReconnect)

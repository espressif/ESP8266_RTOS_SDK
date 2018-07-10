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
 * @file aws_iot_tests_unit_connect.cpp
 * @brief IoT Client Unit Testing - Connect API Tests
 */

#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestHarness_c.h>

TEST_GROUP_C(ConnectTests){
	TEST_GROUP_C_SETUP_WRAPPER(ConnectTests)
	TEST_GROUP_C_TEARDOWN_WRAPPER(ConnectTests)
};

/* B:1 - Init with Null/empty client instance */
TEST_GROUP_C_WRAPPER(ConnectTests, NullClientInit)
/* B:2 - Connect with Null/empty client instance */
TEST_GROUP_C_WRAPPER(ConnectTests, NullClientConnect)
/* B:3 - Connect with Null/Empty endpoint */
TEST_GROUP_C_WRAPPER(ConnectTests, NullHost)
/* B:4 - Connect with Null/Empty port */
TEST_GROUP_C_WRAPPER(ConnectTests, NullPort)
/* B:5 - Connect with Null/Empty root CA path */
TEST_GROUP_C_WRAPPER(ConnectTests, NullRootCAPath)
/* B:6 - Connect with Null/Empty Client certificate path */
TEST_GROUP_C_WRAPPER(ConnectTests, NullClientCertificate)
/* B:7 - Connect with Null/Empty private key Path */
TEST_GROUP_C_WRAPPER(ConnectTests, NullPrivateKeyPath)
/* B:8 - Connect with Null/Empty client ID */
TEST_GROUP_C_WRAPPER(ConnectTests, NullClientID)
/* B:9 - Connect with invalid Endpoint */
TEST_GROUP_C_WRAPPER(ConnectTests, InvalidEndpoint)
/* B:10 - Connect with invalid correct endpoint but invalid port */
TEST_GROUP_C_WRAPPER(ConnectTests, InvalidPort)
/* B:11 - Connect with invalid Root CA path */
TEST_GROUP_C_WRAPPER(ConnectTests, InvalidRootCAPath)
/* B:12 - Connect with invalid Client certificate path */
TEST_GROUP_C_WRAPPER(ConnectTests, InvalidClientCertPath)
/* B:13 - Connect with invalid private key path */
TEST_GROUP_C_WRAPPER(ConnectTests, InvalidPrivateKeyPath)
/* B:14 - Connect, no response timeout */
TEST_GROUP_C_WRAPPER(ConnectTests, NoResponseTimeout)
/* B:15 - Connect, connack malformed, too large */
TEST_GROUP_C_WRAPPER(ConnectTests, ConnackTooLarge)
/* B:16 - Connect, connack malformed, fixed header corrupted */
TEST_GROUP_C_WRAPPER(ConnectTests, FixedHeaderCorrupted)
/* B:17 - Connect, connack malformed, invalid remaining length */
TEST_GROUP_C_WRAPPER(ConnectTests, InvalidRemainingLength)
/* B:18 - Connect, connack returned error, unacceptable protocol version */
TEST_GROUP_C_WRAPPER(ConnectTests, UnacceptableProtocolVersion)
/* B:19 - Connect, connack returned error, identifier rejected */
TEST_GROUP_C_WRAPPER(ConnectTests, IndentifierRejected)
/* B:20 - Connect, connack returned error, Server unavailable */
TEST_GROUP_C_WRAPPER(ConnectTests, ServerUnavailable)
/* B:21 - Connect, connack returned error, bad user name or password */
TEST_GROUP_C_WRAPPER(ConnectTests, BadUserNameOrPassword)
/* B:22 - Connect, connack returned error, not authorized */
TEST_GROUP_C_WRAPPER(ConnectTests, NotAuthorized)
/* B:23 - Connect, connack return after half command timeout delay, success */
TEST_GROUP_C_WRAPPER(ConnectTests, SuccessAfterDelayedConnack)
/* B:24 - Connect, connack returned success */
TEST_GROUP_C_WRAPPER(ConnectTests, ConnectSuccess)
/* B:25 - Connect, flag settings and parameters are recorded in buffer */
TEST_GROUP_C_WRAPPER(ConnectTests, FlagSettingsAndParamsAreRecordedIntoBuf)
/* B:26 - Connect attempt, Disconnect, Manually reconnect */
TEST_GROUP_C_WRAPPER(ConnectTests, ConnectDisconnectConnect)
/* B:27 - Connect attempt, Clean session, Subscribe */
TEST_GROUP_C_WRAPPER(ConnectTests, cleanSessionInitSubscribers)
/* B:28 - Connect attempt, power cycle with clean session false */
TEST_GROUP_C_WRAPPER(ConnectTests, PowerCycleWithCleanSessionFalse)

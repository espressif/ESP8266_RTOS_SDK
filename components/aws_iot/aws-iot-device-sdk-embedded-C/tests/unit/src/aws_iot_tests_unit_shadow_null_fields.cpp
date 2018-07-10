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
 * @file aws_iot_tests_unit_shadow_null_fields.cpp
 * @brief IoT Client Unit Testing - Shadow APIs NULL field Tests
 */

#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestHarness_c.h>

TEST_GROUP_C(ShadowNullFields){
	TEST_GROUP_C_SETUP_WRAPPER(ShadowNullFields)
	TEST_GROUP_C_TEARDOWN_WRAPPER(ShadowNullFields)
};

TEST_GROUP_C_WRAPPER(ShadowNullFields, NullClientInit)
TEST_GROUP_C_WRAPPER(ShadowNullFields, NullClientConnect)
TEST_GROUP_C_WRAPPER(ShadowNullFields, NullHost)
TEST_GROUP_C_WRAPPER(ShadowNullFields, NullPort)
TEST_GROUP_C_WRAPPER(ShadowNullFields, NullClientID)
TEST_GROUP_C_WRAPPER(ShadowNullFields, NullUpdateDocument)
TEST_GROUP_C_WRAPPER(ShadowNullFields, NullClientYield)
TEST_GROUP_C_WRAPPER(ShadowNullFields, NullClientDisconnect)
TEST_GROUP_C_WRAPPER(ShadowNullFields, NullClientShadowGet)
TEST_GROUP_C_WRAPPER(ShadowNullFields, NullClientShadowUpdate)
TEST_GROUP_C_WRAPPER(ShadowNullFields, NullClientShadowDelete)
TEST_GROUP_C_WRAPPER(ShadowNullFields, NullClientSetAutoreconnect)

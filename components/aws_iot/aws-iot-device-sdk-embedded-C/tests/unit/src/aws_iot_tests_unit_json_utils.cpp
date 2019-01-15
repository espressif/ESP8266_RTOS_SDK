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
 * @file aws_iot_tests_unit_json_utils.cpp
 * @brief IoT Client Unit Testing - JSON Utility Tests
 */

#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestHarness_c.h>

TEST_GROUP_C(JsonUtils) {
  TEST_GROUP_C_SETUP_WRAPPER(JsonUtils)
  TEST_GROUP_C_TEARDOWN_WRAPPER(JsonUtils)
};

TEST_GROUP_C_WRAPPER(JsonUtils, ParseStringBasic)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseStringLongerStringIsValid)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseStringEmptyStringIsValid)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseStringErrorOnInteger)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseStringErrorOnBoolean)

TEST_GROUP_C_WRAPPER(JsonUtils, ParseBooleanTrue)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseBooleanFalse)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseBooleanErrorOnString)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseBooleanErrorOnInvalidJson)

TEST_GROUP_C_WRAPPER(JsonUtils, ParseDoubleBasic)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseDoubleNumberWithNoDecimal)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseDoubleSmallDouble)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseDoubleErrorOnString)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseDoubleErrorOnBoolean)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseDoubleErrorOnJsonObject)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseDoubleNegativeNumber)

TEST_GROUP_C_WRAPPER(JsonUtils, ParseFloatBasic)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseFloatNumberWithNoDecimal)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseFloatSmallFloat)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseFloatErrorOnString)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseFloatErrorOnBoolean)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseFloatErrorOnJsonObject)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseFloatNegativeNumber)

TEST_GROUP_C_WRAPPER(JsonUtils, ParseIntegerBasic)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseIntegerLargeInteger)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseIntegerNegativeInteger)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseIntegerErrorOnBoolean)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseIntegerErrorOnString)

TEST_GROUP_C_WRAPPER(JsonUtils, ParseInteger16bitBasic)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseInteger16bitLargeInteger)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseInteger16bitNegativeInteger)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseInteger16bitErrorOnBoolean)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseInteger16bitErrorOnString)

TEST_GROUP_C_WRAPPER(JsonUtils, ParseInteger8bitBasic)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseInteger8bitLargeInteger)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseInteger8bitNegativeInteger)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseInteger8bitErrorOnBoolean)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseInteger8bitErrorOnString)

TEST_GROUP_C_WRAPPER(JsonUtils, ParseUnsignedIntegerBasic)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseUnsignedIntegerLargeInteger)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseUnsignedIntegerErrorOnNegativeInteger)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseUnsignedIntegerErrorOnBoolean)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseUnsignedIntegerErrorOnString)

TEST_GROUP_C_WRAPPER(JsonUtils, ParseUnsignedInteger16bitBasic)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseUnsignedInteger16bitLargeInteger)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseUnsignedInteger16bitErrorOnNegativeInteger)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseUnsignedInteger16bitErrorOnBoolean)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseUnsignedInteger16bitErrorOnString)

TEST_GROUP_C_WRAPPER(JsonUtils, ParseUnsignedInteger8bitBasic)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseUnsignedInteger8bitLargeInteger)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseUnsignedInteger8bitErrorOnNegativeInteger)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseUnsignedInteger8bitErrorOnBoolean)
TEST_GROUP_C_WRAPPER(JsonUtils, ParseUnsignedInteger8bitErrorOnString)

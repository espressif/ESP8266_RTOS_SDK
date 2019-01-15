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
 * @file aws_iot_tests_unit_json_utils_helper.c
 * @brief IoT Client Unit Testing - JSON Utility Tests helper
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <CppUTest/TestHarness_c.h>

#include "aws_iot_json_utils.h"
#include "aws_iot_tests_unit_helper_functions.h"
#include "aws_iot_log.h"

static IoT_Error_t rc = SUCCESS;

static jsmn_parser test_parser;
static jsmntok_t t[128];

TEST_GROUP_C_SETUP(JsonUtils) {
	jsmn_init(&test_parser);
}

TEST_GROUP_C_TEARDOWN(JsonUtils) {

}

TEST_C(JsonUtils, ParseStringBasic) {
	int r;
	const char *json = "{\"x\":\"test1\"}";
	char parsedString[50];

	IOT_DEBUG("\n-->Running Json Utils Tests -  Basic String Parsing \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseStringValue(parsedString, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_STRING("test1", parsedString);
}

TEST_C(JsonUtils, ParseStringLongerStringIsValid) {
	int r;
	const char *json = "{\"x\":\"this is a longer string for test 2\"}";
	char parsedString[50];

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse long string \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseStringValue(parsedString, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_STRING("this is a longer string for test 2", parsedString);
}

TEST_C(JsonUtils, ParseStringEmptyStringIsValid) {	int r;
	const char *json = "{\"x\":\"\"}";
	char parsedString[50];

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse empty string \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseStringValue(parsedString, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_STRING("", parsedString);
}

TEST_C(JsonUtils, ParseStringErrorOnInteger) {
	int r;
	const char *json = "{\"x\":3}";
	char parsedString[50];

	IOT_DEBUG("\n-->Running Json Utils Tests - parse integer as string returns error \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseStringValue(parsedString, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(JSON_PARSE_ERROR, rc);
}

TEST_C(JsonUtils, ParseStringErrorOnBoolean) {
	int r;
	const char *json = "{\"x\":true}";
	char parsedString[50];

	IOT_DEBUG("\n-->Running Json Utils Tests - parse boolean as string returns error \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseStringValue(parsedString, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(JSON_PARSE_ERROR, rc);
}

TEST_C(JsonUtils, ParseBooleanTrue) {
	int r;
	const char *json = "{\"x\":true}";
	bool parsedBool;

	IOT_DEBUG("\n-->Running Json Utils Tests - parse boolean with true value \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseBooleanValue(&parsedBool, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_INT(1, (int) parsedBool);
}

TEST_C(JsonUtils, ParseBooleanFalse) {
	int r;
	const char *json = "{\"x\":false}";
	bool parsedBool;

	IOT_DEBUG("\n-->Running Json Utils Tests - parse boolean with false value \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseBooleanValue(&parsedBool, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_INT(0, (int) parsedBool);
}

TEST_C(JsonUtils, ParseBooleanErrorOnString) {
	int r;
	const char *json = "{\"x\":\"not a bool\"}";
	bool parsedBool;

	IOT_DEBUG("\n-->Running Json Utils Tests - parse string as boolean returns error \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseBooleanValue(&parsedBool, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(JSON_PARSE_ERROR, rc);
}

TEST_C(JsonUtils, ParseBooleanErrorOnInvalidJson) {
	int r;
	const char *json = "{\"x\":frue}"; // Invalid
	bool parsedBool;

	IOT_DEBUG("\n-->Running Json Utils Tests - parse boolean returns error with invalid json \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseBooleanValue(&parsedBool, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(JSON_PARSE_ERROR, rc);
}

TEST_C(JsonUtils, ParseDoubleBasic) {
	int r;
	const char *json = "{\"x\":20.5}";
	double parsedDouble;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse double test \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseDoubleValue(&parsedDouble, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_REAL(20.5, parsedDouble, 0.0);
}

TEST_C(JsonUtils, ParseDoubleNumberWithNoDecimal) {
	int r;
	const char *json = "{\"x\":100}";
	double parsedDouble;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse double number with no decimal \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseDoubleValue(&parsedDouble, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_REAL(100, parsedDouble, 0.0);
}

TEST_C(JsonUtils, ParseDoubleSmallDouble) {
	int r;
	const char *json = "{\"x\":0.000004}";
	double parsedDouble;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse small double value \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseDoubleValue(&parsedDouble, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_REAL(0.000004, parsedDouble, 0.0);
}

TEST_C(JsonUtils, ParseDoubleErrorOnString) {
	int r;
	const char *json = "{\"x\":\"20.5\"}";
	double parsedDouble;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse string as double returns error \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseDoubleValue(&parsedDouble, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(JSON_PARSE_ERROR, rc);
}

TEST_C(JsonUtils, ParseDoubleErrorOnBoolean) {
	int r;
	const char *json = "{\"x\":true}";
	double parsedDouble;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse boolean as double returns error \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseDoubleValue(&parsedDouble, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(JSON_PARSE_ERROR, rc);
}

TEST_C(JsonUtils, ParseDoubleErrorOnJsonObject) {
	int r;
	const char *json = "{\"x\":{}}";
	double parsedDouble;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse json object as double returns error \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseDoubleValue(&parsedDouble, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(JSON_PARSE_ERROR, rc);
}

TEST_C(JsonUtils, ParseDoubleNegativeNumber) {
	int r;
	const char *json = "{\"x\":-56.78}";
	double parsedDouble;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse negative double value \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseDoubleValue(&parsedDouble, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_REAL(-56.78, parsedDouble, 0.0);
}

TEST_C(JsonUtils, ParseFloatBasic) {
	int r;
	const char *json = "{\"x\":20.5}";
	float parsedFloat;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse float test \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseFloatValue(&parsedFloat, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_REAL(20.5, parsedFloat, 0.0);
}

TEST_C(JsonUtils, ParseFloatNumberWithNoDecimal) {
	int r;
	const char *json = "{\"x\":100}";
	float parsedFloat;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse float number with no decimal \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseFloatValue(&parsedFloat, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_REAL(100, parsedFloat, 0.0);
}

TEST_C(JsonUtils, ParseFloatSmallFloat) {
	int r;
	const char *json = "{\"x\":0.0004}";
	float parsedFloat;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse small float value \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseFloatValue(&parsedFloat, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_C(0.0004f == parsedFloat);
}

TEST_C(JsonUtils, ParseFloatErrorOnString) {
	int r;
	const char *json = "{\"x\":\"20.5\"}";
	float parsedFloat;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse string as float returns error \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseFloatValue(&parsedFloat, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(JSON_PARSE_ERROR, rc);
}

TEST_C(JsonUtils, ParseFloatErrorOnBoolean) {
	int r;
	const char *json = "{\"x\":true}";
	float parsedFloat;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse boolean as float returns error \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseFloatValue(&parsedFloat, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(JSON_PARSE_ERROR, rc);
}

TEST_C(JsonUtils, ParseFloatErrorOnJsonObject) {
	int r;
	const char *json = "{\"x\":{}}";
	float parsedDouble;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse json object as float returns error \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseFloatValue(&parsedDouble, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(JSON_PARSE_ERROR, rc);
}

TEST_C(JsonUtils, ParseFloatNegativeNumber) {
	int r;
	const char *json = "{\"x\":-56.78}";
	float parsedFloat;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse negative float value \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseFloatValue(&parsedFloat, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_C(-56.78f == parsedFloat);
}

TEST_C(JsonUtils, ParseIntegerBasic) {
	int r;
	const char *json = "{\"x\":1}";
	int32_t parsedInteger;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse 32 bit integer \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseInteger32Value(&parsedInteger, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_INT(1, parsedInteger);
}

TEST_C(JsonUtils, ParseIntegerLargeInteger) {
	int r;
	const char *json = "{\"x\":2147483647}";
	int32_t parsedInteger;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse large 32 bit integer \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseInteger32Value(&parsedInteger, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_INT(2147483647, parsedInteger);
}

TEST_C(JsonUtils, ParseIntegerNegativeInteger) {
	int r;
	const char *json = "{\"x\":-308}";
	int32_t parsedInteger;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse negative 32 bit integer \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseInteger32Value(&parsedInteger, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_INT(-308, parsedInteger);
}

TEST_C(JsonUtils, ParseIntegerErrorOnBoolean) {
	int r;
	const char *json = "{\"x\":true}";
	int32_t parsedInteger;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse 32 bit integer returns error with boolean \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseInteger32Value(&parsedInteger, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(JSON_PARSE_ERROR, rc);
}

TEST_C(JsonUtils, ParseIntegerErrorOnString) {
	int r;
	const char *json = "{\"x\":\"45\"}";
	int32_t parsedInteger;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse 32 bit integer returns error on string \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseInteger32Value(&parsedInteger, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(JSON_PARSE_ERROR, rc);
}

TEST_C(JsonUtils, ParseInteger16bitBasic) {
	int r;
	const char *json = "{\"x\":1}";
	int16_t parsedInteger;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse 16 bit integer \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseInteger16Value(&parsedInteger, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_INT(1, parsedInteger);
}

TEST_C(JsonUtils, ParseInteger16bitLargeInteger) {
	int r;
	const char *json = "{\"x\":32767}";
	int16_t parsedInteger;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse large 16 bit integer \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseInteger16Value(&parsedInteger, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_INT(32767, parsedInteger);
}

TEST_C(JsonUtils, ParseInteger16bitNegativeInteger) {
	int r;
	const char *json = "{\"x\":-308}";
	int16_t parsedInteger;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse negative 16 bit integer \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseInteger16Value(&parsedInteger, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_INT(-308, parsedInteger);
}

TEST_C(JsonUtils, ParseInteger16bitErrorOnBoolean) {
	int r;
	const char *json = "{\"x\":true}";
	int16_t parsedInteger;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse 16 bit integer returns error with boolean \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseInteger16Value(&parsedInteger, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(JSON_PARSE_ERROR, rc);
}

TEST_C(JsonUtils, ParseInteger16bitErrorOnString) {
	int r;
	const char *json = "{\"x\":\"45\"}";
	int16_t parsedInteger;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse 16 bit integer returns error on string \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseInteger16Value(&parsedInteger, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(JSON_PARSE_ERROR, rc);
}

TEST_C(JsonUtils, ParseInteger8bitBasic) {
	int r;
	const char *json = "{\"x\":1}";
	int8_t parsedInteger;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse 8 bit integer \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseInteger8Value(&parsedInteger, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_INT(1, parsedInteger);
}

TEST_C(JsonUtils, ParseInteger8bitLargeInteger) {
	int r;
	const char *json = "{\"x\":127}";
	int8_t parsedInteger;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse large 8 bit integer \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseInteger8Value(&parsedInteger, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_INT(127, parsedInteger);
}

TEST_C(JsonUtils, ParseInteger8bitNegativeInteger) {
	int r;
	const char *json = "{\"x\":-30}";
	int8_t parsedInteger;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse negative 8 bit integer \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseInteger8Value(&parsedInteger, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_INT(-30, parsedInteger);
}

TEST_C(JsonUtils, ParseInteger8bitErrorOnBoolean) {
	int r;
	const char *json = "{\"x\":true}";
	int8_t parsedInteger;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse 8 bit integer returns error with boolean \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseInteger8Value(&parsedInteger, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(JSON_PARSE_ERROR, rc);
}

TEST_C(JsonUtils, ParseInteger8bitErrorOnString) {
	int r;
	const char *json = "{\"x\":\"45\"}";
	int8_t parsedInteger;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse 8 bit integer returns error on string \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseInteger8Value(&parsedInteger, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(JSON_PARSE_ERROR, rc);
}

TEST_C(JsonUtils, ParseUnsignedIntegerBasic) {
	int r;
	const char *json = "{\"x\":1}";
	uint32_t parsedInteger;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse unsigned 32 bit integer \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseUnsignedInteger32Value(&parsedInteger, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_C(1 == parsedInteger);
}

TEST_C(JsonUtils, ParseUnsignedIntegerLargeInteger) {
	int r;
	const char *json = "{\"x\":2147483647}";
	uint32_t parsedInteger;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse large unsigned 32 bit integer \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseUnsignedInteger32Value(&parsedInteger, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_C(2147483647 == parsedInteger);
}

TEST_C(JsonUtils, ParseUnsignedIntegerErrorOnNegativeInteger) {
	int r;
	const char *json = "{\"x\":-308}";
	uint32_t parsedInteger;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse unsigned 32 bit integer returns error with negative value \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseUnsignedInteger32Value(&parsedInteger, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(JSON_PARSE_ERROR, rc);
}

TEST_C(JsonUtils, ParseUnsignedIntegerErrorOnBoolean) {
	int r;
	const char *json = "{\"x\":true}";
	uint32_t parsedInteger;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse unsigned 32 bit integer returns error with boolean \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseUnsignedInteger32Value(&parsedInteger, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(JSON_PARSE_ERROR, rc);
}

TEST_C(JsonUtils, ParseUnsignedIntegerErrorOnString) {
	int r;
	const char *json = "{\"x\":\"45\"}";
	uint32_t parsedInteger;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse unsigned 32 bit integer returns error on string \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseUnsignedInteger32Value(&parsedInteger, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(JSON_PARSE_ERROR, rc);
}

TEST_C(JsonUtils, ParseUnsignedInteger16bitBasic) {
	int r;
	const char *json = "{\"x\":1}";
	uint16_t parsedInteger;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse unsigned 16 bit integer \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseUnsignedInteger16Value(&parsedInteger, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_INT(1, parsedInteger);
}

TEST_C(JsonUtils, ParseUnsignedInteger16bitLargeInteger) {
	int r;
	const char *json = "{\"x\":65535}";
	uint16_t parsedInteger;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse large unsigned 16 bit integer \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseUnsignedInteger16Value(&parsedInteger, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_INT(65535, parsedInteger);
}

TEST_C(JsonUtils, ParseUnsignedInteger16bitErrorOnNegativeInteger) {
	int r;
	const char *json = "{\"x\":-308}";
	uint16_t parsedInteger;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse unsigned 16 bit integer returns error on negative value \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseUnsignedInteger16Value(&parsedInteger, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(JSON_PARSE_ERROR, rc);
}

TEST_C(JsonUtils, ParseUnsignedInteger16bitErrorOnBoolean) {
	int r;
	const char *json = "{\"x\":true}";
	uint16_t parsedInteger;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse unsigned 16 bit integer returns error with boolean \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseUnsignedInteger16Value(&parsedInteger, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(JSON_PARSE_ERROR, rc);
}

TEST_C(JsonUtils, ParseUnsignedInteger16bitErrorOnString) {
	int r;
	const char *json = "{\"x\":\"45\"}";
	uint16_t parsedInteger;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse unsigned 16 bit integer returns error on string \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseUnsignedInteger16Value(&parsedInteger, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(JSON_PARSE_ERROR, rc);
}

TEST_C(JsonUtils, ParseUnsignedInteger8bitBasic) {
	int r;
	const char *json = "{\"x\":1}";
	uint8_t parsedInteger;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse unsigned 8 bit integer \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseUnsignedInteger8Value(&parsedInteger, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_INT(1, parsedInteger);
}

TEST_C(JsonUtils, ParseUnsignedInteger8bitLargeInteger) {
	int r;
	const char *json = "{\"x\":255}";
	uint8_t parsedInteger;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse large unsigned 8 bit integer \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseUnsignedInteger8Value(&parsedInteger, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(SUCCESS, rc);
	CHECK_EQUAL_C_INT(255, parsedInteger);
}

TEST_C(JsonUtils, ParseUnsignedInteger8bitErrorOnNegativeInteger) {
	int r;
	const char *json = "{\"x\":-30}";
	uint8_t parsedInteger;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse unsigned 8 bit integer returns error on negative value \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseUnsignedInteger8Value(&parsedInteger, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(JSON_PARSE_ERROR, rc);
}

TEST_C(JsonUtils, ParseUnsignedInteger8bitErrorOnBoolean) {
	int r;
	const char *json = "{\"x\":true}";
	uint8_t parsedInteger;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse unsigned 8 bit integer returns error with boolean \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseUnsignedInteger8Value(&parsedInteger, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(JSON_PARSE_ERROR, rc);
}

TEST_C(JsonUtils, ParseUnsignedInteger8bitErrorOnString) {
	int r;
	const char *json = "{\"x\":\"45\"}";
	uint8_t parsedInteger;

	IOT_DEBUG("\n-->Running Json Utils Tests - Parse unsigned 8 bit integer returns error on string \n");

	r = jsmn_parse(&test_parser, json, strlen(json), t, sizeof(t) / sizeof(t[0]));
	rc = parseUnsignedInteger8Value(&parsedInteger, json, t + 2);

	CHECK_EQUAL_C_INT(3, r);
	CHECK_EQUAL_C_INT(JSON_PARSE_ERROR, rc);
}

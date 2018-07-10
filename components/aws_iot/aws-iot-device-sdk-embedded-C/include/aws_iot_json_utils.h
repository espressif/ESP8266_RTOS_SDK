/*
 * Copyright 2010-2015 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

/**
 * @file aws_json_utils.h
 * @brief Utilities for manipulating JSON
 *
 * json_utils provides JSON parsing utilities for use with the IoT SDK.
 * Underlying JSON parsing relies on the Jasmine JSON parser.
 *
 */

#ifndef AWS_IOT_SDK_SRC_JSON_UTILS_H_
#define AWS_IOT_SDK_SRC_JSON_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "aws_iot_error.h"
#include "jsmn.h"

// utility functions
/**
 * @brief          JSON Equality Check
 *
 * Given a token pointing to a particular JSON node and an
 * input string, check to see if the key is equal to the string.
 *
 * @param json      json string
 * @param tok     	json token - pointer to key to test for equality
 * @param s			input string for key to test equality
 *
 * @return         	0 if equal, 1 otherwise
 */
int8_t jsoneq(const char *json, jsmntok_t *tok, const char *s);

/**
 * @brief          Parse a signed 32-bit integer value from a JSON node.
 *
 * Given a JSON node parse the integer value from the value.
 *
 * @param jsonString	json string
 * @param tok     		json token - pointer to JSON node
 * @param i				address of int32_t to be updated
 *
 * @return         		SUCCESS - success
 * @return				JSON_PARSE_ERROR - error parsing value
 */
IoT_Error_t parseInteger32Value(int32_t *i, const char *jsonString, jsmntok_t *token);

/**
 * @brief          Parse a signed 16-bit integer value from a JSON node.
 *
 * Given a JSON node parse the integer value from the value.
 *
 * @param jsonString	json string
 * @param tok     		json token - pointer to JSON node
 * @param i				address of int16_t to be updated
 *
 * @return         		SUCCESS - success
 * @return				JSON_PARSE_ERROR - error parsing value
 */
IoT_Error_t parseInteger16Value(int16_t *i, const char *jsonString, jsmntok_t *token);

/**
 * @brief          Parse a signed 8-bit integer value from a JSON node.
 *
 * Given a JSON node parse the integer value from the value.
 *
 * @param jsonString	json string
 * @param tok     		json token - pointer to JSON node
 * @param i				address of int8_t to be updated
 *
 * @return         		SUCCESS - success
 * @return				JSON_PARSE_ERROR - error parsing value
 */
IoT_Error_t parseInteger8Value(int8_t *i, const char *jsonString, jsmntok_t *token);

/**
 * @brief          Parse an unsigned 32-bit integer value from a JSON node.
 *
 * Given a JSON node parse the integer value from the value.
 *
 * @param jsonString	json string
 * @param tok     		json token - pointer to JSON node
 * @param i				address of uint32_t to be updated
 *
 * @return         		SUCCESS - success
 * @return				JSON_PARSE_ERROR - error parsing value
 */
IoT_Error_t parseUnsignedInteger32Value(uint32_t *i, const char *jsonString, jsmntok_t *token);

/**
 * @brief          Parse an unsigned 16-bit integer value from a JSON node.
 *
 * Given a JSON node parse the integer value from the value.
 *
 * @param jsonString	json string
 * @param tok     		json token - pointer to JSON node
 * @param i				address of uint16_t to be updated
 *
 * @return         		SUCCESS - success
 * @return				JSON_PARSE_ERROR - error parsing value
 */
IoT_Error_t parseUnsignedInteger16Value(uint16_t *i, const char *jsonString, jsmntok_t *token);

/**
 * @brief          Parse an unsigned 8-bit integer value from a JSON node.
 *
 * Given a JSON node parse the integer value from the value.
 *
 * @param jsonString	json string
 * @param tok     		json token - pointer to JSON node
 * @param i				address of uint8_t to be updated
 *
 * @return         		SUCCESS - success
 * @return				JSON_PARSE_ERROR - error parsing value
 */
IoT_Error_t parseUnsignedInteger8Value(uint8_t *i, const char *jsonString, jsmntok_t *token);

/**
 * @brief          Parse a float value from a JSON node.
 *
 * Given a JSON node parse the float value from the value.
 *
 * @param jsonString	json string
 * @param tok     		json token - pointer to JSON node
 * @param f				address of float to be updated
 *
 * @return         		SUCCESS - success
 * @return				JSON_PARSE_ERROR - error parsing value
 */
IoT_Error_t parseFloatValue(float *f, const char *jsonString, jsmntok_t *token);

/**
 * @brief          Parse a double value from a JSON node.
 *
 * Given a JSON node parse the double value from the value.
 *
 * @param jsonString	json string
 * @param tok     		json token - pointer to JSON node
 * @param d				address of double to be updated
 *
 * @return         		SUCCESS - success
 * @return				JSON_PARSE_ERROR - error parsing value
 */
IoT_Error_t parseDoubleValue(double *d, const char *jsonString, jsmntok_t *token);

/**
 * @brief          Parse a boolean value from a JSON node.
 *
 * Given a JSON node parse the boolean value from the value.
 *
 * @param jsonString	json string
 * @param tok     		json token - pointer to JSON node
 * @param b				address of boolean to be updated
 *
 * @return         		SUCCESS - success
 * @return				JSON_PARSE_ERROR - error parsing value
 */
IoT_Error_t parseBooleanValue(bool *b, const char *jsonString, jsmntok_t *token);

/**
 * @brief          Parse a string value from a JSON node.
 *
 * Given a JSON node parse the string value from the value.
 *
 * @param jsonString	json string
 * @param tok     		json token - pointer to JSON node
 * @param s				address of string to be updated
 *
 * @return         		SUCCESS - success
 * @return				JSON_PARSE_ERROR - error parsing value
 */
IoT_Error_t parseStringValue(char *buf, const char *jsonString, jsmntok_t *token);

#ifdef __cplusplus
}
#endif

#endif /* AWS_IOT_SDK_SRC_JSON_UTILS_H_ */

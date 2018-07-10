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
 * @file aws_json_utils.c
 * @brief Utilities for manipulating JSON
 *
 * json_utils provides JSON parsing utilities for use with the IoT SDK.
 * Underlying JSON parsing relies on the Jasmine JSON parser.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "aws_iot_json_utils.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
#include <cinttypes>
#else

#include <inttypes.h>

#endif

#include "aws_iot_log.h"

int8_t jsoneq(const char *json, jsmntok_t *tok, const char *s) {
	if(tok->type == JSMN_STRING) {
		if((int) strlen(s) == tok->end - tok->start) {
			if(strncmp(json + tok->start, s, (size_t) (tok->end - tok->start)) == 0) {
				return 0;
			}
		}
	}
	return -1;
}

IoT_Error_t parseUnsignedInteger32Value(uint32_t *i, const char *jsonString, jsmntok_t *token) {
	if(token->type != JSMN_PRIMITIVE) {
		IOT_WARN("Token was not an integer");
		return JSON_PARSE_ERROR;
	}

	if(('-' == (char) (jsonString[token->start])) || (1 != sscanf(jsonString + token->start, "%" SCNu32, i))) {
		IOT_WARN("Token was not an unsigned integer.");
		return JSON_PARSE_ERROR;
	}

	return SUCCESS;
}

IoT_Error_t parseUnsignedInteger16Value(uint16_t *i, const char *jsonString, jsmntok_t *token) {
	if(token->type != JSMN_PRIMITIVE) {
		IOT_WARN("Token was not an integer");
		return JSON_PARSE_ERROR;
	}

	if(('-' == (char) (jsonString[token->start])) || (1 != sscanf(jsonString + token->start, "%" SCNu16, i))) {
		IOT_WARN("Token was not an unsigned integer.");
		return JSON_PARSE_ERROR;
	}

	return SUCCESS;
}

IoT_Error_t parseUnsignedInteger8Value(uint8_t *i, const char *jsonString, jsmntok_t *token) {
	if(token->type != JSMN_PRIMITIVE) {
		IOT_WARN("Token was not an integer");
		return JSON_PARSE_ERROR;
	}

	uint32_t i_word;
	if(('-' == (char) (jsonString[token->start])) || (1 != sscanf(jsonString + token->start, "%" SCNu32, &i_word))) {
		IOT_WARN("Token was not an unsigned integer.");
		return JSON_PARSE_ERROR;
	}
	if (i_word > UINT8_MAX) {
		IOT_WARN("Token value %u exceeds 8 bits", i_word);
		return JSON_PARSE_ERROR;
	}
	*i = i_word;

	return SUCCESS;
}

IoT_Error_t parseInteger32Value(int32_t *i, const char *jsonString, jsmntok_t *token) {
	if(token->type != JSMN_PRIMITIVE) {
		IOT_WARN("Token was not an integer");
		return JSON_PARSE_ERROR;
	}

	if(1 != sscanf(jsonString + token->start, "%" SCNi32, i)) {
		IOT_WARN("Token was not an integer.");
		return JSON_PARSE_ERROR;
	}

	return SUCCESS;
}

IoT_Error_t parseInteger16Value(int16_t *i, const char *jsonString, jsmntok_t *token) {
	if(token->type != JSMN_PRIMITIVE) {
		IOT_WARN("Token was not an integer");
		return JSON_PARSE_ERROR;
	}

	int32_t i_word;
	if(1 != sscanf(jsonString + token->start, "%" SCNi32, &i_word)) {
		IOT_WARN("Token was not an integer.");
		return JSON_PARSE_ERROR;
	}
	if(i_word < INT16_MIN || i_word > INT16_MAX) {
		IOT_WARN("Token value %d out of range for 16-bit int", i_word);
		return JSON_PARSE_ERROR;
	}
	*i = i_word;

	return SUCCESS;
}

IoT_Error_t parseInteger8Value(int8_t *i, const char *jsonString, jsmntok_t *token) {
	if(token->type != JSMN_PRIMITIVE) {
		IOT_WARN("Token was not an integer");
		return JSON_PARSE_ERROR;
	}

	int32_t i_word;
	if(1 != sscanf(jsonString + token->start, "%" SCNi32, &i_word)) {
		IOT_WARN("Token was not an integer.");
		return JSON_PARSE_ERROR;
	}
	if(i_word < INT8_MIN || i_word > INT8_MAX) {
		IOT_WARN("Token value %d out of range for 8-bit int", i_word);
		return JSON_PARSE_ERROR;
	}
	*i = i_word;

	return SUCCESS;
}

IoT_Error_t parseFloatValue(float *f, const char *jsonString, jsmntok_t *token) {
	if(token->type != JSMN_PRIMITIVE) {
		IOT_WARN("Token was not a float.");
		return JSON_PARSE_ERROR;
	}

	if(1 != sscanf(jsonString + token->start, "%f", f)) {
		IOT_WARN("Token was not a float.");
		return JSON_PARSE_ERROR;
	}

	return SUCCESS;
}

IoT_Error_t parseDoubleValue(double *d, const char *jsonString, jsmntok_t *token) {
	if(token->type != JSMN_PRIMITIVE) {
		IOT_WARN("Token was not a double.");
		return JSON_PARSE_ERROR;
	}

	if(1 != sscanf(jsonString + token->start, "%lf", d)) {
		IOT_WARN("Token was not a double.");
		return JSON_PARSE_ERROR;
	}

	return SUCCESS;
}

IoT_Error_t parseBooleanValue(bool *b, const char *jsonString, jsmntok_t *token) {
	if(token->type != JSMN_PRIMITIVE) {
		IOT_WARN("Token was not a primitive.");
		return JSON_PARSE_ERROR;
	}
	if(jsonString[token->start] == 't' && jsonString[token->start + 1] == 'r' && jsonString[token->start + 2] == 'u'
	   && jsonString[token->start + 3] == 'e') {
		*b = true;
	} else if(jsonString[token->start] == 'f' && jsonString[token->start + 1] == 'a'
			  && jsonString[token->start + 2] == 'l' && jsonString[token->start + 3] == 's'
			  && jsonString[token->start + 4] == 'e') {
		*b = false;
	} else {
		IOT_WARN("Token was not a bool.");
		return JSON_PARSE_ERROR;
	}
	return SUCCESS;
}

IoT_Error_t parseStringValue(char *buf, const char *jsonString, jsmntok_t *token) {
	uint16_t size = 0;
	if(token->type != JSMN_STRING) {
		IOT_WARN("Token was not a string.");
		return JSON_PARSE_ERROR;
	}
	size = (uint16_t) (token->end - token->start);
	memcpy(buf, jsonString + token->start, size);
	buf[size] = '\0';
	return SUCCESS;
}

#ifdef __cplusplus
}
#endif

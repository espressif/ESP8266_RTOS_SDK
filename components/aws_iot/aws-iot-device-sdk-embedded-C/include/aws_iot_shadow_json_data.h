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

#ifndef SRC_SHADOW_AWS_IOT_SHADOW_JSON_DATA_H_
#define SRC_SHADOW_AWS_IOT_SHADOW_JSON_DATA_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file aws_iot_shadow_json_data.h
 * @brief This file is the interface for all the Shadow related JSON functions.
 */

#include <stddef.h>

/**
 * @brief This is a static JSON object that could be used in code
 *
 */
typedef struct jsonStruct jsonStruct_t;

/**
 * @brief Every JSON name value can have a callback. The callback should follow this signature
 */
typedef void (*jsonStructCallback_t)(const char *pJsonValueBuffer, uint32_t valueLength, jsonStruct_t *pJsonStruct_t);

/**
 * @brief All the JSON object types enum
 *
 * JSON number types need to be split into proper integer / floating point data types and sizes on embedded platforms.
 */
typedef enum {
	SHADOW_JSON_INT32,
	SHADOW_JSON_INT16,
	SHADOW_JSON_INT8,
	SHADOW_JSON_UINT32,
	SHADOW_JSON_UINT16,
	SHADOW_JSON_UINT8,
	SHADOW_JSON_FLOAT,
	SHADOW_JSON_DOUBLE,
	SHADOW_JSON_BOOL,
	SHADOW_JSON_STRING,
	SHADOW_JSON_OBJECT
} JsonPrimitiveType;

/**
 * @brief This is the struct form of a JSON Key value pair
 */
struct jsonStruct {
	const char *pKey; ///< JSON key
	void *pData; ///< pointer to the data (JSON value)
	JsonPrimitiveType type; ///< type of JSON
	jsonStructCallback_t cb; ///< callback to be executed on receiving the Key value pair
};

/**
 * @brief Initialize the JSON document with Shadow expected name/value
 *
 * This Function will fill the JSON Buffer with a null terminated string. Internally it uses snprintf
 * This function should always be used First, followed by iot_shadow_add_reported and/or iot_shadow_add_desired.
 * Always finish the call sequence with iot_finalize_json_document
 *
 * @note Ensure the size of the Buffer is enough to hold the entire JSON Document.
 *
 *
 * @param pJsonDocument The JSON Document filled in this char buffer
 * @param maxSizeOfJsonDocument maximum size of the pJsonDocument that can be used to fill the JSON document
 * @return An IoT Error Type defining if the buffer was null or the entire string was not filled up
 */
IoT_Error_t aws_iot_shadow_init_json_document(char *pJsonDocument, size_t maxSizeOfJsonDocument);

/**
 * @brief Add the reported section of the JSON document of jsonStruct_t
 *
 * This is a variadic function and please be careful with the usage. count is the number of jsonStruct_t types that you would like to add in the reported section
 * This function will add "reported":{<all the values that needs to be added>}
 *
 * @note Ensure the size of the Buffer is enough to hold the reported section + the init section. Always use the same JSON document buffer used in the iot_shadow_init_json_document function. This function will accommodate the size of previous null terminated string, so pass teh max size of the buffer
 *
 *
 * @param pJsonDocument The JSON Document filled in this char buffer
 * @param maxSizeOfJsonDocument maximum size of the pJsonDocument that can be used to fill the JSON document
 * @param count total number of arguments(jsonStruct_t object) passed in the arguments
 * @return An IoT Error Type defining if the buffer was null or the entire string was not filled up
 */
IoT_Error_t aws_iot_shadow_add_reported(char *pJsonDocument, size_t maxSizeOfJsonDocument, uint8_t count, ...);

/**
 * @brief Add the desired section of the JSON document of jsonStruct_t
 *
 * This is a variadic function and please be careful with the usage. count is the number of jsonStruct_t types that you would like to add in the reported section
 * This function will add "desired":{<all the values that needs to be added>}
 *
 * @note Ensure the size of the Buffer is enough to hold the reported section + the init section. Always use the same JSON document buffer used in the iot_shadow_init_json_document function. This function will accommodate the size of previous null terminated string, so pass the max size of the buffer
 *
 *
 * @param pJsonDocument The JSON Document filled in this char buffer
 * @param maxSizeOfJsonDocument maximum size of the pJsonDocument that can be used to fill the JSON document
 * @param count total number of arguments(jsonStruct_t object) passed in the arguments
 * @return An IoT Error Type defining if the buffer was null or the entire string was not filled up
 */
IoT_Error_t aws_iot_shadow_add_desired(char *pJsonDocument, size_t maxSizeOfJsonDocument, uint8_t count, ...);

/**
 * @brief Finalize the JSON document with Shadow expected client Token.
 *
 * This function will automatically increment the client token every time this function is called.
 *
 * @note Ensure the size of the Buffer is enough to hold the entire JSON Document. If the finalized section is not invoked then the JSON doucment will not be valid
 *
 *
 * @param pJsonDocument The JSON Document filled in this char buffer
 * @param maxSizeOfJsonDocument maximum size of the pJsonDocument that can be used to fill the JSON document
 * @return An IoT Error Type defining if the buffer was null or the entire string was not filled up
 */
IoT_Error_t aws_iot_finalize_json_document(char *pJsonDocument, size_t maxSizeOfJsonDocument);

/**
 * @brief Fill the given buffer with client token for tracking the Repsonse.
 *
 * This function will add the AWS_IOT_MQTT_CLIENT_ID with a sequence number. Every time this function is used the sequence number gets incremented
 *
 *
 * @param pBufferToBeUpdatedWithClientToken buffer to be updated with the client token string
 * @param maxSizeOfJsonDocument maximum size of the pBufferToBeUpdatedWithClientToken that can be used
 * @return An IoT Error Type defining if the buffer was null or the entire string was not filled up
 */

IoT_Error_t aws_iot_fill_with_client_token(char *pBufferToBeUpdatedWithClientToken, size_t maxSizeOfJsonDocument);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SHADOW_AWS_IOT_SHADOW_JSON_DATA_H_ */

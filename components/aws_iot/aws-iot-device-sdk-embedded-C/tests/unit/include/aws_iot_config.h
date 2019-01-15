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
 * @file aws_iot_config.h
 * @brief IoT Client Unit Testing - IoT Config
 */

#ifndef IOT_TESTS_UNIT_CONFIG_H_
#define IOT_TESTS_UNIT_CONFIG_H_

// Get from console
// =================================================
#define AWS_IOT_MQTT_HOST              "localhost"
#define AWS_IOT_MQTT_PORT              8883
#define AWS_IOT_MQTT_CLIENT_ID         "C-SDK_UnitTestClient"
#define AWS_IOT_MY_THING_NAME          "C-SDK_UnitTestThing"
#define AWS_IOT_ROOT_CA_FILENAME       "rootCA.crt"
#define AWS_IOT_CERTIFICATE_FILENAME   "cert.crt"
#define AWS_IOT_PRIVATE_KEY_FILENAME   "privkey.pem"
// =================================================

// MQTT PubSub
#define AWS_IOT_MQTT_TX_BUF_LEN 512
#define AWS_IOT_MQTT_RX_BUF_LEN 512
#define AWS_IOT_MQTT_NUM_SUBSCRIBE_HANDLERS 5

// Thing Shadow specific configs
#define SHADOW_MAX_SIZE_OF_RX_BUFFER 512
#define MAX_SIZE_OF_UNIQUE_CLIENT_ID_BYTES 80  /** {"clientToken": ">>uniqueClientID<<+sequenceNumber"}*/
#define MAX_SIZE_CLIENT_ID_WITH_SEQUENCE MAX_SIZE_OF_UNIQUE_CLIENT_ID_BYTES + 10 /** {"clientToken": ">>uniqueClientID+sequenceNumber<<"}*/
#define MAX_SIZE_CLIENT_TOKEN_CLIENT_SEQUENCE MAX_SIZE_CLIENT_ID_WITH_SEQUENCE + 20 /** >>{"clientToken": "uniqueClientID+sequenceNumber"}<<*/
#define MAX_SIZE_OF_THINGNAME 30
#define MAX_ACKS_TO_COMEIN_AT_ANY_GIVEN_TIME 10
#define MAX_THINGNAME_HANDLED_AT_ANY_GIVEN_TIME 10
#define MAX_JSON_TOKEN_EXPECTED 120
#define MAX_SHADOW_TOPIC_LENGTH_WITHOUT_THINGNAME 60
#define MAX_SIZE_OF_THING_NAME 20
#define MAX_SHADOW_TOPIC_LENGTH_BYTES MAX_SHADOW_TOPIC_LENGTH_WITHOUT_THINGNAME + MAX_SIZE_OF_THING_NAME

// Auto Reconnect specific config
#define AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL 1000
#define AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL 128000

#endif /* IOT_TESTS_UNIT_CONFIG_H_ */

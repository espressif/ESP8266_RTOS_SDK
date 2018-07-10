
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
 * @file aws_iot_test_integration_common.h
 * @brief Integration Test common header
 */

#ifndef TESTS_INTEGRATION_H_
#define TESTS_INTEGRATION_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <getopt.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include "aws_iot_mqtt_client_interface.h"
#include "aws_iot_log.h"
#include "aws_iot_integ_tests_config.h"
#include "aws_iot_config.h"

int aws_iot_mqtt_tests_basic_connectivity();
int aws_iot_mqtt_tests_multiple_clients();
int aws_iot_mqtt_tests_auto_reconnect();

#endif /* TESTS_INTEGRATION_COMMON_H_ */

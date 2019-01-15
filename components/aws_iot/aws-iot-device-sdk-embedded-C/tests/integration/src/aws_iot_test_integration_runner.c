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
 * @file aws_iot_test_integration_runner.c
 * @brief Integration Test runner
 */

#include "aws_iot_test_integration_common.h"

int main() {
	int rc = 0;
	printf("\n\n");
	printf("*************************************************************************************************\n");
	printf("* Starting TEST 1 MQTT Version 3.1.1 Basic Subscribe QoS 1 Publish QoS 1 with Single Client     *\n");
	printf("*************************************************************************************************\n");
	rc = aws_iot_mqtt_tests_basic_connectivity();
	if(0 != rc) {
		printf("\n********************************************************************************************************\n");
		printf("* TEST 1 MQTT Version 3.1.1 Basic Subscribe QoS 1 Publish QoS 1 with Single Client FAILED! RC : %4d   *\n", rc);
		printf("********************************************************************************************************\n");
		return 1;
	}
	printf("\n*************************************************************************************************\n");
	printf("* Test 1 MQTT Version 3.1.1 Basic Subscribe QoS 1 Publish QoS 1 with Single Client SUCCESS!!    *\n");
	printf("*************************************************************************************************\n");

	printf("\n\n");
	printf("************************************************************************************************************\n");
	printf("* Starting TEST 2 MQTT Version 3.1.1 Multithreaded Subscribe QoS 1 Publish QoS 1 with Multiple Clients     *\n");
	printf("************************************************************************************************************\n");
	rc = aws_iot_mqtt_tests_multiple_clients();
	if(0 != rc) {
		printf("\n*******************************************************************************************************************\n");
		printf("* TEST 2 MQTT Version 3.1.1 Multithreaded Subscribe QoS 1 Publish QoS 1 with Multiple Clients FAILED! RC : %4d   *\n", rc);
		printf("*******************************************************************************************************************\n");
		return 1;
	}
	printf("\n*************************************************************************************************************\n");
	printf("* TEST 2 MQTT Version 3.1.1 Multithreaded Subscribe QoS 1 Publish QoS 1 with Multiple Clients SUCCESS!!     *\n");
	printf("*************************************************************************************************************\n");

	printf("\n\n");
	printf("*********************************************************\n");
	printf("* Starting TEST 3 MQTT Version 3.1.1 Auto Reconnect     *\n");
	printf("*********************************************************\n");
	rc = aws_iot_mqtt_tests_auto_reconnect();
	if(0 != rc) {
		printf("\n***************************************************************\n");
		printf("* TEST 3 MQTT Version 3.1.1 Auto Reconnect FAILED! RC : %4d  *\n", rc);
		printf("***************************************************************\n");
		return 1;
	}
	printf("\n**********************************************************\n");
	printf("* TEST 3 MQTT Version 3.1.1 Auto Reconnect SUCCESS!!     *\n");
	printf("**********************************************************\n");

	return 0;
}

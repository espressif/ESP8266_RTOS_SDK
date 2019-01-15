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

#ifndef TESTS_INTEGRATION_INTEG_TESTS_CONFIG_H_
#define TESTS_INTEGRATION_INTEG_TESTS_CONFIG_H_

/* Number of messages to publish in each publish thread */
#define PUBLISH_COUNT 100

/* Maximum number of threads to create for the multi-threading test */
#define MAX_PUB_THREAD_COUNT 3

/* Minimum percentage of messages that must be received back by the yield thread.
 * This is here ONLY because sometimes the yield thread doesn't get scheduled before the publish
 * thread when it is created. In every other case, 100% messages should be received. */
#define RX_RECEIVE_PERCENTAGE 99.0f

/* Max number of initial connect retries */
#define CONNECT_MAX_ATTEMPT_COUNT 3

/* Interval that each thread sleeps for */
#define THREAD_SLEEP_INTERVAL_USEC 500000

/* Test topic to publish on */
#define INTEGRATION_TEST_TOPIC "Tests/Integration/EmbeddedC"

/* Client ID to be used for single client tests */
#define INTEGRATION_TEST_CLIENT_ID "EMB_C_SDK_INTEG_TESTER"

/* Client IDs to be used for multiple client tests */
#define INTEGRATION_TEST_CLIENT_ID_PUB "EMB_C_SDK_INTEG_TESTER_PUB"
#define INTEGRATION_TEST_CLIENT_ID_SUB "EMB_C_SDK_INTEG_TESTER_SUB"

#endif /* TESTS_INTEGRATION_INTEG_TESTS_CONFIG_H_ */

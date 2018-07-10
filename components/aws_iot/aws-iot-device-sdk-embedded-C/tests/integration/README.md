## Integration Tests
This folder contains integration tests to verify Embedded C SDK functionality. These have been tested to work with Linux but haven't been ported to any specific platform. For additional information about porting the Device SDK for embedded C onto additional platforms please refer to the [PortingGuide](https://github.com/aws/aws-iot-device-sdk-embedded-c/blob/master/PortingGuide.md/).  
To run these tests, follow the below steps:

 * Place device identity cert and private key in locations referenced in the `certs` folder
 * Download certificate authority CA file from [Symantec](https://www.symantec.com/content/en/us/enterprise/verisign/roots/VeriSign-Class%203-Public-Primary-Certification-Authority-G5.pem) and place in `certs` folder
 * Ensure the names of the cert files are the same as in the `aws_iot_config.h` file
 * Ensure the certificate has an attached policy which allows the proper permissions for AWS IoT
 * Update the Host endpoint in the `aws_iot_config.h` file
 * Build the example using make.  (''make''). The tests will run automatically as a part of the build process
 * For more detailed Debug output, enable the IOT_DEBUG flag in `Logging level control` section of the Makefile. IOT_TRACE can be enabled as well for very detailed information on what functions are being executed
 * More information on the each test is below
 
### Integration test configuration
For all the tests below, there is additional configuration in the `integ_tests_config.h`. The configuration options are explained below:

 * PUBLISH_COUNT - Number of messages to publish in each publish thread
 * MAX_PUB_THREAD_COUNT - Maximum number of threads to create for the multi-threading test
 * RX_RECEIVE_PERCENTAGE - Minimum percentage of messages that must be received back by the yield thread. This is here ONLY because sometimes the yield thread doesn't get scheduled before the publish thread when it is created. In every other case, 100% messages should be received
 * CONNECT_MAX_ATTEMPT_COUNT - Max number of initial connect retries
 * THREAD_SLEEP_INTERVAL_USEC - Interval that each thread sleeps for
 * INTEGRATION_TEST_TOPIC - Test topic to publish on
 * INTEGRATION_TEST_CLIENT_ID - Client ID to be used for single client tests
 * INTEGRATION_TEST_CLIENT_ID_PUB, INTEGRATION_TEST_CLIENT_ID_SUB - Client IDs to be used for multiple client tests
    
### Test 1 - Basic Connectivity Test
This test verifies basic connectivity with the server. It creates one client instance and connects to the server. It subscribes to the Integration Test topic. Then it creates two threads, one publish thread and one yield thread. The publish thread publishes `PUBLISH_COUNT` messages on the test topic and the yield thread receives them. Once all the messages are published, the program waits for 1 sec to ensure all the messages have sufficient time to be received.
The test ends with the program verifying that all the messages were received and no other errors occurred. 

### Test 2 - Multiple Client Connectivity Test
This test verifies usage of multiple clients in the same application. It creates two client instances and both connect to the server. One client instance subscribes to the Integration Test topic. The other client instance publishes `PUBLISH_COUNT` messages on the test topic and the yield instance receives them. Once all the messages are published, the program waits for 1 sec to ensure all the messages have sufficient time to be received.
The test ends with the program verifying that all the messages were received and no other errors occurred.

### Test 3 - Auto Reconnect Test
This test verifies Auto-reconnect functionality. It creates one client instance. Then it performs 3 separate tests

 * Tests the disconnect handler by causing a TLS layer disconnect.
 * Tests manual reconnect API by causing a TLS layer disconnect with auto-reconnect disabled
 * Lastly, it tests the Auto-reconnect API by enabling auto-reconnect. It renames the rootCA file to a different name to prevent immediate reconnect, verifies the error codes are returned properly and that the reconnect algorithm is running. Once that check is satisfied, it renames the rootCA file back to the original names and verifies the client was able to reconnect. 

### Test 4 - Multi-threading Validation Test
This test is used to validate thread-safe operations. This creates on client instance, one yield thread, one thread to test subscribe/unsubscribe behavior and MAX_PUB_THREAD_COUNT number of publish threads. Then it proceeds to publish PUBLISH_COUNT messages on the test topic from each publish thread. The subscribe/unsubscribe thread runs in the background constantly subscribing and unsubscribing to a second test topic. The yield threads records which messages were received.

The test verifies whether all the messages that were published were received or not. It also checks for errors that could occur in multi-threaded scenarios. The test has been run with 10 threads sending 500 messages each and verified to be working fine. It can be used as a reference testing application to validate whether your use case will work with multi-threading enabled.

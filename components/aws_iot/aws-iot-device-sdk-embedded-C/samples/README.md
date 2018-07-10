## Overview
This folder contains several samples that demonstrate various SDK functions. The Readme file also includes a walk-through of the subscribe publish sample to explain how the SDK is used. The samples are currently provided with Makefiles for building them on linux. For each sample:

 * Explore the makefile. The makefile for each sample provides a reference on how to set up makefiles for client applications
 * Explore the example.  It connects to AWS IoT platform using MQTT and demonstrates few actions that can be performed by the SDK
 * Download certificate authority CA file from [Symantec](https://www.symantec.com/content/en/us/enterprise/verisign/roots/VeriSign-Class%203-Public-Primary-Certification-Authority-G5.pem) and place in location referenced in the example (certs/)
 * Ensure you have [created a thing](https://docs.aws.amazon.com/iot/latest/developerguide/create-thing.html) through your AWS IoT Console with name matching the definition AWS_IOT_MY_THING_NAME in the `aws_iot_config.h` file
 * Place device identity cert and private key in locations referenced in the example (certs/)
 * Ensure the names of the cert files are the same as in the `aws_iot_config.h` file
 * Ensure the certificate has an attached policy which allows the proper permissions for AWS IoT
 * Build the example using make (`make`)
 * Run sample application (./subscribe_publish_sample or ./shadow_sample).  The sample will print status messages to stdout
 * All samples are written in C unless otherwise mentioned. The following sample applications are included:
	* `subscribe_publish_sample` - a simple pub/sub MQTT example
	* `subscribe_publish_cpp_sample` - a simple pub/sub MQTT example written in C++
	* `subscribe_publish_library_sample` - a simple pub/sub MQTT example which builds the SDK as a separate library
	* `shadow_sample` - a simple device shadow example using a connected window example
	* `shadow_sample_console_echo` - a sample to work with the AWS IoT Console interactive guide

## Subscribe Publish Sample
This is a simple pub/sub MQTT example. It connects a single MQTT client to the server and subscribes to a test topic. Then it proceeds to publish messages on this topic and yields after each publish to ensure that the message was received.

 * The sample first creates an instance of the AWS_IoT_Client
 * The next step is to initialize the client. The aws_iot_mqtt_init API is called for this purpose. The API takes the client instance and an IoT_Client_Init_Params variable to set the initial values for the client. The Init params include values like host URL, port, certificates, disconnect handler etc.
 * If the call to the init API was successful, we can proceed to call connect. The API is called aws_iot_mqtt_connect. It takes the client instance and IoT_Client_Connect_Params variable as arguments. The IoT_Client_Connect_Params is optional after the first call to connect as the client retains the original values that were provided to support reconnect. The Connect params include values like Client Id, MQTT Version etc.
 * If the connect API call was successful, we can proceed to subscribe and publish on this connect. The connect API call will return an error code, specific to the type of error that occurred, in case the call fails.
 * It is important to remember here that there is no dynamic memory allocation in the SDK. Any values that are passed as a pointer to the APIs should not be freed unless they are not required any further. For example, if the variable that stores the certificate path is freed after the init call is made, the connect call will fail. Similarly, if it is freed after the connect API returns success, any future connect calls (including reconnects) will fail.
 * The next step for this sample is to subscribe to the test topic. The API to be called for subscribe is aws_iot_mqtt_subscribe. It takes as arguments, the IoT Client instance, topic name, the length of the topic name, QoS, the subscribe callback handler and an optional pointer to some data to be returned to the subscribe handler
 * The next step it to call the publish API to send a message on the test topic. The sample sends two different messages, one QoS0 and one QoS1. The
 * The publish API takes the client instance, topic name to publish to, topic name length and a variable of type IoT_Publish_Message_Params. The IoT_Publish_Message_Params contains the payload, length of the payload and QoS.
 * If the publish API calls are successful, the sample proceeds to call the yield API. The yield API takes the client instance and a timeout value in milliseconds as arguments.
 * The yield API is called to let the SDK process any incoming messages. It also periodically sends out the PING request to prevent disconnect and, if enabled, it also performs auto-reconnect and resubscribe.
 * The yield API should be called periodically to process the PING request as well as read any messages in the receive buffer. It should be called once at least every TTL/2 time periods to ensure disconnect does not happen. There can only be one yield in progress at a time. Therefore, in multi-threaded scenarios one thread can be a dedicated yield thread while other threads handle other operations.
 * The sample sends out messages equal to the value set in publish count unless infinite publishing flag is set

For further information on each API please read the API documentation.

## Subscribe Publish Cpp Sample
This is the same sample as above but it is built using a C++ compiler. It demonstrates how the SDK can be used in a C++ program.

## Subscribe Publish Library Sample
This is also the same code as the Subscribe Publish sample. In this case, the SDK is built as a separate library and then used in the sample program.
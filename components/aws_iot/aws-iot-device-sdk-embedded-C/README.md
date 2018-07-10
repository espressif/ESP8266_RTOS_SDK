## ***** NOTICE *****
This repository is moving to a new branching system. The master branch will now contain bug fixes/features that have been minimally tested to ensure nothing major is broken. The release branch will contain new releases for the SDK that have been tested thoroughly on all supported platforms. Please ensure that you are tracking the release branch for all production work.

This change will allow us to push out bug fixes quickly and avoid having situations where issues stay open for a very long time.

## Overview

The AWS IoT device SDK for embedded C is a collection of C source files which can be used in embedded applications to securely connect to the [AWS IoT platform](http://docs.aws.amazon.com/iot/latest/developerguide/what-is-aws-iot.html). It includes transport clients **MQTT**, **TLS** implementations and examples for their use. It also supports AWS IoT specific features such as **Thing Shadow**. It is distributed in source form and intended to be built into customer firmware along with application code, other libraries and RTOS. For additional information about porting the Device SDK for embedded C onto additional platforms please refer to the [PortingGuide](https://github.com/aws/aws-iot-device-sdk-embedded-c/blob/master/PortingGuide.md/).

## Features
The Device SDK simplifies access to the Pub/Sub functionality of the AWS IoT broker via MQTT and provide APIs to interact with Thing Shadows. The SDK has been tested to work with the AWS IoT platform to ensure best interoperability of a device with the AWS IoT platform.

### MQTT Connection
The Device SDK provides functionality to create and maintain a mutually authenticated TLS connection over which it runs MQTT. This connection is used for any further publish operations and allow for subscribing to MQTT topics which will call a configurable callback function when these topics are received.

### Thing Shadow
The Device SDK implements the specific protocol for Thing Shadows to retrieve, update and delete Thing Shadows adhering to the protocol that is implemented to ensure correct versioning and support for client tokens. It abstracts the necessary MQTT topic subscriptions by automatically subscribing to and unsubscribing from the reserved topics as needed for each API call. Inbound state change requests are automatically signalled via a configurable callback.

## Design Goals of this SDK
The embedded C SDK was specifically designed for resource constrained devices (running on micro-controllers and RTOS).

Primary aspects are:
 * Flexibility in picking and choosing functionality (reduce memory footprint)
 * Static memory only (no malloc’s)
 * Configurable resource usage(JSON tokens, MQTT subscription handlers, etc…)
 * Can be ported to a different RTOS, uses wrappers for OS specific functions
 
For more information on the Architecture of the SDK refer [here](http://aws-iot-device-sdk-embedded-c-docs.s3-website-us-east-1.amazonaws.com/index.html)

## Collection of Metrics
Beginning with Release v2.2.0 of the SDK, AWS collects usage metrics indicating which language and version of the SDK is being used. This allows us to prioritize our resources towards addressing issues faster in SDKs that see the most and is an important data point. However, we do understand that not all customers would want to report this data by default. In that case, the sending of usage metrics can be easily disabled by the user by setting the `DISABLE_METRICS` flag to true in the
`aws_iot_config.h` for each application.

## How to get started ?
Ensure you understand the AWS IoT platform and create the necessary certificates and policies. For more information on the AWS IoT platform please visit the [AWS IoT developer guide](http://docs.aws.amazon.com/iot/latest/developerguide/iot-security-identity.html).

In order to quickly get started with the AWS IoT platform, we have ported the SDK for POSIX type Operating Systems like Ubuntu, OS X and RHEL. The SDK is configured for the mbedTLS library and can be built out of the box with *GCC* using *make utility*. You'll need to download mbedTLS from the official ARMmbed repository. We recommend that you pick the latest version in order to have up-to-date security fixes.

## Installation
This section explains the individual steps to retrieve the necessary files and be able to build your first application using the AWS IoT device SDK for embedded C.

Steps:

 * Create a directory to hold your application e.g. (/home/<user>/aws_iot/my_app)
 * Change directory to this new directory
 * Download the SDK to device and place in the newly created directory
 * Expand the tarball (tar -xf <tarball.tar>).  This will create the below directories:
    * `certs` - TLS certificates directory
    * `docs` - SDK API and file documentation. This folder is not present on GitHub. You can access the documentation [here](http://aws-iot-device-sdk-embedded-c-docs.s3-website-us-east-1.amazonaws.com/index.html)
    * `external_libs` - The mbedTLS and jsmn source files
    * `include` - The AWS IoT SDK header files
    * `platform` - Platform specific files for timer, TLS and threading layers
    * `samples` - The sample applications
    * `src` - The AWS IoT SDK source files
    * `tests` - Contains tests for verifying that the SDK is functioning as expected. More information can be found [here](https://github.com/aws/aws-iot-device-sdk-embedded-c/blob/master/tests/README.md)
 * View further information on how to use the SDK in the Readme file for samples that can be found [here](https://github.com/aws/aws-iot-device-sdk-embedded-c/blob/master/samples/README.md)
 	
Also, for a guided example on getting started with the Thing Shadow, visit the AWS IoT Console's [Interactive Guide](https://console.aws.amazon.com/iot).

## Porting to different platforms
As Embedded devices run on different Real Time Operating Systems and TCP/IP stacks, it is one of the important design goals for the Device SDK to keep it portable. Please refer to the [porting guide](https://github.com/aws/aws-iot-device-sdk-embedded-C/blob/master/PortingGuide.md) to get more information on how to make this SDK run on your devices (i.e. micro-controllers).

## Migrating from 1.x to 2.x
The 2.x branch makes several changes to the SDK. This section provides information on what changes will be required in the client application for migrating from v1.x to 2.x.

 * The first change is in the folder structure. Client applications using the SDK now need to keep only the certs, external_libs, include, src and platform folder in their application. The folder descriptions can be found above
 * All the SDK headers are in the `include` folder. These need to be added to the makefile as include directories
 * The source files are in the `src` folder. These need to be added to the makefile as one of the source directories
 * Similar to 1.x, the platform folder contains the platform specific headers and source files. These need to be added to the makefile as well
 * The `platform/threading` folder only needs to be added if multi-threading is required, and the `_ENABLE_THREAD_SUPPORT_` macro is defined in config 
 * The list below provides a mapping for migrating from the major APIs used in 1.x to the new APIs:

    | Description | 1.x | 2.x |
    | :---------- | :-- | :-- |
    | Initializing the client | ```void aws_iot_mqtt_init(MQTTClient_t *pClient);``` | ```IoT_Error_t aws_iot_mqtt_init(AWS_IoT_Client *pClient, IoT_Client_Init_Params *pInitParams);``` |
    | Connect | ```IoT_Error_t aws_iot_mqtt_connect(MQTTConnectParams *pParams);``` | ```IoT_Error_t aws_iot_mqtt_connect(AWS_IoT_Client *pClient, IoT_Client_Connect_Params *pConnectParams);``` |
    | Subscribe | ```IoT_Error_t aws_iot_mqtt_subscribe(MQTTSubscribeParams *pParams);``` | ```IoT_Error_t aws_iot_mqtt_subscribe(AWS_IoT_Client *pClient, const char *pTopicName, uint16_t topicNameLen, QoS qos, pApplicationHandler_t pApplicationHandler, void *pApplicationHandlerData);``` |
    | Unsubscribe | ```IoT_Error_t aws_iot_mqtt_unsubscribe(char *pTopic);``` | ```IoT_Error_t aws_iot_mqtt_unsubscribe(AWS_IoT_Client *pClient, const char *pTopicFilter, uint16_t topicFilterLen);``` |
    | Yield | ```IoT_Error_t aws_iot_mqtt_yield(int timeout);``` | ```IoT_Error_t aws_iot_mqtt_yield(AWS_IoT_Client *pClient, uint32_t timeout_ms);``` |
    | Publish | ```IoT_Error_t aws_iot_mqtt_publish(MQTTPublishParams *pParams);``` | ```IoT_Error_t aws_iot_mqtt_publish(AWS_IoT_Client *pClient, const char *pTopicName, uint16_t topicNameLen, IoT_Publish_Message_Params *pParams);``` |
    | Disconnect | ```IoT_Error_t aws_iot_mqtt_disconnect(void);``` | ```IoT_Error_t aws_iot_mqtt_disconnect(AWS_IoT_Client *pClient);``` |

You can find more information on how to use the new APIs in the Readme file for samples that can be found [here](https://github.com/aws/aws-iot-device-sdk-embedded-c/blob/master/samples/README.md)

## Resources
[API Documentation](http://aws-iot-device-sdk-embedded-c-docs.s3-website-us-east-1.amazonaws.com/index.html)

[MQTT 3.1.1 Spec](http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/csprd02/mqtt-v3.1.1-csprd02.html)

## Support
If you have any technical questions about AWS IoT Device SDK, use the [AWS IoT forum](https://forums.aws.amazon.com/forum.jspa?forumID=210).
For any other questions on AWS IoT, contact [AWS Support](https://aws.amazon.com/contact-us/).

## Sample APIs
Connecting to the AWS IoT MQTT platform

```
AWS_IoT_Client client;
rc = aws_iot_mqtt_init(&client, &iotInitParams);
rc = aws_iot_mqtt_connect(&client, &iotConnectParams);
```


Subscribe to a topic

```
AWS_IoT_Client client;
rc = aws_iot_mqtt_subscribe(&client, "sdkTest/sub", 11, QOS0, iot_subscribe_callback_handler, NULL);
```


Update Thing Shadow from a device

``` 
rc = aws_iot_shadow_update(&mqttClient, AWS_IOT_MY_THING_NAME, pJsonDocumentBuffer, ShadowUpdateStatusCallback,
                            pCallbackContext, TIMEOUT_4SEC, persistenSubscription);
```

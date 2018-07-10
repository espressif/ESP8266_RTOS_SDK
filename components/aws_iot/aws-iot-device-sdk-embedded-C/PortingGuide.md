# Porting Guide

## Scope
The scope of this document is to provide instructions to modify the provided source files and functions in this SDK to run in a variety of embedded C–based environments (e.g. real-time OS, embedded Linux) and to be adjusted to use a specific TLS implementation as available with specific hardware platforms.

## Contents of the SDK

The C-code files of this SDK are delivered via the following directory structure (see comment behind folder name for an explanation of its content).

Current SDK Directory Layout (mbedTLS)

|--`certs` (Private key, device certificate and Root CA) <br>
|--`docs` (Developer guide & API documentation) <br>
|--`external_libs` (external libraries - jsmn, mbedTLS) <br>
|--`include` (Header files of the AWS IoT device SDK) <br>
|--`src` (Source files of the AWS IoT device SDK) <br>
|--`platform` (Platform specific files) <br>
|--`samples` (Samples including makefiles for building on mbedTLS) <br> 
|--`tests` (Tests for verifying SDK is functioning as expected) <br> 

All makefiles in this SDK were configured using the documented folder structure above, so moving or renaming folders will require modifications to makefiles.

## Explanation of folders and their content

 * `certs` : This directory is initially empty and will need to contain the private key, the client certificate and the root CA. The client certificate and private key can be downloaded from the AWS IoT console or be created using the AWS CLI commands. The root CA can be downloaded from [Symantec](https://www.symantec.com/content/en/us/enterprise/verisign/roots/VeriSign-Class%203-Public-Primary-Certification-Authority-G5.pem).

 * `docs` : SDK API and file documentation.

 * `external_libs` : The mbedTLS and jsmn source files. The jsmn source files are always present. The mbedTLS source files are only included when downloading the tarball version of the SDK.

 * `include` : This directory contains the header files that an application using the SDK needs to include.

 * `src` : This directory contains the SDK source code including the MQTT library, device shadow code and utilities.

 * `platform` : Platform specific files for timer, TLS and threading layers. Includes a reference implementation for the linux using mbedTLS and pthread.

 * `samples` : This directory contains sample applications as well as their makefiles. The samples include a simple MQTT example which publishes and subscribes to the AWS IoT service as well as a device shadow example that shows example usage of the device shadow functionality.

 * `tests` : Contains tests for verifying SDK functionality. For further details please check the readme file included with the tests [here](https://github.com/aws/aws-iot-device-sdk-embedded-C/blob/master/tests/README.md/).

## Integrating the SDK into your environment

This section explains the API calls that need to be implemented in order for the Device SDK to run on your platform. The SDK interfaces follow the driver model where only prototypes are defined by the Device SDK itself while the implementation is delegated to the user of the SDK to adjust it to the platform in use. The following sections list the needed functionality for the device SDK to run successfully on any given platform.

### Timer Functions

A timer implementation is necessary to handle request timeouts (sending MQTT connect, subscribe, etc. commands) as well as connection maintenance (MQTT keep-alive pings). Timers need millisecond resolution and are polled for expiration so these can be implemented using a "milliseconds since startup" free-running counter if desired. In the synchronous sample provided with this SDK only one command will be "in flight" at one point in time plus the client's ping timer. 

Define the `Timer` Struct as in `timer_platform.h`

`void init_timer(Timer *);`
init_timer - A timer structure is initialized to a clean state.

`bool has_timer_expired(Timer *);`
has_timer_expired - a polling function to determine if the timer has expired.

`void countdown_ms(Timer *, uint32_t);`
countdown_ms - set the timer to expire in x milliseconds and start the timer.

`void countdown_sec(Timer *, uint32_t);`
countdown_sec - set the timer to expire in x seconds and start the timer.

`uint32_t left_ms(Timer *);`
left_ms - query time in milliseconds left on the timer.


### Network Functions

In order for the MQTT client stack to be able to communicate via the TCP/IP network protocol stack using a mutually authenticated TLS connection, the following API calls need to be implemented for your platform. 

For additional details about API parameters refer to the [API documentation](http://aws-iot-device-sdk-embedded-c-docs.s3-website-us-east-1.amazonaws.com/index.html).

Define the `TLSDataParams` Struct as in `network_platform.h`
This is used for data specific to the TLS library being used.

`IoT_Error_t iot_tls_init(Network *pNetwork, char *pRootCALocation, const char *pDeviceCertLocation,
  						 const char *pDevicePrivateKeyLocation, const char *pDestinationURL,
  						 uint16_t DestinationPort, uint32_t timeout_ms, bool ServerVerificationFlag);`
Initialize the network client / structure.  

`IoT_Error_t iot_tls_connect(Network *pNetwork, TLSConnectParams *TLSParams);`
Create a TLS TCP socket to the configure address using the credentials provided via the NewNetwork API call. This will include setting up certificate locations / arrays.


`IoT_Error_t iot_tls_write(Network*, unsigned char*, size_t, Timer *, size_t *);`
Write to the TLS network buffer.

`IoT_Error_t iot_tls_read(Network*, unsigned char*,  size_t, Timer *, size_t *);`
Read from the TLS network buffer.

`IoT_Error_t iot_tls_disconnect(Network *pNetwork);`
Disconnect API

`IoT_Error_t iot_tls_destroy(Network *pNetwork);`
Clean up the connection

`IoT_Error_t iot_tls_is_connected(Network *pNetwork);`
Check if the TLS layer is still connected

The TLS library generally provides the API for the underlying TCP socket.


### Threading Functions

The MQTT client uses a state machine to control operations in multi-threaded situations. However it requires a mutex implementation to guarantee thread safety. This is not required in situations where thread safety is not important and it is disabled by default. The _ENABLE_THREAD_SUPPORT_ macro needs to be defined in aws_iot_config.h to enable this layer. You will also need to add the -lpthread linker flag for the compiler if you are using the provided reference implementation.   

For additional details about API parameters refer to the [API documentation](http://aws-iot-device-sdk-embedded-c-docs.s3-website-us-east-1.amazonaws.com/index.html).

Define the `IoT_Mutex_t` Struct as in `threads_platform.h`
This is used for data specific to the TLS library being used.

`IoT_Error_t aws_iot_thread_mutex_init(IoT_Mutex_t *);`
Initialize the mutex provided as argument.  

`IoT_Error_t aws_iot_thread_mutex_lock(IoT_Mutex_t *);`
Lock the mutex provided as argument

`IoT_Error_t aws_iot_thread_mutex_unlock(IoT_Mutex_t *);`
Unlock the mutex provided as argument.

`IoT_Error_t aws_iot_thread_mutex_destroy(IoT_Mutex_t *);`
Destroy the mutex provided as argument.

The threading layer provides the implementation of mutexes used for thread-safe operations.

### Sample Porting:

Marvell has ported the SDK for their development boards. [These](https://github.com/marvell-iot/aws_starter_sdk/tree/master/sdk/external/aws_iot/platform/wmsdk) files are example implementations of the above mentioned functions. 
This provides a port of the timer and network layer. The threading layer is not a part of this port.

## Time source for certificate validation

As part of the TLS handshake the device (client) needs to validate the server certificate which includes validation of the certificate lifetime requiring that the device is aware of the actual time. Devices should be equipped with a real time clock or should be able to obtain the current time via NTP. Bypassing validation of the lifetime of a certificate is not recommended as it exposes the device to a security vulnerability, as it will still accept server certificates even when they have already has_timer_expired.

## Integration into operating system
### Single-Threaded implementation

The single threaded implementation implies that the sample application code (SDK + MQTT client) is called periodically by the firmware application running on the main thread. This is done by calling the function `aws_iot_mqtt_yield` (in the simple pub-sub example) and by calling `aws_iot_shadow_yield()` (in the device shadow example). In both cases the keep-alive time is set to 10 seconds. This means that the yield functions need to be called at a minimum frequency of once every 10 seconds. Note however that the `iot_mqtt_yield()` function takes care of reading incoming MQTT messages from the IoT service as well and hence should be called more frequently depending on the timing requirements of an application. All incoming messages can only be processed at the frequency at which `yield` is called.

### Multi-Threaded implementation

In the simple multi-threaded case the `yield` function can be moved to a background thread. Ensure this task runs at the frequency described above. In this case, depending on the OS mechanism, a message queue or mailbox could be used to proxy incoming MQTT messages from the callback to the worker task responsible for responding to or dispatching messages. A similar mechanism could be employed to queue publish messages from threads into a publish queue that are processed by a publishing task. Ensure the threading layer is enabled as the library is not thread safe otherwise.
There is a validation test for the multi-threaded implementation that can be found with the integration tests. You can find further details in the Readme for the integration tests [here](https://github.com/aws/aws-iot-device-sdk-embedded-C/blob/master/tests/integration/README.md). We have run the validation test with 10 threads sending 500 messages each and verified to be working fine. It can be used as a reference testing application to validate whether your use case will work with multi-threading enabled.

## Sample applications

The sample apps in this SDK provide a working implementation for mbedTLS. They use a reference implementation for linux provided with the SDK. Threading layer is enabled in the subscribe publish sample.

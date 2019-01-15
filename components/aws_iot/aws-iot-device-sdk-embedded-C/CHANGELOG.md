# Change Log
## [2.2.1](https://github.com/aws/aws-iot-device-sdk-embedded-C/releases/tag/v2.2.1) (Dec 26, 2017)

Bugfixes:

  - [#115](https://github.com/aws/aws-iot-device-sdk-embedded-C/issues/115) - Issue with new client metrics

Pull requests:

  - [#112](https://github.com/aws/aws-iot-device-sdk-embedded-C/pull/112) - Initialize msqParams.isRetained to 0 in publishToShadowAction()
  - [#118](https://github.com/aws/aws-iot-device-sdk-embedded-C/pull/118) - mqttInitParams.mqttPacketTimeout_ms initialized

## [2.2.0](https://github.com/aws/aws-iot-device-sdk-embedded-C/releases/tag/v2.2.0) (Nov 22, 2017)

New Features:
 
  - Added SDK metrics string into connect packet

Bugfixes:

  - [#49](https://github.com/aws/aws-iot-device-sdk-embedded-C/issues/49) - Add support for SHADOW_JSON_STRING as supported value
  - [#57](https://github.com/aws/aws-iot-device-sdk-embedded-C/issues/57) - remove unistd.h
  - [#58](https://github.com/aws/aws-iot-device-sdk-embedded-C/issues/58) - Fix return type of aws_iot_mqtt_internal_is_topic_matched
  - [#59](https://github.com/aws/aws-iot-device-sdk-embedded-C/issues/95) - Fix extraneous assignment
  - [#62](https://github.com/aws/aws-iot-device-sdk-embedded-C/issues/62) - Clearing SubscriptionList entries in shadowActionAcks after subscription failure
  - [#63](https://github.com/aws/aws-iot-device-sdk-embedded-C/issues/63) - Stack overflow when IOT_DEBUG is enabled
  - [#66](https://github.com/aws/aws-iot-device-sdk-embedded-C/issues/66) - Bug in send packet function
  - [#69](https://github.com/aws/aws-iot-device-sdk-embedded-C/issues/69) - Fix for broken deleteActionHandler in shadow API
  - [#71](https://github.com/aws/aws-iot-device-sdk-embedded-C/issues/71) - Prevent messages on /update/accepted from incrementing shadowJsonVersionNum in delta
  - [#73](https://github.com/aws/aws-iot-device-sdk-embedded-C/issues/73) - wait for all messages to be received in subscribe publish sample
  - [#96](https://github.com/aws/aws-iot-device-sdk-embedded-C/issues/96) - destroy TLS instance even if disconnect send fails
  - Fix for aws_iot_mqtt_resubscribe not properly resubscribing to all topics
  - Update MbedTLS Network layer Readme to remove specific version link
  - Fix for not Passing througheError code on aws_iot_shadow_connect failure
  - Allow sending of SHADOW_JSON_OBJECT to the shadow
  - Ignore delta token callback for metadata
  - Fix infinite publish exiting early in subscribe publish sample

Improvements:

  - Updated jsmn to latest commit
  - Change default keepalive interval to 600 seconds

Pull requests:
  
  - [#29](https://github.com/aws/aws-iot-device-sdk-embedded-C/pull/29) - three small fixes
  - [#59](https://github.com/aws/aws-iot-device-sdk-embedded-C/pull/59) - Fixed MQTT header constructing and parsing
  - [#88](https://github.com/aws/aws-iot-device-sdk-embedded-C/pull/88) - Fix username and password are confused
  - [#78](https://github.com/aws/aws-iot-device-sdk-embedded-C/pull/78) - Fixed compilation warnings
  - [#102](https://github.com/aws/aws-iot-device-sdk-embedded-C/pull/102) - Corrected markdown headers
  - [#105](https://github.com/aws/aws-iot-device-sdk-embedded-C/pull/105) - Fixed warnings when compiling

## [2.1.1](https://github.com/aws/aws-iot-device-sdk-embedded-C/releases/tag/v2.1.1) (Sep 5, 2016)

Bugfixes/Improvements:

  - Network layer interface improvements to address reported issues
  - Incorporated GitHub pull request [#41](https://github.com/aws/aws-iot-device-sdk-embedded-c/pull/41) 
  - Bugfixes for issues [#36](https://github.com/aws/aws-iot-device-sdk-embedded-C/issues/36) and [#33](https://github.com/aws/aws-iot-device-sdk-embedded-C/issues/33)

## [2.1.0](https://github.com/aws/aws-iot-device-sdk-embedded-C/releases/tag/v2.1.0) (Jun 15, 2016)

New features:

  - Added unit tests, further details can be found in the testing readme [here](https://github.com/aws/aws-iot-device-sdk-embedded-c/blob/master/tests/README.md)
  - Added sample to demonstrate building the SDK as library
  - Added sample to demonstrate building the SDK in C++

Bugfixes/Improvements:

  - Increased default value of Maximum Reconnect Wait interval to 128 secs
  - Increased default value of MQTT Command Timeout in Shadow Connect to 20 secs
  - Shadow null/length checks
  - Client Id Length not passed correctly in shadow connect
  - Add extern C to headers and source files, added sample to demonstrate usage with C++ 
  - Delete/Accepted not being reported, callback added for delete/accepted
  - Append IOT_ to all Debug macros (eg. DEBUG is now IOT_DEBUG)
  - Fixed exit on error for subscribe_publish_sample

## [2.0.0](https://github.com/aws/aws-iot-device-sdk-embedded-C/releases/tag/v2.0.0) (April 28, 2016)

New features:

  - Refactored API to make it instance specific. This is a breaking change in the API from 1.x releases because a Client Instance parameter had to be added to all APIs
  - Added Threading library porting layer wrapper
  - Added support for multiple connections from one application
  - Shadows and connections de-linked, connection needs to be set up separately, can be used independently of shadow
  - Added integration tests for testing SDK functionality

Bugfixes/Improvements:

  - Yield cannot be called again while waiting for application callback to return
  - Fixed issue with TLS layer handles not being cleaned up properly on connection failure reported [here](https://github.com/aws/aws-iot-device-sdk-embedded-C/issues/16)
  - Renamed timer_linux.h to timer_platform.h as requested [here](https://github.com/aws/aws-iot-device-sdk-embedded-C/issues/5)
  - Adds support for disconnect handler to shadow. A similar pull request can be found [here](https://github.com/aws/aws-iot-device-sdk-embedded-C/pull/9)
  - New SDK folder structure, cleaned and simplified code structure
  - Removed Paho Wrapper, Merge MQTT into SDK code, added specific error codes
  - Refactored Network and Timer layer wrappers, added specific error codes
  - Refactored samples and makefiles
  
## [1.1.2](https://github.com/aws/aws-iot-device-sdk-embedded-C/releases/tag/v1.1.2) (April 22,  2016)

Bugfixes/Improvements:

  - Signature mismatch in MQTT library file fixed
  - Makefiles have a protective target on the top to prevent accidental execution

## [1.1.1](https://github.com/aws/aws-iot-device-sdk-embedded-C/releases/tag/v1.1.1) (April 1, 2016)

Bugfixes/Improvements:

  - Removing the Executable bit from all the files in the repository. Fixing [this](https://github.com/aws/aws-iot-device-sdk-embedded-C/issues/14) issue
  - Refactoring MQTT client to remove declaration after statement warnings
  - Fixing [this](https://forums.aws.amazon.com/thread.jspa?threadID=222467&tstart=0) bug
 

## [1.1.0](https://github.com/aws/aws-iot-device-sdk-embedded-C/releases/tag/v1.1.0) (February 10, 2016)
Features:

  - Auto Reconnect and Resubscribe
  
Bugfixes/Improvements:

  - MQTT buffer handling incase of bigger message
  - Large timeout values converted to seconds and milliseconds
  - Dynamic loading of Shadow parameters. Client ID and Thing Name are not hard-coded
  - MQTT Library refactored


## [1.0.1](https://github.com/aws/aws-iot-device-sdk-embedded-C/releases/tag/v1.0.1) (October 21, 2015)

Bugfixes/Improvements:

  - Paho name changed to Eclipse Paho
  - Renamed the Makefiles in the samples directory 
  - Device Shadow - Delete functionality macro fixed
  - `subscribe_publish_sample` updated

## [1.0.0](https://github.com/aws/aws-iot-device-sdk-embedded-C/releases/tag/v1.0.0) (October 8, 2015)

Features:

  - Release to github
  - SDK tarballs made available for public download

Bugfixes/Improvements:
  - Updated API documentation
 
## 0.4.0 (October 5, 2015)

Features:

  - Thing Shadow Actions - Update, Delete, Get for any Thing Name
  - aws_iot_config.h file for easy configuration of parameters
  - Sample app for talking with console's Interactive guide
  - disconnect handler for the MQTT client library
  
Bugfixes/Improvements:

  - mbedTLS read times out every 10 ms instead of hanging for ever
  - mbedTLS handshake failure handled

## 0.3.0 (September 14, 2015)

Features:

  - Testing with mbedTLS, prepping for relase

Bugfixes/Improvements:

  - Refactored to break out timer and network interfaces

## 0.2.0 (September 2, 2015)

Features:

  - Added initial Shadow implementation + example
  - Added hostname verification to OpenSSL example
  - Added iot_log interface
  - Initial API Docs (Doxygen)

Bugfixes/Improvements:

  - Fixed yield timeout
  - Refactored APIs to pass by reference vs value

## 0.1.0 (August 12, 2015)

Features:

  - Initial beta release
  - MQTT Publish and Subscribe
  - TLS mutual auth on linux with OpenSSL

Bugfixes/Improvements:
	- N/A

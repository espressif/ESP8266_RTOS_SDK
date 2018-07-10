# Tests
This folder contains tests to verify SDK functionality. These have been tested to work with Linux but haven't been ported to any specific platform. For additional information about porting the Device SDK for embedded C onto additional platforms please refer to the [PortingGuide](https://github.com/aws/aws-iot-device-sdk-embedded-c/blob/master/PortingGuide.md/).  
A description for each folder is given below

## integration
This folder contains integration tests that run directly against the server. For further information on how to run these tests check out the [Integration Test README](https://github.com/aws/aws-iot-device-sdk-embedded-c/blob/master/tests/integration/README.md/).

## unit
This folder contains unit tests that test SDK functionality against a Mock TLS layer. They are built using the CppUTest testing framework. For further information on how to run these tests check out the [Unit Test README](https://github.com/aws/aws-iot-device-sdk-embedded-c/blob/master/tests/unit/README.md/). 
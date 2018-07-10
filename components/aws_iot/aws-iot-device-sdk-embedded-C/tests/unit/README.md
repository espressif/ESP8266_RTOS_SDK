## Unit Tests
This folder contains unit tests to verify Embedded C SDK functionality. These have been tested to work with Linux using CppUTest as the testing framework.
CppUTest is not provided along with this code. It needs to be separately downloaded. These tests have been verified to work with CppUTest v3.6, which can be found [here](https://github.com/cpputest/cpputest/tree/v3.6).
Each test contains a comment describing what is being tested. The Tests can be run using the Makefile provided in the root folder for the SDK. There are a total of 187 tests.

To run these tests, follow the below steps:

 * Copy the code for CppUTest v3.6 from github to external_libs/CppUTest
 * Navigate to SDK Root folder
 * run `make run-unit-tests`
 
This will run all unit tests and generate coverage report in the build_output folder. The report can be viewed by opening <SDK_Root>/build_output/generated-coverage/index.html in a browser.
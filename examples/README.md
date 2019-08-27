# Examples

This directory contains a range of example ESP8266_RTOS_SDK projects. These are intended to demonstrate parts of ESP8266_RTOS_SDK functionality, and to provide code that you can copy and adapt into your own projects.

# Example Layout

The examples are grouped into subdirectories by category. Each category directory contains one or more example projects:

* `get-started` contains some very simple examples with minimal functionality.
* `peripherals` contains examples showing driver functionality for the various onboard ESP8266 peripherals.
* `protocols` contains examples showing network protocol interactions.
* `provisioning` contains examples showing how to configurate target AP information to ESP8266.
* `storage` contains examples showing data storage methods using SPI flash.
* `system` contains examples which demonstrate some debugging, factory-test and OTA.
* `wifi` contains examples of advanced Wi-Fi features. (For network protocol examples, see `protocols` instead.)

# Using Examples

Building an example is the same as building any other project:

* Follow the Getting Started instructions which include building the "Hello World" example.
* Change into the directory of the new example you'd like to build.
* Run `make menuconfig` to open the project configuration menu. Most examples have a project-specific "Example Configuration" section here (for example, to set the WiFi SSID & password to use).
* `make` to build the example.
* Follow the printed instructions to flash, or run `make flash`.

# Copying Examples

Each example is a standalone project. The examples *do not have to be inside the SDK directory*. You can copy an example directory to anywhere on your computer in order to make a copy that you can modify and work with.

The `IDF_PATH` environment variable is the only thing that connects the example to the rest of ESP8266_RTOS_SDK.

If you're looking for a more bare-bones project to start from, try [esp-idf-template](https://github.com/espressif/esp-idf-template).

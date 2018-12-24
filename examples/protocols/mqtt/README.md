# ESP8266 MQTT Client Demo

## 1. Introduction

This MQTT demo is based on the Eclipse Paho MQTT library, and demonstrates a working MQTT client actions(subscribe, publish, ping). Using this MQTT demo, you can connect to the MQTT broker, subscribe to a topic and publish messages to the predefined topic.

Also, this demo will ping the MQTT broker in the defined interval if no sending or receiving action happens. And we add some APIs to realize the SSL functionality, these SSL APIs provide the one-way certification and two-way certification.

## 2. Configuration
* export `IDF_PATH`
* `make menuconfig` -> `Example Configuration` to config your example
* `make menuconfig` -> `Component config` -> `MQTT(Paho)` -> to config your MQTT parameters

## 3. Compiling & Execution

Once all the aforementioned works are done, the MQTT client demo will:

* Connect to the MQTT Broker
* Subscribe to the topic "/espressif/sub"
* Publish messages to the topic "/espressif/pub"
* MQTT keep alive interval is 30s, so if no sending and receiving actions happended during this interval, ping request will be sent and ping response is expected to be received.
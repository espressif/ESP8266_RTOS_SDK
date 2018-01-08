# Paho MQTT demo

This example shows how to use the Eclipse Paho MQTT as an example of ESP8266 RTOS SDK. In this demo, the following functions can be realized: MQTT publish, subscribe and ping.

1. Config SSID and PASSWORD of the Wi-Fi AP to be connected in user_config.h

2. Config MQTT Broker to be connected in MQTTEcho.c

3. Export SDK_PATH and BIN_PATH, run gen_misc.sh to compile, then download and run the demo

4. MQTT client will connect with the MQTT Broker, subscribe to the topic "ESP8266/sample/sub", and will publish messages
   to the topic "ESP8266/sample/pub"

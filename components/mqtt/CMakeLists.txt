if(CONFIG_MQTT_USING_IBM)
set(COMPONENT_ADD_INCLUDEDIRS
    "ibm-mqtt/MQTTClient-C/src"
    "ibm-mqtt/MQTTClient-C/src/FreeRTOS"
    "ibm-mqtt/MQTTPacket/src")

set(COMPONENT_SRCDIRS
    "ibm-mqtt/MQTTClient-C/src"
    "ibm-mqtt/MQTTClient-C/src/FreeRTOS"
    "ibm-mqtt/MQTTPacket/src")

endif()

if (CONFIG_MQTT_USING_ESP)
set(COMPONENT_ADD_INCLUDEDIRS "esp-mqtt/include")
set(COMPONENT_PRIV_INCLUDEDIRS "esp-mqtt/lib/include")
set(COMPONENT_SRCS "esp-mqtt/mqtt_client.c"
                   "esp-mqtt/lib/mqtt_msg.c"
                   "esp-mqtt/lib/mqtt_outbox.c"
                   "esp-mqtt/lib/platform_idf.c")

endif()

set(COMPONENT_REQUIRES lwip http_parser tcp_transport freertos lwip mbedtls openssl)

register_component()

if(CONFIG_MQTT_USING_IBM)
target_compile_options(${COMPONENT_LIB} PUBLIC -DMQTT_TASK -DMQTTCLIENT_PLATFORM_HEADER=MQTTFreeRTOS.h)
endif()

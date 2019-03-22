if(CONFIG_MQTT_USING_IBM)
set(COMPONENT_ADD_INCLUDEDIRS
    paho/MQTTClient-C/src
    paho/MQTTClient-C/src/FreeRTOS
    paho/MQTTPacket/src)

set(COMPONENT_SRCDIRS
    paho/MQTTClient-C/src
    paho/MQTTClient-C/src/FreeRTOS
    paho/MQTTPacket/src)

set(COMPONENT_REQUIRES freertos lwip ssl)
elif (CONFIG_MQTT_USING_ESP)
set(COMPONENT_ADD_INCLUDEDIRS esp-mqtt/include)
set(COMPONENT_PRIV_INCLUDEDIRS "esp-mqtt/lib/include")
set(COMPONENT_SRCS "esp-mqtt/mqtt_client.c"
                   "esp-mqtt/lib/mqtt_msg.c"
                   "esp-mqtt/lib/mqtt_outbox.c"
                   "esp-mqtt/lib/platform_esp32_idf.c")

set(COMPONENT_REQUIRES lwip http_parser mbedtls tcp_transport)
endif()

register_component()

target_compile_options(${COMPONENT_NAME} PUBLIC -DMQTT_TASK)


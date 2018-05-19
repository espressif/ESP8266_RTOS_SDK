set(COMPONENT_ADD_INCLUDEDIRS
    paho/MQTTClient-C/src
    paho/MQTTClient-C/src/FreeRTOS
    paho/MQTTPacket/src)

set(COMPONENT_SRCDIRS
    paho/MQTTClient-C/src
    paho/MQTTClient-C/src/FreeRTOS
    paho/MQTTPacket/src)

set(COMPONENT_REQUIRES freertos lwip ssl)

register_component()

target_compile_options(${COMPONENT_NAME} PUBLIC -DMQTT_TASK)

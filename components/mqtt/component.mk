
ifdef CONFIG_MQTT_USING_IBM
COMPONENT_ADD_INCLUDEDIRS += ibm-mqtt/MQTTClient-C/src \
                             ibm-mqtt/MQTTClient-C/src/FreeRTOS \
                             ibm-mqtt/MQTTPacket/src

COMPONENT_SRCDIRS += ibm-mqtt/MQTTClient-C/src \
                     ibm-mqtt/MQTTClient-C/src/FreeRTOS \
                     ibm-mqtt/MQTTPacket/src

CFLAGS += -DMQTT_TASK
else
ifdef CONFIG_MQTT_USING_ESP
COMPONENT_ADD_INCLUDEDIRS := esp-mqtt/include
COMPONENT_SRCDIRS :=  esp-mqtt esp-mqtt/lib
COMPONENT_PRIV_INCLUDEDIRS := esp-mqtt/lib/include
endif
endif


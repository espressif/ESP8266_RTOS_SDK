#
# Component Makefile
#
COMPONENT_ADD_INCLUDEDIRS += paho/MQTTClient-C/src \
                             paho/MQTTClient-C/src/FreeRTOS \
                             paho/MQTTPacket/src

COMPONENT_SRCDIRS += paho/MQTTClient-C/src \
                     paho/MQTTClient-C/src/FreeRTOS \
                     paho/MQTTPacket/src

CFLAGS += -DMQTT_TASK

COMPONENT_SUBMODULES += esp-mqtt
COMPONENT_ADD_INCLUDEDIRS := esp-mqtt/include
COMPONENT_SRCDIRS :=  esp-mqtt esp-mqtt/lib
COMPONENT_PRIV_INCLUDEDIRS := esp-mqtt/lib/include
#Due to RTOS version is lower than idf 4.0, so move some define in file mqtt_supported_features.h to here.
CFLAGS += -DMQTT_SUPPORTED_FEATURE_WS_SUBPROTOCOL -DMQTT_SUPPORTED_FEATURE_TRANSPORT_ERR_REPORTING

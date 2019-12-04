#
# Component Makefile
#

ifdef CONFIG_AWS_IOT_SDK

COMPONENT_ADD_INCLUDEDIRS := include aws-iot-device-sdk-embedded-C/include

COMPONENT_SRCDIRS := aws-iot-device-sdk-embedded-C/src port

else
# Disable AWS IoT support
COMPONENT_ADD_INCLUDEDIRS :=
COMPONENT_ADD_LDFLAGS :=
COMPONENT_SRCDIRS :=
endif

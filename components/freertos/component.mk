
# Component Makefile
#

COMPONENT_ADD_INCLUDEDIRS += include \
                             include/freertos \
                             include/freertos/private \
                             port/esp8266/include \
                             port/esp8266/include/freertos

ifndef CONFIG_DISABLE_FREERTOS
COMPONENT_SRCDIRS := freertos port/esp8266
endif

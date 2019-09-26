
# Component Makefile
#

COMPONENT_ADD_INCLUDEDIRS += include \
                             include/freertos \
                             include/freertos/private \
                             port/esp8266/include \
                             port/esp8266/include/freertos

COMPONENT_SRCDIRS := port/esp8266
ifndef CONFIG_DISABLE_FREERTOS
COMPONENT_SRCDIRS += freertos
endif

COMPONENT_ADD_LDFRAGMENTS += linker.lf

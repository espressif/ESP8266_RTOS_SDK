set(COMPONENT_ADD_INCLUDEDIRS include 
                              include/freertos
                              include/freertos/private
                              port/esp8266/include
                              port/esp8266/include/freertos)
set(COMPONENT_SRCDIRS freertos port/esp8266)
set(COMPONENT_REQUIRES esp8266)

register_component()

set(COMPONENT_ADD_INCLUDEDIRS include 
                              include/freertos
                              include/freertos/private
                              include/port
                              include/port/freertos)
set(COMPONENT_SRCDIRS freertos port)
set(COMPONENT_REQUIRES esp8266)

register_component()

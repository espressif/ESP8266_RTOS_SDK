set(COMPONENT_ADD_INCLUDEDIRS include include/spiffs)
set(COMPONENT_SRCDIRS library)

set(COMPONENT_REQUIRES  freertos)
set(COMPONENT_PRIV_REQUIRES esp8266)

register_component()

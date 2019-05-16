set(COMPONENT_SRCS "esp_tls.c")
set(COMPONENT_ADD_INCLUDEDIRS ".")

set(COMPONENT_REQUIRES ssl)
set(COMPONENT_PRIV_REQUIRES lwip)

register_component()

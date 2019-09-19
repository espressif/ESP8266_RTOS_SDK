if(CONFIG_ENABLE_MDNS)
set(COMPONENT_SRCS "src/mdns.c"
                   "src/mdns_networking.c")

set(COMPONENT_PRIV_INCLUDEDIRS "private_include")
endif()

set(COMPONENT_ADD_INCLUDEDIRS "include")
set(COMPONENT_REQUIRES "lwip" "mbedtls" "tcpip_adapter")

if(CONFIG_ENABLE_MDNS_CONSOLE)
set(COMPONENT_SRCS "${COMPONENT_SRCS}"
                   "src/mdns_console.c")
endif()

set(COMPONENT_REQUIRES "console" "tcpip_adapter" "newlib")

register_component()


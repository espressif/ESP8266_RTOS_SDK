if(CONFIG_ENABLE_MDNS)
set(COMPONENT_SRCS "src/mdns.c"
                   "src/mdns_console.c"
                   "src/mdns_networking.c")

set(COMPONENT_PRIV_INCLUDEDIRS "private_include")
endif()

set(COMPONENT_ADD_INCLUDEDIRS "include")
set(COMPONENT_REQUIRES "lwip" "ssl" "tcpip_adapter")

register_component()


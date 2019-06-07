
if(CONFIG_USING_ESP_VFS)
    set(COMPONENT_SRCS "vfs.c"
                       "vfs_uart.c")
else()
    set(COMPONENT_SRCDIRS "port")    
endif()

set(COMPONENT_ADD_INCLUDEDIRS "include")

set(COMPONENT_REQUIRES "lwip")

register_component()

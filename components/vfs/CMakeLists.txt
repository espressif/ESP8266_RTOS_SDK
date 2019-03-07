if(CONFIG_USING_ESP_VFS)
    set(COMPONENT_SRCS "vfs.c"
                       "vfs_uart.c")
    set(COMPONENT_ADD_INCLUDEDIRS "include")

    set(COMPONENT_REQUIRES)

    register_component()
endif()

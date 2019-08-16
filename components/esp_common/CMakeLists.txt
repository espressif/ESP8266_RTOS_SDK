if(BOOTLOADER_BUILD)
    # For bootloader, all we need from esp_common is headers
    idf_component_register(INCLUDE_DIRS include)
    set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "-Wl,--gc-sections")
else()
    # Regular app build
    idf_component_register(SRCS "src/stack_check.c"
                        INCLUDE_DIRS include)

    set_source_files_properties(
        "src/stack_check.c"
        PROPERTIES COMPILE_FLAGS
        -fno-stack-protector)
    set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY LINK_LIBRARIES "-Wl,--gc-sections")
    set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "-Wl,--gc-sections")
endif()

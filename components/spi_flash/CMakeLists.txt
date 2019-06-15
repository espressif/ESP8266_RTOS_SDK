if(BOOTLOADER_BUILD)
    # Bootloader needs SPIUnlock from this file, but doesn't
    # need other parts of this component
    set(COMPONENT_SRCDIRS "src" "port")
    set(COMPONENT_PRIV_REQUIRES "bootloader_support")
else()
    set(COMPONENT_SRCDIRS "src" "port")
    set(COMPONENT_PRIV_REQUIRES "esp8266" "freertos" "bootloader_support")
endif()

set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_REQUIRES)

register_component()

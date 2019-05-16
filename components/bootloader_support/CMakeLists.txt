set(COMPONENT_SRCDIRS "src")

if(${BOOTLOADER_BUILD})
    set(COMPONENT_ADD_INCLUDEDIRS "include include_priv")
    set(COMPONENT_REQUIRES)
    set(COMPONENT_PRIV_REQUIRES spi_flash util)
else()
    set(COMPONENT_ADD_INCLUDEDIRS "include")
    set(COMPONENT_PRIV_INCLUDEDIRS "include_priv")
    set(COMPONENT_REQUIRES)
    set(COMPONENT_PRIV_REQUIRES spi_flash util ssl)
endif()

register_component()

if(CONFIG_APP_UPDATE_CHECK_APP_SUM)
target_compile_definitions(${COMPONENT_NAME} PUBLIC -D CONFIG_ENABLE_BOOT_CHECK_SUM=1)
endif()

if(CONFIG_APP_UPDATE_CHECK_APP_HASH)
target_compile_definitions(${COMPONENT_NAME} PUBLIC -D CONFIG_ENABLE_BOOT_CHECK_SHA256=1)
endif()

set(COMPONENT_SRCDIRS ".")
set(COMPONENT_ADD_INCLUDEDIRS "include")

set(COMPONENT_REQUIRES "spi_flash")
set(COMPONENT_PRIV_REQUIRES "bootloader_support" "util")

register_component()

if(CONFIG_APP_UPDATE_CHECK_APP_SUM)
target_compile_definitions(${COMPONENT_NAME} PUBLIC -D CONFIG_ENABLE_BOOT_CHECK_SUM=1)
endif()

if(CONFIG_APP_UPDATE_CHECK_APP_HASH)
target_compile_definitions(${COMPONENT_NAME} PUBLIC -D CONFIG_ENABLE_BOOT_CHECK_SHA256=1)
endif()

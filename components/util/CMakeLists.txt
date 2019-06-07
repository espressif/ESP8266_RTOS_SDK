set(COMPONENT_SRCDIRS "src")
set(COMPONENT_ADD_INCLUDEDIRS "include")

set(COMPONENT_REQUIRES)

register_component()

if(NOT IS_BOOTLOADER_BUILD)
target_compile_definitions(${COMPONENT_NAME} PRIVATE -DUSING_IBUS_FASTER_GET)
endif()

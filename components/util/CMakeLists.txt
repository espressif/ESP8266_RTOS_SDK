set(COMPONENT_SRCDIRS "src")
set(COMPONENT_ADD_INCLUDEDIRS "include")

register_component()

if(NOT BOOTLOADER_BUILD)
target_compile_definitions(${COMPONENT_LIB} PUBLIC -DUSING_IBUS_FASTER_GET)
endif()

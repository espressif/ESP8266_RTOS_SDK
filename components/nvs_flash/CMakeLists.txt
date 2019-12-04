set(COMPONENT_SRCDIRS "src")
set(COMPONENT_ADD_INCLUDEDIRS "include")

set(COMPONENT_REQUIRES "spi_flash" "util")

register_component()

target_link_libraries(nvs_flash stdc++)

component_compile_definitions(
    NVS_CRC_HEADER_FILE="crc.h"
)
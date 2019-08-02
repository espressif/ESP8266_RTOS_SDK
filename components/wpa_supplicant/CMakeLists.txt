set(COMPONENT_SRCDIRS "src/crypto" "port")
set(COMPONENT_ADD_INCLUDEDIRS "include" "port/include")

set(COMPONENT_PRIV_REQUIRES "freertos" "heap" "newlib" "util")

register_component()

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-strict-aliasing)
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    __ets__
    EMBEDDED_SUPP
    ESPRESSIF_USE
    )

set(COMPONENT_SRCDIRS "src/crypto" "port")
set(COMPONENT_ADD_INCLUDEDIRS "include" "port/include")

set(COMPONENT_REQUIRES "")
set(COMPONENT_PRIV_REQUIRES "freertos" "heap" "newlib" "util")

register_component()

component_compile_options(-Wno-strict-aliasing)
component_compile_definitions(
    __ets__
    EMBEDDED_SUPP
    ESPRESSIF_USE
    )

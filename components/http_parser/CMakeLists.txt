set(COMPONENT_ADD_INCLUDEDIRS "include")
set(COMPONENT_SRCDIRS "src")

register_component()

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-implicit-fallthrough)

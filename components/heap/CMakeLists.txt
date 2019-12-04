set(COMPONENT_SRCDIRS "src" "port/esp8266")

set(COMPONENT_ADD_INCLUDEDIRS "include" "port/esp8266/include")

set(COMPONENT_REQUIRES "log" "newlib")

register_component()

if(CONFIG_HEAP_TRACING)
    set(WRAP_FUNCTIONS
        calloc
        malloc
        free
        realloc
        heap_caps_malloc
        heap_caps_free
        heap_caps_realloc
        heap_caps_malloc_default
        heap_caps_realloc_default)

    foreach(wrap ${WRAP_FUNCTIONS})
        target_link_libraries(heap "-Wl,--wrap=${wrap}")
    endforeach()

endif()

set(COMPONENT_ADD_INCLUDEDIRS include include/freertos include/freertos/private port/esp8266/include port/esp8266/include/freertos)
set(COMPONENT_SRCDIRS "freertos" "port/esp8266")
set(COMPONENT_ADD_LDFRAGMENTS "linker.lf")

register_component()

target_link_libraries(${COMPONENT_LIB} "-Wl,--undefined=uxTopUsedPriority")

set_source_files_properties(
    tasks.c
    event_groups.c
    timers.c
    queue.c
    ringbuf.c
    PROPERTIES COMPILE_DEFINITIONS
    _ESP_FREERTOS_INTERNAL
    )

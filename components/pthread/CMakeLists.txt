if(CONFIG_ENABLE_PTHREAD)
    set(COMPONENT_SRCDIRS "src")
    set(COMPONENT_ADD_INCLUDEDIRS "include")
    set(COMPONENT_REQUIRES)
endif()

register_component()

if(CONFIG_ENABLE_STATIC_TASK_CLEAN_UP_HOOK)
    target_link_libraries(pthread "-Wl,--wrap=vPortCleanUpTCB")
endif()

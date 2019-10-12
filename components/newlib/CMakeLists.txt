set(COMPONENT_SRCDIRS newlib/port)
set(COMPONENT_ADD_INCLUDEDIRS newlib/port/include newlib/include)


if(CONFIG_NEWLIB_LIBRARY_LEVEL_NANO)
    set(LIBC c_nano)
elseif(CONFIG_NEWLIB_LIBRARY_LEVEL_NORMAL)
    set(LIBC c)
elseif(CONFIG_NEWLIB_LIBRARY_LEVEL_FLOAT_NANO)
    set(LIBC c_fnano)
endif()

set(LIBM m)

set(COMPONENT_PRIV_REQUIRES "vfs" "lwip")  # for sys/ioctl.h

register_component()

if(CONFIG_NEWLIB_LIBRARY_LEVEL_FLOAT_NANO)
    target_link_libraries(${COMPONENT_LIB} PUBLIC "-u _printf_float" "-u _scanf_float")
endif()

target_compile_definitions(${COMPONENT_LIB} PUBLIC
    -D_CLOCKS_PER_SEC_=CONFIG_FREERTOS_HZ -D_POSIX_THREADS=1 -D_UNIX98_THREAD_MUTEX_ATTRIBUTES=1
    )

target_link_libraries(${COMPONENT_LIB} PUBLIC "-L ${CMAKE_CURRENT_SOURCE_DIR}/newlib/lib")

set(blobs "${LIBC}" "${LIBM}")
foreach(blob ${blobs})
    add_library(${blob} STATIC IMPORTED)
    set_property(TARGET ${blob} PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/newlib/lib/lib${blob}.a)
    target_link_libraries(${COMPONENT_LIB} PUBLIC ${blob})

    foreach(_blob ${blobs})
        if(NOT _blob STREQUAL ${blob})
            set_property(TARGET ${blob} APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${_blob})
        endif()
    endforeach()

    set_property(TARGET ${blob} APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${COMPONENT_LIB})
endforeach()

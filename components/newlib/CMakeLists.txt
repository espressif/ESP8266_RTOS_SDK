set(COMPONENT_SRCDIRS newlib/port)
set(COMPONENT_ADD_INCLUDEDIRS newlib/include newlib/port/include)


if(CONFIG_NEWLIB_NANO_FORMAT)
    set(LIBC c_nano)
else()
    set(LIBC c)
endif()

set(LIBM m)

set(COMPONENT_REQUIRES vfs)  # for sys/ioctl.h

register_component()

target_compile_definitions(newlib PUBLIC
    -D_CLOCKS_PER_SEC_=CONFIG_FREERTOS_HZ -D_POSIX_THREADS=1 -D_UNIX98_THREAD_MUTEX_ATTRIBUTES=1
    )

target_link_libraries(newlib "-L ${CMAKE_CURRENT_SOURCE_DIR}/newlib/lib")
target_link_libraries(newlib "${LIBC}" "${LIBM}")

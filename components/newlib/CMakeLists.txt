set(include_dirs platform_include)
set(srcs src/esp_malloc.c
         src/locks.c
         src/random.c
         src/reent_init.c
         src/select.c
         src/syscall.c
         src/termios.c
         src/time.c)

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "${include_dirs}"
                    PRIV_REQUIRES esp8266 freertos)  

if(CONFIG_NEWLIB_NANO_FORMAT)
set(LIBC c_nano)
else()
set(LIBC c)
endif()

set(LIBM m)

# Toolchain libraries require code defined in this component
idf_component_get_property(newlib newlib COMPONENT_LIB)
target_link_libraries(${COMPONENT_LIB} INTERFACE ${LIBC} ${LIBM} gcc "$<TARGET_FILE:${newlib}>")

# Forces the linker to include __errno from this component,
# instead of the implementations provided by newlib.
set(EXTRA_LINK_FLAGS "-u __errno")
target_link_libraries(${COMPONENT_LIB} INTERFACE "${EXTRA_LINK_FLAGS}")


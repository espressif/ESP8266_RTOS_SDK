if (NOT XTENSA_COMPILER_PREFIX)
    message(FATAL_ERROR "XTENSA_COMPILER_PREFIX variable is not set")
endif()

set(CMAKE_SYSTEM_NAME Generic)

set(XTENSA_C_COMPILER    "${XTENSA_COMPILER_PREFIX}gcc")
set(XTENSA_CXX_COMPILER  "${XTENSA_COMPILER_PREFIX}g++")
set(XTENSA_ASM_COMPILER  "${XTENSA_COMPILER_PREFIX}gcc")

if (CMAKE_C_COMPILER AND CMAKE_CXX_COMPILER)
    # Toolchain configuration is already provided. Check if it is a correct one.
    get_filename_component(C_COMPILER_FILENAME ${CMAKE_C_COMPILER} NAME)
    get_filename_component(CXX_COMPILER_FILENAME ${CMAKE_CXX_COMPILER} NAME)

    if (NOT ${C_COMPILER_FILENAME} STREQUAL ${XTENSA_C_COMPILER})
        message(FATAL_ERROR "Invalid toolchain configuration. Xtensa C compiler expected (${XTENSA_C_COMPILER}), but ${C_COMPILER_FILENAME} was provided.")
    endif()

    if (NOT ${CXX_COMPILER_FILENAME} STREQUAL ${XTENSA_CXX_COMPILER})
        message(FATAL_ERROR "Invalid toolchain configuration. Xtensa C++ compiler expected (${XTENSA_CXX_COMPILER}), but ${CXX_COMPILER_FILENAME} was provided.")
    endif()

    if (CMAKE_ASM_COMPILER)
        get_filename_component(ASM_COMPILER_FILENAME ${CMAKE_ASM_COMPILER} NAME)
        if (NOT ${ASM_COMPILER_FILENAME} STREQUAL ${XTENSA_ASM_COMPILER})
           message(WARNING "Invalid toolchain configuration. Xtensa ASM compiler expected (${XTENSA_ASM_COMPILER}), but ${ASM_COMPILER_FILENAME} was provided. Overriding it to ${C_COMPILER_FILENAME}")
           set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})
        endif()
    endif()
else()
    # Go with the defaults
    set(CMAKE_C_COMPILER    ${XTENSA_C_COMPILER})
    set(CMAKE_CXX_COMPILER  ${XTENSA_CXX_COMPILER})
    set(CMAKE_ASM_COMPILER  ${XTENSA_ASM_COMPILER})
endif()

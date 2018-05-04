set(CMAKE_SYSTEM_NAME Generic)

set(CMAKE_C_COMPILER xtensa-lx106-elf-gcc)
set(CMAKE_CXX_COMPILER xtensa-lx106-elf-g++)
set(CMAKE_ASM_COMPILER xtensa-lx106-elf-gcc)
set(CMAKE_OBJCOPY_COMPILER xtensa-lx106-elf-objcopy)

set(CMAKE_EXE_LINKER_FLAGS "-nostdlib" CACHE STRING "Linker Base Flags")

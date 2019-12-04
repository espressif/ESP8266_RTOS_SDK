cmake_minimum_required(VERSION 3.5)

if(NOT SDKCONFIG)
    message(FATAL_ERROR "Bootloader subproject expects the SDKCONFIG variable to be passed "
        "in by the parent build process.")
endif()

if(NOT IDF_PATH)
    message(FATAL_ERROR "Bootloader subproject expects the IDF_PATH variable to be passed "
        "in by the parent build process.")
endif()

set(COMPONENTS esptool_py bootloader bootloader_support spi_flash log esp8266 util partition_table)
set(BOOTLOADER_BUILD 1)
add_definitions(-DBOOTLOADER_BUILD=1)

set(COMPONENT_REQUIRES_COMMON log esp8266 spi_flash)

set(MAIN_SRCS main/bootloader_start.c)

include("${IDF_PATH}/tools/cmake/project.cmake")
project(bootloader)

target_linker_script(bootloader.elf
    "main/esp8266.bootloader.ld"
    "main/esp8266.bootloader.rom.ld")
# Imported from esp8266 component
target_linker_script(bootloader.elf ${ESP8266_BOOTLOADER_LINKER_SCRIPTS})
target_link_libraries(bootloader.elf ${ESP8266_BOOTLOADER_LIBS})

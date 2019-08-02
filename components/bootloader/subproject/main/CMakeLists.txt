idf_component_register(SRCS "bootloader_start.c"
                    REQUIRES bootloader bootloader_support spi_flash)

idf_build_get_property(target IDF_TARGET)
set(scripts "${target}.bootloader.ld"
            "${target}.bootloader.rom.ld"
            "${ESP8266_BOOTLOADER_LINKER_SCRIPTS}")

target_linker_script(${COMPONENT_LIB} INTERFACE "${scripts}")

target_link_libraries(${COMPONENT_LIB} INTERFACE "${ESP8266_BOOTLOADER_LIBS}")

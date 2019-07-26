set(srcs "src/partition"
         "src/spi_flash_raw.c"
         "src/spi_flash.c")
if(BOOTLOADER_BUILD)
    set(srcs "${srcs}" "port/port.c")
    set(priv_requires "bootloader_support")
else()
    set(priv_requires "esp8266" "freertos" "bootloader_support")
endif()

idf_component_register(SRCS "${srcs}"
                       PRIV_REQUIRES "${priv_requires}"
                       INCLUDE_DIRS "include"
                       LDFRAGMENTS "linker.lf")

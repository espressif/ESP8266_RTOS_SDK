if(BOOTLOADER_BUILD)
   set(priv_requires "bootloader_support")
   idf_component_register(SRCS "src/partition.c" "src/spi_flash_raw.c" "src/spi_flash.c" "port/port.c"
                       PRIV_REQUIRES "${priv_requires}"
                       INCLUDE_DIRS "include"
                       LDFRAGMENTS "linker.lf")

else()
    set(priv_requires "esp8266" "freertos" "bootloader_support")
    idf_component_register(SRCS "src/partition.c" "src/spi_flash_raw.c" "src/spi_flash.c"
                           PRIV_REQUIRES "${priv_requires}"
                           INCLUDE_DIRS "include"
                           LDFRAGMENTS "linker.lf")
endif()

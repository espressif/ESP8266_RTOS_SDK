set(srcs "src/partition"
         "src/spi_flash_raw.c"
         "src/spi_flash.c")
if(BOOTLOADER_BUILD)
    set(srcs "${srcs}" "port/port.c")
    set(priv_requires "bootloader_support")
else()

    if (CONFIG_ENABLE_SPI_FLASH_PATCH)
        list(APPEND srcs "src/patch/common.c")
        if(CONFIG_ENABLE_TH25Q16HB_PATCH_0)
            list(APPEND srcs "src/patch/th25q16hb.c")
        endif()

        if(CONFIG_ENABLE_FM25Q16A_PATCH_0)
            list(APPEND srcs "src/patch/fm25q16a.c")
        endif()
    endif()

    set(priv_requires "esp8266" "freertos" "bootloader_support")
endif()

idf_component_register(SRCS "${srcs}"
                       PRIV_REQUIRES "${priv_requires}"
                       INCLUDE_DIRS "include"
                       LDFRAGMENTS "linker.lf")

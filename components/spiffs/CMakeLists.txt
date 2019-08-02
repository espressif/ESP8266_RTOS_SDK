set(include_dirs "include")

if(CONFIG_USING_SPIFFS)
set(priv_include_dirs "." "spiffs/src")
set(srcs "esp_spiffs.c"
         "spiffs_api.c"
         "spiffs/src/spiffs_cache.c"
         "spiffs/src/spiffs_check.c"
         "spiffs/src/spiffs_gc.c"
         "spiffs/src/spiffs_hydrogen.c"
         "spiffs/src/spiffs_nucleus.c")
endif()

set(requires "spi_flash" "vfs")
set(priv_requires "bootloader_support")

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "${include_dirs}"
                       PRIV_INCLUDE_DIRS "${priv_include_dirs}"
                       REQUIRES "${requires}"
                       PRIV_REQUIRES "${priv_requires}")

if(CONFIG_USING_SPIFFS)
set(COMPONENT_ADD_INCLUDEDIRS "include")
set(COMPONENT_PRIV_INCLUDEDIRS "." "spiffs/src")
set(COMPONENT_SRCS "esp_spiffs.c"
                   "spiffs_api.c"
                   "spiffs/src/spiffs_cache.c"
                   "spiffs/src/spiffs_check.c"
                   "spiffs/src/spiffs_gc.c"
                   "spiffs/src/spiffs_hydrogen.c"
                   "spiffs/src/spiffs_nucleus.c")
endif()

set(COMPONENT_REQUIRES  "freertos" "spi_flash")
set(COMPONENT_PRIV_REQUIRES "esp8266" "bootloader_support")

register_component()


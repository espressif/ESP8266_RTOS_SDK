set(COMPONENT_SRCDIRS source driver)

set(COMPONENT_ADD_INCLUDEDIRS include)

set(COMPONENT_PRIV_INCLUDEDIRS include/driver)

set(COMPONENT_REQUIRES lwip)
set(COMPONENT_PRIV_REQUIRES freertos)

register_component()

target_link_libraries(${COMPONENT_NAME} "-L ${CMAKE_CURRENT_SOURCE_DIR}/lib")
if(NOT CONFIG_NO_BLOBS)
    target_link_libraries(${COMPONENT_NAME} gcc hal core
                                            net80211 phy pp smartconfig ssc wpa espnow wps)
endif()

target_link_libraries(${COMPONENT_NAME} "-u call_user_start")

set(ESPTOOLPY_FLASHSIZE ${CONFIG_ESPTOOLPY_FLASHSIZE})

if(ESPTOOLPY_FLASHSIZE STREQUAL "512KB")
set(ESP8266_LINKER_SCRIPTS eagle.app.v6.new.512.${CONFIG_ESPTOOLPY_APP_NUM}.ld)
endif()
if(ESPTOOLPY_FLASHSIZE STREQUAL "1MB")
set(ESP8266_LINKER_SCRIPTS eagle.app.v6.new.1024.${CONFIG_ESPTOOLPY_APP_NUM}.ld)
endif()
if(ESPTOOLPY_FLASHSIZE STREQUAL "2MB")
set(ESP8266_LINKER_SCRIPTS eagle.app.v6.new.1024.${CONFIG_ESPTOOLPY_APP_NUM}.ld)
endif()
if(ESPTOOLPY_FLASHSIZE STREQUAL "2MB-c1")
set(ESP8266_LINKER_SCRIPTS eagle.app.v6.new.2048.ld)
endif()
if(ESPTOOLPY_FLASHSIZE STREQUAL "4MB")
set(ESP8266_LINKER_SCRIPTS eagle.app.v6.new.1024.${CONFIG_ESPTOOLPY_APP_NUM}.ld)
endif()
if(ESPTOOLPY_FLASHSIZE STREQUAL "4MB-c1")
set(ESP8266_LINKER_SCRIPTS eagle.app.v6.new.2048.ld)
endif()
if(ESPTOOLPY_FLASHSIZE STREQUAL "8MB")
set(ESP8266_LINKER_SCRIPTS eagle.app.v6.new.2048.ld)
endif()
if(ESPTOOLPY_FLASHSIZE STREQUAL "16MB")
set(ESP8266_LINKER_SCRIPTS eagle.app.v6.new.2048.ld)
endif()

target_linker_script(${COMPONENT_NAME}
                     ld/${ESP8266_LINKER_SCRIPTS}
                     ld/eagle.app.v6.common.ld
                     ld/eagle.rom.addr.v6.ld)

target_compile_options(${COMPONENT_NAME} PUBLIC -Wno-error=char-subscripts -Wno-error=unknown-pragmas -Wno-error=implicit-function-declaration
                                                -Wno-error=pointer-sign -Wno-error=switch -Wno-error=maybe-uninitialized -Wno-error=format=
                                                -Wno-error=unused-value -Wno-error=address -Wno-error=return-type -Wno-error=format-extra-args
                                                -Wno-error=format-zero-length -Wno-error=unused-label -Wno-error=sizeof-pointer-memaccess)

target_compile_options(${COMPONENT_NAME} PUBLIC -DICACHE_FLASH)

if(BOOTLOADER_BUILD)
    # For bootloader, all we need from esp8266 is headers
    set(COMPONENT_ADD_INCLUDEDIRS include)
    # set(COMPONENT_REQUIRES ${COMPONENTS})
    set(COMPONENT_SRCS source/ets_printf.c)
    register_component(esp8266)

    # as cmake won't attach linker args to a header-only library, attach
    # linker args directly to the bootloader.elf
    set(ESP8266_BOOTLOADER_LINKER_SCRIPTS
        "${CMAKE_CURRENT_SOURCE_DIR}/ld/esp8266.rom.ld"
        PARENT_SCOPE
        )

    set(ESP8266_BOOTLOADER_LIBS
        "-L ${CMAKE_CURRENT_SOURCE_DIR}/lib"
        "core"
        PARENT_SCOPE
        )

else()
    # Regular app build

    set(COMPONENT_SRCDIRS "driver source")
    set(COMPONENT_ADD_INCLUDEDIRS "include")
    set(COMPONENT_PRIV_INCLUDEDIRS "include/driver")

    set(COMPONENT_REQUIRES newlib)
    # driver is a public requirement because esp_sleep.h uses gpio_num_t & touch_pad_t
    # tcpip_adapter is a public requirement because esp_event.h uses tcpip_adapter types
    set(COMPONENT_PRIV_REQUIRES "log" "nvs_flash" "spi_flash" "tcpip_adapter" "bootloader_support" "util" "esp_ringbuf")

    register_component()

    target_link_libraries(esp8266 "-L ${CMAKE_CURRENT_SOURCE_DIR}/lib")
    if(NOT CONFIG_NO_BLOBS)
        target_link_libraries(esp8266 gcc hal core net80211 phy pp smartconfig ssc wpa espnow wps)
    endif()
    target_linker_script(esp8266 "${CMAKE_CURRENT_BINARY_DIR}/esp8266_out.ld" "${CMAKE_CURRENT_BINARY_DIR}/esp8266_common_out.ld")

    target_linker_script(esp8266
        "ld/esp8266.rom.ld"
        "ld/esp8266.peripherals.ld"
        )

    target_link_libraries(esp8266 "-u call_user_start")

    # Preprocess esp8266.ld linker script to include configuration, becomes esp8266_out.ld
    set(LD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ld)
    add_custom_command(
        OUTPUT esp8266_out.ld
        COMMAND "${CMAKE_C_COMPILER}" -C -P -x c -E -o esp8266_out.ld ${CFLAGS} -I ${CONFIG_DIR} ${LD_DIR}/esp8266.ld
        MAIN_DEPENDENCY ${LD_DIR}/esp8266.ld ${SDKCONFIG_H}
        COMMENT "Generating memory map linker script..."
        VERBATIM)
    add_custom_command(
        OUTPUT esp8266_common_out.ld
        COMMAND "${CMAKE_C_COMPILER}" -C -P -x c -E -o esp8266_common_out.ld -I ${CONFIG_DIR} ${LD_DIR}/esp8266.common.ld
        MAIN_DEPENDENCY ${LD_DIR}/esp8266.common.ld ${SDKCONFIG_H}
        COMMENT "Generating section linker script..."
        VERBATIM)
    add_custom_target(esp8266_linker_script DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/esp8266_out.ld" "${CMAKE_CURRENT_BINARY_DIR}/esp8266_common_out.ld")
    add_dependencies(esp8266 esp8266_linker_script)

    if(CONFIG_ESP8266_PHY_INIT_DATA_IN_PARTITION)
        set(PHY_INIT_DATA_BIN phy_init_data.bin)

        # To get the phy_init_data.bin file, compile phy_init_data.h as a C file and then objcopy
        # the object file to a raw binary
        add_custom_command(
            OUTPUT ${PHY_INIT_DATA_BIN}
            DEPENDS ${CMAKE_CURRENT_LIST_DIR}/phy_init_data.h
            COMMAND ${CMAKE_C_COMPILER} -x c -c
            -I ${CMAKE_CURRENT_LIST_DIR} -I ${CMAKE_CURRENT_LIST_DIR}/include -I ${CMAKE_BINARY_DIR}
            -o phy_init_data.obj
            ${CMAKE_CURRENT_LIST_DIR}/phy_init_data.h
            COMMAND ${CMAKE_OBJCOPY} -O binary phy_init_data.obj ${PHY_INIT_DATA_BIN}
            )
        add_custom_target(phy_init_data ALL DEPENDS ${PHY_INIT_DATA_BIN})
        add_dependencies(flash phy_init_data)

    endif()

    if(CONFIG_ESP_FILENAME_MACRO_NO_PATH)
        target_compile_definitions(${COMPONENT_NAME} PUBLIC -D __ESP_FILE__=__FILE__)
    endif()

    if(CONFIG_ESP_FILENAME_MACRO_RAW)
        target_compile_definitions(${COMPONENT_NAME} PUBLIC -D __ESP_FILE__=__FILE__)
    endif()

    if(CONFIG_ESP_FILENAME_MACRO_NULL)
        target_compile_definitions(${COMPONENT_NAME} PUBLIC -D __ESP_FILE__="null")
    endif()

endif()

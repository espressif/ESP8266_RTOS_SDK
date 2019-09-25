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

    set(srcs
        "source/chip_boot.c"
        "source/backtrace.c"
        "source/esp_sleep.c"
        "source/esp_err_to_name.c"
        "source/esp_timer.c"
        "source/esp_wifi_os_adapter.c"
        "source/esp_wifi.c"
        "source/ets_printf.c"
        "source/event_default_handlers.c"
        "source/event_loop.c"
        "source/phy_init.c"
        "source/reset_reason.c"
        "source/startup.c"
        "source/system_api.c"
        "source/task_wdt.c"
        "source/rom.c"
        "source/hw_random.c"
        "driver/adc.c"
        "driver/gpio.c"
        "driver/hw_timer.c"
        "driver/i2c.c"
        "driver/i2s.c"
        "driver/pwm.c"
        "driver/spi.c"
        "driver/hspi_logic_layer.c"
        "driver/uart.c"
        "driver/ir_tx.c"
        "driver/ir_rx.c"
        "driver/ledc.c")

    set(include_dirs "include" "include/driver")

    set(priv_requires "wpa_supplicant" "log" "spi_flash" "tcpip_adapter" "esp_ringbuf" "bootloader_support" "nvs_flash" "util")
    set(fragments linker.lf ld/esp8266_fragments.lf ld/esp8266_bss_fragments.lf)

    idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "${include_dirs}"
                    REQUIRES "${requires}"
                    PRIV_REQUIRES "${priv_requires}"
                    LDFRAGMENTS "${fragments}"
                    REQUIRED_IDF_TARGETS esp8266)

    target_link_libraries(${COMPONENT_LIB} PUBLIC "-L ${CMAKE_CURRENT_SOURCE_DIR}/lib" "-lstdc++")

    if(NOT CONFIG_NO_BLOBS)
        set(blobs "gcc" "hal" "core" "net80211" "phy" "rtc" "clk" "pp" "smartconfig" "ssc" "wpa" "espnow" "wps" "wpa2")
        foreach(blob ${blobs})
            add_library(${blob} STATIC IMPORTED)
            set_property(TARGET ${blob} PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/lib/lib${blob}.a)
            target_link_libraries(${COMPONENT_LIB} PUBLIC ${blob})

            foreach(_blob ${blobs})
                if(NOT _blob STREQUAL ${blob})
                    set_property(TARGET ${blob} APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${_blob})
                endif()
            endforeach()

            set_property(TARGET ${blob} APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${COMPONENT_LIB})
        endforeach()
    endif()
    target_linker_script(${COMPONENT_LIB} INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/esp8266_out.ld")

    target_linker_script(${COMPONENT_LIB} INTERFACE "${CMAKE_CURRENT_LIST_DIR}/ld/esp8266.project.ld.in" 
                        PROCESS "${CMAKE_CURRENT_BINARY_DIR}/ld/esp8266.project.ld")

    target_linker_script(${COMPONENT_LIB} INTERFACE "ld/esp8266.rom.ld")
    target_linker_script(${COMPONENT_LIB} INTERFACE "ld/esp8266.peripherals.ld")

    target_link_libraries(${COMPONENT_LIB} INTERFACE "-u call_user_start")

    # Preprocess esp8266.ld linker script to include configuration, becomes esp8266_out.ld
    set(LD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ld)

    partition_table_get_partition_info(app_offset "--partition-boot-default" "offset")
    partition_table_get_partition_info(app_size "--partition-boot-default" "size")
    set(CFLAGS ${CFLAGS} -DAPP_OFFSET=${app_offset} -DAPP_SIZE=${app_size})

    add_custom_command(
        OUTPUT esp8266_out.ld
        COMMAND "${CMAKE_C_COMPILER}" -C -P -x c -E -o esp8266_out.ld ${CFLAGS} -I ${CONFIG_DIR} ${LD_DIR}/esp8266.ld
        MAIN_DEPENDENCY ${LD_DIR}/esp8266.ld ${SDKCONFIG_H}
        COMMENT "Generating memory map linker script..."
        VERBATIM)
    add_custom_target(esp8266_linker_script DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/esp8266_out.ld")
    add_dependencies(${COMPONENT_LIB} esp8266_linker_script)

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
        target_compile_definitions(${COMPONENT_LIB} PUBLIC -D __ESP_FILE__=__FILE__)
    endif()

    if(CONFIG_ESP_FILENAME_MACRO_RAW)
        target_compile_definitions(${COMPONENT_LIB} PUBLIC -D __ESP_FILE__=__FILE__)
    endif()

    if(CONFIG_ESP_FILENAME_MACRO_NULL)
        target_compile_definitions(${COMPONENT_LIB} PUBLIC -D __ESP_FILE__="null")
    endif()

endif()

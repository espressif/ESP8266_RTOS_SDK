set(COMPONENT_SRCDIRS ".")
set(COMPONENT_ADD_INCLUDEDIRS "include")

set(COMPONENT_REQUIRES "spi_flash" "partition_table")
set(COMPONENT_PRIV_REQUIRES "bootloader_support" "util")

register_component()

# Add custom target for generating empty otadata partition for flashing
if(${OTADATA_PARTITION_OFFSET})
    add_custom_command(OUTPUT "${PROJECT_BINARY_DIR}/${BLANK_OTADATA_FILE}"
        COMMAND ${PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/gen_empty_partition.py
        --size ${OTADATA_PARTITION_SIZE} "${PROJECT_BINARY_DIR}/${BLANK_OTADATA_FILE}")

    add_custom_target(blank_ota_data ALL DEPENDS "${PROJECT_BINARY_DIR}/${BLANK_OTADATA_FILE}")
    add_dependencies(flash blank_ota_data)
endif()

if(CONFIG_APP_UPDATE_CHECK_APP_SUM)
target_compile_definitions(${COMPONENT_NAME} PUBLIC -D CONFIG_ENABLE_BOOT_CHECK_SUM=1)
endif()

if(CONFIG_APP_UPDATE_CHECK_APP_HASH)
target_compile_definitions(${COMPONENT_NAME} PUBLIC -D CONFIG_ENABLE_BOOT_CHECK_SHA256=1)
endif()

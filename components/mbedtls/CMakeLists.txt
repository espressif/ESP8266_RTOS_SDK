if (CONFIG_MBEDTLS_V2)
set(MBEDTLS_V2 1)
message("Use MBEDTLS_V2")
endif()

include("${IDF_PATH}/components/mbedtls/mbedtls_v2/CMakeLists.txt")
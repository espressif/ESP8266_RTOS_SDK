# The following four lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(subscribe_publish)

if(CONFIG_EXAMPLE_EMBEDDED_CERTS)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "main/certs/aws-root-ca.pem" TEXT)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "main/certs/certificate.pem.crt" TEXT)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "main/certs/private.pem.key" TEXT)
endif()

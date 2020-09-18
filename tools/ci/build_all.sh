#!/bin/bash

# Build makefile examples

mkdir -p build_log
export LOG_PATH=`pwd`/build_log

mkdir -p build_examples
cd build_examples

mkdir -p ${LOG_PATH}
${IDF_PATH}/tools/ci/build_examples.sh

# Build cmake examples

mkdir -p build_log_cmake
export LOG_PATH=`pwd`/build_log_cmake

mkdir -p build_examples_cmake
cd build_examples_cmake

mkdir -p ${LOG_PATH}
${IDF_PATH}/tools/ci/build_examples_cmake.sh

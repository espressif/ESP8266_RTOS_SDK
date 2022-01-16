#!/bin/bash

#Build for esp8266

# mkspiffs ver. 0.2.3-7-gf248296
# Build configuration name: custom
# SPIFFS ver. 0.3.7-5-gf5e26c4
# Extra build flags: -DSPIFFS_OBJ_META_LEN=4 -DSPIFFS_ALIGNED_OBJECT_INDEX_TABLES=4
# SPIFFS configuration:
#   SPIFFS_OBJ_NAME_LEN: 32
#   SPIFFS_OBJ_META_LEN: 4
#   SPIFFS_USE_MAGIC: 1
#   SPIFFS_USE_MAGIC_LENGTH: 1
#   SPIFFS_ALIGNED_OBJECT_INDEX_TABLES: 4

cd ${IDF_PATH}/tools/mkspiffs/
make clean && make dist -j12 CPPFLAGS="-DSPIFFS_OBJ_META_LEN=4 -DSPIFFS_ALIGNED_OBJECT_INDEX_TABLES=4" BUILD_CONFIG_NAME=-custom
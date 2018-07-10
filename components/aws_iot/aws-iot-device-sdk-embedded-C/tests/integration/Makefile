#This target is to ensure accidental execution of Makefile as a bash script will not execute commands like rm in unexpected directories and exit gracefully.
.prevent_execution:
	exit 0

CC = gcc
RM = rm

DEBUG =

#IoT client directory
IOT_CLIENT_DIR = ../..

APP_DIR = $(IOT_CLIENT_DIR)/tests/integration
APP_NAME = integration_tests_mbedtls
MT_APP_NAME = integration_tests_mbedtls_mt
APP_SRC_FILES = $(shell find $(APP_DIR)/src/ -name '*.c')
MT_APP_SRC_FILES = $(shell find $(APP_DIR)/multithreadingTest/ -name '*.c')
APP_INCLUDE_DIRS = -I $(APP_DIR)/include

PLATFORM_DIR = $(IOT_CLIENT_DIR)/platform/linux

#MbedTLS directory
TEMP_MBEDTLS_SRC_DIR = $(IOT_CLIENT_DIR)/external_libs/mbedTLS
TLS_LIB_DIR = $(TEMP_MBEDTLS_SRC_DIR)/library
TLS_INCLUDE_DIR = -I $(TEMP_MBEDTLS_SRC_DIR)/include

EXTERNAL_LIBS += -L$(TLS_LIB_DIR)
LD_FLAG += -Wl,-rpath,$(TLS_LIB_DIR)
LD_FLAG += -ldl $(TLS_LIB_DIR)/libmbedtls.a $(TLS_LIB_DIR)/libmbedcrypto.a $(TLS_LIB_DIR)/libmbedx509.a -lpthread

# Logging level control
#LOG_FLAGS += -DENABLE_IOT_DEBUG
#LOG_FLAGS += -DENABLE_IOT_TRACE
LOG_FLAGS += -DENABLE_IOT_INFO
LOG_FLAGS += -DENABLE_IOT_WARN
LOG_FLAGS += -DENABLE_IOT_ERROR
COMPILER_FLAGS += $(LOG_FLAGS)

#IoT client directory
PLATFORM_COMMON_DIR = $(PLATFORM_DIR)/common
PLATFORM_THREAD_DIR = $(PLATFORM_DIR)/pthread
PLATFORM_NETWORK_DIR = $(PLATFORM_DIR)/mbedtls

IOT_INCLUDE_DIRS = -I $(PLATFORM_COMMON_DIR)
IOT_INCLUDE_DIRS += -I $(PLATFORM_THREAD_DIR)
IOT_INCLUDE_DIRS += -I $(PLATFORM_NETWORK_DIR)
IOT_INCLUDE_DIRS += -I $(IOT_CLIENT_DIR)/include
IOT_INCLUDE_DIRS += -I $(IOT_CLIENT_DIR)/external_libs/jsmn

IOT_SRC_FILES += $(shell find $(IOT_CLIENT_DIR)/src/ -name '*.c')
IOT_SRC_FILES += $(shell find $(IOT_CLIENT_DIR)/external_libs/jsmn/ -name '*.c')
IOT_SRC_FILES += $(shell find $(PLATFORM_NETWORK_DIR)/ -name '*.c')
IOT_SRC_FILES += $(shell find $(PLATFORM_COMMON_DIR)/ -name '*.c')
IOT_SRC_FILES += $(shell find $(PLATFORM_THREAD_DIR)/ -name '*.c')

#Aggregate all include and src directories
INCLUDE_ALL_DIRS += $(IOT_INCLUDE_DIRS)
INCLUDE_ALL_DIRS += $(APP_INCLUDE_DIRS)
INCLUDE_ALL_DIRS += $(TLS_INCLUDE_DIR)

SRC_FILES += $(APP_SRC_FILES)
SRC_FILES += $(IOT_SRC_FILES)

MT_SRC_FILES += $(MT_APP_SRC_FILES)
MT_SRC_FILES += $(IOT_SRC_FILES)

COMPILER_FLAGS += -g
COMPILER_FLAGS += $(LOG_FLAGS)
PRE_MAKE_CMDS += cd $(TEMP_MBEDTLS_SRC_DIR) && make

MAKE_CMD = $(CC) $(SRC_FILES) $(COMPILER_FLAGS) -g3 -o $(APP_DIR)/$(APP_NAME) $(EXTERNAL_LIBS) $(LD_FLAG) $(INCLUDE_ALL_DIRS);
MAKE_MT_CMD = $(CC) $(MT_SRC_FILES) $(COMPILER_FLAGS) -g3 -D_ENABLE_THREAD_SUPPORT_ -o $(APP_DIR)/$(MT_APP_NAME) $(EXTERNAL_LIBS) $(LD_FLAG) $(INCLUDE_ALL_DIRS);

ifeq ($(CODE_SIZE_ENABLE),Y)
POST_MAKE_CMDS += $(CC) -c $(SRC_FILES) $(INCLUDE_ALL_DIRS) -fstack-usage;
POST_MAKE_CMDS += (size --format=Berkeley *.o > $(APP_NAME)_size_info.txt);
POST_MAKE_CMDS += (cat *.su >> $(APP_NAME)_stack_usage.txt);
POST_MAKE_CMDS += ($(RM) *.o);
POST_MAKE_CMDS += ($(RM) *.su);
CLEAN_CMD += ($(RM) -f $(APP_NAME)_size_info.txt);
CLEAN_CMD += ($(RM) -f $(APP_NAME)_stack_usage.txt);
endif

all:
	$(PRE_MAKE_CMDS)
	$(DEBUG)$(MAKE_CMD)
	$(DEBUG)$(MAKE_MT_CMD)
	./$(APP_NAME)
	./$(MT_APP_NAME)
	$(POST_MAKE_CMDS)

clean:
	$(RM) -f $(APP_DIR)/$(APP_NAME)
	$(RM) -f $(APP_DIR)/$(MT_APP_NAME)
	$(CLEAN_CMD)

ALL_TARGETS_CLEAN += test-integration-assert-clean
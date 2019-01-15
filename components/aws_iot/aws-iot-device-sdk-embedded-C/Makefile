#This target is to ensure accidental execution of Makefile as a bash script will not execute commands like rm in unexpected directories and exit gracefully.
.prevent_execution:
	exit 0

#Set this to @ to keep the makefile quiet
ifndef SILENCE
	SILENCE = @ 
endif

CC = gcc
RM = rm

DEBUG =

#--- Inputs ----#
COMPONENT_NAME = IotSdkC

ALL_TARGETS := build-cpputest
ALL_TARGETS_CLEAN :=

CPPUTEST_USE_EXTENSIONS = Y
CPP_PLATFORM = Gcc
CPPUTEST_CFLAGS += -std=gnu99
CPPUTEST_LDFLAGS += -lpthread
CPPUTEST_CFLAGS += -D__USE_BSD
CPPUTEST_USE_GCOV = Y

#IoT client directory
IOT_CLIENT_DIR = .

APP_DIR = $(IOT_CLIENT_DIR)/tests/unit
APP_NAME = aws_iot_sdk_unit_tests
APP_SRC_FILES = $(shell find $(APP_DIR)/src -name '*.cpp')
APP_SRC_FILES = $(shell find $(APP_DIR)/src -name '*.c')
APP_INCLUDE_DIRS = -I $(APP_DIR)/include

CPPUTEST_DIR = $(IOT_CLIENT_DIR)/external_libs/CppUTest

# Provide paths for CppUTest to run Unit Tests otherwise build will fail
ifndef CPPUTEST_INCLUDE
    CPPUTEST_INCLUDE = $(CPPUTEST_DIR)/include
endif

ifndef CPPUTEST_BUILD_LIB
    CPPUTEST_BUILD_LIB = $(CPPUTEST_DIR)
endif

CPPUTEST_LDFLAGS += -ldl $(CPPUTEST_BUILD_LIB)/libCppUTest.a

PLATFORM_DIR = $(IOT_CLIENT_DIR)/platform/linux

#MbedTLS directory
TEMP_MBEDTLS_SRC_DIR = $(APP_DIR)/tls_mock
TLS_LIB_DIR = $(TEMP_MBEDTLS_SRC_DIR)
TLS_INCLUDE_DIR = -I $(TEMP_MBEDTLS_SRC_DIR)

# Logging level control
LOG_FLAGS += -DENABLE_IOT_DEBUG
#LOG_FLAGS += -DENABLE_IOT_TRACE
LOG_FLAGS += -DENABLE_IOT_INFO
LOG_FLAGS += -DENABLE_IOT_WARN
LOG_FLAGS += -DENABLE_IOT_ERROR
COMPILER_FLAGS += $(LOG_FLAGS)

EXTERNAL_LIBS += -L$(CPPUTEST_BUILD_LIB)

#IoT client directory
PLATFORM_COMMON_DIR = $(PLATFORM_DIR)/common

IOT_INCLUDE_DIRS = -I $(PLATFORM_COMMON_DIR)
IOT_INCLUDE_DIRS += -I $(IOT_CLIENT_DIR)/include
IOT_INCLUDE_DIRS += -I $(IOT_CLIENT_DIR)/external_libs/jsmn

IOT_SRC_FILES += $(shell find $(PLATFORM_COMMON_DIR)/ -name '*.c')
IOT_SRC_FILES += $(shell find $(IOT_CLIENT_DIR)/src/ -name '*.c')
IOT_SRC_FILES += $(shell find $(IOT_CLIENT_DIR)/external_libs/jsmn/ -name '*.c')

#Aggregate all include and src directories
INCLUDE_DIRS += $(IOT_INCLUDE_DIRS)
INCLUDE_DIRS += $(APP_INCLUDE_DIRS)
INCLUDE_DIRS += $(TLS_INCLUDE_DIR)
INCLUDE_DIRS += $(CPPUTEST_INCLUDE)

TEST_SRC_DIRS = $(APP_DIR)/src

SRC_FILES += $(APP_SRC_FILES)
SRC_FILES += $(IOT_SRC_FILES)

COMPILER_FLAGS += -g
COMPILER_FLAGS += $(LOG_FLAGS)
PRE_MAKE_CMDS = cd $(CPPUTEST_DIR) &&
PRE_MAKE_CMDS += cmake CMakeLists.txt &&
PRE_MAKE_CMDS += make &&
PRE_MAKE_CMDS += cd - &&
PRE_MAKE_CMDS += pwd &&
PRE_MAKE_CMDS += cp -f $(CPPUTEST_DIR)/src/CppUTest/libCppUTest.a $(CPPUTEST_DIR)/libCppUTest.a &&
PRE_MAKE_CMDS += cp -f $(CPPUTEST_DIR)/src/CppUTestExt/libCppUTestExt.a $(CPPUTEST_DIR)/libCppUTestExt.a

# Using TLS Mock for running Unit Tests
MOCKS_SRC += $(APP_DIR)/tls_mock/aws_iot_tests_unit_mock_tls_params.c
MOCKS_SRC += $(APP_DIR)/tls_mock/aws_iot_tests_unit_mock_tls.c

ISYSTEM_HEADERS += $(IOT_ISYSTEM_HEADERS)
CPPUTEST_CPPFLAGS +=  $(ISYSTEM_HEADERS)
CPPUTEST_CPPFLAGS +=  $(LOG_FLAGS)

LCOV_EXCLUDE_PATTERN = "tests/unit/*"
LCOV_EXCLUDE_PATTERN += "tests/integration/*"
LCOV_EXCLUDE_PATTERN += "external_libs/*"

#use this section for running a specific group of tests, comment this to run all
#ONLY FOR TESTING PURPOSE
#COMMAND_LINE_ARGUMENTS += -g CommonTests
#COMMAND_LINE_ARGUMENTS += -v

build-cpputest:
	$(PRE_MAKE_CMDS)

include CppUTestMakefileWorker.mk

.PHONY: run-unit-tests
run-unit-tests: $(ALL_TARGETS)
	@echo $(ALL_TARGETS)

.PHONY: clean
clean:
	$(MAKE) -C $(CPPUTEST_DIR) clean
	$(RM) -rf build_output
	$(RM) -rf gcov
	$(RM) -rf objs
	$(RM) -rf testLibs
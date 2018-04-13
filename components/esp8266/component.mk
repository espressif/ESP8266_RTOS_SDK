#
# Component Makefile
#
COMPONENT_ADD_INCLUDEDIRS += include
COMPONENT_PRIV_INCLUDEDIRS := include/driver
COMPONENT_SRCDIRS := driver

LIBS ?=
ifndef CONFIG_NO_BLOBS
LIBS += airkiss cirom crypto espnow gcc hal core minic mirom net80211 \
        phy pp pwm smartconfig ssc wpa wps
endif

ESPTOOLPY_FLASHSIZE ?= $(CONFIG_ESPTOOLPY_FLASHSIZE)

ifeq ($(CONFIG_ESPTOOLPY_APP_NUM),"app1")
BIN_APP_NUM := app1
endif
ifeq ($(CONFIG_ESPTOOLPY_APP_NUM),"app2")
BIN_APP_NUM := app2
endif

ifeq ($(ESPTOOLPY_FLASHSIZE), "512KB")
ESP8266_LINKER_SCRIPTS := eagle.app.v6.new.512.$(BIN_APP_NUM).ld
endif
ifeq ($(ESPTOOLPY_FLASHSIZE), "1MB")
ESP8266_LINKER_SCRIPTS := eagle.app.v6.new.1024.$(BIN_APP_NUM).ld
endif
ifeq ($(ESPTOOLPY_FLASHSIZE), "2MB")
ESP8266_LINKER_SCRIPTS := eagle.app.v6.new.1024.$(BIN_APP_NUM).ld
endif
ifeq ($(ESPTOOLPY_FLASHSIZE), "2MB-c1")
ESP8266_LINKER_SCRIPTS := eagle.app.v6.new.2048.ld
endif
ifeq ($(ESPTOOLPY_FLASHSIZE), "4MB")
ESP8266_LINKER_SCRIPTS := eagle.app.v6.new.1024.$(BIN_APP_NUM).ld
endif
ifeq ($(ESPTOOLPY_FLASHSIZE), "4MB-c1")
ESP8266_LINKER_SCRIPTS := eagle.app.v6.new.2048.ld
endif
ifeq ($(ESPTOOLPY_FLASHSIZE), "8MB")
ESP8266_LINKER_SCRIPTS := eagle.app.v6.new.2048.ld
endif
ifeq ($(ESPTOOLPY_FLASHSIZE), "16MB")
ESP8266_LINKER_SCRIPTS := eagle.app.v6.new.2048.ld
endif

#Linker scripts used to link the final application.
#Warning: These linker scripts are only used when the normal app is compiled; the bootloader
#specifies its own scripts.
LINKER_SCRIPTS += $(ESP8266_LINKER_SCRIPTS) eagle.app.v6.common.ld eagle.rom.addr.v6.ld

COMPONENT_ADD_LDFLAGS += -L$(COMPONENT_PATH)/lib \
                         $(addprefix -l,$(LIBS)) \
                         -L $(COMPONENT_PATH)/ld \
                         -Wl,--no-check-sections \
                         -u call_user_start \
                         $(addprefix -T ,$(LINKER_SCRIPTS))

ALL_LIB_FILES := $(patsubst %,$(COMPONENT_PATH)/lib/lib%.a,$(LIBS))

# final linking of project ELF depends on all binary libraries, and
# all linker scripts
COMPONENT_ADD_LINKER_DEPS := $(ALL_LIB_FILES) $(addprefix ld/,$(LINKER_SCRIPTS))

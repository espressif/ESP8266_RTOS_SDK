#
# Component Makefile
#
COMPONENT_ADD_INCLUDEDIRS += include
COMPONENT_PRIV_INCLUDEDIRS := include/driver
COMPONENT_SRCDIRS := driver source

LIBS ?=
ifndef CONFIG_NO_BLOBS
LIBS += airkiss crypto espnow gcc hal core net80211 \
        phy pp pwm smartconfig ssc wpa wps
endif

ifeq ($(CONFIG_ESPTOOLPY_APP_NUM),"app1")
BIN_APP_NUM := app1
endif
ifeq ($(CONFIG_ESPTOOLPY_APP_NUM),"app2")
BIN_APP_NUM := app2
endif

#Linker scripts used to link the final application.
#Warning: These linker scripts are only used when the normal app is compiled; the bootloader
#specifies its own scripts.
LINKER_SCRIPTS += esp8266.common.ld esp8266.rom.ld

COMPONENT_ADD_LDFLAGS += -L$(COMPONENT_PATH)/lib \
                         $(addprefix -l,$(LIBS)) \
                         -L $(COMPONENT_PATH)/ld \
                         -T esp8266_out.ld       \
                         -Wl,--no-check-sections \
                         -u call_user_start      \
                         $(addprefix -T ,$(LINKER_SCRIPTS))

ALL_LIB_FILES := $(patsubst %,$(COMPONENT_PATH)/lib/lib%.a,$(LIBS))

# final linking of project ELF depends on all binary libraries, and
# all linker scripts
COMPONENT_ADD_LINKER_DEPS := $(ALL_LIB_FILES) $(addprefix ld/,$(LINKER_SCRIPTS))

# Preprocess esp8266.ld linker script into esp8266_out.ld
#
# The library doesn't really depend on esp8266_out.ld, but it
# saves us from having to add the target to a Makefile.projbuild
$(COMPONENT_LIBRARY): esp8266_out.ld

esp8266_out.ld: $(COMPONENT_PATH)/ld/esp8266.ld ../include/sdkconfig.h
	$(CC) -I ../include -C -P -x c -E $< -o $@

COMPONENT_EXTRA_CLEAN := esp8266_out.ld
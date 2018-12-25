#
# Component Makefile
#
ifdef IS_BOOTLOADER_BUILD
COMPONENT_OBJS := source/ets_printf.o
COMPONENT_SRCDIRS := source
else
COMPONENT_ADD_INCLUDEDIRS += include
COMPONENT_PRIV_INCLUDEDIRS := include/driver
COMPONENT_SRCDIRS := driver source

LIBS ?=
ifndef CONFIG_NO_BLOBS
LIBS += gcc hal core net80211 \
        phy pp smartconfig ssc wpa espnow wps
endif

#Linker scripts used to link the final application.
#Warning: These linker scripts are only used when the normal app is compiled; the bootloader
#specifies its own scripts.
LINKER_SCRIPTS += esp8266.rom.ld esp8266.peripherals.ld

COMPONENT_ADD_LDFLAGS += -L$(COMPONENT_PATH)/lib \
                         $(addprefix -l,$(LIBS)) \
                         -L $(COMPONENT_PATH)/ld \
                         -T esp8266_out.ld       \
                         -T esp8266_common_out.ld \
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
$(COMPONENT_LIBRARY): esp8266_out.ld esp8266_common_out.ld

OUTLD_CFLAGS := -DAPP_OFFSET=$(APP_OFFSET) -DAPP_SIZE=$(APP_SIZE)

esp8266_out.ld: $(COMPONENT_PATH)/ld/esp8266.ld ../include/sdkconfig.h
	$(CC) $(OUTLD_CFLAGS) -I ../include -C -P -x c -E $< -o $@

esp8266_common_out.ld: $(COMPONENT_PATH)/ld/esp8266.common.ld ../include/sdkconfig.h
	$(CC) -I ../include -C -P -x c -E $< -o $@

COMPONENT_EXTRA_CLEAN := esp8266_out.ld

endif
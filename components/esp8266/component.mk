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
ifndef CONFIG_ESP8266_WIFI_DEBUG_LOG_ENABLE
LIBS += gcc hal core net80211 \
        phy rtc clk pp smartconfig ssc wpa espnow wps wpa2
else
LIBS += gcc hal core_dbg net80211_dbg \
        phy rtc clk pp_dbg smartconfig ssc wpa_dbg espnow_dbg wps_dbg wpa2_dbg
endif
endif

#Linker scripts used to link the final application.
#Warning: These linker scripts are only used when the normal app is compiled; the bootloader
#specifies its own scripts.
LINKER_SCRIPTS += esp8266.rom.ld esp8266.peripherals.ld

COMPONENT_ADD_LDFRAGMENTS += ld/esp8266_fragments.lf ld/esp8266_bss_fragments.lf linker.lf

COMPONENT_ADD_LDFLAGS += -L$(COMPONENT_PATH)/lib \
                         $(addprefix -l,$(LIBS)) \
                         -L $(COMPONENT_PATH)/ld \
                         -T esp8266_out.ld       \
                         -T $(COMPONENT_BUILD_DIR)/esp8266.project.ld \
                         -Wl,--no-check-sections \
                         -u call_user_start      \
                         -u g_esp_sys_info \
                         $(addprefix -T ,$(LINKER_SCRIPTS))

ALL_LIB_FILES := $(patsubst %,$(COMPONENT_PATH)/lib/lib%.a,$(LIBS))

# final linking of project ELF depends on all binary libraries, and
# all linker scripts
COMPONENT_ADD_LINKER_DEPS := $(ALL_LIB_FILES) $(addprefix ld/, $(filter-out $(COMPONENT_BUILD_DIR)/esp8266.project.ld, $(LINKER_SCRIPTS))) \
                             $(COMPONENT_BUILD_DIR)/esp8266.project.ld

# Preprocess esp8266.ld linker script into esp8266_out.ld
#
# The library doesn't really depend on esp8266_out.ld, but it
# saves us from having to add the target to a Makefile.projbuild
$(COMPONENT_LIBRARY): esp8266_out.ld

esp8266_out.ld: $(COMPONENT_PATH)/ld/esp8266.ld ../include/sdkconfig.h
	$(CC) $(CFLAGS) -I ../include -C -P -x c -E $< -o $@

COMPONENT_EXTRA_CLEAN := esp8266_out.ld $(COMPONENT_BUILD_DIR)/esp8266.project.ld

endif
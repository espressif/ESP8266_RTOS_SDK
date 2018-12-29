#
# Main bootloader Makefile.
#
# This is basically the same as a component makefile, but in the case of the bootloader
# we pull in bootloader-specific linker arguments.
#

LINKER_SCRIPTS := \
	esp8266.bootloader.ld \
	$(IDF_PATH)/components/esp8266/ld/esp8266.rom.ld \
	esp8266.bootloader.rom.ld

COMPONENT_ADD_LDFLAGS += -L $(IDF_PATH)/components/esp8266/lib -lcore -L $(COMPONENT_PATH) $(addprefix -T ,$(LINKER_SCRIPTS))

COMPONENT_ADD_LINKER_DEPS := $(LINKER_SCRIPTS)

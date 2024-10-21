#
# Component Makefile
#

COMPONENT_SRCDIRS := src

ifdef IS_BOOTLOADER_BUILD
COMPONENT_OBJS := src/spi_flash.o src/spi_flash_raw.o
endif

COMPONENT_ADD_LDFRAGMENTS += linker.lf

CFLAGS += -DPARTITION_QUEUE_HEADER=\"sys/queue.h\"

ifdef IS_BOOTLOADER_BUILD
COMPONENT_SRCDIRS += port
COMPONENT_OBJS += port/port.o
else
ifdef CONFIG_ENABLE_SPI_FLASH_PATCH
COMPONENT_SRCDIRS += src/patch
ifdef CONFIG_ENABLE_TH25Q16HB_PATCH_0
COMPONENT_SRCDIRS += src/patch/th25q16hb.c
endif

ifdef CONFIG_ENABLE_FM25Q16A_PATCH_0
COMPONENT_SRCDIRS += src/patch/fm25q16a.c"
endif
endif
endif

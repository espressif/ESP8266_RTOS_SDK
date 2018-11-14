#
# Component Makefile
#

COMPONENT_SRCDIRS := src

ifdef IS_BOOTLOADER_BUILD
COMPONENT_OBJS := src/spi_flash.o src/spi_flash_raw.o
endif

CFLAGS += -DPARTITION_QUEUE_HEADER=\"sys/queue.h\"

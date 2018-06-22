#
# Component Makefile
#

ifndef IS_BOOTLOADER_BUILD
COMPONENT_SRCDIRS := src
endif

CFLAGS += -DPARTITION_QUEUE_HEADER=\"sys/queue.h\"

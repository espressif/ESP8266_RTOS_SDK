#
# Component Makefile
#

COMPONENT_ADD_INCLUDEDIRS := platform_include
COMPONENT_SRCDIRS := src

ifdef CONFIG_NEWLIB_NANO_FORMAT
LIBC := c_nano
else  # CONFIG_NEWLIB_NANO_FORMAT
LIBC := c
endif  # CONFIG_NEWLIB_NANO_FORMAT

# Order of linking matters: libnewlib.a should go before libc.a
COMPONENT_ADD_LDFLAGS := -lnewlib -l$(LIBC) -lm -u __errno


COMPONENT_SRCDIRS := src
COMPONENT_ADD_INCLUDEDIRS := include
COMPONENT_PRIV_INCLUDEDIRS :=

src/http_parser.o: CFLAGS += -Wno-implicit-fallthrough

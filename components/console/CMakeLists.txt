if(CONFIG_USING_ESP_CONSOLE)
set(COMPONENT_ADD_INCLUDEDIRS .)
set(COMPONENT_SRCS "commands.c"
                   "split_argv.c"
                   "argtable3/argtable3.c"
                   "linenoise/linenoise.c")
else()
set(COMPONENT_ADD_INCLUDEDIRS "")
set(COMPONENT_SRCS "")
endif()

set(COMPONENT_REQUIRES)

register_component()


set(COMPONENT_SRCDIRS source/library source/platform)
set(COMPONENT_ADD_INCLUDEDIRS include include/mqtt)

set(COMPONENT_REQUIRES freertos lwip ssl)

register_component()

target_compile_options(${COMPONENT_NAME} PUBLIC -DMQTT_TASK)

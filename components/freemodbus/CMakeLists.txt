# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
set(srcs
    "common/esp_modbus_master.c"
    "common/esp_modbus_slave.c"
    "modbus/mb.c"
    "modbus/mb_m.c"
    "modbus/tcp/mbtcp.c"
    "modbus/tcp/mbtcp_m.c"
    "port/port.c"
    "port/portevent.c"
    "port/portevent_m.c"
    "port/portother.c"
    "port/portother_m.c"
    "port/porttimer.c"
    "port/porttimer_m.c"
    "modbus/functions/mbfunccoils.c"
    "modbus/functions/mbfunccoils_m.c"
    "modbus/functions/mbfuncdiag.c"
    "modbus/functions/mbfuncdisc.c"
    "modbus/functions/mbfuncdisc_m.c"
    "modbus/functions/mbfuncholding.c"
    "modbus/functions/mbfuncholding_m.c"
    "modbus/functions/mbfuncinput.c"
    "modbus/functions/mbfuncinput_m.c"
    "modbus/functions/mbfuncother.c"
    "modbus/functions/mbutils.c"
    "tcp_slave/port/port_tcp_slave.c"
    "tcp_slave/modbus_controller/mbc_tcp_slave.c"
    "tcp_master/modbus_controller/mbc_tcp_master.c"
    "tcp_master/port/port_tcp_master.c"
    "common/esp_modbus_master_tcp.c"
    "common/esp_modbus_slave_tcp.c")

set(include_dirs common/include)

set(priv_include_dirs common port modbus modbus/ascii modbus/functions
                                modbus/rtu modbus/tcp modbus/include)

list(APPEND priv_include_dirs   tcp_slave/port tcp_slave/modbus_controller
                                tcp_master/port tcp_master/modbus_controller)

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "${include_dirs}"
                    PRIV_INCLUDE_DIRS "${priv_include_dirs}")

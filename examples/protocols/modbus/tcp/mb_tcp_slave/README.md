# Modbus Slave Example

This example demonstrates using of FreeModbus TCP slave stack port implementation for ESP8266. The external Modbus host is able to read/write device parameters using Modbus protocol transport. The parameters accessible thorough Modbus are located in `mb_example_common/modbus_params.h\c` files and can be updated by user. 
These are represented in structures holding_reg_params, input_reg_params, coil_reg_params, discrete_reg_params for holding registers, input parameters, coils and discrete inputs accordingly. The app_main application demonstrates how to setup Modbus stack and use notifications about parameters change from host system. 
The FreeModbus stack located in `components/freemodbus` folder and contain `/port` folder inside which contains FreeModbus stack port for ESP8266. There are some parameters that can be configured in KConfig file to start stack correctly (See description below for more information).

The slave example uses shared parameter structures defined in ```examples/protocols/modbus/mb_example_common``` folder.

## Hardware required :
Option 1:
The ESP8266 development board flashed with modbus_tcp_slave example + external Modbus master host software.

Option 2:
The modbus_tcp_master example application configured as described in its README.md file and flashed into ESP8266 board.
Note: The ```Example Data (Object) Dictionary``` in the modbus_tcp_master example can be edited to address parameters from other slaves connected into Modbus segment.

## How to setup and use an example:

### Configure the application
Start the command below to show the configuration menu:
```
make menuconfig
```

Follow the instructions in `examples/common_components/protocol_examples_common` for further configuration.

The communication parameters of freemodbus stack (Component config->Modbus configuration) allow to configure it appropriately but usually it is enough to use default settings.
See the help strings of parameters for more information.

### Setup external Modbus master software
Option 1:
Configure the external Modbus master software according to port configuration parameters used in application.
As an example the Modbus Poll application can be used with this example.
Option 2:
Setup ESP8266 development board and set modbus_tcp_master example configuration as described in its README.md file.
Setup one or more slave boards and connect them into the same Modbus segment (See configuration above). 

### Build and flash software
Build the project and flash it to the board, then run monitor tool to view serial output:
```
make flash monitor
```

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP8266 RTOS to build projects.

## Example Output
Example output of the application:
```
I (2881) tcpip_adapter: sta ip: 192.168.3.20, mask: 255.255.255.0, gw: 192.168.3.1
I (2888) example_connect: Connected to tm_20#
I (2891) example_connect: IPv4 address: 192.168.3.20
I (2896) wifi: pm stop
I (2904) MB_TCP_SLAVE_PORT: Socket (#54), listener  on port: 502, errno=0
I (2913) MB_TCP_SLAVE_PORT: Protocol stack initialized.
I (2922) SLAVE_TEST: Modbus slave stack initialized.
I (2930) SLAVE_TEST: Start modbus test...
I (19106) MB_TCP_SLAVE_PORT: Socket (#55), accept client connection from address: 192.168.3.21
I (19265) SLAVE_TEST: INPUT READ (18911980 us), ADDR:1, TYPE:8, INST_ADDR:0x3ffeaba8, SIZE:2
I (19296) SLAVE_TEST: HOLDING READ (18943192 us), ADDR:1, TYPE:2, INST_ADDR:0x3ffeace4, SIZE:2
I (19326) SLAVE_TEST: INPUT READ (18973448 us), ADDR:3, TYPE:8, INST_ADDR:0x3ffeabac, SIZE:2
I (19358) SLAVE_TEST: HOLDING READ (19004891 us), ADDR:3, TYPE:2, INST_ADDR:0x3ffeace8, SIZE:2
I (19397) SLAVE_TEST: INPUT READ (19044277 us), ADDR:5, TYPE:8, INST_ADDR:0x3ffeabb0, SIZE:2
I (19426) SLAVE_TEST: HOLDING READ (19073113 us), ADDR:5, TYPE:2, INST_ADDR:0x3ffeacec, SIZE:2
I (19460) SLAVE_TEST: COILS READ (19107124 us), ADDR:0, TYPE:32, INST_ADDR:0x3ffeaba4, SIZE:8
I (19498) SLAVE_TEST: COILS READ (19145299 us), ADDR:8, TYPE:32, INST_ADDR:0x3ffeaba5, SIZE:8
I (20034) SLAVE_TEST: INPUT READ (19681079 us), ADDR:1, TYPE:8, INST_ADDR:0x3ffeaba8, SIZE:2
I (20076) SLAVE_TEST: HOLDING READ (19723188 us), ADDR:1, TYPE:2, INST_ADDR:0x3ffeace4, SIZE:2
I (20108) SLAVE_TEST: INPUT READ (19754952 us), ADDR:3, TYPE:8, INST_ADDR:0x3ffeabac, SIZE:2
I (20136) SLAVE_TEST: HOLDING READ (19783314 us), ADDR:3, TYPE:2, INST_ADDR:0x3ffeace8, SIZE:2
I (20177) SLAVE_TEST: INPUT READ (19822984 us), ADDR:5, TYPE:8, INST_ADDR:0x3ffeabb0, SIZE:2
I (20211) SLAVE_TEST: HOLDING READ (19857553 us), ADDR:5, TYPE:2, INST_ADDR:0x3ffeacec, SIZE:2
I (20247) SLAVE_TEST: COILS READ (19893160 us), ADDR:0, TYPE:32, INST_ADDR:0x3ffeaba4, SIZE:8
I (20279) SLAVE_TEST: COILS READ (19925017 us), ADDR:8, TYPE:32, INST_ADDR:0x3ffeaba5, SIZE:8
I (20813) SLAVE_TEST: INPUT READ (20459320 us), ADDR:1, TYPE:8, INST_ADDR:0x3ffeaba8, SIZE:2
I (20848) SLAVE_TEST: HOLDING READ (20494363 us), ADDR:1, TYPE:2, INST_ADDR:0x3ffeace4, SIZE:2
I (20879) SLAVE_TEST: INPUT READ (20524902 us), ADDR:3, TYPE:8, INST_ADDR:0x3ffeabac, SIZE:2
I (20908) SLAVE_TEST: HOLDING READ (20554668 us), ADDR:3, TYPE:2, INST_ADDR:0x3ffeace8, SIZE:2
I (20975) SLAVE_TEST: INPUT READ (20621131 us), ADDR:5, TYPE:8, INST_ADDR:0x3ffeabb0, SIZE:2
I (21018) SLAVE_TEST: HOLDING READ (20664381 us), ADDR:5, TYPE:2, INST_ADDR:0x3ffeacec, SIZE:2
I (21049) SLAVE_TEST: COILS READ (20695402 us), ADDR:0, TYPE:32, INST_ADDR:0x3ffeaba4, SIZE:8
I (21077) SLAVE_TEST: COILS READ (20723316 us), ADDR:8, TYPE:32, INST_ADDR:0x3ffeaba5, SIZE:8
I (21614) SLAVE_TEST: INPUT READ (21259932 us), ADDR:1, TYPE:8, INST_ADDR:0x3ffeaba8, SIZE:2
I (21647) SLAVE_TEST: HOLDING READ (21293443 us), ADDR:1, TYPE:2, INST_ADDR:0x3ffeace4, SIZE:2
I (21692) SLAVE_TEST: INPUT READ (21338194 us), ADDR:3, TYPE:8, INST_ADDR:0x3ffeabac, SIZE:2
I (21739) SLAVE_TEST: HOLDING READ (21385227 us), ADDR:3, TYPE:2, INST_ADDR:0x3ffeace8, SIZE:2
I (21771) SLAVE_TEST: INPUT READ (21417793 us), ADDR:5, TYPE:8, INST_ADDR:0x3ffeabb0, SIZE:2
I (21809) SLAVE_TEST: HOLDING READ (21455368 us), ADDR:5, TYPE:2, INST_ADDR:0x3ffeacec, SIZE:2
I (21842) SLAVE_TEST: COILS READ (21487628 us), ADDR:0, TYPE:32, INST_ADDR:0x3ffeaba4, SIZE:8
I (21877) SLAVE_TEST: COILS READ (21522833 us), ADDR:8, TYPE:32, INST_ADDR:0x3ffeaba5, SIZE:8
I (22431) SLAVE_TEST: INPUT READ (22077259 us), ADDR:1, TYPE:8, INST_ADDR:0x3ffeaba8, SIZE:2
I (22470) SLAVE_TEST: HOLDING READ (22116077 us), ADDR:1, TYPE:2, INST_ADDR:0x3ffeace4, SIZE:2
I (22497) SLAVE_TEST: INPUT READ (22143396 us), ADDR:3, TYPE:8, INST_ADDR:0x3ffeabac, SIZE:2
I (22531) SLAVE_TEST: HOLDING READ (22177761 us), ADDR:3, TYPE:2, INST_ADDR:0x3ffeace8, SIZE:2
I (22567) SLAVE_TEST: INPUT READ (22213536 us), ADDR:5, TYPE:8, INST_ADDR:0x3ffeabb0, SIZE:2
I (22599) SLAVE_TEST: HOLDING READ (22245560 us), ADDR:5, TYPE:2, INST_ADDR:0x3ffeacec, SIZE:2
I (22628) SLAVE_TEST: COILS READ (22274374 us), ADDR:0, TYPE:32, INST_ADDR:0x3ffeaba4, SIZE:8
I (22660) SLAVE_TEST: COILS READ (22306273 us), ADDR:8, TYPE:32, INST_ADDR:0x3ffeaba5, SIZE:8
I (23216) SLAVE_TEST: INPUT READ (22862829 us), ADDR:1, TYPE:8, INST_ADDR:0x3ffeaba8, SIZE:2
I (23289) SLAVE_TEST: HOLDING READ (22935222 us), ADDR:1, TYPE:2, INST_ADDR:0x3ffeace4, SIZE:2
I (23327) SLAVE_TEST: INPUT READ (22973935 us), ADDR:3, TYPE:8, INST_ADDR:0x3ffeabac, SIZE:2
I (23359) SLAVE_TEST: HOLDING READ (23006060 us), ADDR:3, TYPE:2, INST_ADDR:0x3ffeace8, SIZE:2
I (23388) SLAVE_TEST: INPUT READ (23034291 us), ADDR:5, TYPE:8, INST_ADDR:0x3ffeabb0, SIZE:2
I (23428) SLAVE_TEST: HOLDING READ (23074133 us), ADDR:5, TYPE:2, INST_ADDR:0x3ffeacec, SIZE:2
I (23458) SLAVE_TEST: COILS READ (23103983 us), ADDR:0, TYPE:32, INST_ADDR:0x3ffeaba4, SIZE:8
I (23487) SLAVE_TEST: COILS READ (23132996 us), ADDR:8, TYPE:32, INST_ADDR:0x3ffeaba5, SIZE:8
I (24012) SLAVE_TEST: INPUT READ (23659114 us), ADDR:1, TYPE:8, INST_ADDR:0x3ffeaba8, SIZE:2
I (24048) SLAVE_TEST: HOLDING READ (23694832 us), ADDR:1, TYPE:2, INST_ADDR:0x3ffeace4, SIZE:2
I (24077) SLAVE_TEST: INPUT READ (23723499 us), ADDR:3, TYPE:8, INST_ADDR:0x3ffeabac, SIZE:2
I (24122) SLAVE_TEST: HOLDING READ (23768128 us), ADDR:3, TYPE:2, INST_ADDR:0x3ffeace8, SIZE:2
I (24169) SLAVE_TEST: INPUT READ (23816157 us), ADDR:5, TYPE:8, INST_ADDR:0x3ffeabb0, SIZE:2
I (24210) SLAVE_TEST: HOLDING READ (23856338 us), ADDR:5, TYPE:2, INST_ADDR:0x3ffeacec, SIZE:2
I (24272) SLAVE_TEST: COILS READ (23918867 us), ADDR:0, TYPE:32, INST_ADDR:0x3ffeaba4, SIZE:8
I (24276) SLAVE_TEST: Modbus controller destroyed.
```
The output lines describe type of operation, its timestamp, modbus address, access type, storage address in parameter structure and number of registers accordingly.


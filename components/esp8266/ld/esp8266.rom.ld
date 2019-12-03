PROVIDE ( SPI_sector_erase = 0x400040c0 );
PROVIDE ( SPI_page_program = 0x40004174 );
PROVIDE ( SPI_read_data = 0x400042ac );
PROVIDE ( SPI_read_status = 0x400043c8 );
PROVIDE ( SPI_write_status = 0x40004400 );
PROVIDE ( SPI_write_enable = 0x4000443c );
PROVIDE ( Wait_SPI_Idle = 0x4000448c );
PROVIDE ( Enable_QMode = 0x400044c0 );
PROVIDE ( Disable_QMode = 0x40004508 );

PROVIDE ( Cache_Read_Enable = 0x40004678 );
PROVIDE ( Cache_Read_Disable = 0x400047f0 );

PROVIDE ( lldesc_build_chain = 0x40004f40 );
PROVIDE ( lldesc_num2link = 0x40005050 );
PROVIDE ( lldesc_set_owner = 0x4000507c );

PROVIDE ( __adddf3 = 0x4000c538 );
PROVIDE ( __addsf3 = 0x4000c180 );
PROVIDE ( __divdf3 = 0x4000cb94 );
PROVIDE ( __divdi3 = 0x4000ce60 );
PROVIDE ( __divsi3 = 0x4000dc88 );
PROVIDE ( __extendsfdf2 = 0x4000cdfc );
PROVIDE ( __fixdfsi = 0x4000ccb8 );
PROVIDE ( __fixunsdfsi = 0x4000cd00 );
PROVIDE ( __fixunssfsi = 0x4000c4c4 );
PROVIDE ( __floatsidf = 0x4000e2f0 );
PROVIDE ( __floatsisf = 0x4000e2ac );
PROVIDE ( __floatunsidf = 0x4000e2e8 );
PROVIDE ( __floatunsisf = 0x4000e2a4 );
PROVIDE ( __muldf3 = 0x4000c8f0 );
PROVIDE ( __muldi3 = 0x40000650 );
PROVIDE ( __mulsf3 = 0x4000c3dc );
PROVIDE ( __subdf3 = 0x4000c688 );
PROVIDE ( __subsf3 = 0x4000c268 );
PROVIDE ( __truncdfsf2 = 0x4000cd5c );
PROVIDE ( __udivdi3 = 0x4000d310 );
PROVIDE ( __udivsi3 = 0x4000e21c );
PROVIDE ( __umoddi3 = 0x4000d770 );
PROVIDE ( __umodsi3 = 0x4000e268 );
PROVIDE ( __umulsidi3 = 0x4000dcf0 );

PROVIDE ( bzero = 0x4000de84 );
PROVIDE ( memcmp = 0x4000dea8 );
PROVIDE ( memcpy = 0x4000df48 );
PROVIDE ( memmove = 0x4000e04c );
PROVIDE ( memset = 0x4000e190 );

PROVIDE ( strcmp = 0x4000bdc8 );
PROVIDE ( strcpy = 0x4000bec8 );
PROVIDE ( strlen = 0x4000bf4c );
PROVIDE ( strncmp = 0x4000bfa8 );
PROVIDE ( strncpy = 0x4000c0a0 );
PROVIDE ( strstr = 0x4000e1e0 );

PROVIDE ( gpio_input_get = 0x40004cf0 );
PROVIDE ( gpio_pin_wakeup_disable = 0x40004ed4 );
PROVIDE ( gpio_pin_wakeup_enable = 0x40004e90 );

PROVIDE ( ets_io_vprintf = 0x40001f00 );
PROVIDE ( uart_rx_one_char = 0x40003b8c );

PROVIDE ( rom_i2c_readReg = 0x40007268 );
PROVIDE ( rom_i2c_readReg_Mask = 0x4000729c );
PROVIDE ( rom_i2c_writeReg = 0x400072d8 );
PROVIDE ( rom_i2c_writeReg_Mask = 0x4000730c );

PROVIDE ( rom_software_reboot = 0x40000080 );

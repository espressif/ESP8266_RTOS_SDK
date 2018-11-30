#ifndef MODBUS_MASTER_H_
#define MODBUS_MASTER_H_

#include "modbus.h"

typedef struct
{
	uint8_t		slave_id;
	uint8_t		func_code;

	uint16_t	read_addr;
	uint16_t	read_qty;
	uint16_t	write_addr;
	uint16_t	write_qty;

	uint8_t		rx_enable;
	uint8_t	 	rx_buf[512];
	uint16_t	rx_len;
	uint8_t		tx_buf[512];
	uint16_t	tx_len;

	bool		(* master_command)(uint16_t timeout);
} Modbus_Master;

extern Modbus_Master	Master;

void modbus_master_init(void);
bool modbus_master_validate_rx(Modbus_Master *master);
void uart0_receive_one_byte(uint8_t data);
void uart0_receive_complete(void);

bool modbus_master_read_coils(Modbus_Master *master, uint8_t id, uint8_t start, uint8_t qty, uint8_t* result);
bool modbus_master_write_single_coil(Modbus_Master *master, uint8_t id, uint8_t address, uint8_t value);
bool modbus_master_write_multiple_coils(Modbus_Master *master, uint8_t id, uint8_t start, uint8_t qty, uint8_t* values);

bool modbus_master_read_holding_regs(Modbus_Master *master, uint8_t id, uint8_t start, uint8_t qty, uint16_t* holdingregs);
bool modbus_master_Write_single_holding_reg(Modbus_Master *master, uint8_t id, uint8_t address, uint16_t value);
bool modbus_master_Write_multiple_holding_regs(Modbus_Master *master, uint8_t id, uint8_t start, uint8_t qty, uint16_t* values);

#endif
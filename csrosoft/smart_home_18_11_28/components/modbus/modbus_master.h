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

	uint8_t		(* master_send_receive)(uint16_t timeout);
} Modbus_Master;

extern Modbus_Master	Master;

void Modbus_Master_Init(void);
void uart0_receive_one_byte(uint8_t data);
void uart0_receive_complete(void);

uint8_t Modbus_Master_Read_Coils(Modbus_Master *master, uint8_t slaveid, uint8_t startaddr, uint8_t quantity, uint8_t* result);
uint8_t Modbus_Master_Write_Single_Coil(Modbus_Master *master, uint8_t slaveid, uint8_t address, uint8_t value);
uint8_t Modbus_Master_Write_Multiple_Coils(Modbus_Master *master, uint8_t slaveid, uint8_t startaddr, uint8_t quantity, uint8_t* values);

uint8_t Modbus_Master_Read_Holding_Regs(Modbus_Master *master, uint8_t slaveid, uint8_t startaddr, uint8_t quantity, uint16_t* holdingregs);
uint8_t Modbus_Master_Write_Single_Holding_Reg(Modbus_Master *master, uint8_t slaveid, uint8_t address, uint16_t value);
uint8_t Modbus_Master_Write_Multiple_Holding_Regs(Modbus_Master *master, uint8_t slaveid, uint8_t startaddr, uint8_t quantity, uint16_t* values);

#endif
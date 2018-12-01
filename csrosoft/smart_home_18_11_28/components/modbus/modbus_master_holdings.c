#include "modbus_master.h"


bool modbus_master_read_holding_regs(Modbus_Master *master, uint8_t id, uint8_t start, uint8_t qty, uint16_t* holdingregs)
{
    master->slave_id = id;
    master->func_code = FUNC_READ_HOLDING_REGISTER;
    master->read_addr = start;
    master->read_qty = qty;

    master->tx_len = 0;
    master->tx_buf[master->tx_len++] = master->slave_id;
    master->tx_buf[master->tx_len++] = master->func_code;
    master->tx_buf[master->tx_len++] = master->read_addr >> 8;
    master->tx_buf[master->tx_len++] = master->read_addr & 0xFF;
    master->tx_buf[master->tx_len++] = master->read_qty >> 8;
    master->tx_buf[master->tx_len++] = master->read_qty & 0xFF;
 
    if (master->master_command(MODBUS_TIMEOUT) == true) {
        if (modbus_master_validate_rx(master) == true) {
            if (master->rx_buf[2] != (master->read_qty*2)) {
                return false;
            }
            for(size_t i = 0; i < master->read_qty; i++)
            {
                holdingregs[i] = master->rx_buf[3 + i * 2] * 256 + master->rx_buf[4 + i * 2];
            }
            return true;
        }
    }
    return false;
}


bool modbus_master_Write_single_holding_reg(Modbus_Master *master, uint8_t id, uint8_t address, uint16_t value)
{
    master->slave_id = id;
    master->func_code = FUNC_WRITE_REGISTER;
    master->write_addr = address;

    master->tx_len = 0;
    master->tx_buf[master->tx_len++] = master->slave_id;
    master->tx_buf[master->tx_len++] = master->func_code;
    master->tx_buf[master->tx_len++] = master->write_addr >> 8;
    master->tx_buf[master->tx_len++] = master->write_addr & 0xFF;
    master->tx_buf[master->tx_len++] = value >> 8;
    master->tx_buf[master->tx_len++] = value & 0xFF;

    if (master->master_command(MODBUS_TIMEOUT) == true) {
        if (modbus_master_validate_rx(master) == true) {
            if (master->rx_buf[2] != (master->write_addr>>8)) {
                return false;
            }
            if (master->rx_buf[3] != (master->write_addr&0xFF)) {
                return false;
            }
            if (master->rx_buf[4] != (value>>8)) {
                return false;
            }
            if (master->rx_buf[5] != (value&0xFF)) {
                return false;
            }
            return true;
        }
    }
    return false;    
}


bool modbus_master_Write_multiple_holding_regs(Modbus_Master *master, uint8_t id, uint8_t start, uint8_t qty, uint16_t* values)
{
    master->slave_id = id;
    master->func_code = FUNC_WRITE_MULTIPLE_REGISTERS;
    master->write_addr = start;
    master->write_qty = qty;

    master->tx_len = 0;
    master->tx_buf[master->tx_len++] = master->slave_id;
    master->tx_buf[master->tx_len++] = master->func_code;
    master->tx_buf[master->tx_len++] = master->write_addr >> 8;
    master->tx_buf[master->tx_len++] = master->write_addr & 0xFF;
    master->tx_buf[master->tx_len++] = master->write_qty >> 8;
    master->tx_buf[master->tx_len++] = master->write_qty & 0xFF;
    master->tx_buf[master->tx_len++] = 2 * master->write_qty;
    for(size_t i = 0; i < master->write_qty; i++)
    {
        master->tx_buf[master->tx_len++] = values[i] >> 8;
        master->tx_buf[master->tx_len++] = values[i] & 0xFF;
    }

    if (master->master_command(MODBUS_TIMEOUT) == true) {
        if (modbus_master_validate_rx(master) == true) {
            if (master->rx_buf[2] != (master->write_addr>>8)) {
                return false;
            }
            if (master->rx_buf[3] != (master->write_addr&0xFF)) {
                return false;
            }
            if (master->rx_buf[4] != (master->write_qty>>8)) {
                return false;
            }
            if (master->rx_buf[5] != (master->write_qty&0xFF)) {
                return false;
            }
            return true;
        }
    }
    return false;    
}

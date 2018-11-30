#include "modbus_master.h"


bool modbus_master_read_coils(Modbus_Master *master, uint8_t id, uint8_t start, uint8_t qty, uint8_t* result)
{
    master->slave_id = id;
    master->func_code = FUNC_READ_COILS;
    master->read_addr = start;
    master->read_qty = qty;

    master->tx_len = 0;
    master->tx_buf[master->tx_len++] = master->slave_id;
    master->tx_buf[master->tx_len++] = master->func_code;
    master->tx_buf[master->tx_len++] = master->read_addr >> 8;
    master->tx_buf[master->tx_len++] = master->read_addr & 0xFF;
    master->tx_buf[master->tx_len++] = master->read_qty >> 8;
    master->tx_buf[master->tx_len++] = master->read_qty & 0xFF;
    debug("111");
    if (master->master_command(MODBUS_TIMEOUT) == true) {
        debug("222");
        if (modbus_master_validate_rx(master) == true) {
            debug("333");
            if (master->rx_buf[2] != (master->read_qty%8 == 0 ? master->read_qty/8 : master->read_qty/8+1)) {
                return false;
            }
            debug("444");
            for(size_t i = 0; i < qty; i++)
            {
                uint8_t value = master->rx_buf[3 + i / 8];
                result[i] = 0x01 & (value >> (i % 8));
            }
            debug("555");
            return true;
        }
    }
    debug("666");
    return false;
}


bool modbus_master_write_single_coil(Modbus_Master *master, uint8_t id, uint8_t address, uint8_t value)
{
    master->slave_id = id;
    master->func_code = FUNC_WRITE_SINGLE_COIL;
    master->write_addr = address;

    master->tx_len = 0;
    master->tx_buf[master->tx_len++] = master->slave_id;
    master->tx_buf[master->tx_len++] = master->func_code;
    master->tx_buf[master->tx_len++] = master->write_addr >> 8;
    master->tx_buf[master->tx_len++] = master->write_addr & 0xFF;
    master->tx_buf[master->tx_len++] = (value == 1) ? 0xFF : 0x00;
    master->tx_buf[master->tx_len++] = 0x00;
    if (master->master_command(MODBUS_TIMEOUT) == true) {
        if (modbus_master_validate_rx(master) == true) {
            if (master->rx_buf[2] != (master->write_addr>>8)) {
                return false;
            }
            if (master->rx_buf[3] != (master->write_addr&0xFF)) {
                return false;
            }
            if (master->rx_buf[4] != ((value == 1) ? 0xFF : 0x00)) {
                return false;
            }
            if (master->rx_buf[5] != 0x00) {
                return false;
            }
            return true;
        }
    }
    return false;
}


bool modbus_master_write_multiple_coils(Modbus_Master *master, uint8_t id, uint8_t start, uint8_t qty, uint8_t* values)
{
    uint8_t byte_count = (qty % 8 == 0) ? qty / 8 : qty / 8 + 1;
    uint8_t data[32];
    for(size_t i = 0; i < byte_count*8; i++)
    {
        if (i < qty) {
            if (values[i] == 1) {
                data[i / 8] = data[i / 8] >> 1 | 0x80;
            }
            else {
                data[i / 8] = (data[i / 8] >> 1) & 0x7F;
            }
        }
        else {
            data[i / 8] = data[i / 8] >> 1;
        }
    }
    
    master->slave_id = id;
    master->func_code = FUNC_WRITE_MULTIPLE_COILS;
    master->write_addr = start;
    master->write_qty = qty;

    master->tx_len = 0;
    master->tx_buf[master->tx_len++] = master->slave_id;
    master->tx_buf[master->tx_len++] = master->func_code;
    master->tx_buf[master->tx_len++] = master->write_addr >> 8;
    master->tx_buf[master->tx_len++] = master->write_addr & 0xFF;
    master->tx_buf[master->tx_len++] = master->write_qty >> 8;
    master->tx_buf[master->tx_len++] = master->write_qty & 0xFF;
    master->tx_buf[master->tx_len++] = byte_count;

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
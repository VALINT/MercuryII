


#include "CE2826.h"


void CE2826_write_creg(uint8_t addr, uint8_t data)
{
    _i2c_start_operation();
    _i2c_send_byte(CE2826_ADDRESS | WRITE);
    _i2c_send_byte(addr);
    _i2c_send_byte(data);
    _i2c_stop_operation();
}

void CE2826_read_creg(uint8_t addr)
{
    _i2c_start_operation();
    _i2c_send_byte(CE2826_ADDRESS | READ);
    _i2c_send_byte(addr);
    _i2c_reseive_last_byte();
    _i2c_stop_operation();
}

